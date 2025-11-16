#include "PluginProcessor.h"

#include "PluginEditor.h"

OctobIRProcessor::OctobIRProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "Parameters", createParameterLayout()) {}

OctobIRProcessor::~OctobIRProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout OctobIRProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "blend", "Blend", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f, juce::String(),
      juce::AudioProcessorParameter::genericParameter, [](float value, int) {
        int percentage = static_cast<int>(value * 100.0f);
        return juce::String(percentage) + "%";
      }));

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

  float blendValue = apvts_.getRawParameterValue("blend")->load();
  irProcessor_.setBlend(blendValue);

  if (totalNumInputChannels >= 2 && totalNumOutputChannels >= 2) {
    float* channelDataL = buffer.getWritePointer(0);
    float* channelDataR = buffer.getWritePointer(1);
    irProcessor_.processStereo(channelDataL, channelDataR, channelDataL, channelDataR,
                               static_cast<size_t>(buffer.getNumSamples()));
  } else if (totalNumInputChannels >= 1 && totalNumOutputChannels >= 1) {
    float* channelData = buffer.getWritePointer(0);
    irProcessor_.processMono(channelData, channelData, static_cast<size_t>(buffer.getNumSamples()));
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
