#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "octobass-core/BassProcessor.hpp"

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

}  // namespace

class BassProcessorTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    proc.setSampleRate(44100.0);
    proc.setMaxBlockSize(kBlockSize);
  }

  BassProcessor proc;
  std::string irAPath_ = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
};

TEST_F(BassProcessorTest, InitialDefaults)
{
  EXPECT_FLOAT_EQ(proc.getCrossoverFrequency(), DefaultCrossoverFrequency);
  EXPECT_FLOAT_EQ(proc.getLowBandLevel(), DefaultBandLevelDb);
  EXPECT_FLOAT_EQ(proc.getHighBandLevel(), DefaultBandLevelDb);
  EXPECT_FLOAT_EQ(proc.getOutputGain(), DefaultOutputGainDb);
  EXPECT_FLOAT_EQ(proc.getDryWetMix(), DefaultDryWetMix);
  EXPECT_FALSE(proc.isIRLoaded());
  EXPECT_EQ(proc.getLatencySamples(), 0);
}

TEST_F(BassProcessorTest, ParameterClamping)
{
  proc.setLowBandLevel(-50.0f);
  EXPECT_FLOAT_EQ(proc.getLowBandLevel(), MinBandLevelDb);

  proc.setLowBandLevel(50.0f);
  EXPECT_FLOAT_EQ(proc.getLowBandLevel(), MaxBandLevelDb);

  proc.setHighBandLevel(-50.0f);
  EXPECT_FLOAT_EQ(proc.getHighBandLevel(), MinBandLevelDb);

  proc.setHighBandLevel(50.0f);
  EXPECT_FLOAT_EQ(proc.getHighBandLevel(), MaxBandLevelDb);

  proc.setOutputGain(-50.0f);
  EXPECT_FLOAT_EQ(proc.getOutputGain(), MinOutputGainDb);

  proc.setOutputGain(50.0f);
  EXPECT_FLOAT_EQ(proc.getOutputGain(), MaxOutputGainDb);

  proc.setDryWetMix(-1.0f);
  EXPECT_FLOAT_EQ(proc.getDryWetMix(), 0.0f);

  proc.setDryWetMix(2.0f);
  EXPECT_FLOAT_EQ(proc.getDryWetMix(), 1.0f);
}

TEST_F(BassProcessorTest, NoIR_MagnitudePreserved)
{
  // Without an IR loaded, the LR4 crossover splits and recombines as all-pass.
  // The output RMS should match the input RMS (magnitude-flat, phase may differ).
  constexpr size_t kNumBlocks = 16;
  constexpr size_t kTotalSamples = kNumBlocks * kBlockSize;
  constexpr size_t kSkip = 4 * kBlockSize;
  auto input = generateSine(100.0f, 44100.0f, kTotalSamples);

  std::vector<float> output(kTotalSamples);
  for (size_t b = 0; b < kNumBlocks; ++b)
    proc.processMono(input.data() + b * kBlockSize, output.data() + b * kBlockSize, kBlockSize);

  // Compare RMS
  double inputRMS = 0.0, outputRMS = 0.0;
  for (size_t i = kSkip; i < kTotalSamples; ++i)
  {
    inputRMS += static_cast<double>(input[i]) * input[i];
    outputRMS += static_cast<double>(output[i]) * output[i];
  }
  inputRMS = std::sqrt(inputRMS / static_cast<double>(kTotalSamples - kSkip));
  outputRMS = std::sqrt(outputRMS / static_cast<double>(kTotalSamples - kSkip));

  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);
  EXPECT_NEAR(ratioDb, 0.0, 1.0) << "Without IR, output level should match input (all-pass)";
}

TEST_F(BassProcessorTest, DryWet_FullDry)
{
  proc.setDryWetMix(0.0f);

  constexpr size_t kNumSamples = kBlockSize;
  auto input = generateSine(100.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);

  proc.processMono(input.data(), output.data(), kNumSamples);

  for (size_t i = 0; i < kNumSamples; ++i)
    EXPECT_FLOAT_EQ(output[i], input[i]) << "Full dry should pass input through at sample " << i;
}

