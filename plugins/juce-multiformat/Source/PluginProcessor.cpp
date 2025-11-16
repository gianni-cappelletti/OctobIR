#include "PluginProcessor.h"

#include "PluginEditor.h"

OctobIRProcessor::OctobIRProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

OctobIRProcessor::~OctobIRProcessor() {}

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
  juce::ValueTree state("OctobIRState");
  state.setProperty("irPath", currentIRPath_, nullptr);

  juce::MemoryOutputStream stream(destData, false);
  state.writeToStream(stream);
}

void OctobIRProcessor::setStateInformation(const void* data, int sizeInBytes) {
  juce::ValueTree state = juce::ValueTree::readFromData(data, static_cast<size_t>(sizeInBytes));

  if (state.isValid()) {
    juce::String path = state.getProperty("irPath").toString();
    if (path.isNotEmpty()) {
      loadImpulseResponse(path);
    }
  }
}

void OctobIRProcessor::loadImpulseResponse(const juce::String& filepath) {
  std::string error;
  if (irProcessor_.loadImpulseResponse(filepath.toStdString(), error)) {
    currentIRPath_ = filepath;
    DBG("Loaded IR: " + filepath);
  } else {
    DBG("Failed to load IR: " + juce::String(error));
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new OctobIRProcessor();
}
