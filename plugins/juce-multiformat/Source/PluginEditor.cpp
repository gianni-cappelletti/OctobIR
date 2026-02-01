#include "PluginEditor.h"

#include "PluginProcessor.h"

VerticalMeter::VerticalMeter(const juce::String& name, float minValue, float maxValue)
    : name_(name), minValue_(minValue), maxValue_(maxValue) {
  startTimerHz(30);
}

void VerticalMeter::setValue(float value) {
  currentValue_ = value;
}

void VerticalMeter::setThresholdMarkers(float low, float high) {
  lowThreshold_ = low;
  highThreshold_ = high;
}

void VerticalMeter::setBlendRangeMarkers(float min, float max) {
  minBlend_ = min;
  maxBlend_ = max;
}

void VerticalMeter::paint(juce::Graphics& g) {
  auto bounds = getLocalBounds();
  int labelHeight = 20;
  auto meterBounds = bounds.removeFromBottom(bounds.getHeight() - labelHeight);

  g.setColour(juce::Colour(0xff2a2a2a));
  g.fillRect(meterBounds);

  g.setColour(juce::Colour(0xff444444));
  g.drawRect(meterBounds, 1);

  float normalizedValue = (currentValue_ - minValue_) / (maxValue_ - minValue_);
  normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));

  int valueHeight = static_cast<int>(normalizedValue * meterBounds.getHeight());
  auto fillBounds = meterBounds.removeFromBottom(valueHeight);

  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff00ff00), fillBounds.getX(),
                                         fillBounds.getBottom(), juce::Colour(0xffffff00),
                                         fillBounds.getX(), fillBounds.getY(), false));
  g.fillRect(fillBounds);

  if (showThresholds_) {
    float lowNorm = (lowThreshold_ - minValue_) / (maxValue_ - minValue_);
    float highNorm = (highThreshold_ - minValue_) / (maxValue_ - minValue_);

    int lowY = static_cast<int>(meterBounds.getBottom() - lowNorm * meterBounds.getHeight());
    int highY = static_cast<int>(meterBounds.getBottom() - highNorm * meterBounds.getHeight());

    g.setColour(juce::Colour(0xffff6600));
    g.drawLine(static_cast<float>(meterBounds.getX()), static_cast<float>(lowY),
               static_cast<float>(meterBounds.getRight()), static_cast<float>(lowY), 2.0f);

    g.setColour(juce::Colour(0xffff0000));
    g.drawLine(static_cast<float>(meterBounds.getX()), static_cast<float>(highY),
               static_cast<float>(meterBounds.getRight()), static_cast<float>(highY), 2.0f);
  }

  if (showBlendRange_) {
    float minNorm = (minBlend_ - minValue_) / (maxValue_ - minValue_);
    float maxNorm = (maxBlend_ - minValue_) / (maxValue_ - minValue_);

    int minY = static_cast<int>(meterBounds.getBottom() - minNorm * meterBounds.getHeight());
    int maxY = static_cast<int>(meterBounds.getBottom() - maxNorm * meterBounds.getHeight());

    g.setColour(juce::Colour(0xff00aaff));
    g.drawLine(static_cast<float>(meterBounds.getX()), static_cast<float>(minY),
               static_cast<float>(meterBounds.getRight()), static_cast<float>(minY), 2.0f);
    g.drawLine(static_cast<float>(meterBounds.getX()), static_cast<float>(maxY),
               static_cast<float>(meterBounds.getRight()), static_cast<float>(maxY), 2.0f);
  }

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  g.drawText(name_, bounds.removeFromTop(labelHeight), juce::Justification::centred, true);
}

void VerticalMeter::timerCallback() {
  repaint();
}