TEST_F(BassProcessorTest, DryWet_FullWet)
{
  proc.setDryWetMix(1.0f);

  constexpr size_t kNumSamples = kBlockSize;
  auto input = generateSine(100.0f, 44100.0f, kNumSamples);
  std::vector<float> output(kNumSamples);

  proc.processMono(input.data(), output.data(), kNumSamples);

  // Full wet should give processed signal (which may differ from input due to crossover transient)
  float outPeak = peakLevel(output);
  EXPECT_GT(outPeak, 1e-6f) << "Full wet output should not be silent";
}

TEST_F(BassProcessorTest, IRLoading)
{
  std::string err;
  ASSERT_TRUE(proc.loadImpulseResponse(irAPath_, err)) << "IR load failed: " << err;
  EXPECT_EQ(proc.getCurrentIRPath(), irAPath_);

  // octobir-core uses staging: IR is swapped in during the first processMono call
  std::vector<float> in(kBlockSize, 0.0f);
  std::vector<float> out(kBlockSize, 0.0f);
  proc.processMono(in.data(), out.data(), kBlockSize);

  EXPECT_TRUE(proc.isIRLoaded());
}

TEST_F(BassProcessorTest, ClearIR)
{
  std::string err;
  ASSERT_TRUE(proc.loadImpulseResponse(irAPath_, err)) << err;

  // Process a block to swap in the pending IR
  std::vector<float> in(kBlockSize, 0.0f);
  std::vector<float> out(kBlockSize, 0.0f);
  proc.processMono(in.data(), out.data(), kBlockSize);
  EXPECT_TRUE(proc.isIRLoaded());

  proc.clearImpulseResponse();
  EXPECT_TRUE(proc.getCurrentIRPath().empty());

  // Clear also uses staging in octobir-core — process a block to swap
  proc.processMono(in.data(), out.data(), kBlockSize);
  EXPECT_FALSE(proc.isIRLoaded());
}

TEST_F(BassProcessorTest, LatencyReporting)
{
  // Without IR, latency is 0
  EXPECT_EQ(proc.getLatencySamples(), 0);

  std::string err;
  ASSERT_TRUE(proc.loadImpulseResponse(irAPath_, err)) << err;

  // Process blocks to trigger the pending IR swap
  std::vector<float> in(kBlockSize, 0.0f);
  std::vector<float> out(kBlockSize, 0.0f);
  proc.processMono(in.data(), out.data(), kBlockSize);
  proc.processMono(in.data(), out.data(), kBlockSize);

  // Latency is >= 0 (may be 0 for single-IR config with small partition)
  EXPECT_GE(proc.getLatencySamples(), 0);

  // After clearing, latency returns to 0
  proc.clearImpulseResponse();
  proc.processMono(in.data(), out.data(), kBlockSize);
  EXPECT_EQ(proc.getLatencySamples(), 0);
}

TEST_F(BassProcessorTest, Reset_ClearsAllState)
{
  constexpr size_t kNumSamples = kBlockSize;
  auto input = generateSine(100.0f, 44100.0f, kNumSamples, 0.8f);
  std::vector<float> output(kNumSamples);

  proc.processMono(input.data(), output.data(), kNumSamples);

  proc.reset();

  // Process silence after reset — output should be silent
  std::vector<float> silence(kNumSamples, 0.0f);
  proc.processMono(silence.data(), output.data(), kNumSamples);

  for (size_t i = 0; i < kNumSamples; ++i)
    EXPECT_FLOAT_EQ(output[i], 0.0f) << "Output not silent at sample " << i << " after reset";
}

TEST_F(BassProcessorTest, SilentInput_SilentOutput)
{
  constexpr size_t kNumSamples = kBlockSize;
  std::vector<float> silence(kNumSamples, 0.0f);
  std::vector<float> output(kNumSamples, 1.0f);  // fill with non-zero to detect changes

  proc.processMono(silence.data(), output.data(), kNumSamples);

  for (size_t i = 0; i < kNumSamples; ++i)
    EXPECT_FLOAT_EQ(output[i], 0.0f) << "Output not silent at sample " << i;
}
