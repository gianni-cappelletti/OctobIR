#include "PluginEditor.h"

#include "PluginProcessor.h"

VerticalMeter::VerticalMeter(const juce::String& name, float minValue, float maxValue)
    : name_(name), minValue_(minValue), maxValue_(maxValue)
{
  startTimerHz(30);
}

void VerticalMeter::setValue(float value)
{
  currentValue_ = value;
}

void VerticalMeter::setThresholdMarkers(float low, float high)
{
  lowThreshold_ = low;
  highThreshold_ = high;
}

void VerticalMeter::setBlendRangeMarkers(float min, float max)
{
  minBlend_ = min;
  maxBlend_ = max;
}

void VerticalMeter::paint(juce::Graphics& g)
{
  auto bounds = getLocalBounds();
  int labelHeight = 20;
  auto meterBounds = bounds.removeFromBottom(bounds.getHeight() - labelHeight);

  g.setColour(juce::Colour(0xff2a2a2a));
  g.fillRect(meterBounds);

  g.setColour(juce::Colour(0xff444444));
  g.drawRect(meterBounds, 1);

  float normalizedValue = (currentValue_ - minValue_) / (maxValue_ - minValue_);
  normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));

  float ledDiameter = (static_cast<float>(meterBounds.getWidth()) - 8.0f) / 2.0f;
  float totalHeight = numLEDs_ * ledDiameter + (numLEDs_ - 1) * ledSpacing_;
  float startY = meterBounds.getY() + (meterBounds.getHeight() - totalHeight) / 2.0f;
  float centerX = meterBounds.getX() + meterBounds.getWidth() / 2.0f;

  for (int i = 0; i < numLEDs_; ++i)
  {
    int reversedIndex = numLEDs_ - 1 - i;
    float ledY = startY + i * (ledDiameter + ledSpacing_);
    auto ledBounds =
        juce::Rectangle<float>(centerX - ledDiameter / 2.0f, ledY, ledDiameter, ledDiameter);

    float ledThreshold = static_cast<float>(reversedIndex) / static_cast<float>(numLEDs_);
    bool isLit = normalizedValue >= ledThreshold;

    juce::Colour ledColor;
    if (name_ == "Blend")
    {
      ledColor = getBlendLEDColor(reversedIndex, isLit);
    }
    else
    {
      ledColor = getLEDColor(reversedIndex, isLit);
    }

    g.setColour(ledColor);
    g.fillEllipse(ledBounds);

    if (isLit)
    {
      g.setColour(ledColor.withAlpha(0.3f));
      g.fillEllipse(ledBounds.expanded(2.0f));
    }

    g.setColour(juce::Colour(0xff444444));
    g.drawEllipse(ledBounds, 1.0f);
  }

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  g.drawText(name_, bounds.removeFromTop(labelHeight), juce::Justification::centred, true);
}

juce::Colour VerticalMeter::getLEDColor(int ledIndex, bool isLit) const
{
  float position = static_cast<float>(ledIndex) / static_cast<float>(numLEDs_ - 1);

  if (position < 0.67f)
  {
    return isLit ? juce::Colour(0xff00ff00) : juce::Colour(0xff003300);
  }
  else if (position < 0.83f)
  {
    return isLit ? juce::Colour(0xffffff00) : juce::Colour(0xff333300);
  }
  else
  {
    return isLit ? juce::Colour(0xffff0000) : juce::Colour(0xff330000);
  }
}

juce::Colour VerticalMeter::getBlendLEDColor(int ledIndex, bool isLit) const
{
  float position = static_cast<float>(ledIndex) / static_cast<float>(numLEDs_ - 1);

  if (position < 0.33f)
  {
    return isLit ? juce::Colour(0xff00aaff) : juce::Colour(0xff003344);
  }
  else if (position < 0.67f)
  {
    return isLit ? juce::Colour(0xffaaaaaa) : juce::Colour(0xff222222);
  }
  else
  {
    return isLit ? juce::Colour(0xffff6600) : juce::Colour(0xff331100);
  }
}

void VerticalMeter::timerCallback()
{
  repaint();
}

