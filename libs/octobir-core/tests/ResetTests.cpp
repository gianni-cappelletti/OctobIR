#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";

class ResetTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    processor.setSampleRate(44100.0);
    processor.setMaxBlockSize(kBlockSize);
  }

  static constexpr int kBlockSize = 512;
  IRProcessor processor;
};

// After processing non-silent audio through a loaded IR, reset() must flush the
// convolution engine's internal state so that feeding silence produces immediate
// silence (no stale convolution tail leaking through).
TEST_F(ResetTest, Reset_FlushesConvolutionTail)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> loud(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);

  // Process several blocks of non-silent audio to build up convolution state.
  for (int b = 0; b < 10; ++b)
    processor.processMono(loud.data(), output.data(), kBlockSize);

  // Verify output was non-silent before reset.
  float peakBefore = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peakBefore = std::max(peakBefore, std::abs(output[i]));
  ASSERT_GT(peakBefore, 1e-6f) << "Output should be non-silent before reset";

  processor.reset();

  // Feed silence after reset.
  std::vector<Sample> silence(kBlockSize, 0.0f);
  processor.processMono(silence.data(), output.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
    EXPECT_NEAR(output[i], 0.0f, 1e-6f)
        << "Stale convolution tail at sample " << i << " after reset";
}

// Reset should not unload IRs. After reset, processing non-silent audio should
// still produce convolved output.
TEST_F(ResetTest, Reset_DoesNotUnloadIR)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);

  // Prime the engine.
  for (int b = 0; b < 5; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  processor.reset();

  EXPECT_TRUE(processor.isIR1Loaded()) << "IR1 should still be loaded after reset";

  // Process enough blocks for the convolution engine to produce output again.
  for (int b = 0; b < 10; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  float peak = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peak = std::max(peak, std::abs(output[i]));
  EXPECT_GT(peak, 1e-6f) << "Output should be non-silent after reset with IR still loaded";
}

// Reset in stereo mode should flush both channels.
TEST_F(ResetTest, Reset_FlushesConvolutionTail_Stereo)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> loud(kBlockSize, 0.5f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 10; ++b)
    processor.processStereo(loud.data(), loud.data(), outL.data(), outR.data(), kBlockSize);

  processor.reset();

  std::vector<Sample> silence(kBlockSize, 0.0f);
  processor.processStereo(silence.data(), silence.data(), outL.data(), outR.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
  {
    EXPECT_NEAR(outL[i], 0.0f, 1e-6f) << "Stale L tail at sample " << i;
    EXPECT_NEAR(outR[i], 0.0f, 1e-6f) << "Stale R tail at sample " << i;
  }
}
