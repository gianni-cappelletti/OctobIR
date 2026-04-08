#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "dr_wav.h"
#include "octobass-core/BassProcessor.hpp"

using namespace octob;

namespace
{

constexpr int kBlockSize = 512;

std::vector<float> loadWavMono(const std::string& path, unsigned int& outSampleRate,
                               drwav_uint64& outFrameCount)
{
  drwav_uint32 channels = 0;
  drwav_uint32 sampleRate = 0;
  drwav_uint64 totalFrames = 0;

  float* raw = drwav_open_file_and_read_pcm_frames_f32(path.c_str(), &channels, &sampleRate,
                                                       &totalFrames, nullptr);
  if (!raw)
    return {};

  outSampleRate = sampleRate;
  outFrameCount = totalFrames;

  std::vector<float> mono(static_cast<size_t>(totalFrames));
  for (drwav_uint64 i = 0; i < totalFrames; ++i)
  {
    float sum = 0.0f;
    for (drwav_uint32 ch = 0; ch < channels; ++ch)
      sum += raw[i * channels + ch];
    mono[static_cast<size_t>(i)] = sum / static_cast<float>(channels);
  }

  drwav_free(raw, nullptr);
  return mono;
}

void normalizeToUnitPeak(std::vector<float>& samples)
{
  float peak = 0.0f;
  for (float s : samples)
    peak = std::max(peak, std::abs(s));
  if (peak > 0.0f)
    for (float& s : samples)
      s /= peak;
}

// Process input through BassProcessor in blocks, return the full output.
std::vector<float> processFullSignal(BassProcessor& proc, const std::vector<float>& input)
{
  const size_t totalFrames = input.size();
  std::vector<float> output(totalFrames);

  std::vector<float> inBlock(kBlockSize, 0.0f);
  std::vector<float> outBlock(kBlockSize, 0.0f);

  size_t framesConsumed = 0;
  while (framesConsumed < totalFrames)
  {
    size_t toCopy = std::min(static_cast<size_t>(kBlockSize), totalFrames - framesConsumed);
    std::fill(inBlock.begin(), inBlock.end(), 0.0f);
    std::copy(input.begin() + static_cast<ptrdiff_t>(framesConsumed),
              input.begin() + static_cast<ptrdiff_t>(framesConsumed + toCopy), inBlock.begin());

    proc.processMono(inBlock.data(), outBlock.data(), kBlockSize);

    size_t outCopy = std::min(toCopy, totalFrames - framesConsumed);
    std::copy(outBlock.begin(), outBlock.begin() + static_cast<ptrdiff_t>(outCopy),
              output.begin() + static_cast<ptrdiff_t>(framesConsumed));
    framesConsumed += kBlockSize;
  }

  return output;
}

double computeRMS(const std::vector<float>& buf)
{
  double sum = 0.0;
  for (float s : buf)
    sum += static_cast<double>(s) * s;
  return std::sqrt(sum / static_cast<double>(buf.size()));
}

float peakLevel(const std::vector<float>& buf)
{
  float peak = 0.0f;
  for (float s : buf)
    peak = std::max(peak, std::abs(s));
  return peak;
}

}  // namespace

class BassProcessorAudioTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    bassPath_ = std::string(TEST_DATA_DIR) + "/INPUT_bass.wav";
    irAPath_ = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";

    unsigned int sr = 0;
    drwav_uint64 fc = 0;
    bassInput_ = loadWavMono(bassPath_, sr, fc);
    ASSERT_FALSE(bassInput_.empty()) << "Failed to load " << bassPath_;
    bassSampleRate_ = sr;
  }

  std::string bassPath_;
  std::string irAPath_;
  std::vector<float> bassInput_;
  unsigned int bassSampleRate_;
};

TEST_F(BassProcessorAudioTest, ProcessBass_NonSilent)
{
  BassProcessor proc;
  proc.setSampleRate(static_cast<double>(bassSampleRate_));
  proc.setMaxBlockSize(kBlockSize);

  auto output = processFullSignal(proc, bassInput_);

  float peak = peakLevel(output);
  EXPECT_GT(peak, 1e-6f) << "Output is silent after processing bass track";
}

TEST_F(BassProcessorAudioTest, ProcessBass_NonClipping)
{
  BassProcessor proc;
  proc.setSampleRate(static_cast<double>(bassSampleRate_));
  proc.setMaxBlockSize(kBlockSize);

  auto output = processFullSignal(proc, bassInput_);

  float peak = peakLevel(output);
  EXPECT_LT(peak, 10.0f) << "Output peak (" << peak << ") exceeds sanity limit";
}

