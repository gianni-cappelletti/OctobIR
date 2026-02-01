#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class VerticalMeter : public juce::Component, private juce::Timer
{
 public:
  VerticalMeter(const juce::String& name, float minValue, float maxValue);
  void setValue(float value);
  void setThresholdMarkers(float low, float high);
  void setBlendRangeMarkers(float min, float max);
  void setShowThresholds(bool show)
  {
    showThresholds_ = show;
    repaint();
  }
  void setShowBlendRange(bool show)
  {
    showBlendRange_ = show;
    repaint();
  }
  void paint(juce::Graphics& g) override;

 private:
  void timerCallback() override;

  juce::String name_;
  float minValue_;
  float maxValue_;
  float currentValue_ = 0.0f;
  float lowThreshold_ = 0.0f;
  float highThreshold_ = 0.0f;
  float minBlend_ = 0.0f;
  float maxBlend_ = 0.0f;
  bool showThresholds_ = false;
  bool showBlendRange_ = false;
};

class OctobIREditor : public juce::AudioProcessorEditor, private juce::Timer
{
 public:
  explicit OctobIREditor(OctobIRProcessor&);
  ~OctobIREditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  void timerCallback() override;

  OctobIRProcessor& audioProcessor;

  juce::Label ir1TitleLabel_;
  juce::TextButton loadButton_;
  juce::Label irPathLabel_;

  juce::Label ir2TitleLabel_;
  juce::TextButton loadButton2_;
  juce::Label ir2PathLabel_;

  VerticalMeter inputLevelMeter_;
  VerticalMeter blendMeter_;

  juce::ToggleButton dynamicModeButton_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dynamicModeAttachment_;

  juce::ToggleButton sidechainEnableButton_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sidechainEnableAttachment_;

  juce::TextButton swapIROrderButton_;

  juce::Label blendLabel_;
  juce::Slider blendSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment_;

  juce::Label thresholdLabel_;
  juce::Slider thresholdSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment_;

  juce::Label rangeDbLabel_;
  juce::Slider rangeDbSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rangeDbAttachment_;

  juce::Label kneeWidthDbLabel_;
  juce::Slider kneeWidthDbSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kneeWidthDbAttachment_;

  juce::Label detectionModeLabel_;
  juce::ComboBox detectionModeCombo_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> detectionModeAttachment_;

  juce::Label attackTimeLabel_;
  juce::Slider attackTimeSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackTimeAttachment_;

  juce::Label releaseTimeLabel_;
  juce::Slider releaseTimeSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseTimeAttachment_;

  juce::Label outputGainLabel_;
  juce::Slider outputGainSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment_;

  juce::Label latencyLabel_;

  void loadButtonClicked();
  void loadButton2Clicked();
  void swapIROrderClicked();
  void updateLatencyDisplay();
  void updateMeters();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIREditor)
};