OctobIREditor::OctobIREditor(OctobIRProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      inputLevelMeter_("Input Level", -96.0f, 0.0f),
      blendMeter_("Blend", -1.0f, 1.0f)
{
  addAndMakeVisible(ir1TitleLabel_);
  ir1TitleLabel_.setText("IR A (-1.0)", juce::dontSendNotification);
  ir1TitleLabel_.setJustificationType(juce::Justification::centredLeft);
  ir1TitleLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  ir1TitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(loadButton1_);
  loadButton1_.setButtonText("Load");
  loadButton1_.onClick = [this] { loadButton1Clicked(); };

  addAndMakeVisible(clearButton1_);
  clearButton1_.setButtonText("Clear");
  clearButton1_.onClick = [this] { clearButton1Clicked(); };

  addAndMakeVisible(prevButton1_);
  prevButton1_.setButtonText("<");
  prevButton1_.onClick = [this] { prevButton1Clicked(); };

  addAndMakeVisible(nextButton1_);
  nextButton1_.setButtonText(">");
  nextButton1_.onClick = [this] { nextButton1Clicked(); };

  addAndMakeVisible(ir1PathLabel_);
  ir1PathLabel_.setText(audioProcessor.getCurrentIR1Path().isEmpty()
                            ? "No IR loaded"
                            : audioProcessor.getCurrentIR1Path(),
                        juce::dontSendNotification);
  ir1PathLabel_.setJustificationType(juce::Justification::centred);
  ir1PathLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
  ir1PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(ir1EnableButton_);
  ir1EnableButton_.setButtonText("Enable");
  ir1EnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
      audioProcessor.getAPVTS(), "irAEnable", ir1EnableButton_);

  addAndMakeVisible(ir2TitleLabel_);
  ir2TitleLabel_.setText("IR B (+1.0)", juce::dontSendNotification);
  ir2TitleLabel_.setJustificationType(juce::Justification::centredLeft);
  ir2TitleLabel_.setFont(juce::FontOptions(14.0f, juce::Font::bold));
  ir2TitleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(loadButton2_);
  loadButton2_.setButtonText("Load");
  loadButton2_.onClick = [this] { loadButton2Clicked(); };

  addAndMakeVisible(clearButton2_);
  clearButton2_.setButtonText("Clear");
  clearButton2_.onClick = [this] { clearButton2Clicked(); };

  addAndMakeVisible(prevButton2_);
  prevButton2_.setButtonText("<");
  prevButton2_.onClick = [this] { prevButton2Clicked(); };

  addAndMakeVisible(nextButton2_);
  nextButton2_.setButtonText(">");
  nextButton2_.onClick = [this] { nextButton2Clicked(); };

  addAndMakeVisible(ir2PathLabel_);
  ir2PathLabel_.setText(audioProcessor.getCurrentIR2Path().isEmpty()
                            ? "No IR loaded"
                            : audioProcessor.getCurrentIR2Path(),
                        juce::dontSendNotification);
  ir2PathLabel_.setJustificationType(juce::Justification::centred);
  ir2PathLabel_.setColour(juce::Label::backgroundColourId, juce::Colour(0xff2a2a2a));
  ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);

  addAndMakeVisible(ir2EnableButton_);
  ir2EnableButton_.setButtonText("Enable");
  ir2EnableAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
      audioProcessor.getAPVTS(), "irBEnable", ir2EnableButton_);

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
  kneeWidthDbLabel_.setText("Knee", juce::dontSendNotification);
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

  setSize(700, 760);
}

OctobIREditor::~OctobIREditor()
{
  stopTimer();
}

void OctobIREditor::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colour(0xff1a1a1a));

  g.setColour(juce::Colours::white);
  g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
  g.drawText("OctobIR", getLocalBounds().removeFromTop(50), juce::Justification::centred, true);
}