OctobIREditor::OctobIREditor(OctobIRProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      inputLevelMeter_("Input Level", -96.0f, 0.0f),
      blendMeter_("Blend", -1.0f, 1.0f) {
  addAndMakeVisible(ir1TitleLabel_);
  ir1TitleLabel_.setText("IR A (-1.0)", juce::dontSendNotification);
  ir1TitleLabel_.setJustificationType(juce::Justification::centredLeft);
  ir1TitleLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  ir1TitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(loadButton_);
  loadButton_.setButtonText("Load");
  loadButton_.onClick = [this] { loadButtonClicked(); };

  addAndMakeVisible(irPathLabel_);
  irPathLabel_.setText(audioProcessor.getCurrentIRPath().isEmpty()
                           ? "No IR loaded"
                           : audioProcessor.getCurrentIRPath(),
                       juce::dontSendNotification);
  irPathLabel_.setJustificationType(juce::Justification::centred);
  irPathLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
  irPathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(ir2TitleLabel_);
  ir2TitleLabel_.setText("IR B (+1.0)", juce::dontSendNotification);
  ir2TitleLabel_.setJustificationType(juce::Justification::centredLeft);
  ir2TitleLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  ir2TitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(loadButton2_);
  loadButton2_.setButtonText("Load");
  loadButton2_.onClick = [this] { loadButton2Clicked(); };

  addAndMakeVisible(ir2PathLabel_);
  ir2PathLabel_.setText(audioProcessor.getCurrentIR2Path().isEmpty()
                            ? "No IR loaded"
                            : audioProcessor.getCurrentIR2Path(),
                        juce::dontSendNotification);
  ir2PathLabel_.setJustificationType(juce::Justification::centred);
  ir2PathLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
  ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(inputLevelMeter_);
  addAndMakeVisible(blendMeter_);

  addAndMakeVisible(dynamicModeButton_);
  dynamicModeButton_.setButtonText("Dynamic Mode");
  dynamicModeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
      audioProcessor.getAPVTS(), "dynamicMode", dynamicModeButton_);

  addAndMakeVisible(sidechainEnableButton_);
  sidechainEnableButton_.setButtonText("Sidechain Enable");
  sidechainEnableAttachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
          audioProcessor.getAPVTS(), "sidechainEnable", sidechainEnableButton_);

  addAndMakeVisible(swapIROrderButton_);
  swapIROrderButton_.setButtonText("Swap IR Order");
  swapIROrderButton_.onClick = [this] { swapIROrderClicked(); };

  addAndMakeVisible(blendLabel_);
  blendLabel_.setText("Static Blend", juce::dontSendNotification);
  blendLabel_.setJustificationType(juce::Justification::centredLeft);
  blendLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(blendSlider_);
  blendSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  blendSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
  blendAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "blend", blendSlider_);

  addAndMakeVisible(lowBlendLabel_);
  lowBlendLabel_.setText("Low Blend", juce::dontSendNotification);
  lowBlendLabel_.setJustificationType(juce::Justification::centredLeft);
  lowBlendLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(lowBlendSlider_);
  lowBlendSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  lowBlendSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
  lowBlendAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "lowBlend", lowBlendSlider_);

  addAndMakeVisible(highBlendLabel_);
  highBlendLabel_.setText("High Blend", juce::dontSendNotification);
  highBlendLabel_.setJustificationType(juce::Justification::centredLeft);
  highBlendLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(highBlendSlider_);
  highBlendSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  highBlendSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
  highBlendAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "highBlend", highBlendSlider_);

  addAndMakeVisible(thresholdLabel_);
  thresholdLabel_.setText("Threshold", juce::dontSendNotification);
  thresholdLabel_.setJustificationType(juce::Justification::centredLeft);
  thresholdLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(thresholdSlider_);
  thresholdSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  thresholdSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  thresholdAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "threshold", thresholdSlider_);

  addAndMakeVisible(rangeDbLabel_);
  rangeDbLabel_.setText("Range", juce::dontSendNotification);
  rangeDbLabel_.setJustificationType(juce::Justification::centredLeft);
  rangeDbLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(rangeDbSlider_);
  rangeDbSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  rangeDbSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  rangeDbAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "rangeDb", rangeDbSlider_);

  addAndMakeVisible(kneeWidthDbLabel_);
  kneeWidthDbLabel_.setText("Knee Width", juce::dontSendNotification);
  kneeWidthDbLabel_.setJustificationType(juce::Justification::centredLeft);
  kneeWidthDbLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(kneeWidthDbSlider_);
  kneeWidthDbSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  kneeWidthDbSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  kneeWidthDbAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "kneeWidthDb", kneeWidthDbSlider_);

  addAndMakeVisible(detectionModeLabel_);
  detectionModeLabel_.setText("Detection Mode", juce::dontSendNotification);
  detectionModeLabel_.setJustificationType(juce::Justification::centredLeft);
  detectionModeLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(detectionModeCombo_);
  detectionModeCombo_.addItem("Peak", 1);
  detectionModeCombo_.addItem("RMS", 2);
  detectionModeAttachment_ =
      std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
          audioProcessor.getAPVTS(), "detectionMode", detectionModeCombo_);

  addAndMakeVisible(rmsWindowMsLabel_);
  rmsWindowMsLabel_.setText("RMS Window", juce::dontSendNotification);
  rmsWindowMsLabel_.setJustificationType(juce::Justification::centredLeft);
  rmsWindowMsLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(rmsWindowMsSlider_);
  rmsWindowMsSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  rmsWindowMsSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  rmsWindowMsAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "rmsWindowMs", rmsWindowMsSlider_);

  addAndMakeVisible(attackTimeLabel_);
  attackTimeLabel_.setText("Attack Time", juce::dontSendNotification);
  attackTimeLabel_.setJustificationType(juce::Justification::centredLeft);
  attackTimeLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(attackTimeSlider_);
  attackTimeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  attackTimeSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  attackTimeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "attackTime", attackTimeSlider_);

  addAndMakeVisible(releaseTimeLabel_);
  releaseTimeLabel_.setText("Release Time", juce::dontSendNotification);
  releaseTimeLabel_.setJustificationType(juce::Justification::centredLeft);
  releaseTimeLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(releaseTimeSlider_);
  releaseTimeSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  releaseTimeSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  releaseTimeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "releaseTime", releaseTimeSlider_);

  addAndMakeVisible(outputGainLabel_);
  outputGainLabel_.setText("Output Gain", juce::dontSendNotification);
  outputGainLabel_.setJustificationType(juce::Justification::centredLeft);
  outputGainLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(outputGainSlider_);
  outputGainSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
  outputGainSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 20);
  outputGainAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
      audioProcessor.getAPVTS(), "outputGain", outputGainSlider_);

  addAndMakeVisible(latencyLabel_);
  latencyLabel_.setJustificationType(juce::Justification::centred);
  latencyLabel_.setColour(juce::Label::textColourId, juce::Colour(0xff88ccff));
  updateLatencyDisplay();

  startTimerHz(30);

  setSize(700, 850);
}

