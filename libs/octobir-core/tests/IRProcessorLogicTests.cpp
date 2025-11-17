#include <gtest/gtest.h>

#include <cmath>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

class IRProcessorLogicTest : public ::testing::Test {
 protected:
  IRProcessor processor;
};

TEST_F(IRProcessorLogicTest, BlendGainCalculation_FullyIR1) {
  processor.setBlend(-1.0f);
  float normalizedBlend = (processor.getBlend() + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

  EXPECT_NEAR(gain1, 1.0f, 0.001f);
  EXPECT_NEAR(gain2, 0.0f, 0.001f);
}

TEST_F(IRProcessorLogicTest, BlendGainCalculation_FullyIR2) {
  processor.setBlend(1.0f);
  float normalizedBlend = (processor.getBlend() + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

  EXPECT_NEAR(gain1, 0.0f, 0.001f);
  EXPECT_NEAR(gain2, 1.0f, 0.001f);
}

TEST_F(IRProcessorLogicTest, BlendGainCalculation_Center) {
  processor.setBlend(0.0f);
  float normalizedBlend = (processor.getBlend() + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

  EXPECT_NEAR(gain1, std::sqrt(0.5f), 0.001f);
  EXPECT_NEAR(gain2, std::sqrt(0.5f), 0.001f);
}

TEST_F(IRProcessorLogicTest, BlendGainCalculation_VerifyEqualPowerSum) {
  processor.setBlend(0.3f);
  float normalizedBlend = (processor.getBlend() + 1.0f) * 0.5f;
  float gain1 = std::sqrt(1.0f - normalizedBlend);
  float gain2 = std::sqrt(normalizedBlend);

  float powerSum = gain1 * gain1 + gain2 * gain2;
  EXPECT_NEAR(powerSum, 1.0f, 0.001f);
}
