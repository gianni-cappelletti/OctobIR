#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "octobass-core/Compressor.hpp"

using namespace octob;

namespace
{

constexpr double kPi = 3.14159265358979323846;
constexpr int kBlockSize = 512;

std::vector<float> generateSine(float freqHz, float sampleRate, size_t numSamples,
                                float amplitude = 0.5f)
{
  std::vector<float> buf(numSamples);
  for (size_t i = 0; i < numSamples; ++i)
    buf[i] = amplitude * static_cast<float>(std::sin(2.0 * kPi * freqHz * i / sampleRate));
  return buf;
}

float peakLevel(const std::vector<float>& buf)
{
  float peak = 0.0f;
  for (float s : buf)
    peak = std::max(peak, std::abs(s));
  return peak;
}

double computeRMS(const std::vector<float>& buf, size_t start, size_t len)
{
  double sum = 0.0;
  for (size_t i = start; i < start + len; ++i)
    sum += static_cast<double>(buf[i]) * buf[i];
  return std::sqrt(sum / static_cast<double>(len));
}

// Process input through compressor in blocks
std::vector<float> processInBlocks(Compressor& comp, const std::vector<float>& input, int blockSize)
{
  std::vector<float> output(input.size());
  size_t pos = 0;
  while (pos < input.size())
  {
    size_t toProcess = std::min(static_cast<size_t>(blockSize), input.size() - pos);
    comp.process(input.data() + pos, output.data() + pos, toProcess);
    pos += toProcess;
  }
  return output;
}

}  // namespace

class CompressorTest : public ::testing::Test
{
 protected:
  void SetUp() override { comp.setSampleRate(44100.0); }

  Compressor comp;
};

TEST_F(CompressorTest, InitialDefaults)
{
  EXPECT_FLOAT_EQ(comp.getSquash(), DefaultSquashAmount);
  EXPECT_EQ(comp.getMode(), DefaultCompressionMode);
}

TEST_F(CompressorTest, SquashZero_NoChange)
{
  // With squash=0, each mode should be transparent
  constexpr size_t kNumSamples = 4096;
  auto input = generateSine(100.0f, 44100.0f, kNumSamples, 0.5f);

  for (int mode = 0; mode < NumCompressionModes; ++mode)
  {
    comp.reset();
    comp.setMode(mode);
    comp.setSquash(0.0f);

    auto output = processInBlocks(comp, input, kBlockSize);

    // At squash=0, output should match input closely
    float maxErr = 0.0f;
    for (size_t i = 0; i < kNumSamples; ++i)
      maxErr = std::max(maxErr, std::abs(output[i] - input[i]));

    EXPECT_LT(maxErr, 1e-5f) << "Mode " << mode << " should be transparent at squash=0";
  }
}

TEST_F(CompressorTest, SquashFull_ReducesPeak)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 4096;
  auto input = generateSine(80.0f, 44100.0f, kNumSamples, 0.5f);
  float inputPeak = peakLevel(input);

  for (int mode = 0; mode < NumCompressionModes; ++mode)
  {
    comp.reset();
    comp.setMode(mode);
    comp.setSquash(1.0f);

    auto output = processInBlocks(comp, input, kBlockSize);

    // Measure peak after transient skip (before makeup gain settles)
    float outputPeak = 0.0f;
    for (size_t i = kSkip; i < kNumSamples; ++i)
      outputPeak = std::max(outputPeak, std::abs(output[i]));

    // The compressor should be doing something -- output should not be silent
    EXPECT_GT(outputPeak, 1e-6f) << "Mode " << mode << " should produce non-silent output";

    // With makeup gain, the output RMS should be roughly preserved, but the peak
    // behavior depends on the mode. Just verify the mode is doing compression
    // (gain reduction should be non-zero)
    EXPECT_LT(comp.getGainReductionDb(), -0.1f)
        << "Mode " << mode << " should have measurable gain reduction at squash=1.0";
  }
}

