#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";

class SampleRateChangeTest : public ::testing::Test
{
 protected:
  static constexpr int kBlockSize = 512;
  IRProcessor processor;
};

// Changing sample rate after loading an IR should not produce silence or crash.
// The IR is re-initialized at the new rate internally.
TEST_F(SampleRateChangeTest, ChangeSampleRate_StillProducesOutput)
{
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;

  // Prime at 44100 Hz.
  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);
  for (int b = 0; b < 5; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  // Switch to 96000 Hz.
  processor.setSampleRate(96000.0);

  // Process several blocks at the new rate.
  for (int b = 0; b < 10; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  float peak = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peak = std::max(peak, std::abs(output[i]));
  EXPECT_GT(peak, 1e-6f) << "Output is silent after changing sample rate from 44100 to 96000";
}

// Switching to a lower sample rate should also work.
TEST_F(SampleRateChangeTest, DownsampleRate_StillProducesOutput)
{
  processor.setSampleRate(96000.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);
  for (int b = 0; b < 5; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  processor.setSampleRate(44100.0);

  for (int b = 0; b < 10; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  float peak = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peak = std::max(peak, std::abs(output[i]));
  EXPECT_GT(peak, 1e-6f) << "Output is silent after changing sample rate from 96000 to 44100";
}

// Latency should be reported as non-negative after a sample rate change with a loaded IR.
TEST_F(SampleRateChangeTest, LatencyRemainsValid)
{
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;

  // Trigger the pending IR swap.
  std::vector<Sample> input(kBlockSize, 0.0f);
  std::vector<Sample> output(kBlockSize, 0.0f);
  processor.processMono(input.data(), output.data(), kBlockSize);

  int latencyBefore = processor.getLatencySamples();
  EXPECT_GE(latencyBefore, 0);

  processor.setSampleRate(48000.0);
  processor.processMono(input.data(), output.data(), kBlockSize);

  int latencyAfter = processor.getLatencySamples();
  EXPECT_GE(latencyAfter, 0) << "Latency should be non-negative after sample rate change";
}

// Setting the same sample rate should be a no-op (no re-initialization).
TEST_F(SampleRateChangeTest, SameSampleRate_NoReinitialize)
{
  processor.setSampleRate(44100.0);
  processor.setMaxBlockSize(kBlockSize);
  processor.setIRBEnabled(false);

  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output1(kBlockSize, 0.0f);
  for (int b = 0; b < 10; ++b)
    processor.processMono(input.data(), output1.data(), kBlockSize);

  float peakBefore = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peakBefore = std::max(peakBefore, std::abs(output1[i]));

  // Set same rate - should not disrupt the convolution state.
  processor.setSampleRate(44100.0);

  std::vector<Sample> output2(kBlockSize, 0.0f);
  processor.processMono(input.data(), output2.data(), kBlockSize);

  float peakAfter = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peakAfter = std::max(peakAfter, std::abs(output2[i]));

  EXPECT_NEAR(peakAfter, peakBefore, peakBefore * 0.01f)
      << "Setting the same sample rate should not alter output";
}
