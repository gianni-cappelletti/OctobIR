#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

class IRProcessorTest : public ::testing::Test
{
 protected:
  IRProcessor processor;
};

TEST_F(IRProcessorTest, ParameterClamping_Blend)
{
  processor.setBlend(2.0f);
  EXPECT_FLOAT_EQ(processor.getBlend(), 1.0f);

  processor.setBlend(-2.0f);
  EXPECT_FLOAT_EQ(processor.getBlend(), -1.0f);

  processor.setBlend(0.5f);
  EXPECT_FLOAT_EQ(processor.getBlend(), 0.5f);
}

TEST_F(IRProcessorTest, ParameterClamping_Threshold)
{
  processor.setThreshold(10.0f);
  EXPECT_FLOAT_EQ(processor.getThreshold(), 0.0f);

  processor.setThreshold(-80.0f);
  EXPECT_FLOAT_EQ(processor.getThreshold(), -60.0f);

  processor.setThreshold(-30.0f);
  EXPECT_FLOAT_EQ(processor.getThreshold(), -30.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_RangeDb)
{
  processor.setRangeDb(100.0f);
  EXPECT_FLOAT_EQ(processor.getRangeDb(), 60.0f);

  processor.setRangeDb(0.5f);
  EXPECT_FLOAT_EQ(processor.getRangeDb(), 1.0f);

  processor.setRangeDb(20.0f);
  EXPECT_FLOAT_EQ(processor.getRangeDb(), 20.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_KneeWidthDb)
{
  processor.setKneeWidthDb(30.0f);
  EXPECT_FLOAT_EQ(processor.getKneeWidthDb(), 20.0f);

  processor.setKneeWidthDb(-5.0f);
  EXPECT_FLOAT_EQ(processor.getKneeWidthDb(), 0.0f);

  processor.setKneeWidthDb(10.0f);
  EXPECT_FLOAT_EQ(processor.getKneeWidthDb(), 10.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_AttackTime)
{
  processor.setAttackTime(2000.0f);
  EXPECT_FLOAT_EQ(processor.getAttackTime(), 500.0f);

  processor.setAttackTime(0.5f);
  EXPECT_FLOAT_EQ(processor.getAttackTime(), 1.0f);

  processor.setAttackTime(100.0f);
  EXPECT_FLOAT_EQ(processor.getAttackTime(), 100.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_ReleaseTime)
{
  processor.setReleaseTime(2000.0f);
  EXPECT_FLOAT_EQ(processor.getReleaseTime(), 1000.0f);

  processor.setReleaseTime(0.5f);
  EXPECT_FLOAT_EQ(processor.getReleaseTime(), 1.0f);

  processor.setReleaseTime(250.0f);
  EXPECT_FLOAT_EQ(processor.getReleaseTime(), 250.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_IRATrimGain)
{
  processor.setIRATrimGain(20.0f);
  EXPECT_FLOAT_EQ(processor.getIRATrimGain(), 12.0f);

  processor.setIRATrimGain(-20.0f);
  EXPECT_FLOAT_EQ(processor.getIRATrimGain(), -12.0f);

  processor.setIRATrimGain(6.0f);
  EXPECT_FLOAT_EQ(processor.getIRATrimGain(), 6.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_IRBTrimGain)
{
  processor.setIRBTrimGain(20.0f);
  EXPECT_FLOAT_EQ(processor.getIRBTrimGain(), 12.0f);

  processor.setIRBTrimGain(-20.0f);
  EXPECT_FLOAT_EQ(processor.getIRBTrimGain(), -12.0f);

  processor.setIRBTrimGain(-3.0f);
  EXPECT_FLOAT_EQ(processor.getIRBTrimGain(), -3.0f);
}

TEST_F(IRProcessorTest, ParameterClamping_OutputGain)
{
  processor.setOutputGain(30.0f);
  EXPECT_FLOAT_EQ(processor.getOutputGain(), 24.0f);

  processor.setOutputGain(-30.0f);
  EXPECT_FLOAT_EQ(processor.getOutputGain(), -24.0f);

  processor.setOutputGain(6.0f);
  EXPECT_FLOAT_EQ(processor.getOutputGain(), 6.0f);
}

TEST_F(IRProcessorTest, OutputGain_AppliesCorrectGain)
{
  processor.setOutputGain(6.0f);
  constexpr float expectedGain = 1.9952623149688796f;

  constexpr size_t numFrames = 4;
  Sample input[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample output[numFrames] = {0.0f};

  processor.processMono(input, output, numFrames);

  for (size_t i = 0; i < numFrames; ++i)
  {
    EXPECT_NEAR(output[i], input[i] * expectedGain, 0.001f);
  }
}

TEST_F(IRProcessorTest, OutputGain_ZeroGainProducesSameOutput)
{
  processor.setOutputGain(0.0f);

  constexpr size_t numFrames = 4;
  Sample input[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample output[numFrames] = {0.0f};

  processor.processMono(input, output, numFrames);

  for (size_t i = 0; i < numFrames; ++i)
  {
    EXPECT_FLOAT_EQ(output[i], input[i]);
  }
}

TEST_F(IRProcessorTest, StateManagement_InitialState)
{
  EXPECT_FALSE(processor.isIR1Loaded());
  EXPECT_FALSE(processor.isIR2Loaded());
  EXPECT_EQ(processor.getCurrentIR1Path(), "");
  EXPECT_EQ(processor.getCurrentIR2Path(), "");
  EXPECT_FLOAT_EQ(processor.getBlend(), 0.0f);
  EXPECT_FALSE(processor.getDynamicModeEnabled());
  EXPECT_FALSE(processor.getSidechainEnabled());
}

TEST_F(IRProcessorTest, DynamicMode_DisabledSetsCurrentBlendToBlend)
{
  processor.setBlend(0.5f);
  processor.setDynamicModeEnabled(true);
  processor.setDynamicModeEnabled(false);

  EXPECT_FLOAT_EQ(processor.getCurrentBlend(), 0.5f);
}

TEST_F(IRProcessorTest, Passthrough_NoIRsLoaded)
{
  constexpr size_t numFrames = 4;
  Sample input[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample output[numFrames] = {0.0f};

  processor.processMono(input, output, numFrames);

  for (size_t i = 0; i < numFrames; ++i)
  {
    EXPECT_FLOAT_EQ(output[i], input[i]);
  }
}

TEST_F(IRProcessorTest, Passthrough_Stereo_NoIRsLoaded)
{
  constexpr size_t numFrames = 4;
  Sample inputL[numFrames] = {1.0f, -0.5f, 0.25f, 0.0f};
  Sample inputR[numFrames] = {0.5f, -0.25f, 0.125f, 0.0f};
  Sample outputL[numFrames] = {0.0f};
  Sample outputR[numFrames] = {0.0f};

  processor.processStereo(inputL, inputR, outputL, outputR, numFrames);

  for (size_t i = 0; i < numFrames; ++i)
  {
    EXPECT_FLOAT_EQ(outputL[i], inputL[i]);
    EXPECT_FLOAT_EQ(outputR[i], inputR[i]);
  }
}

struct DynamicBlendCase
{
  float thresholdDb;
  float rangeDb;
  float kneeWidthDb;
  float inputLevelDb;
  float expectedBlend;
  std::string description;
};

class DynamicBlendTest : public ::testing::TestWithParam<DynamicBlendCase>
{
 protected:
  IRProcessor processor;
};

TEST_P(DynamicBlendTest, ReturnsExpectedBlend)
{
  const auto& tc = GetParam();
  processor.setThreshold(tc.thresholdDb);
  processor.setRangeDb(tc.rangeDb);
  processor.setKneeWidthDb(tc.kneeWidthDb);
  EXPECT_NEAR(processor.calculateDynamicBlend(tc.inputLevelDb), tc.expectedBlend, 1e-5f);
}

INSTANTIATE_TEST_SUITE_P(
    AllBranches, DynamicBlendTest,
    ::testing::Values(
        // Branch 1: below knee — full dry
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -40.0f, -1.0f, "BelowKnee"},
        // Branch 1: exactly at kneeStart boundary
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -35.0f, -1.0f, "AtKneeStart"},
        // Branch 2: above range — full wet
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -5.0f, 1.0f, "AboveRange"},
        // Branch 2: exactly at range end
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -10.0f, 1.0f, "AtRangeEnd"},
        // Branch 3: knee midpoint — quadratic ramp
        // kneeStart=-35, kneeEnd=-25; input=-30: overshoot=0.5, blendPos=0.125, blend=-0.75
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -30.0f, -0.75f, "KneeMidpoint"},
        // Branch 4: linear region midpoint
        // aboveKnee=7.5, effectiveRange=15, blendPos=0.75, blend=0.5
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -17.5f, 0.5f, "LinearMidpoint"},
        // Branch 4: exactly at kneeEnd — blend starts at kneeContribution (0.5 -> return 0.0)
        DynamicBlendCase{-30.0f, 20.0f, 10.0f, -25.0f, 0.0f, "AtKneeEnd"},
        // Hard knee (kneeWidthDb=0): below threshold
        DynamicBlendCase{-30.0f, 20.0f, 0.0f, -35.0f, -1.0f, "HardKneeBelowThreshold"},
        // Hard knee: midpoint — kneeContribution=0, blendPos=0.5, blend=0.0
        DynamicBlendCase{-30.0f, 20.0f, 0.0f, -20.0f, 0.0f, "HardKneeMidpoint"},
        // C7 fix: range endpoint falls inside knee — blend must reach 1.0
        // threshold=-30, range=1, knee=20: rangeEnd=-29 is inside knee (-40 to -20)
        // input=-28 >= -29 triggers branch 2
        DynamicBlendCase{-30.0f, 1.0f, 20.0f, -28.0f, 1.0f, "RangeInsideKnee"}),
    [](const ::testing::TestParamInfo<DynamicBlendCase>& info) { return info.param.description; });