OctobIREditor::~OctobIREditor() {
  stopTimer();
}

void OctobIREditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(0xff1a1a1a));

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
  g.drawText("OctobIR", getLocalBounds().removeFromTop(50), juce::Justification::centred, true);
}

void OctobIREditor::resized() {
  auto bounds = getLocalBounds().reduced(15);
  bounds.removeFromTop(50);

  auto topSection = bounds.removeFromTop(110);
  auto irRow = topSection.removeFromTop(50);

  auto ir1Section = irRow.removeFromLeft(getWidth() / 2 - 20);
  ir1TitleLabel_.setBounds(ir1Section.removeFromLeft(50));
  loadButton_.setBounds(ir1Section.removeFromLeft(70).reduced(2));
  irPathLabel_.setBounds(ir1Section.reduced(2));

  irRow.removeFromLeft(10);

  auto ir2Section = irRow;
  ir2TitleLabel_.setBounds(ir2Section.removeFromLeft(50));
  loadButton2_.setBounds(ir2Section.removeFromLeft(70).reduced(2));
  ir2PathLabel_.setBounds(ir2Section.reduced(2));

  topSection.removeFromTop(10);

  auto modeButtonRow = topSection.removeFromTop(30);
  dynamicModeButton_.setBounds(modeButtonRow.removeFromLeft(140).reduced(2));
  modeButtonRow.removeFromLeft(5);
  sidechainEnableButton_.setBounds(modeButtonRow.removeFromLeft(140).reduced(2));
  modeButtonRow.removeFromLeft(5);
  swapIROrderButton_.setBounds(modeButtonRow.removeFromLeft(120).reduced(2));

  bounds.removeFromTop(10);

  auto meterSection = bounds.removeFromTop(200);
  auto meterLeft = meterSection.removeFromLeft(60);
  inputLevelMeter_.setBounds(meterLeft);
  meterSection.removeFromLeft(10);
  auto meterRight = meterSection.removeFromLeft(60);
  blendMeter_.setBounds(meterRight);

  bounds.removeFromTop(15);

  auto blendRow = bounds.removeFromTop(25);
  blendLabel_.setBounds(blendRow.removeFromLeft(120));
  blendSlider_.setBounds(blendRow);

  bounds.removeFromTop(5);

  auto outputGainRow = bounds.removeFromTop(25);
  outputGainLabel_.setBounds(outputGainRow.removeFromLeft(120));
  outputGainSlider_.setBounds(outputGainRow);

  bounds.removeFromTop(15);

  auto lowBlendRow = bounds.removeFromTop(25);
  lowBlendLabel_.setBounds(lowBlendRow.removeFromLeft(120));
  lowBlendSlider_.setBounds(lowBlendRow);

  bounds.removeFromTop(5);

  auto highBlendRow = bounds.removeFromTop(25);
  highBlendLabel_.setBounds(highBlendRow.removeFromLeft(120));
  highBlendSlider_.setBounds(highBlendRow);

  bounds.removeFromTop(5);

  auto thresholdRow = bounds.removeFromTop(25);
  thresholdLabel_.setBounds(thresholdRow.removeFromLeft(120));
  thresholdSlider_.setBounds(thresholdRow);

  bounds.removeFromTop(5);

  auto rangeDbRow = bounds.removeFromTop(25);
  rangeDbLabel_.setBounds(rangeDbRow.removeFromLeft(120));
  rangeDbSlider_.setBounds(rangeDbRow);

  bounds.removeFromTop(5);

  auto kneeWidthDbRow = bounds.removeFromTop(25);
  kneeWidthDbLabel_.setBounds(kneeWidthDbRow.removeFromLeft(120));
  kneeWidthDbSlider_.setBounds(kneeWidthDbRow);

  bounds.removeFromTop(5);

  auto detectionModeRow = bounds.removeFromTop(25);
  detectionModeLabel_.setBounds(detectionModeRow.removeFromLeft(120));
  detectionModeCombo_.setBounds(detectionModeRow);

  bounds.removeFromTop(5);

  auto rmsWindowMsRow = bounds.removeFromTop(25);
  rmsWindowMsLabel_.setBounds(rmsWindowMsRow.removeFromLeft(120));
  rmsWindowMsSlider_.setBounds(rmsWindowMsRow);

  bounds.removeFromTop(5);

  auto attackTimeRow = bounds.removeFromTop(25);
  attackTimeLabel_.setBounds(attackTimeRow.removeFromLeft(120));
  attackTimeSlider_.setBounds(attackTimeRow);

  bounds.removeFromTop(5);

  auto releaseTimeRow = bounds.removeFromTop(25);
  releaseTimeLabel_.setBounds(releaseTimeRow.removeFromLeft(120));
  releaseTimeSlider_.setBounds(releaseTimeRow);

  bounds.removeFromTop(15);

  latencyLabel_.setBounds(bounds.removeFromTop(25));
}

