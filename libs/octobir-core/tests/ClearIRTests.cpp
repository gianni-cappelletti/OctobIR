#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
static const std::string kIrBPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_b.wav";

class ClearIRTest : public ::testing::Test
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

// After clearing IR1 (the only loaded IR), output must revert to passthrough.
TEST_F(ClearIRTest, ClearIR1_ReturnsToPassthrough)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);

  // Prime the engine so the pending IR is applied.
  processor.processMono(input.data(), output.data(), kBlockSize);

  processor.clearImpulseResponse1();

  // After clearing, the next process call should apply the pending clear.
  processor.processMono(input.data(), output.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
    EXPECT_FLOAT_EQ(output[i], input[i]) << "Expected passthrough at sample " << i;
}

// After clearing IR2 (the only loaded IR), output must revert to passthrough.
TEST_F(ClearIRTest, ClearIR2_ReturnsToPassthrough)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err)) << err;
  processor.setIRAEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);

  processor.processMono(input.data(), output.data(), kBlockSize);

  processor.clearImpulseResponse2();
  processor.processMono(input.data(), output.data(), kBlockSize);

  for (int i = 0; i < kBlockSize; ++i)
    EXPECT_FLOAT_EQ(output[i], input[i]) << "Expected passthrough at sample " << i;
}

// Clearing one IR in a dual-IR setup should not affect the other slot.
TEST_F(ClearIRTest, ClearIR1_DoesNotAffectIR2)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err)) << err;

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);
  processor.processMono(input.data(), output.data(), kBlockSize);

  processor.clearImpulseResponse1();
  processor.setIRAEnabled(false);

  processor.processMono(input.data(), output.data(), kBlockSize);

  EXPECT_TRUE(processor.isIR2Loaded()) << "IR2 should still be loaded after clearing IR1";

  // Output should be non-silent (IR2 is still active).
  float peak = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peak = std::max(peak, std::abs(output[i]));
  EXPECT_GT(peak, 1e-6f) << "Output should not be silent with IR2 still loaded";
}

// Clear then re-load: verifies no stale state remains from the previous IR.
TEST_F(ClearIRTest, ClearAndReload_ProducesOutput)
{
  std::string err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  processor.setIRBEnabled(false);

  std::vector<Sample> input(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);
  processor.processMono(input.data(), output.data(), kBlockSize);

  processor.clearImpulseResponse1();
  processor.processMono(input.data(), output.data(), kBlockSize);

  ASSERT_TRUE(processor.loadImpulseResponse1(kIrBPath, err)) << err;

  // Process several blocks to let the convolution engine produce output.
  for (int b = 0; b < 10; ++b)
    processor.processMono(input.data(), output.data(), kBlockSize);

  float peak = 0.0f;
  for (int i = 0; i < kBlockSize; ++i)
    peak = std::max(peak, std::abs(output[i]));
  EXPECT_GT(peak, 1e-6f) << "Output should be non-silent after clearing and reloading an IR";
}
