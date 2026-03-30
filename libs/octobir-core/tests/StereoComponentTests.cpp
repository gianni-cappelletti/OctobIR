#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "dr_wav.h"
#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kStereoIRPath = std::string(TEST_DATA_DIR) + "/INPUT_long_stereo_hall.wav";
static const std::string kMonoIRPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
static const std::string kDryPath = std::string(TEST_DATA_DIR) + "/INPUT_amp_output_no_ir.wav";
static const std::string kControlStereoPath =
    std::string(TEST_DATA_DIR) + "/CONTROL_amp_output_with_long_stereo_hall.wav";

namespace
{

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

double pearsonCorrelation(const std::vector<float>& a, const std::vector<float>& b)
{
  const size_t n = std::min(a.size(), b.size());
  double meanA = 0.0, meanB = 0.0;
  for (size_t i = 0; i < n; ++i)
  {
    meanA += a[i];
    meanB += b[i];
  }
  meanA /= static_cast<double>(n);
  meanB /= static_cast<double>(n);

  double num = 0.0, varA = 0.0, varB = 0.0;
  for (size_t i = 0; i < n; ++i)
  {
    double da = a[i] - meanA;
    double db = b[i] - meanB;
    num += da * db;
    varA += da * da;
    varB += db * db;
  }
  double denom = std::sqrt(varA * varB);
  return (denom > 0.0) ? num / denom : 0.0;
}

int findAlignmentLag(const std::vector<float>& a, const std::vector<float>& b, int maxLag)
{
  const size_t n = std::min(a.size(), b.size());
  int bestLag = 0;
  double bestR = -2.0;

  for (int lag = -maxLag; lag <= maxLag; ++lag)
  {
    const size_t skip = static_cast<size_t>(std::abs(lag));
    if (skip >= n)
      continue;
    const size_t len = n - skip;

    std::vector<float> aSlice, bSlice;
    if (lag >= 0)
    {
      aSlice.assign(a.begin(), a.begin() + static_cast<ptrdiff_t>(len));
      bSlice.assign(b.begin() + static_cast<ptrdiff_t>(lag),
                    b.begin() + static_cast<ptrdiff_t>(lag + len));
    }
    else
    {
      aSlice.assign(a.begin() + static_cast<ptrdiff_t>(-lag),
                    a.begin() + static_cast<ptrdiff_t>(-lag + len));
      bSlice.assign(b.begin(), b.begin() + static_cast<ptrdiff_t>(len));
    }

    double r = pearsonCorrelation(aSlice, bSlice);
    if (r > bestR)
    {
      bestR = r;
      bestLag = lag;
    }
  }
  return bestLag;
}

std::vector<float> processAndAlignMono(IRProcessor& processor, const std::vector<float>& dryInput,
                                       int blockSize)
{
  const size_t totalInputFrames = dryInput.size();
  std::vector<float> rawOutput;
  rawOutput.reserve(totalInputFrames + 2048);

  std::vector<float> inputBlock(blockSize, 0.0f);
  std::vector<float> outputBlock(blockSize, 0.0f);

  size_t framesConsumed = 0;
  while (framesConsumed < totalInputFrames)
  {
    size_t toCopy = std::min(static_cast<size_t>(blockSize), totalInputFrames - framesConsumed);
    std::fill(inputBlock.begin(), inputBlock.end(), 0.0f);
    std::copy(dryInput.begin() + static_cast<ptrdiff_t>(framesConsumed),
              dryInput.begin() + static_cast<ptrdiff_t>(framesConsumed + toCopy),
              inputBlock.begin());
    processor.processMono(inputBlock.data(), outputBlock.data(), blockSize);
    rawOutput.insert(rawOutput.end(), outputBlock.begin(), outputBlock.end());
    framesConsumed += blockSize;
  }

  const int latency = processor.getLatencySamples();

  size_t tailFlushed = 0;
  while (tailFlushed < static_cast<size_t>(latency))
  {
    std::fill(inputBlock.begin(), inputBlock.end(), 0.0f);
    processor.processMono(inputBlock.data(), outputBlock.data(), blockSize);
    rawOutput.insert(rawOutput.end(), outputBlock.begin(), outputBlock.end());
    tailFlushed += blockSize;
  }

  std::vector<float> aligned(
      rawOutput.begin() + latency,
      rawOutput.begin() + latency + static_cast<ptrdiff_t>(totalInputFrames));
  return aligned;
}

}  // namespace

