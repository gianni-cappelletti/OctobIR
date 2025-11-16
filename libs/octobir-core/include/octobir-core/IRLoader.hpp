#pragma once

#include "Types.hpp"

#include <string>
#include <vector>

class WDL_ImpulseBuffer;

namespace octob {

struct IRLoadResult {
  bool success = false;
  std::string errorMessage;
  size_t numSamples = 0;
  int numChannels = 0;
  SampleRate sampleRate = 0.0;
};

class IRLoader {
 public:
  IRLoader();
  ~IRLoader();

  IRLoadResult loadFromFile(const std::string& filepath);

  bool resampleAndInitialize(WDL_ImpulseBuffer& impulseBuffer, SampleRate targetSampleRate);

  SampleRate getIRSampleRate() const { return irSampleRate_; }
  size_t getNumSamples() const { return numSamples_; }
  int getNumChannels() const { return numChannels_; }

 private:
  std::vector<Sample> irBuffer_;
  SampleRate irSampleRate_ = 0.0;
  size_t numSamples_ = 0;
  int numChannels_ = 0;
};

}  // namespace octob
