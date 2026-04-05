#pragma once

#include <octobir-core/IRProcessor.hpp>
#include <string>
#include <vector>

#include "Crossover.hpp"
#include "Types.hpp"

namespace octob
{

class BassProcessor
{
 public:
  BassProcessor();
  ~BassProcessor();

  void setSampleRate(SampleRate sampleRate);
  void setMaxBlockSize(FrameCount maxBlockSize);

  // IR loading (delegates to internal IRProcessor, single slot)
  bool loadImpulseResponse(const std::string& filepath, std::string& errorMessage);
  void clearImpulseResponse();
  bool isIRLoaded() const;
  std::string getCurrentIRPath() const;

  // Crossover
  void setCrossoverFrequency(float frequencyHz);

  // Levels
  void setLowBandLevel(float levelDb);
  void setHighBandLevel(float levelDb);
  void setOutputGain(float gainDb);
  void setDryWetMix(float mix);

  // Processing
  void processMono(const Sample* input, Sample* output, FrameCount numFrames);

  void reset();

  // Queries
  int getLatencySamples() const;
  float getCrossoverFrequency() const { return crossover_.getFrequency(); }
  float getLowBandLevel() const { return lowBandLevelDb_; }
  float getHighBandLevel() const { return highBandLevelDb_; }
  float getOutputGain() const { return outputGainDb_; }
  float getDryWetMix() const { return dryWetMix_; }

 private:
  Crossover crossover_;
  IRProcessor irProcessor_;

  std::vector<Sample> lowBandBuffer_;
  std::vector<Sample> highBandBuffer_;
  std::vector<Sample> dryBuffer_;

  // Delay compensation for low band path
  std::vector<Sample> lowBandDelayBuffer_;
  size_t lowBandDelayWritePos_;
  int currentIRLatency_;

  float lowBandLevelDb_;
  float highBandLevelDb_;
  float outputGainDb_;
  float dryWetMix_;

  float lowBandLevelLinear_;
  float highBandLevelLinear_;
  float outputGainLinear_;

  std::string currentIRPath_;

  void updateDelayBuffer();

  static float clamp(float value, float minVal, float maxVal);
  static float dbToLinear(float db);

  static void writeToDelayBuffer(std::vector<Sample>& buffer, size_t& writePos, const Sample* input,
                                 FrameCount numFrames);
  static void readFromDelayBuffer(const std::vector<Sample>& buffer, size_t writePos,
                                  Sample* output, FrameCount numFrames, int delaySamples);
};

}  // namespace octob
