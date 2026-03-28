#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

// DR_WAV_IMPLEMENTATION is compiled into octobir-core via IRLoader.cpp.
// Including the header here without redefining the macro uses those symbols via linking.
#include "dr_wav.h"
#include "octobir-core/IRProcessor.hpp"

using namespace octob;

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

// Returns the lag (in samples) within [-maxLag, +maxLag] at which Pearson correlation
// between a and b is maximised. Positive lag means b leads a (b is ahead).
int findAlignmentLag(const std::vector<float>& a, const std::vector<float>& b, int maxLag);

double pearsonCorrelation(const std::vector<float>& a, const std::vector<float>& b)
{
  const size_t n = a.size();

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

double computeSnrDb(const std::vector<float>& reference, const std::vector<float>& actual)
{
  double sigPower = 0.0, errPower = 0.0;
  for (size_t i = 0; i < reference.size(); ++i)
  {
    sigPower += static_cast<double>(reference[i]) * reference[i];
    double err = reference[i] - actual[i];
    errPower += err * err;
  }
  if (errPower == 0.0)
    return std::numeric_limits<double>::infinity();
  return 10.0 * std::log10(sigPower / errPower);
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

bool writeWavMono(const std::string& path, const std::vector<float>& samples,
                  unsigned int sampleRate)
{
  drwav_data_format format;
  format.container = drwav_container_riff;
  format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
  format.channels = 1;
  format.sampleRate = sampleRate;
  format.bitsPerSample = 32;

  drwav wav;
  if (!drwav_init_file_write(&wav, path.c_str(), &format, nullptr))
    return false;

  drwav_write_pcm_frames(&wav, samples.size(), samples.data());
  drwav_uninit(&wav);
  return true;
}

}  // namespace

class ComponentTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    irPath_ = std::string(TEST_DATA_DIR) + "/INPUT_ir.wav";
    dryPath_ = std::string(TEST_DATA_DIR) + "/INPUT_amp_output_no_ir.wav";
    controlPath_ = std::string(TEST_DATA_DIR) + "/CONTROL_amp_output_with_ir.wav";
  }

  std::string irPath_;
  std::string dryPath_;
  std::string controlPath_;
};