TEST_F(BassProcessorAudioTest, ProcessBass_MagnitudePreserved)
{
  // With no IR and default levels, the LR4 crossover recombines as all-pass.
  // Output magnitude (RMS) should match input, though phase differs.
  BassProcessor proc;
  proc.setSampleRate(static_cast<double>(bassSampleRate_));
  proc.setMaxBlockSize(kBlockSize);

  auto output = processFullSignal(proc, bassInput_);

  double inputRMS = computeRMS(bassInput_);
  double outputRMS = computeRMS(output);

  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);
  std::cout << "[BassMagnitude] Input RMS: " << inputRMS << ", Output RMS: " << outputRMS
            << ", Ratio: " << ratioDb << " dB\n";
  EXPECT_NEAR(ratioDb, 0.0, 1.0) << "Output magnitude should match input (all-pass crossover)";
}

TEST_F(BassProcessorAudioTest, ProcessBass_WithIR_NonSilent)
{
  BassProcessor proc;
  proc.setSampleRate(static_cast<double>(bassSampleRate_));
  proc.setMaxBlockSize(kBlockSize);

  std::string err;
  ASSERT_TRUE(proc.loadImpulseResponse(irAPath_, err)) << "IR load failed: " << err;

  auto output = processFullSignal(proc, bassInput_);

  float peak = peakLevel(output);
  EXPECT_GT(peak, 1e-6f) << "Output is silent after processing bass with IR";
}

TEST_F(BassProcessorAudioTest, ProcessBass_WithIR_DiffersFromDry)
{
  // Process without IR
  BassProcessor procDry;
  procDry.setSampleRate(static_cast<double>(bassSampleRate_));
  procDry.setMaxBlockSize(kBlockSize);
  auto outputDry = processFullSignal(procDry, bassInput_);

  // Process with IR
  BassProcessor procWet;
  procWet.setSampleRate(static_cast<double>(bassSampleRate_));
  procWet.setMaxBlockSize(kBlockSize);
  std::string err;
  ASSERT_TRUE(procWet.loadImpulseResponse(irAPath_, err)) << err;
  auto outputWet = processFullSignal(procWet, bassInput_);

  // They should differ (the high band is convolved)
  double diffSum = 0.0;
  size_t len = std::min(outputDry.size(), outputWet.size());
  for (size_t i = 0; i < len; ++i)
  {
    double d = outputDry[i] - outputWet[i];
    diffSum += d * d;
  }
  double diffRMS = std::sqrt(diffSum / static_cast<double>(len));

  EXPECT_GT(diffRMS, 1e-6) << "IR convolution should change the output";
}

TEST_F(BassProcessorAudioTest, ProcessBass_MagnitudePreserved_SkipTransient)
{
  constexpr size_t kSkip = 4 * kBlockSize;

  BassProcessor proc;
  proc.setSampleRate(static_cast<double>(bassSampleRate_));
  proc.setMaxBlockSize(kBlockSize);

  auto output = processFullSignal(proc, bassInput_);

  ASSERT_GT(output.size(), kSkip) << "Output too short for transient skip";

  size_t compareLen = output.size() - kSkip;

  double inputRMS = 0.0, outputRMS = 0.0;
  for (size_t i = kSkip; i < output.size(); ++i)
  {
    inputRMS += static_cast<double>(bassInput_[i]) * bassInput_[i];
    outputRMS += static_cast<double>(output[i]) * output[i];
  }
  inputRMS = std::sqrt(inputRMS / static_cast<double>(compareLen));
  outputRMS = std::sqrt(outputRMS / static_cast<double>(compareLen));

  double ratioDb = 20.0 * std::log10(outputRMS / inputRMS);
  EXPECT_NEAR(ratioDb, 0.0, 0.5)
      << "After transient skip, output magnitude should closely match input";
}

TEST_F(BassProcessorAudioTest, ProcessBass_WithSquash_NonSilent)
{
  BassProcessor proc;
  proc.setSampleRate(static_cast<double>(bassSampleRate_));
  proc.setMaxBlockSize(kBlockSize);
  proc.setSquash(0.5f);

  auto output = processFullSignal(proc, bassInput_);

  float peak = peakLevel(output);
  EXPECT_GT(peak, 1e-6f) << "Output should not be silent with squash=0.5";
}

