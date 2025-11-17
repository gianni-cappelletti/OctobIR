#include "PluginProcessor.h"

#include "PluginEditor.h"

OctobIRProcessor::OctobIRProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout()) {}

OctobIRProcessor::~OctobIRProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout OctobIRProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterBool>("dynamicMode", "Dynamic Mode", false));

  layout.add(
      std::make_unique<juce::AudioParameterBool>("sidechainEnable", "Sidechain Enable", false));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "blend", "Static Blend", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 2); }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "minBlend", "Min Blend", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), -1.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 2); }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "maxBlend", "Max Blend", juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 1.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 2); }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lowThreshold", "Low Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -40.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + " dB"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "highThreshold", "High Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -10.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + " dB"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "attackTime", "Attack Time", juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 50.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(static_cast<int>(value)) + " ms"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "releaseTime", "Release Time", juce::NormalisableRange<float>(1.0f, 1000.0f, 1.0f), 200.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(static_cast<int>(value)) + " ms"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "outputGain", "Output Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
      juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + " dB"; }));

  return layout;
}

const juce::String OctobIRProcessor::getName() const {
  return JucePlugin_Name;
}

bool OctobIRProcessor::acceptsMidi() const {
  return false;
}

bool OctobIRProcessor::producesMidi() const {
  return false;
}

bool OctobIRProcessor::isMidiEffect() const {
  return false;
}

double OctobIRProcessor::getTailLengthSeconds() const {
  return 0.0;
}

int OctobIRProcessor::getNumPrograms() {
  return 1;
}

int OctobIRProcessor::getCurrentProgram() {
  return 0;
}

void OctobIRProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String OctobIRProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void OctobIRProcessor::changeProgramName(int index, const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void OctobIRProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  irProcessor_.setSampleRate(sampleRate);
}

void OctobIRProcessor::releaseResources() {
  irProcessor_.reset();
}

bool OctobIRProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
    return false;

  auto sidechain = layouts.getChannelSet(true, 1);
  if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() &&
      sidechain != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void OctobIRProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  bool dynamicMode = apvts_.getRawParameterValue("dynamicMode")->load() > 0.5f;
  bool sidechainEnabled = apvts_.getRawParameterValue("sidechainEnable")->load() > 0.5f;
  float blend = apvts_.getRawParameterValue("blend")->load();
  float minBlend = apvts_.getRawParameterValue("minBlend")->load();
  float maxBlend = apvts_.getRawParameterValue("maxBlend")->load();
  float lowThreshold = apvts_.getRawParameterValue("lowThreshold")->load();
  float highThreshold = apvts_.getRawParameterValue("highThreshold")->load();
  float attackTime = apvts_.getRawParameterValue("attackTime")->load();
  float releaseTime = apvts_.getRawParameterValue("releaseTime")->load();
  float outputGain = apvts_.getRawParameterValue("outputGain")->load();

  irProcessor_.setDynamicModeEnabled(dynamicMode);
  irProcessor_.setSidechainEnabled(sidechainEnabled);
  irProcessor_.setBlend(blend);
  irProcessor_.setMinBlend(minBlend);
  irProcessor_.setMaxBlend(maxBlend);
  irProcessor_.setLowThreshold(lowThreshold);
  irProcessor_.setHighThreshold(highThreshold);
  irProcessor_.setAttackTime(attackTime);
  irProcessor_.setReleaseTime(releaseTime);
  irProcessor_.setOutputGain(outputGain);

  auto mainInputChannels = getBusBuffer(buffer, true, 0);
  auto sidechainBuffer = getBusBuffer(buffer, true, 1);
  bool hasSidechain = sidechainBuffer.getNumChannels() != 0;

  if (dynamicMode && sidechainEnabled && hasSidechain) {
    if (mainInputChannels.getNumChannels() >= 2 && totalNumOutputChannels >= 2) {
      float* mainL = mainInputChannels.getWritePointer(0);
      float* mainR = mainInputChannels.getWritePointer(1);
      const float* scL =
          sidechainBuffer.getNumChannels() >= 1 ? sidechainBuffer.getReadPointer(0) : mainL;
      const float* scR =
          sidechainBuffer.getNumChannels() >= 2 ? sidechainBuffer.getReadPointer(1) : scL;

      float* outL = buffer.getWritePointer(0);
      float* outR = buffer.getWritePointer(1);

      irProcessor_.processStereoWithSidechain(mainL, mainR, scL, scR, outL, outR,
                                              static_cast<size_t>(buffer.getNumSamples()));
    } else if (mainInputChannels.getNumChannels() >= 1 && totalNumOutputChannels >= 1) {
      float* main = mainInputChannels.getWritePointer(0);
      const float* sc =
          sidechainBuffer.getNumChannels() >= 1 ? sidechainBuffer.getReadPointer(0) : main;
      float* out = buffer.getWritePointer(0);

      irProcessor_.processMonoWithSidechain(main, sc, out,
                                            static_cast<size_t>(buffer.getNumSamples()));
    }
  } else {
    if (mainInputChannels.getNumChannels() >= 2 && totalNumOutputChannels >= 2) {
      float* channelDataL = buffer.getWritePointer(0);
      float* channelDataR = buffer.getWritePointer(1);
      irProcessor_.processStereo(channelDataL, channelDataR, channelDataL, channelDataR,
                                 static_cast<size_t>(buffer.getNumSamples()));
    } else if (mainInputChannels.getNumChannels() >= 1 && totalNumOutputChannels >= 1) {
      float* channelData = buffer.getWritePointer(0);
      irProcessor_.processMono(channelData, channelData,
                               static_cast<size_t>(buffer.getNumSamples()));
    }
  }
}

