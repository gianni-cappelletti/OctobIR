#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <octob-ir-core/IRProcessor.hpp>

class OctobIRProcessor : public juce::AudioProcessor {
 public:
  OctobIRProcessor();
  ~OctobIRProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

  void processBlock(juce::AudioBuffer<float>& buffer,
                    juce::MidiBuffer& midiMessages) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  void loadImpulseResponse(const juce::String& filepath);
  juce::String getCurrentIRPath() const { return currentIRPath_; }

 private:
  octob::IRProcessor irProcessor_;
  juce::String currentIRPath_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OctobIRProcessor)
};