// Test compression on a signal that is entirely below the crossover frequency,
// so that ALL energy passes through the compression path with no high-band dilution.
TEST_F(BassProcessorAudioTest, ProcessBass_LowBandSquash_ReducesLevel)
{
  constexpr float kFreqHz = 80.0f;
  constexpr size_t kDurationSamples = 88200;
  constexpr size_t kSkip = 44100;

  // Generate a pure sub-crossover tone (80Hz << 250Hz default crossover)
  std::vector<float> lowTone(kDurationSamples);
  for (size_t i = 0; i < kDurationSamples; ++i)
    lowTone[i] =
        0.5f * static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * kFreqHz *
                                           static_cast<double>(i) / bassSampleRate_));

  // Process without compression
  BassProcessor procDry;
  procDry.setSampleRate(static_cast<double>(bassSampleRate_));
  procDry.setMaxBlockSize(kBlockSize);
  auto outputDry = processFullSignal(procDry, lowTone);

  double dryRMS = 0.0;
  for (size_t i = kSkip; i < kDurationSamples; ++i)
    dryRMS += static_cast<double>(outputDry[i]) * outputDry[i];
  dryRMS = std::sqrt(dryRMS / static_cast<double>(kDurationSamples - kSkip));

  for (int mode = 0; mode < octob::NumCompressionModes; ++mode)
  {
    BassProcessor procWet;
    procWet.setSampleRate(static_cast<double>(bassSampleRate_));
    procWet.setMaxBlockSize(kBlockSize);
    procWet.setSquash(1.0f);
    procWet.setCompressionMode(mode);
    auto outputWet = processFullSignal(procWet, lowTone);

    double wetRMS = 0.0;
    for (size_t i = kSkip; i < kDurationSamples; ++i)
      wetRMS += static_cast<double>(outputWet[i]) * outputWet[i];
    wetRMS = std::sqrt(wetRMS / static_cast<double>(kDurationSamples - kSkip));

    double ratioDb = 20.0 * std::log10(wetRMS / dryRMS);
    std::cout << "[LowBandSquash] Mode " << mode << ": Dry RMS=" << dryRMS
              << ", Wet RMS=" << wetRMS << ", Reduction=" << ratioDb << " dB\n";

    EXPECT_LT(ratioDb, -3.0)
        << "Mode " << mode
        << " at full squash should significantly reduce a sub-crossover signal";
  }
}

// Verify compression does not boost the signal above its input peak through
// the full BassProcessor chain, using an isolated low-band signal.
TEST_F(BassProcessorAudioTest, ProcessBass_LowBandSquash_NoOvershoot)
{
  constexpr size_t kBurstLen = 66150;
  constexpr size_t kSilenceLen = 4410;
  constexpr size_t kReburstLen = 22050;
  constexpr size_t kTotal = kBurstLen + kSilenceLen + kReburstLen;
  constexpr float kFreq = 80.0f;
  constexpr float kAmplitude = 0.6f;
  constexpr double kTwoPi = 2.0 * 3.14159265358979323846;

  std::vector<float> input(kTotal, 0.0f);
  for (size_t i = 0; i < kBurstLen; ++i)
    input[i] = kAmplitude * static_cast<float>(
                                std::sin(kTwoPi * kFreq * static_cast<double>(i) / bassSampleRate_));
  for (size_t i = kBurstLen + kSilenceLen; i < kTotal; ++i)
  {
    size_t j = i - kBurstLen - kSilenceLen;
    input[i] = kAmplitude * static_cast<float>(
                                std::sin(kTwoPi * kFreq * static_cast<double>(j) / bassSampleRate_));
  }

  float inputPeak = peakLevel(input);

  for (int mode : {0, 1, 2, 3})
  {
    BassProcessor proc;
    proc.setSampleRate(static_cast<double>(bassSampleRate_));
    proc.setMaxBlockSize(kBlockSize);
    proc.setSquash(1.0f);
    proc.setCompressionMode(mode);

    auto output = processFullSignal(proc, input);
    float outputPeak = peakLevel(output);

    EXPECT_LE(outputPeak, inputPeak * 1.05f)
        << "Mode " << mode << " output peak (" << outputPeak
        << ") should not overshoot input peak (" << inputPeak << ")";
  }
}

TEST_F(BassProcessorAudioTest, ProcessBass_AllModes_ProduceOutput)
{
  for (int mode = 0; mode < octob::NumCompressionModes; ++mode)
  {
    BassProcessor proc;
    proc.setSampleRate(static_cast<double>(bassSampleRate_));
    proc.setMaxBlockSize(kBlockSize);
    proc.setSquash(1.0f);
    proc.setCompressionMode(mode);

    auto output = processFullSignal(proc, bassInput_);

    float peak = peakLevel(output);
    EXPECT_GT(peak, 1e-6f) << "Mode " << mode << " should produce non-silent output";
    EXPECT_LT(peak, 10.0f) << "Mode " << mode << " should not clip excessively";
  }
}