class StereoComponentTest : public ::testing::Test
{
 protected:
  static constexpr int kBlockSize = 512;
};

// Mono processing with a stereo IR downmixes both IR channels to mono
// in the processor after convolution. The reference was generated by a tool
// that downmixes the raw IR to mono BEFORE convolving (and before minimum
// phase conversion). Because minimum phase conversion is nonlinear
// (minPhase(L) + minPhase(R) != minPhase(L+R)), the outputs differ somewhat.
//
// The r > 0.80 threshold reflects this known architectural limitation. A proper
// fix would require separate mono-downmixed IR storage in the loader so that
// minimum phase is applied to the downmix rather than per-channel.
TEST_F(StereoComponentTest, MonoProcessing_StereoIR_MatchesDownmixReference)
{
  unsigned int drySampleRate = 0;
  drwav_uint64 dryFrameCount = 0;
  std::vector<float> dryInput = loadWavMono(kDryPath, drySampleRate, dryFrameCount);
  ASSERT_FALSE(dryInput.empty());
  ASSERT_EQ(drySampleRate, 44100u);

  unsigned int controlSampleRate = 0;
  drwav_uint64 controlFrameCount = 0;
  std::vector<float> controlOutput =
      loadWavMono(kControlStereoPath, controlSampleRate, controlFrameCount);
  ASSERT_FALSE(controlOutput.empty());

  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string errMsg;
  ASSERT_TRUE(processor.loadImpulseResponse1(kStereoIRPath, errMsg))
      << "Stereo IR load failed: " << errMsg;

  std::vector<float> actualOutput = processAndAlignMono(processor, dryInput, kBlockSize);

  float peak = 0.0f;
  for (float s : actualOutput)
    peak = std::max(peak, std::abs(s));
  ASSERT_GT(peak, 1e-6f) << "Output is silent after stereo IR convolution";

  size_t compareLen = std::min(actualOutput.size(), controlOutput.size());
  actualOutput.resize(compareLen);
  controlOutput.resize(compareLen);
  normalizeToUnitPeak(actualOutput);
  normalizeToUnitPeak(controlOutput);

  constexpr int kMaxLagSamples = 200;
  const int lag = findAlignmentLag(actualOutput, controlOutput, kMaxLagSamples);

  std::vector<float> aAligned, bAligned;
  if (lag >= 0)
  {
    const size_t skip = static_cast<size_t>(lag);
    const size_t len = std::min(actualOutput.size(), controlOutput.size() - skip);
    aAligned.assign(actualOutput.begin(), actualOutput.begin() + static_cast<ptrdiff_t>(len));
    bAligned.assign(controlOutput.begin() + static_cast<ptrdiff_t>(lag),
                    controlOutput.begin() + static_cast<ptrdiff_t>(lag + len));
  }
  else
  {
    const size_t skip = static_cast<size_t>(-lag);
    const size_t len = std::min(actualOutput.size() - skip, controlOutput.size());
    aAligned.assign(actualOutput.begin() + static_cast<ptrdiff_t>(-lag),
                    actualOutput.begin() + static_cast<ptrdiff_t>(-lag + len));
    bAligned.assign(controlOutput.begin(), controlOutput.begin() + static_cast<ptrdiff_t>(len));
  }

  double r = pearsonCorrelation(aAligned, bAligned);

  std::cout << "[StereoHall] Latency: " << processor.getLatencySamples() << " samples\n";
  std::cout << "[StereoHall] Alignment lag: " << lag << " samples\n";
  std::cout << "[StereoHall] Pearson r (at best lag): " << r << "\n";
  EXPECT_GT(r, 0.80) << "Correlation too low (r=" << r << ", lag=" << lag << " samples). "
                     << "Stereo IR mono downmix or convolution may be broken.";
}

// Stereo processing with a stereo IR should produce different L and R outputs
// because the hall IR has distinct L/R channels.
TEST_F(StereoComponentTest, StereoProcessing_LRChannelsDiffer)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kStereoIRPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.25f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 10; ++b)
    processor.processStereo(input.data(), input.data(), outL.data(), outR.data(), kBlockSize);

  bool anyDifference = false;
  for (int i = 0; i < kBlockSize; ++i)
  {
    if (std::abs(outL[i] - outR[i]) > 1e-6f)
    {
      anyDifference = true;
      break;
    }
  }
  EXPECT_TRUE(anyDifference) << "L and R channels are identical despite using a stereo IR. "
                             << "The stereo IR channels may not be loaded correctly.";
}

