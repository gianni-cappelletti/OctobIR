#pragma once

#include <memory>
#include <string>

#include "AudioBuffer.hpp"
#include "IRLoader.hpp"
#include "Types.hpp"

class WDL_ImpulseBuffer;
class WDL_ConvolutionEngine;

namespace octob {

class IRProcessor {
 public:
  IRProcessor();
  ~IRProcessor();

  bool loadImpulseResponse(const std::string& filepath, std::string& errorMessage);

  void setSampleRate(SampleRate sampleRate);

  void processMono(const Sample* input, Sample* output, FrameCount numFrames);
  void processStereo(const Sample* inputL, const Sample* inputR, Sample* outputL, Sample* outputR,
                     FrameCount numFrames);
  void processDualMono(const Sample* inputL, const Sample* inputR, Sample* outputL, Sample* outputR,
                       FrameCount numFrames);

  bool isIRLoaded() const { return irLoaded_; }
  std::string getCurrentIRPath() const { return currentIRPath_; }
  SampleRate getIRSampleRate() const;
  size_t getIRNumSamples() const;
  int getNumIRChannels() const;

  void reset();

 private:
  std::unique_ptr<WDL_ImpulseBuffer> impulseBuffer_;
  std::unique_ptr<WDL_ConvolutionEngine> convolutionEngine_;
  std::unique_ptr<IRLoader> irLoader_;

  SampleRate sampleRate_ = 44100.0;
  std::string currentIRPath_;
  bool irLoaded_ = false;
};

}  // namespace octob