void OctobIREditor::timerCallback() {
  updateMeters();
}

void OctobIREditor::updateMeters() {
  inputLevelMeter_.setValue(audioProcessor.getCurrentInputLevel());
  blendMeter_.setValue(audioProcessor.getCurrentBlend());

  bool dynamicMode = audioProcessor.getAPVTS().getRawParameterValue("dynamicMode")->load() > 0.5f;

  inputLevelMeter_.setShowThresholds(dynamicMode);
  blendMeter_.setShowBlendRange(dynamicMode);

  if (dynamicMode) {
    float threshold = audioProcessor.getAPVTS().getRawParameterValue("threshold")->load();
    float rangeDb = audioProcessor.getAPVTS().getRawParameterValue("rangeDb")->load();
    inputLevelMeter_.setThresholdMarkers(threshold, threshold + rangeDb);

    float lowBlend = audioProcessor.getAPVTS().getRawParameterValue("lowBlend")->load();
    float highBlend = audioProcessor.getAPVTS().getRawParameterValue("highBlend")->load();
    blendMeter_.setBlendRangeMarkers(lowBlend, highBlend);
  }
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

void OctobIREditor::swapIROrderClicked() {
  auto& apvts = audioProcessor.getAPVTS();

  float currentLowBlend = apvts.getRawParameterValue("lowBlend")->load();
  float currentHighBlend = apvts.getRawParameterValue("highBlend")->load();

  if (auto* lowParam = apvts.getParameter("lowBlend")) {
    lowParam->setValueNotifyingHost(lowParam->convertTo0to1(currentHighBlend));
  }

  if (auto* highParam = apvts.getParameter("highBlend")) {
    highParam->setValueNotifyingHost(highParam->convertTo0to1(currentLowBlend));
  }
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
