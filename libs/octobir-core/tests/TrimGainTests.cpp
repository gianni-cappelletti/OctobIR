#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
static const std::string kIrBPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_b.wav";

class TrimGainTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    processor.setSampleRate(44100.0);
    processor.setMaxBlockSize(kBlockSize);
  }

  float measurePeak(IRProcessor& proc, const std::vector<Sample>& input, int settleBlocks)
  {
    std::vector<Sample> output(kBlockSize, 0.0f);
    for (int b = 0; b < settleBlocks; ++b)
      proc.processMono(input.data(), output.data(), kBlockSize);

    float peak = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
      peak = std::max(peak, std::abs(output[i]));
    return peak;
  }

  static constexpr int kBlockSize = 512;
  IRProcessor processor;
};

// +6 dB trim on IR A should roughly double the convolved signal level.
TEST_F(TrimGainTest, IRA_TrimGainIncreasesOutput)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.25f);

  processor.setIRATrimGain(0.0f);
  float peakAt0dB = measurePeak(processor, input, 20);
  ASSERT_GT(peakAt0dB, 1e-6f) << "Baseline output is silent";

  // Reload to get a fresh convolution engine state.
  IRProcessor processor2;
  processor2.setSampleRate(44100.0);
  processor2.setMaxBlockSize(kBlockSize);
  ASSERT_TRUE(processor2.loadImpulseResponse1(kIrAPath, err)) << err;
  processor2.setIRBEnabled(false);
  processor2.setIRATrimGain(6.0f);

  float peakAt6dB = measurePeak(processor2, input, 20);

  float ratio = peakAt6dB / peakAt0dB;
  // +6 dB = factor of ~1.995. Allow some tolerance for convolution engine artifacts.
  EXPECT_NEAR(ratio, 2.0f, 0.15f) << "Expected ~2x increase with +6 dB trim, got ratio " << ratio;
}

// Negative trim should reduce output level.
TEST_F(TrimGainTest, IRA_NegativeTrimReducesOutput)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.25f);

  processor.setIRATrimGain(0.0f);
  float peakAt0dB = measurePeak(processor, input, 20);

  IRProcessor processor2;
  processor2.setSampleRate(44100.0);
  processor2.setMaxBlockSize(kBlockSize);
  ASSERT_TRUE(processor2.loadImpulseResponse1(kIrAPath, err)) << err;
  processor2.setIRBEnabled(false);
  processor2.setIRATrimGain(-6.0f);

  float peakAtNeg6dB = measurePeak(processor2, input, 20);

  EXPECT_LT(peakAtNeg6dB, peakAt0dB) << "Output should decrease with negative trim";
  float ratio = peakAtNeg6dB / peakAt0dB;
  EXPECT_NEAR(ratio, 0.5f, 0.1f) << "Expected ~0.5x with -6 dB trim, got ratio " << ratio;
}

// IR B trim should independently affect the IR B slot.
TEST_F(TrimGainTest, IRB_TrimGainIncreasesOutput)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err)) << err;
  processor.setIRAEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.25f);

  processor.setIRBTrimGain(0.0f);
  float peakAt0dB = measurePeak(processor, input, 20);
  ASSERT_GT(peakAt0dB, 1e-6f) << "Baseline output is silent";

  IRProcessor processor2;
  processor2.setSampleRate(44100.0);
  processor2.setMaxBlockSize(kBlockSize);
  ASSERT_TRUE(processor2.loadImpulseResponse2(kIrBPath, err)) << err;
  processor2.setIRAEnabled(false);
  processor2.setIRBTrimGain(6.0f);

  float peakAt6dB = measurePeak(processor2, input, 20);

  float ratio = peakAt6dB / peakAt0dB;
  EXPECT_NEAR(ratio, 2.0f, 0.15f) << "Expected ~2x increase with +6 dB trim on IR B, got ratio "
                                  << ratio;
}

// In dual-IR mode, trim gains should independently affect each slot's contribution.
TEST_F(TrimGainTest, DualIR_TrimGainsAreIndependent)
{
  std::string err;
  IRProcessor procA;
  procA.setSampleRate(44100.0);
  procA.setMaxBlockSize(kBlockSize);
  ASSERT_TRUE(procA.loadImpulseResponse1(kIrAPath, err)) << err;
  ASSERT_TRUE(procA.loadImpulseResponse2(kIrBPath, err)) << err;
  procA.setIRATrimGain(6.0f);
  procA.setIRBTrimGain(0.0f);

  IRProcessor procB;
  procB.setSampleRate(44100.0);
  procB.setMaxBlockSize(kBlockSize);
  ASSERT_TRUE(procB.loadImpulseResponse1(kIrAPath, err)) << err;
  ASSERT_TRUE(procB.loadImpulseResponse2(kIrBPath, err)) << err;
  procB.setIRATrimGain(0.0f);
  procB.setIRBTrimGain(6.0f);

  std::vector<Sample> input(kBlockSize, 0.25f);
  float peakBoostA = measurePeak(procA, input, 20);
  float peakBoostB = measurePeak(procB, input, 20);

  // The peaks should differ because the IRs have different frequency content.
  // The key assertion is that neither is zero.
  EXPECT_GT(peakBoostA, 1e-6f);
  EXPECT_GT(peakBoostB, 1e-6f);
  EXPECT_NE(peakBoostA, peakBoostB) << "Boosting different IR slots should produce different peaks";
}