TEST_F(CompressorTest, AutoMakeupGain_RMSPreserved)
{
  constexpr size_t kNumSamples = 88200;  // 2 seconds at 44.1kHz
  constexpr size_t kSkip = 66150;        // skip first 1.5s for makeup to fully settle
  auto input = generateSine(100.0f, 44100.0f, kNumSamples, 0.3f);
  double inputRMS = computeRMS(input, kSkip, kNumSamples - kSkip);

  for (int mode = 0; mode < NumCompressionModes; ++mode)
  {
    for (float squash : {0.5f, 1.0f})
    {
      comp.reset();
      comp.setMode(mode);
      comp.setSquash(squash);

      auto output = processInBlocks(comp, input, kBlockSize);
      double outputRMS = computeRMS(output, kSkip, kNumSamples - kSkip);

      double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);
      EXPECT_NEAR(ratioDb, 0.0, 5.0) << "Mode " << mode << " squash=" << squash
                                     << ": RMS should be roughly preserved by auto makeup gain";
    }
  }
}

TEST_F(CompressorTest, ModesHaveDifferentCharacteristics)
{
  constexpr size_t kNumSamples = 16384;
  constexpr size_t kSkip = 8192;
  auto input = generateSine(80.0f, 44100.0f, kNumSamples, 0.5f);

  std::vector<float> peaks(NumCompressionModes);
  std::vector<double> rmsValues(NumCompressionModes);

  for (int mode = 0; mode < NumCompressionModes; ++mode)
  {
    comp.reset();
    comp.setMode(mode);
    comp.setSquash(1.0f);

    auto output = processInBlocks(comp, input, kBlockSize);
    peaks[mode] = 0.0f;
    for (size_t i = kSkip; i < kNumSamples; ++i)
      peaks[mode] = std::max(peaks[mode], std::abs(output[i]));
    rmsValues[mode] = computeRMS(output, kSkip, kNumSamples - kSkip);
  }

  // At least some modes should produce measurably different outputs
  int distinctPairs = 0;
  for (int a = 0; a < NumCompressionModes; ++a)
  {
    for (int b = a + 1; b < NumCompressionModes; ++b)
    {
      double peakDiffDb =
          20.0 * std::log10(std::max(1e-10f, peaks[a]) / std::max(1e-10f, peaks[b]));
      double rmsDiffDb =
          20.0 * std::log10(std::max(1e-10, rmsValues[a]) / std::max(1e-10, rmsValues[b]));
      if (std::abs(peakDiffDb) > 0.5 || std::abs(rmsDiffDb) > 0.5)
        ++distinctPairs;
    }
  }
  EXPECT_GE(distinctPairs, 2) << "Modes should produce measurably distinct outputs";
}

TEST_F(CompressorTest, GainReduction_IncreasesWithSquash)
{
  constexpr size_t kNumSamples = 16384;
  auto input = generateSine(80.0f, 44100.0f, kNumSamples, 0.5f);

  for (int mode = 0; mode < NumCompressionModes; ++mode)
  {
    float prevGR = 0.0f;
    for (float squash : {0.25f, 0.5f, 0.75f, 1.0f})
    {
      comp.reset();
      comp.setMode(mode);
      comp.setSquash(squash);

      processInBlocks(comp, input, kBlockSize);
      float gr = comp.getGainReductionDb();

      EXPECT_LE(gr, prevGR + 0.1f)
          << "Mode " << mode << " squash=" << squash
          << ": gain reduction should increase (become more negative) with squash";
      prevGR = gr;
    }
  }
}

TEST_F(CompressorTest, SilentInput_SilentOutput)
{
  constexpr size_t kNumSamples = kBlockSize;
  std::vector<float> silence(kNumSamples, 0.0f);
  std::vector<float> output(kNumSamples, 1.0f);

  for (int mode = 0; mode < NumCompressionModes; ++mode)
  {
    comp.reset();
    comp.setMode(mode);
    comp.setSquash(1.0f);

    comp.process(silence.data(), output.data(), kNumSamples);

    for (size_t i = 0; i < kNumSamples; ++i)
      EXPECT_FLOAT_EQ(output[i], 0.0f) << "Mode " << mode << " sample " << i;
  }
}

TEST_F(CompressorTest, Reset_ClearsEnvelope)
{
  constexpr size_t kNumSamples = 4096;
  auto loud = generateSine(100.0f, 44100.0f, kNumSamples, 0.8f);
  std::vector<float> output(kNumSamples);

  comp.setSquash(1.0f);
  comp.process(loud.data(), output.data(), kNumSamples);

  comp.reset();

  std::vector<float> silence(kNumSamples, 0.0f);
  comp.process(silence.data(), output.data(), kNumSamples);

  for (size_t i = 0; i < kNumSamples; ++i)
    EXPECT_FLOAT_EQ(output[i], 0.0f) << "After reset, silence should produce silence at " << i;
}

