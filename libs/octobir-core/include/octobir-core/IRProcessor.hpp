#pragma once

#include <memory>
#include <string>

#include "AudioBuffer.hpp"
#include "IRLoader.hpp"
#include "Types.hpp"

class WDL_ImpulseBuffer;
class WDL_ConvolutionEngine_Div;

namespace octob {

class IRProcessor {
 public:
  IRProcessor();
  ~IRProcessor();

  bool loadImpulseResponse(const std::string& filepath, std::string& errorMessage);
  bool loadImpulseResponse2(const std::string& filepath, std::string& errorMessage);

  void setSampleRate(SampleRate sampleRate);
  void setBlend(float blend);

  void processMono(const Sample* input, Sample* output, FrameCount numFrames);
  void processStereo(const Sample* inputL, const Sample* inputR, Sample* outputL, Sample* outputR,
                     FrameCount numFrames);
  void processDualMono(const Sample* inputL, const Sample* inputR, Sample* outputL, Sample* outputR,
                       FrameCount numFrames);

  bool isIRLoaded() const { return irLoaded_; }
  bool isIR2Loaded() const { return ir2Loaded_; }
  std::string getCurrentIRPath() const { return currentIRPath_; }
  std::string getCurrentIR2Path() const { return currentIR2Path_; }
  SampleRate getIRSampleRate() const;
  SampleRate getIR2SampleRate() const;
  size_t getIRNumSamples() const;
  size_t getIR2NumSamples() const;
  int getNumIRChannels() const;
  int getNumIR2Channels() const;
  int getLatencySamples() const;
  float getBlend() const { return blend_; }

  void reset();

 private:
  std::unique_ptr<WDL_ImpulseBuffer> impulseBuffer_;
  std::unique_ptr<WDL_ConvolutionEngine_Div> convolutionEngine_;
  std::unique_ptr<IRLoader> irLoader_;

  std::unique_ptr<WDL_ImpulseBuffer> impulseBuffer2_;
  std::unique_ptr<WDL_ConvolutionEngine_Div> convolutionEngine2_;
  std::unique_ptr<IRLoader> irLoader2_;

  SampleRate sampleRate_ = 44100.0;
  std::string currentIRPath_;
  std::string currentIR2Path_;
  bool irLoaded_ = false;
  bool ir2Loaded_ = false;
  int latencySamples_ = 0;
  int latencySamples2_ = 0;
  float blend_ = 0.0f;
};

}  // namespace octob
