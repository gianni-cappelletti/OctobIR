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

  juce::TextButton loadButton_;
  juce::Label irPathLabel_;
  juce::Label latencyLabel_;

  void loadButtonClicked();
  void updateLatencyDisplay();

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIREditor)
};