void OctobIREditor::resized()
{
  auto bounds = getLocalBounds().reduced(15);
  bounds.removeFromTop(50);

  auto topSection = bounds.removeFromTop(110);
  auto irRow = topSection.removeFromTop(50);

  auto ir1Section = irRow.removeFromLeft(getWidth() / 2 - 20);
  ir1TitleLabel_.setBounds(ir1Section.removeFromLeft(50));
  loadButton1_.setBounds(ir1Section.removeFromLeft(70).reduced(2));
  clearButton1_.setBounds(ir1Section.removeFromLeft(70).reduced(2));
  prevButton1_.setBounds(ir1Section.removeFromLeft(35).reduced(2));
  nextButton1_.setBounds(ir1Section.removeFromLeft(35).reduced(2));
  ir1EnableButton_.setBounds(ir1Section.removeFromLeft(70).reduced(2));
  ir1PathLabel_.setBounds(ir1Section.reduced(2));

  irRow.removeFromLeft(10);

  auto ir2Section = irRow;
  ir2TitleLabel_.setBounds(ir2Section.removeFromLeft(50));
  loadButton2_.setBounds(ir2Section.removeFromLeft(70).reduced(2));
  clearButton2_.setBounds(ir2Section.removeFromLeft(70).reduced(2));
  prevButton2_.setBounds(ir2Section.removeFromLeft(35).reduced(2));
  nextButton2_.setBounds(ir2Section.removeFromLeft(35).reduced(2));
  ir2EnableButton_.setBounds(ir2Section.removeFromLeft(70).reduced(2));
  ir2PathLabel_.setBounds(ir2Section.reduced(2));

  topSection.removeFromTop(10);

  auto modeButtonRow = topSection.removeFromTop(30);
  dynamicModeButton_.setBounds(modeButtonRow.removeFromLeft(140).reduced(2));
  modeButtonRow.removeFromLeft(5);
  sidechainEnableButton_.setBounds(modeButtonRow.removeFromLeft(140).reduced(2));
  modeButtonRow.removeFromLeft(5);
  swapIROrderButton_.setBounds(modeButtonRow.removeFromLeft(120).reduced(2));

  bounds.removeFromTop(10);

  auto meterSection = bounds.removeFromRight(140);
  auto meterLeft = meterSection.removeFromLeft(60);
  inputLevelMeter_.setBounds(meterLeft);
  meterSection.removeFromLeft(10);
  auto meterRight = meterSection.removeFromLeft(60);
  blendMeter_.setBounds(meterRight);

  bounds.removeFromRight(15);

  auto blendRow = bounds.removeFromTop(25);
  blendLabel_.setBounds(blendRow.removeFromLeft(120));
  blendSlider_.setBounds(blendRow);

  bounds.removeFromTop(5);

  auto outputGainRow = bounds.removeFromTop(25);
  outputGainLabel_.setBounds(outputGainRow.removeFromLeft(120));
  outputGainSlider_.setBounds(outputGainRow);

  bounds.removeFromTop(15);

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

void OctobIREditor::timerCallback()
{
  updateMeters();
}

void OctobIREditor::updateMeters()
{
  inputLevelMeter_.setValue(audioProcessor.getCurrentInputLevel());
  blendMeter_.setValue(audioProcessor.getCurrentBlend());

  bool dynamicMode = audioProcessor.getAPVTS().getRawParameterValue("dynamicMode")->load() > 0.5f;

  inputLevelMeter_.setShowThresholds(dynamicMode);
  blendMeter_.setShowBlendRange(dynamicMode);

  if (dynamicMode)
  {
    float threshold = audioProcessor.getAPVTS().getRawParameterValue("threshold")->load();
    float rangeDb = audioProcessor.getAPVTS().getRawParameterValue("rangeDb")->load();
    inputLevelMeter_.setThresholdMarkers(threshold, threshold + rangeDb);

    blendMeter_.setBlendRangeMarkers(-1.0f, 1.0f);
  }
}

void OctobIREditor::loadButton1Clicked()
{
  auto chooser = std::make_shared<juce::FileChooser>("Select impulse response WAV file for IR 1",
                                                     getLastBrowsedDirectory(), "*.wav");

  auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(
      flags,
      [this, chooser](const juce::FileChooser& fc)
      {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
          updateLastBrowsedDirectory(file);
          juce::String error;
          bool success = audioProcessor.loadImpulseResponse1(file.getFullPathName(), error);
          if (success)
          {
            ir1PathLabel_.setText(file.getFileName(), juce::dontSendNotification);
            ir1PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
            updateLatencyDisplay();
          }
          else
          {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Failed to Load IR 1", error, "OK");
            ir1PathLabel_.setText("Failed to load IR", juce::dontSendNotification);
            ir1PathLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
          }
        }
      });
}

void OctobIREditor::clearButton1Clicked()
{
  audioProcessor.clearImpulseResponse1();
  ir1PathLabel_.setText("No IR loaded", juce::dontSendNotification);
  ir1PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
  updateLatencyDisplay();
}

void OctobIREditor::loadButton2Clicked()
{
  auto chooser = std::make_shared<juce::FileChooser>("Select impulse response WAV file for IR 2",
                                                     getLastBrowsedDirectory(), "*.wav");

  auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

  chooser->launchAsync(
      flags,
      [this, chooser](const juce::FileChooser& fc)
      {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
          updateLastBrowsedDirectory(file);
          juce::String error;
          bool success = audioProcessor.loadImpulseResponse2(file.getFullPathName(), error);
          if (success)
          {
            ir2PathLabel_.setText(file.getFileName(), juce::dontSendNotification);
            ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
          }
          else
          {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Failed to Load IR 2", error, "OK");
            ir2PathLabel_.setText("Failed to load IR", juce::dontSendNotification);
            ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
          }
        }
      });
}

void OctobIREditor::clearButton2Clicked()
{
  audioProcessor.clearImpulseResponse2();
  ir2PathLabel_.setText("No IR loaded", juce::dontSendNotification);
  ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
}