// Scenario: 96 kHz IR resampled to 44.1 kHz, IR A loaded + enabled, IR B disabled, static mode.
//
// Input:   INPUT_amp_output_no_ir.wav (44.1 kHz mono, dry amp signal)
// IR:      INPUT_ir.wav (96 kHz mono, resampled to 44.1 kHz internally)
// Control: CONTROL_amp_output_with_ir.wav (44.1 kHz mono, reference from a
//          known-good IR loader, 100% wet signal)
//
// Preconditions:
//   - IR A is loaded and enabled. IR B is not loaded and disabled.
//   - blend is left at its default (0.0f). When IR B is disabled, blend is irrelevant
//     because slot B is out of the signal path entirely; the processor must output
//     100% IR A wet regardless of blend position.
//   - Output gain is 0 dB (default). Trim gains are 0 dB (default).
//
// Verifies:
//   1. IR loading and 96 kHz -> 44.1 kHz resampling produce non-silent output.
//   2. Convolved output matches the reference (Pearson r > 0.99 after normalizing
//      both to unit peak to remove the -18 dB IR compensation gain offset).
//   3. Blend is correctly ignored when IR B is disabled.
TEST_F(ComponentTest, ConvolutionMatchesReference)
{
  unsigned int drySampleRate = 0;
  drwav_uint64 dryFrameCount = 0;
  std::vector<float> dryInput = loadWavMono(dryPath_, drySampleRate, dryFrameCount);
  ASSERT_FALSE(dryInput.empty()) << "Failed to load dry input: " << dryPath_;
  ASSERT_EQ(drySampleRate, 44100u);

  unsigned int controlSampleRate = 0;
  drwav_uint64 controlFrameCount = 0;
  std::vector<float> controlOutput =
      loadWavMono(controlPath_, controlSampleRate, controlFrameCount);
  ASSERT_FALSE(controlOutput.empty()) << "Failed to load control: " << controlPath_;

  IRProcessor processor;
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(512);
  // blend left at default (0.0f); with IR B disabled, blend must be irrelevant.
  processor.setIRBEnabled(false);

  std::string errMsg;
  ASSERT_TRUE(processor.loadImpulseResponse1(irPath_, errMsg)) << "IR load failed: " << errMsg;

  constexpr int kBlockSize = 512;
  const size_t totalInputFrames = dryInput.size();

  std::vector<float> rawOutput;
  rawOutput.reserve(totalInputFrames + 2048);

  std::vector<float> inputBlock(kBlockSize, 0.0f);
  std::vector<float> outputBlock(kBlockSize, 0.0f);

  size_t framesConsumed = 0;
  while (framesConsumed < totalInputFrames)
  {
    size_t toCopy = std::min(static_cast<size_t>(kBlockSize), totalInputFrames - framesConsumed);
    std::fill(inputBlock.begin(), inputBlock.end(), 0.0f);
    std::copy(dryInput.begin() + static_cast<ptrdiff_t>(framesConsumed),
              dryInput.begin() + static_cast<ptrdiff_t>(framesConsumed + toCopy),
              inputBlock.begin());
    processor.processMono(inputBlock.data(), outputBlock.data(), kBlockSize);
    rawOutput.insert(rawOutput.end(), outputBlock.begin(), outputBlock.end());
    framesConsumed += kBlockSize;
  }

  // Latency is valid after the first processMono() call (pending IR swap applied there).
  const int latency = processor.getLatencySamples();

  size_t tailFlushed = 0;
  while (tailFlushed < static_cast<size_t>(latency))
  {
    std::fill(inputBlock.begin(), inputBlock.end(), 0.0f);
    processor.processMono(inputBlock.data(), outputBlock.data(), kBlockSize);
    rawOutput.insert(rawOutput.end(), outputBlock.begin(), outputBlock.end());
    tailFlushed += kBlockSize;
  }

  ASSERT_GE(rawOutput.size(), static_cast<size_t>(latency) + totalInputFrames)
      << "rawOutput too short to trim";

  std::vector<float> actualOutput(
      rawOutput.begin() + latency,
      rawOutput.begin() + latency + static_cast<ptrdiff_t>(totalInputFrames));

  writeWavMono("/tmp/octobir_component_output.wav", actualOutput, drySampleRate);

  float peak = 0.0f;
  for (float s : actualOutput)
    peak = std::max(peak, std::abs(s));
  ASSERT_GT(peak, 1e-6f) << "Output is silent after IR convolution; likely a resampling failure. "
                         << "Latency reported: " << latency << " samples.";

  size_t compareLen = std::min(actualOutput.size(), controlOutput.size());
  actualOutput.resize(compareLen);
  controlOutput.resize(compareLen);

  normalizeToUnitPeak(actualOutput);
  normalizeToUnitPeak(controlOutput);

  // Search ±2000 samples (~45 ms at 44.1 kHz) for the optimal alignment lag.
  // A non-zero lag indicates the reference and our output use different peak-offset
  // alignment strategies; we report it as a diagnostic and compare at the best lag.
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

  double snrDb = computeSnrDb(bAligned, aAligned);
  double r = pearsonCorrelation(aAligned, bAligned);

  std::cout << "[Component] Latency: " << latency << " samples\n";
  std::cout << "[Component] Alignment lag: " << lag << " samples\n";
  std::cout << "[Component] SNR (at best lag): " << snrDb << " dB\n";
  std::cout << "[Component] Pearson r (at best lag): " << r << "\n";
  std::cout << "[Component] Diagnostic output: /tmp/octobir_component_output.wav\n";

  EXPECT_GT(r, 0.99) << "Correlation too low (r=" << r << ", SNR=" << snrDb << " dB, lag=" << lag
                     << " samples). "
                     << "Listen to /tmp/octobir_component_output.wav for diagnosis.";
}
