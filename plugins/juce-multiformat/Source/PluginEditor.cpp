#include "PluginEditor.h"

#include "PluginProcessor.h"

OctobIREditor::OctobIREditor(OctobIRProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  addAndMakeVisible(loadButton_);
  loadButton_.setButtonText("Load IR");
  loadButton_.onClick = [this] { loadButtonClicked(); };

  addAndMakeVisible(irPathLabel_);
  irPathLabel_.setText(audioProcessor.getCurrentIRPath().isEmpty()
                           ? "No IR loaded"
                           : audioProcessor.getCurrentIRPath(),
                       juce::dontSendNotification);
  irPathLabel_.setJustificationType(juce::Justification::centred);
  irPathLabel_.setColour(juce::Label::backgroundColourId,
                         juce::Colours::darkgrey);
  irPathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  setSize(400, 300);
}

OctobIREditor::~OctobIREditor() {}

void OctobIREditor::paint(juce::Graphics& g) {
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(20.0f);
  g.drawFittedText("OctobIR", getLocalBounds().removeFromTop(60),
                   juce::Justification::centred, 1);
}

void OctobIREditor::resized() {
  auto bounds = getLocalBounds().reduced(20);

  bounds.removeFromTop(60);

  loadButton_.setBounds(bounds.removeFromTop(40).reduced(100, 0));
  bounds.removeFromTop(20);
  irPathLabel_.setBounds(bounds.removeFromTop(30));
}

void OctobIREditor::loadButtonClicked() {
  auto chooser = std::make_shared<juce::FileChooser>(
      "Select impulse response WAV file",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav");

  auto flags = juce::FileBrowserComponent::openMode |
               juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
    auto file = fc.getResult();
    if (file.existsAsFile()) {
      audioProcessor.loadImpulseResponse(file.getFullPathName());
      irPathLabel_.setText(file.getFileName(), juce::dontSendNotification);
    }
  });
}
