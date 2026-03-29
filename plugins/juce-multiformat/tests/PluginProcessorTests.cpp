#include <gtest/gtest.h>

#include "PluginProcessor.h"

static const std::string kIrAPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_a.wav";
static const std::string kIrBPath = std::string(TEST_DATA_DIR) + "/INPUT_ir_b.wav";

class PluginProcessorTest : public ::testing::Test
{
 protected:
  OctobIRProcessor processor;
};

// IR path management

TEST_F(PluginProcessorTest, LoadIR1_UpdatesPath)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err)) << err;
  EXPECT_EQ(processor.getCurrentIR1Path(), juce::String(kIrAPath));
}

TEST_F(PluginProcessorTest, LoadIR2_UpdatesPath)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err)) << err;
  EXPECT_EQ(processor.getCurrentIR2Path(), juce::String(kIrBPath));
}

TEST_F(PluginProcessorTest, ClearIR1_ClearsPath)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  processor.clearImpulseResponse1();
  EXPECT_TRUE(processor.getCurrentIR1Path().isEmpty());
}

TEST_F(PluginProcessorTest, LoadIR1_AutoEnablesIRA)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  float enabled = processor.getAPVTS().getRawParameterValue("irAEnable")->load();
  EXPECT_GT(enabled, 0.5f);
}

// The plugin's reported latency must match the core's reported latency after the first
// process call. For IRs with pre-delay (peak offset > 0), this will be non-zero.
// The test IRs happen to have 0 peak offset, but the synchronization must still hold.
TEST_F(PluginProcessorTest, LoadIR1_PluginLatencyMatchesCore)
{
  processor.prepareToPlay(44100.0, 512);
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));

  juce::AudioBuffer<float> buf(2, 512);
  buf.clear();
  juce::MidiBuffer midi;
  processor.processBlock(buf, midi);

  EXPECT_EQ(processor.getLatencySamples(), processor.getIRProcessor().getLatencySamples());
}

// Swap: path exchange

TEST_F(PluginProcessorTest, SwapExchangesPaths)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err));

  processor.swapImpulseResponses();

  EXPECT_EQ(processor.getCurrentIR1Path(), juce::String(kIrBPath));
  EXPECT_EQ(processor.getCurrentIR2Path(), juce::String(kIrAPath));
}

// Swap: enable state preservation (slot-relative, not IR-relative)

TEST_F(PluginProcessorTest, SwapPreservesEnableStates_BothEnabled)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err));
  // Both slots auto-enabled by loading.

  processor.swapImpulseResponses();

  float aEnabled = processor.getAPVTS().getRawParameterValue("irAEnable")->load();
  float bEnabled = processor.getAPVTS().getRawParameterValue("irBEnable")->load();
  EXPECT_GT(aEnabled, 0.5f) << "Slot A should remain enabled after swap";
  EXPECT_GT(bEnabled, 0.5f) << "Slot B should remain enabled after swap";
}

TEST_F(PluginProcessorTest, SwapPreservesEnableStates_SlotADisabled)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err));

  if (auto* param = processor.getAPVTS().getParameter("irAEnable"))
    param->setValueNotifyingHost(0.0f);

  processor.swapImpulseResponses();

  // Swap preserves slot enable states: slot A was disabled, so it stays disabled
  // even though slot A now holds IR B.
  float aEnabled = processor.getAPVTS().getRawParameterValue("irAEnable")->load();
  float bEnabled = processor.getAPVTS().getRawParameterValue("irBEnable")->load();
  EXPECT_LT(aEnabled, 0.5f) << "Slot A should remain disabled after swap";
  EXPECT_GT(bEnabled, 0.5f) << "Slot B should remain enabled after swap";
}

TEST_F(PluginProcessorTest, SwapPreservesBlend)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err));

  auto* blendParam = processor.getAPVTS().getParameter("blend");
  ASSERT_NE(blendParam, nullptr);
  blendParam->setValueNotifyingHost(blendParam->convertTo0to1(0.3f));

  const float blendBefore = processor.getAPVTS().getRawParameterValue("blend")->load();

  processor.swapImpulseResponses();

  const float blendAfter = processor.getAPVTS().getRawParameterValue("blend")->load();
  EXPECT_NEAR(blendAfter, blendBefore, 0.01f);
}

TEST_F(PluginProcessorTest, SwapExchangesTrimGains)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  ASSERT_TRUE(processor.loadImpulseResponse2(kIrBPath, err));

  auto* trimAParam = processor.getAPVTS().getParameter("irATrimGain");
  auto* trimBParam = processor.getAPVTS().getParameter("irBTrimGain");
  ASSERT_NE(trimAParam, nullptr);
  ASSERT_NE(trimBParam, nullptr);
  trimAParam->setValueNotifyingHost(trimAParam->convertTo0to1(3.0f));
  trimBParam->setValueNotifyingHost(trimBParam->convertTo0to1(-3.0f));

  processor.swapImpulseResponses();

  EXPECT_NEAR(processor.getAPVTS().getRawParameterValue("irATrimGain")->load(), -3.0f, 0.01f);
  EXPECT_NEAR(processor.getAPVTS().getRawParameterValue("irBTrimGain")->load(), 3.0f, 0.01f);
}

TEST_F(PluginProcessorTest, SwapWithOneSlotEmpty_DoesNotCrash)
{
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));
  // Slot 2 left empty.

  ASSERT_NO_THROW(processor.swapImpulseResponses());

  // IR A moved from slot 1 to slot 2; slot 1 is now empty.
  EXPECT_TRUE(processor.getCurrentIR1Path().isEmpty());
  EXPECT_EQ(processor.getCurrentIR2Path(), juce::String(kIrAPath));
}

// State serialization

TEST_F(PluginProcessorTest, StateRoundTrip_Parameters)
{
  processor.prepareToPlay(44100.0, 512);
  juce::String err;
  ASSERT_TRUE(processor.loadImpulseResponse1(kIrAPath, err));

  auto* blendParam = processor.getAPVTS().getParameter("blend");
  ASSERT_NE(blendParam, nullptr);
  blendParam->setValueNotifyingHost(blendParam->convertTo0to1(0.25f));

  auto* gainParam = processor.getAPVTS().getParameter("outputGain");
  ASSERT_NE(gainParam, nullptr);
  gainParam->setValueNotifyingHost(gainParam->convertTo0to1(6.0f));

  juce::MemoryBlock state;
  processor.getStateInformation(state);

  OctobIRProcessor processor2;
  processor2.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

  const float blendOrig = processor.getAPVTS().getRawParameterValue("blend")->load();
  const float blend2 = processor2.getAPVTS().getRawParameterValue("blend")->load();
  EXPECT_NEAR(blend2, blendOrig, 0.01f);

  const float gainOrig = processor.getAPVTS().getRawParameterValue("outputGain")->load();
  const float gain2 = processor2.getAPVTS().getRawParameterValue("outputGain")->load();
  EXPECT_NEAR(gain2, gainOrig, 0.1f);
}
