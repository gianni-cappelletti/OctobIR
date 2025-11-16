#include "PluginEditor.h"

#include "PluginProcessor.h"

OctobIREditor::OctobIREditor(OctobIRProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
  addAndMakeVisible(ir1TitleLabel_);
  ir1TitleLabel_.setText("IR 1", juce::dontSendNotification);
  ir1TitleLabel_.setJustificationType(juce::Justification::centredLeft);
  ir1TitleLabel_.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  ir1TitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(loadButton_);
  loadButton_.setButtonText("Load IR 1");
  loadButton_.onClick = [this] { loadButtonClicked(); };

  addAndMakeVisible(irPathLabel_);
  irPathLabel_.setText(audioProcessor.getCurrentIRPath().isEmpty()
                           ? "No IR loaded"
                           : audioProcessor.getCurrentIRPath(),
                       juce::dontSendNotification);
  irPathLabel_.setJustificationType(juce::Justification::centred);
  irPathLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
  irPathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(ir2TitleLabel_);
  ir2TitleLabel_.setText("IR 2", juce::dontSendNotification);
  ir2TitleLabel_.setJustificationType(juce::Justification::centredLeft);
  ir2TitleLabel_.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  ir2TitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(loadButton2_);
  loadButton2_.setButtonText("Load IR 2");
  loadButton2_.onClick = [this] { loadButton2Clicked(); };

  addAndMakeVisible(ir2PathLabel_);
  ir2PathLabel_.setText(audioProcessor.getCurrentIR2Path().isEmpty()
                            ? "No IR loaded"
                            : audioProcessor.getCurrentIR2Path(),
                        juce::dontSendNotification);
  ir2PathLabel_.setJustificationType(juce::Justification::centred);
  ir2PathLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::darkgrey);
  ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(blendLabel_);
  blendLabel_.setText("Blend", juce::dontSendNotification);
  blendLabel_.setJustificationType(juce::Justification::centredLeft);
  blendLabel_.setFont(juce::FontOptions(16.0f, juce::Font::bold));
  blendLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(blendSlider_);
  blendSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  blendSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
  blendAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "blend", blendSlider_);

  addAndMakeVisible(latencyLabel_);
  latencyLabel_.setJustificationType(juce::Justification::centred);
  latencyLabel_.setColour(juce::Label::textColourId, juce::Colours::lightblue);
  updateLatencyDisplay();

  setSize(500, 420);
}

OctobIREditor::~OctobIREditor() {}

void OctobIREditor::paint(juce::Graphics& g) {
  g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  g.setColour(juce::Colours::white);
  g.setFont(20.0f);
  g.drawFittedText("OctobIR", getLocalBounds().removeFromTop(60), juce::Justification::centred, 1);
}

void OctobIREditor::resized() {
  auto bounds = getLocalBounds().reduced(20);

  bounds.removeFromTop(60);

  ir1TitleLabel_.setBounds(bounds.removeFromTop(25));
  bounds.removeFromTop(5);
  loadButton_.setBounds(bounds.removeFromTop(40).reduced(80, 0));
  bounds.removeFromTop(10);
  irPathLabel_.setBounds(bounds.removeFromTop(30));
  bounds.removeFromTop(20);

  ir2TitleLabel_.setBounds(bounds.removeFromTop(25));
  bounds.removeFromTop(5);
  loadButton2_.setBounds(bounds.removeFromTop(40).reduced(80, 0));
  bounds.removeFromTop(10);
  ir2PathLabel_.setBounds(bounds.removeFromTop(30));
  bounds.removeFromTop(20);

  blendLabel_.setBounds(bounds.removeFromTop(25));
  bounds.removeFromTop(5);
  blendSlider_.setBounds(bounds.removeFromTop(40).reduced(40, 0));
  bounds.removeFromTop(15);

  latencyLabel_.setBounds(bounds.removeFromTop(25));
}

void OctobIREditor::loadButtonClicked() {
  auto chooser = std::make_shared<juce::FileChooser>(
      "Select impulse response WAV file for IR 1",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav");

  auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
    auto file = fc.getResult();
    if (file.existsAsFile()) {
      juce::String error;
      bool success = audioProcessor.loadImpulseResponse(file.getFullPathName(), error);
      if (success) {
        irPathLabel_.setText(file.getFileName(), juce::dontSendNotification);
        irPathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
        updateLatencyDisplay();
      } else {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Failed to Load IR 1", error, "OK");
        irPathLabel_.setText("Failed to load IR", juce::dontSendNotification);
        irPathLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
      }
    }
  });
}

void OctobIREditor::loadButton2Clicked() {
  auto chooser = std::make_shared<juce::FileChooser>(
      "Select impulse response WAV file for IR 2",
      juce::File::getSpecialLocation(juce::File::userHomeDirectory), "*.wav");

  auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(flags, [this, chooser](const juce::FileChooser& fc) {
    auto file = fc.getResult();
    if (file.existsAsFile()) {
      juce::String error;
      bool success = audioProcessor.loadImpulseResponse2(file.getFullPathName(), error);
      if (success) {
        ir2PathLabel_.setText(file.getFileName(), juce::dontSendNotification);
        ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
      } else {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                               "Failed to Load IR 2", error, "OK");
        ir2PathLabel_.setText("Failed to load IR", juce::dontSendNotification);
        ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
      }
    }
  });
}

void OctobIREditor::updateLatencyDisplay() {
  int latency = audioProcessor.getLatencySamples();
  double sampleRate = audioProcessor.getSampleRate();
  if (sampleRate > 0) {
    double latencyMs = (latency / sampleRate) * 1000.0;
    latencyLabel_.setText(juce::String("Latency: ") + juce::String(latency) + " samples (" +
                              juce::String(latencyMs, 2) + " ms)",
                          juce::dontSendNotification);
  } else {
    latencyLabel_.setText(juce::String("Latency: ") + juce::String(latency) + " samples",
                          juce::dontSendNotification);
  }
}
