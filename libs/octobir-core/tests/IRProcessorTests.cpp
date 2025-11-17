#include <gtest/gtest.h>

#include <cmath>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

class IRProcessorTest : public ::testing::Test {
 protected:
  IRProcessor processor;
};

TEST_F(IRProcessorTest, ParameterClamping_Blend) {
  processor.setBlend(2.0f);
  EXPECT_FLOAT_EQ(processor.getBlend(), 1.0f);

  processor.setBlend(-2.0f);
  EXPECT_FLOAT_EQ(processor.getBlend(), -1.0f);

  processor.setBlend(0.5f);
  EXPECT_FLOAT_EQ(processor.getBlend(), 0.5f);
}

TEST_F(IRProcessorTest, ParameterClamping_LowBlend) {
  processor.setLowBlend(2.0f);
  EXPECT_FLOAT_EQ(processor.getLowBlend(), 1.0f);

  processor.setLowBlend(-2.0f);
  EXPECT_FLOAT_EQ(processor.getLowBlend(), -1.0f);

  processor.setLowBlend(-0.5f);
  EXPECT_FLOAT_EQ(processor.getLowBlend(), -0.5f);
}

TEST_F(IRProcessorTest, ParameterClamping_HighBlend) {
  processor.setHighBlend(2.0f);
  EXPECT_FLOAT_EQ(processor.getHighBlend(), 1.0f);

  processor.setHighBlend(-2.0f);
  EXPECT_FLOAT_EQ(processor.getHighBlend(), -1.0f);

  processor.setHighBlend(0.8f);
  EXPECT_FLOAT_EQ(processor.getHighBlend(), 0.8f);
}

TEST_F(IRProcessorTest, ParameterClamping_LowThreshold) {
  processor.setLowThreshold(10.0f);
  EXPECT_FLOAT_EQ(processor.getLowThreshold(), 0.0f);

  processor.setLowThreshold(-80.0f);
  EXPECT_FLOAT_EQ(processor.getLowThreshold(), -60.0f);

  processor.setLowThreshold(-30.0f);
  EXPECT_FLOAT_EQ(processor.getLowThreshold(), -30.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_HighThreshold) {
  processor.setHighThreshold(10.0f);
  EXPECT_FLOAT_EQ(processor.getHighThreshold(), 0.0f);

  processor.setHighThreshold(-80.0f);
  EXPECT_FLOAT_EQ(processor.getHighThreshold(), -60.0f);

  processor.setHighThreshold(-15.0f);
  EXPECT_FLOAT_EQ(processor.getHighThreshold(), -15.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_AttackTime) {
  processor.setAttackTime(2000.0f);
  EXPECT_FLOAT_EQ(processor.getAttackTime(), 1000.0f);

  processor.setAttackTime(0.5f);
  EXPECT_FLOAT_EQ(processor.getAttackTime(), 1.0f);

  processor.setAttackTime(100.0f);
  EXPECT_FLOAT_EQ(processor.getAttackTime(), 100.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_ReleaseTime) {
  processor.setReleaseTime(2000.0f);
  EXPECT_FLOAT_EQ(processor.getReleaseTime(), 1000.0f);

  processor.setReleaseTime(0.5f);
  EXPECT_FLOAT_EQ(processor.getReleaseTime(), 1.0f);

  processor.setReleaseTime(250.0f);
  EXPECT_FLOAT_EQ(processor.getReleaseTime(), 250.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_OutputGain) {
  processor.setOutputGain(30.0f);
  EXPECT_FLOAT_EQ(processor.getOutputGain(), 24.0f);

  processor.setOutputGain(-30.0f);
  EXPECT_FLOAT_EQ(processor.getOutputGain(), -24.0f);

  processor.setOutputGain(6.0f);
  EXPECT_FLOAT_EQ(processor.getOutputGain(), 6.0f);
}

TEST_F(IRProcessorTest, OutputGain_AppliesCorrectGain) {
  processor.setOutputGain(6.0f);
  constexpr float expectedGain = 1.9952623149688796f;

  constexpr size_t numFrames = 4;
  Sample input[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample output[numFrames] = {0.0f};

  processor.processMono(input, output, numFrames);

  for (size_t i = 0; i < numFrames; ++i) {
    EXPECT_NEAR(output[i], input[i] * expectedGain, 0.001f);
  }
}

TEST_F(IRProcessorTest, OutputGain_ZeroGainProducesSameOutput) {
  processor.setOutputGain(0.0f);

  constexpr size_t numFrames = 4;
  Sample input[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample output[numFrames] = {0.0f};

  processor.processMono(input, output, numFrames);

  for (size_t i = 0; i < numFrames; ++i) {
    EXPECT_FLOAT_EQ(output[i], input[i]);
  }
}

TEST_F(IRProcessorTest, StateManagement_InitialState) {
  EXPECT_FALSE(processor.isIRLoaded());
  EXPECT_FALSE(processor.isIR2Loaded());
  EXPECT_EQ(processor.getCurrentIRPath(), "");
  EXPECT_EQ(processor.getCurrentIR2Path(), "");
  EXPECT_FLOAT_EQ(processor.getBlend(), 0.0f);
  EXPECT_FALSE(processor.getDynamicModeEnabled());
  EXPECT_FALSE(processor.getSidechainEnabled());
}

TEST_F(IRProcessorTest, DynamicMode_DisabledSetsCurrentBlendToBlend) {
  processor.setBlend(0.5f);
  processor.setDynamicModeEnabled(true);
  processor.setDynamicModeEnabled(false);

  EXPECT_FLOAT_EQ(processor.getCurrentBlend(), 0.5f);
}

TEST_F(IRProcessorTest, Passthrough_NoIRsLoaded) {
  constexpr size_t numFrames = 4;
  Sample input[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample output[numFrames] = {0.0f};

  processor.processMono(input, output, numFrames);

  for (size_t i = 0; i < numFrames; ++i) {
    EXPECT_FLOAT_EQ(output[i], input[i]);
  }
}

TEST_F(IRProcessorTest, Passthrough_Stereo_NoIRsLoaded) {
  constexpr size_t numFrames = 4;
  Sample inputL[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample inputR[numFrames] = {0.5f, -0.25f, 0.125f, 0.0f};
  Sample outputL[numFrames] = {0.0f};
  Sample outputR[numFrames] = {0.0f};

  processor.processStereo(inputL, inputR, outputL, outputR, numFrames);

  for (size_t i = 0; i < numFrames; ++i) {
    EXPECT_FLOAT_EQ(outputL[i], inputL[i]);
    EXPECT_FLOAT_EQ(outputR[i], inputR[i]);
  }
}
