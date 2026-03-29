#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include "octobir-core/IRProcessor.hpp"

using namespace octob;

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
static const std::string kIrBPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_b.wav";

class DynamicModeAudioTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    processor.setSampleRate(44100.0);
    processor.setMaxBlockSize(kBlockSize);
    processor.setDynamicModeEnabled(true);
    processor.setThreshold(-30.0f);
    processor.setRangeDb(20.0f);
    processor.setKneeWidthDb(0.0f);
    processor.setAttackTime(1.0f);
    processor.setReleaseTime(1.0f);

    std::string err;
    ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
    ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err)) << err;
  }

  // Process enough blocks to let the envelope settle.
  void pumpBlocks(const Sample* input, size_t numBlocks)
  {
    std::vector<Sample> output(kBlockSize, 0.0f);
    for (size_t b = 0; b < numBlocks; ++b)
      processor.processMono(input, output.data(), kBlockSize);
  }

  static constexpr int kBlockSize = 512;
  IRProcessor processor;
};

// Feed a loud signal well above threshold+range: blend should converge toward +1.0.
TEST_F(DynamicModeAudioTest, LoudInput_BlendMovesTowardWet)
{
  std::vector<Sample> loud(kBlockSize, 0.9f);
  pumpBlocks(loud.data(), 200);

  float blend = processor.getCurrentBlend();
  EXPECT_GT(blend, 0.8f) << "Blend should be near +1.0 with loud input, got " << blend;
}

// Feed silence: blend should converge toward -1.0 (full dry).
TEST_F(DynamicModeAudioTest, SilentInput_BlendMovesTowardDry)
{
  // Prime with loud signal first to move blend away from initial state.
  std::vector<Sample> loud(kBlockSize, 0.9f);
  pumpBlocks(loud.data(), 200);
  ASSERT_GT(processor.getCurrentBlend(), 0.5f);

  std::vector<Sample> silence(kBlockSize, 0.0f);
  pumpBlocks(silence.data(), 200);

  float blend = processor.getCurrentBlend();
  EXPECT_LT(blend, -0.8f) << "Blend should be near -1.0 with silent input, got " << blend;
}

// Feed signal at the threshold midpoint: blend should land near 0.0.
TEST_F(DynamicModeAudioTest, MidLevelInput_BlendNearCenter)
{
  // threshold=-30, range=20, knee=0 => midpoint is at -20 dB => blend=0.0
  // -20 dB peak = 10^(-20/20) = 0.1
  const float midLevel = std::pow(10.0f, -20.0f / 20.0f);
  std::vector<Sample> mid(kBlockSize, midLevel);
  pumpBlocks(mid.data(), 200);

  float blend = processor.getCurrentBlend();
  EXPECT_NEAR(blend, 0.0f, 0.15f) << "Blend should be near 0.0 at threshold midpoint, got "
                                  << blend;
}

// Input level meter must update when dynamic mode is active and IRs are loaded.
TEST_F(DynamicModeAudioTest, InputLevelUpdatesWithDynamicMode)
{
  std::vector<Sample> loud(kBlockSize, 0.5f);
  std::vector<Sample> output(kBlockSize, 0.0f);
  processor.processMono(loud.data(), output.data(), kBlockSize);

  float level = processor.getCurrentInputLevel();
  // 0.5 peak => ~-6.02 dB
  EXPECT_NEAR(level, -6.02f, 0.5f) << "Input level should reflect the 0.5 peak signal";
}

// RMS detection mode: verify the level meter reports RMS (not peak) when mode=1.
TEST_F(DynamicModeAudioTest, RMSDetection_ReportsLowerThanPeak)
{
  processor.setDetectionMode(0);
  std::vector<Sample> sine(kBlockSize);
  for (int i = 0; i < kBlockSize; ++i)
    sine[i] = 0.5f * std::sin(2.0f * static_cast<float>(M_PI) * 1000.0f * i / 44100.0f);

  std::vector<Sample> output(kBlockSize, 0.0f);
  processor.processMono(sine.data(), output.data(), kBlockSize);
  float peakLevel = processor.getCurrentInputLevel();

  processor.setDetectionMode(1);
  processor.processMono(sine.data(), output.data(), kBlockSize);
  float rmsLevel = processor.getCurrentInputLevel();

  EXPECT_LT(rmsLevel, peakLevel) << "RMS level should be lower than peak level for a sine wave";
}