bool OctobIRProcessor::hasEditor() const {
  return true;
}

juce::AudioProcessorEditor* OctobIRProcessor::createEditor() {
  return new OctobIREditor(*this);
}

void OctobIRProcessor::getStateInformation(juce::MemoryBlock& destData) {
  auto state = apvts_.copyState();
  state.setProperty("irPath", currentIRPath_, nullptr);
  state.setProperty("ir2Path", currentIR2Path_, nullptr);

  juce::MemoryOutputStream stream(destData, false);
  state.writeToStream(stream);
}

void OctobIRProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::ValueTree state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));

  if (state.isValid()) {
    apvts_.replaceState(state);

    juce::String path = state.getProperty("irPath").toString();
    if (path.isNotEmpty()) {
      juce::String error;
      loadImpulseResponse(path, error);
    }

    juce::String path2 = state.getProperty("ir2Path").toString();
    if (path2.isNotEmpty()) {
      juce::String error;
      loadImpulseResponse2(path2, error);
    }
  }
}

bool OctobIRProcessor::loadImpulseResponse(const juce::String& filepath,
                                           juce::String& errorMessage) {
  std::string error;
  if (irProcessor_.loadImpulseResponse(filepath.toStdString(), error)) {
    currentIRPath_ = filepath;
    setLatencySamples(irProcessor_.getLatencySamples());
    DBG("Loaded IR1: " + filepath + " (Latency: " + juce::String(irProcessor_.getLatencySamples()) +
        " samples)");
    errorMessage.clear();
    return true;
  } else {
    DBG("Failed to load IR1: " + juce::String(error));
    errorMessage = juce::String(error);
    return false;
  }
}

bool OctobIRProcessor::loadImpulseResponse2(const juce::String& filepath,
                                            juce::String& errorMessage) {
  std::string error;
  if (irProcessor_.loadImpulseResponse2(filepath.toStdString(), error)) {
    currentIR2Path_ = filepath;
    DBG("Loaded IR2: " + filepath);
    errorMessage.clear();
    return true;
  } else {
    DBG("Failed to load IR2: " + juce::String(error));
    errorMessage = juce::String(error);
    return false;
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new OctobIRProcessor();
}
