#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
static const std::string kIrBPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_b.wav";

class SidechainAudioTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    processor.setSampleRate(44100.0);
    processor.setMaxBlockSize(kBlockSize);
    processor.setDynamicModeEnabled(true);
    processor.setSidechainEnabled(true);
    processor.setThreshold(-30.0f);
    processor.setRangeDb(20.0f);
    processor.setKneeWidthDb(0.0f);
    processor.setAttackTime(1.0f);
    processor.setReleaseTime(1.0f);

    std::string err;
    ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
    ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err)) << err;
  }

  static constexpr int kBlockSize = 512;
  IRProcessor processor;
};

// Loud sidechain + silent main input: blend should move toward +1.0 based on the
// sidechain level, not the main input level.
TEST_F(SidechainAudioTest, Mono_LoudSidechain_MovesBlendTowardWet)
{
  std::vector<Sample> mainInput(kBlockSize, 0.0f);
  std::vector<Sample> sidechain(kBlockSize, 0.9f);
  std::vector<Sample> output(kBlockSize, 0.0f);

  for (int b = 0; b < 200; ++b)
    processor.processMonoWithSidechain(mainInput.data(), sidechain.data(), output.data(),
                                       kBlockSize);

  float blend = processor.getCurrentBlend();
  EXPECT_GT(blend, 0.8f) << "Blend should respond to sidechain level, not main input. Got "
                         << blend;
}

// Silent sidechain + loud main input: blend should stay near -1.0.
// This verifies the sidechain input (not main) drives the envelope.
TEST_F(SidechainAudioTest, Mono_SilentSidechain_BlendStaysDry)
{
  std::vector<Sample> mainInput(kBlockSize, 0.9f);
  std::vector<Sample> sidechain(kBlockSize, 0.0f);
  std::vector<Sample> output(kBlockSize, 0.0f);

  for (int b = 0; b < 200; ++b)
    processor.processMonoWithSidechain(mainInput.data(), sidechain.data(), output.data(),
                                       kBlockSize);

  float blend = processor.getCurrentBlend();
  EXPECT_LT(blend, -0.8f) << "Blend should stay dry with silent sidechain. Got " << blend;
}

// Stereo sidechain: loud sidechain on both channels drives the blend.
TEST_F(SidechainAudioTest, Stereo_LoudSidechain_MovesBlendTowardWet)
{
  std::vector<Sample> mainL(kBlockSize, 0.0f);
  std::vector<Sample> mainR(kBlockSize, 0.0f);
  std::vector<Sample> scL(kBlockSize, 0.9f);
  std::vector<Sample> scR(kBlockSize, 0.9f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 200; ++b)
    processor.processStereoWithSidechain(mainL.data(), mainR.data(), scL.data(), scR.data(),
                                         outL.data(), outR.data(), kBlockSize);

  float blend = processor.getCurrentBlend();
  EXPECT_GT(blend, 0.8f) << "Stereo sidechain blend should respond to sidechain level. Got "
                         << blend;
}

// Stereo sidechain: silent sidechain should keep blend dry regardless of main input.
TEST_F(SidechainAudioTest, Stereo_SilentSidechain_BlendStaysDry)
{
  std::vector<Sample> mainL(kBlockSize, 0.9f);
  std::vector<Sample> mainR(kBlockSize, 0.9f);
  std::vector<Sample> scL(kBlockSize, 0.0f);
  std::vector<Sample> scR(kBlockSize, 0.0f);
  std::vector<Sample> outL(kBlockSize, 0.0f);
  std::vector<Sample> outR(kBlockSize, 0.0f);

  for (int b = 0; b < 200; ++b)
    processor.processStereoWithSidechain(mainL.data(), mainR.data(), scL.data(), scR.data(),
                                         outL.data(), outR.data(), kBlockSize);

  float blend = processor.getCurrentBlend();
  EXPECT_LT(blend, -0.8f) << "Stereo sidechain blend should stay dry with silent sidechain. Got "
                          << blend;
}