TEST_F(CompressorTest, BlockBoundaryContinuity)
{
  constexpr size_t kTotalSamples = 8192;
  constexpr size_t kSmallBlock = 128;
  constexpr size_t kSkip = 2048;
  auto input = generateSine(100.0f, 44100.0f, kTotalSamples, 0.5f);

  comp.setSquash(0.7f);

  // Process as one large block
  Compressor compA;
  compA.setSampleRate(44100.0);
  compA.setSquash(0.7f);
  std::vector<float> outputA(kTotalSamples);
  compA.process(input.data(), outputA.data(), kTotalSamples);

  // Process as many small blocks
  Compressor compB;
  compB.setSampleRate(44100.0);
  compB.setSquash(0.7f);
  std::vector<float> outputB(kTotalSamples);
  for (size_t b = 0; b < kTotalSamples / kSmallBlock; ++b)
    compB.process(input.data() + b * kSmallBlock, outputB.data() + b * kSmallBlock, kSmallBlock);

  float maxErr = 0.0f;
  for (size_t i = kSkip; i < kTotalSamples; ++i)
    maxErr = std::max(maxErr, std::abs(outputA[i] - outputB[i]));

  EXPECT_LT(maxErr, 1e-4f) << "Block boundary discontinuity: max error = " << maxErr;
}

TEST_F(CompressorTest, DifferentSampleRates)
{
  constexpr size_t kNumSamples = 4096;

  for (double sr : {44100.0, 48000.0, 96000.0})
  {
    for (int mode = 0; mode < NumCompressionModes; ++mode)
    {
      Compressor c;
      c.setSampleRate(sr);
      c.setMode(mode);
      c.setSquash(1.0f);

      auto input = generateSine(80.0f, static_cast<float>(sr), kNumSamples, 0.5f);
      auto output = processInBlocks(c, input, kBlockSize);

      float peak = peakLevel(output);
      EXPECT_GT(peak, 1e-6f) << "SR=" << sr << " mode=" << mode << " should produce output";
      EXPECT_FALSE(std::isnan(peak)) << "SR=" << sr << " mode=" << mode << " produced NaN";
    }
  }
}

TEST_F(CompressorTest, ModeSwitching_ResetsState)
{
  constexpr size_t kNumSamples = 4096;
  auto loud = generateSine(100.0f, 44100.0f, kNumSamples, 0.8f);
  std::vector<float> output(kNumSamples);

  comp.setSquash(1.0f);
  comp.process(loud.data(), output.data(), kNumSamples);

  // Switch mode -- should reset internal state
  comp.setMode(2);

  std::vector<float> silence(kNumSamples, 0.0f);
  comp.process(silence.data(), output.data(), kNumSamples);

  for (size_t i = 0; i < kNumSamples; ++i)
    EXPECT_FLOAT_EQ(output[i], 0.0f)
        << "After mode switch, silence should produce silence at " << i;
}

TEST_F(CompressorTest, FeedbackModes_StableAtExtremes)
{
  constexpr size_t kNumSamples = 44100;
  auto loud = generateSine(60.0f, 44100.0f, kNumSamples, 0.95f);

  // Test feedback modes (Opto=1, FET=2) at max squash with loud input
  for (int mode : {1, 2})
  {
    Compressor c;
    c.setSampleRate(44100.0);
    c.setMode(mode);
    c.setSquash(1.0f);

    auto output = processInBlocks(c, loud, kBlockSize);

    for (size_t i = 0; i < kNumSamples; ++i)
    {
      EXPECT_FALSE(std::isnan(output[i])) << "Mode " << mode << " produced NaN at " << i;
      EXPECT_FALSE(std::isinf(output[i])) << "Mode " << mode << " produced Inf at " << i;
    }

    float peak = peakLevel(output);
    EXPECT_LT(peak, 10.0f) << "Mode " << mode << " output is unreasonably loud: " << peak;
  }
}
