#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

class OctobIREditor : public juce::AudioProcessorEditor {
 public:
  explicit OctobIREditor(OctobIRProcessor&);
  ~OctobIREditor() override;

  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  OctobIRProcessor& audioProcessor;

  juce::Label ir1TitleLabel_;
  juce::TextButton loadButton_;
  juce::Label irPathLabel_;

  juce::Label ir2TitleLabel_;
  juce::TextButton loadButton2_;
  juce::Label ir2PathLabel_;

  juce::Label blendLabel_;
  juce::Slider blendSlider_;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> blendAttachment_;

  juce::Label latencyLabel_;

  void loadButtonClicked();
  void loadButton2Clicked();
  void updateLatencyDisplay();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIREditor)
};