void OctobIREditor::swapIROrderClicked()
{
  auto& apvts = audioProcessor.getAPVTS();

  float currentLowBlend = apvts.getRawParameterValue("lowBlend")->load();
  float currentHighBlend = apvts.getRawParameterValue("highBlend")->load();

  if (auto* lowParam = apvts.getParameter("lowBlend"))
  {
    lowParam->setValueNotifyingHost(lowParam->convertTo0to1(currentHighBlend));
  }

  if (auto* highParam = apvts.getParameter("highBlend"))
  {
    highParam->setValueNotifyingHost(highParam->convertTo0to1(currentLowBlend));
  }
}

void OctobIREditor::updateLatencyDisplay()
{
  int latency = audioProcessor.getLatencySamples();
  double sampleRate = audioProcessor.getSampleRate();
  if (sampleRate > 0)
  {
    double latencyMs = (latency / sampleRate) * 1000.0;
    latencyLabel_.setText(juce::String("Latency: ") + juce::String(latency) + " samples (" +
                              juce::String(latencyMs, 2) + " ms)",
                          juce::dontSendNotification);
  }
  else
  {
    latencyLabel_.setText(juce::String("Latency: ") + juce::String(latency) + " samples",
                          juce::dontSendNotification);
  }
}

void OctobIREditor::prevButton1Clicked()
{
  cycleIRFile(1, -1);
}

void OctobIREditor::nextButton1Clicked()
{
  cycleIRFile(1, 1);
}

void OctobIREditor::prevButton2Clicked()
{
  cycleIRFile(2, -1);
}

void OctobIREditor::nextButton2Clicked()
{
  cycleIRFile(2, 1);
}

void OctobIREditor::cycleIRFile(int irIndex, int direction)
{
  juce::String currentPath =
      irIndex == 1 ? audioProcessor.getCurrentIR1Path() : audioProcessor.getCurrentIR2Path();

  if (currentPath.isEmpty())
    return;

  juce::File currentFile(currentPath);
  if (!currentFile.existsAsFile())
    return;

  juce::File directory = currentFile.getParentDirectory();
  juce::Array<juce::File> wavFiles;

  for (const auto& entry : juce::RangedDirectoryIterator(
           directory, false, "*.wav", juce::File::findFiles | juce::File::ignoreHiddenFiles))
  {
    wavFiles.add(entry.getFile());
  }

  if (wavFiles.isEmpty())
    return;

  wavFiles.sort();

  int currentIndex = wavFiles.indexOf(currentFile);
  if (currentIndex < 0)
    return;

  int newIndex = currentIndex + direction;
  if (newIndex < 0)
    newIndex = wavFiles.size() - 1;
  else if (newIndex >= wavFiles.size())
    newIndex = 0;

  juce::File newFile = wavFiles[newIndex];
  juce::String error;
  bool success;

  if (irIndex == 1)
  {
    success = audioProcessor.loadImpulseResponse1(newFile.getFullPathName(), error);
    if (success)
    {
      ir1PathLabel_.setText(newFile.getFileName(), juce::dontSendNotification);
      ir1PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
      updateLatencyDisplay();
    }
    else
    {
      juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Failed to Load IR 1",
                                             error, "OK");
      ir1PathLabel_.setText("Failed to load IR", juce::dontSendNotification);
      ir1PathLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
    }
  }
  else
  {
    success = audioProcessor.loadImpulseResponse2(newFile.getFullPathName(), error);
    if (success)
    {
      ir2PathLabel_.setText(newFile.getFileName(), juce::dontSendNotification);
      ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    }
    else
    {
      juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Failed to Load IR 2",
                                             error, "OK");
      ir2PathLabel_.setText("Failed to load IR", juce::dontSendNotification);
      ir2PathLabel_.setColour(juce::Label::textColourId, juce::Colours::red);
    }
  }
}

juce::File OctobIREditor::getLastBrowsedDirectory() const
{
  if (lastBrowsedDirectory_.exists() && lastBrowsedDirectory_.isDirectory())
    return lastBrowsedDirectory_;

  juce::String ir1Path = audioProcessor.getCurrentIR1Path();
  if (ir1Path.isNotEmpty())
  {
    juce::File ir1File(ir1Path);
    if (ir1File.existsAsFile())
      return ir1File.getParentDirectory();
  }

  juce::String ir2Path = audioProcessor.getCurrentIR2Path();
  if (ir2Path.isNotEmpty())
  {
    juce::File ir2File(ir2Path);
    if (ir2File.existsAsFile())
      return ir2File.getParentDirectory();
  }

  return juce::File::getSpecialLocation(juce::File::userHomeDirectory);
}

void OctobIREditor::updateLastBrowsedDirectory(const juce::File& file)
{
  if (file.existsAsFile())
    lastBrowsedDirectory_ = file.getParentDirectory();
}
