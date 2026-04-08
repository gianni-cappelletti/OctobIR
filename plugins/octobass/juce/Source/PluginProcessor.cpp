#include "PluginProcessor.h"

#include "PluginEditor.h"

OctoBassProcessor::OctoBassProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::mono(), true)
                         .withOutput("Output", juce::AudioChannelSet::mono(), true)),
      apvts_(*this, nullptr, "OctoBassParams", createParameterLayout())
{
}

OctoBassProcessor::~OctoBassProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout OctoBassProcessor::createParameterLayout()
{
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "crossoverFrequency", "Crossover",
      juce::NormalisableRange<float>(octob::MinCrossoverFrequency, octob::MaxCrossoverFrequency,
                                     1.0f),
      octob::DefaultCrossoverFrequency, juce::String(),
      juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(static_cast<int>(value)) + " Hz"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "squash", "Squash",
      juce::NormalisableRange<float>(octob::MinSquashAmount, octob::MaxSquashAmount, 0.01f),
      octob::DefaultSquashAmount, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(static_cast<int>(value * 100.0f)) + "%"; }));

  layout.add(std::make_unique<juce::AudioParameterChoice>(
      "compressionMode", "Compression Mode", juce::StringArray("Tight", "Smooth", "Punch", "Glue"),
      octob::DefaultCompressionMode));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "lowBandLevel", "Low Level",
      juce::NormalisableRange<float>(octob::MinBandLevelDb, octob::MaxBandLevelDb, 0.1f),
      octob::DefaultBandLevelDb, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + " dB"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "highBandLevel", "High Level",
      juce::NormalisableRange<float>(octob::MinBandLevelDb, octob::MaxBandLevelDb, 0.1f),
      octob::DefaultBandLevelDb, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + " dB"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "outputGain", "Output Gain",
      juce::NormalisableRange<float>(octob::MinOutputGainDb, octob::MaxOutputGainDb, 0.1f),
      octob::DefaultOutputGainDb, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(value, 1) + " dB"; }));

  layout.add(std::make_unique<juce::AudioParameterFloat>(
      "dryWetMix", "Dry/Wet", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
      octob::DefaultDryWetMix, juce::String(), juce::AudioProcessorParameter::genericParameter,
      [](float value, int) { return juce::String(static_cast<int>(value * 100.0f)) + "%"; }));

  return layout;
}

void OctoBassProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
  bassProcessor_.setSampleRate(sampleRate);
  bassProcessor_.setMaxBlockSize(static_cast<size_t>(samplesPerBlock));
}

void OctoBassProcessor::releaseResources() {}

bool OctoBassProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono() &&
         layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono();
}

void OctoBassProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midiMessages*/)
{
  bassProcessor_.setCrossoverFrequency(*apvts_.getRawParameterValue("crossoverFrequency"));
  bassProcessor_.setSquash(*apvts_.getRawParameterValue("squash"));
  bassProcessor_.setCompressionMode(
      static_cast<int>(*apvts_.getRawParameterValue("compressionMode")));
  bassProcessor_.setLowBandLevel(*apvts_.getRawParameterValue("lowBandLevel"));
  bassProcessor_.setHighBandLevel(*apvts_.getRawParameterValue("highBandLevel"));
  bassProcessor_.setOutputGain(*apvts_.getRawParameterValue("outputGain"));
  bassProcessor_.setDryWetMix(*apvts_.getRawParameterValue("dryWetMix"));

  bassProcessor_.processMono(buffer.getReadPointer(0), buffer.getWritePointer(0),
                             static_cast<size_t>(buffer.getNumSamples()));
}

juce::AudioProcessorEditor* OctoBassProcessor::createEditor()
{
  return new OctoBassEditor(*this);
}

bool OctoBassProcessor::hasEditor() const
{
  return true;
}

const juce::String OctoBassProcessor::getName() const
{
  return JucePlugin_Name;
}

bool OctoBassProcessor::acceptsMidi() const
{
  return false;
}

bool OctoBassProcessor::producesMidi() const
{
  return false;
}

bool OctoBassProcessor::isMidiEffect() const
{
  return false;
}

double OctoBassProcessor::getTailLengthSeconds() const
{
  return 0.0;
}

int OctoBassProcessor::getNumPrograms()
{
  return 1;
}

int OctoBassProcessor::getCurrentProgram()
{
  return 0;
}

void OctoBassProcessor::setCurrentProgram(int /*index*/) {}

const juce::String OctoBassProcessor::getProgramName(int /*index*/)
{
  return {};
}

void OctoBassProcessor::changeProgramName(int /*index*/, const juce::String& /*newName*/) {}

void OctoBassProcessor::getStateInformation(juce::MemoryBlock& destData)
{
  auto state = apvts_.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void OctoBassProcessor::setStateInformation(const void* data, int sizeInBytes)
{
  std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
  if (xmlState != nullptr && xmlState->hasTagName(apvts_.state.getType()))
    apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
  return new OctoBassProcessor();
}