// A mono IR should produce identical L/R output when processed in stereo mode
// with identical L/R input. This contrasts with the stereo IR test above.
TEST_F(StereoComponentTest, MonoIR_StereoProcessing_LRChannelsIdentical)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kMonoIRPath, err)) << err;
  EXPECT_EQ(processor.getNumIR1Channels(), 1);

  std::vector<Sample> input(kBlockSize, 0.25f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 10; ++b)
    processor.processStereo(input.data(), input.data(), outL.data(), outR.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
    EXPECT_FLOAT_EQ(outL[i], outR[i])
        << "L and R should be identical with a mono IR at sample " << i;
}

// Both L and R channels must be non-silent when processing through a stereo IR.
TEST_F(StereoComponentTest, StereoProcessing_BothChannelsNonSilent)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kStereoIRPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.25f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 10; ++b)
    processor.processStereo(input.data(), input.data(), outL.data(), outR.data(), kBlockSize);

  float peakL = 0.0f, peakR = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
  {
    peakL = std::max(peakL, std::abs(outL[i]));
    peakR = std::max(peakR, std::abs(outR[i]));
  }
  EXPECT_GT(peakL, 1e-6f) << "L channel is silent with stereo IR";
  EXPECT_GT(peakR, 1e-6f) << "R channel is silent with stereo IR";
}

// Mono-to-stereo processing with a stereo IR should produce non-silent,
// distinct L/R outputs from a mono input signal.
TEST_F(StereoComponentTest, MonoToStereo_StereoIR_ProducesStereoOutput)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kStereoIRPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.25f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 10; ++b)
    processor.processMonoToStereo(input.data(), outL.data(), outR.data(), kBlockSize);

  float peakL = 0.0f, peakR = 0.0f;
  bool anyDifference = false;
  for (int i = 0; i < kBlockSize; ++i)
  {
    peakL = std::max(peakL, std::abs(outL[i]));
    peakR = std::max(peakR, std::abs(outR[i]));
    if (std::abs(outL[i] - outR[i]) > 1e-6f)
      anyDifference = true;
  }
  EXPECT_GT(peakL, 1e-6f) << "L channel is silent in mono-to-stereo mode";
  EXPECT_GT(peakR, 1e-6f) << "R channel is silent in mono-to-stereo mode";
  EXPECT_TRUE(anyDifference) << "L and R are identical despite stereo IR — "
                             << "mono-to-stereo should preserve the IR's stereo image";
}

// Mono-to-stereo with a mono IR should produce identical L/R output.
TEST_F(StereoComponentTest, MonoToStereo_MonoIR_LRChannelsIdentical)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kMonoIRPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.25f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 10; ++b)
    processor.processMonoToStereo(input.data(), outL.data(), outR.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
    EXPECT_FLOAT_EQ(outL[i], outR[i])
        << "L and R should be identical with a mono IR at sample " << i;
}

// Mono-to-stereo passthrough: with no IRs loaded, the mono input should
// be copied to both L and R output channels.
TEST_F(StereoComponentTest, MonoToStereo_Passthrough_NoIRsLoaded)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);

  std::vector<Sample> input(kBlockSize);
  for (int i = 0; i < kBlockSize; ++i)
    input[i] = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);

  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  processor.processMonoToStereo(input.data(), outL.data(), outR.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
  {
    EXPECT_FLOAT_EQ(outL[i], input[i]) << "L passthrough mismatch at sample " << i;
    EXPECT_FLOAT_EQ(outR[i], input[i]) << "R passthrough mismatch at sample " << i;
  }
}

// Stereo IR loads successfully and reports 2 channels.
TEST_F(StereoComponentTest, StereoIR_LoadsWithCorrectChannelCount)
{
  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kStereoIRPath, err)) << err;
  EXPECT_EQ(processor.getNumIR1Channels(), 2);

  // ir1Loaded_ is set via the pending swap mechanism — trigger it.
  std::vector<Sample> buf(kBlockSize, 0.0f);
  processor.processMono(buf.data(), buf.data(), kBlockSize);
  EXPECT_TRUE(processor.isIR1Loaded());
}
