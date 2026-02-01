#pragma once

#include <vector>

#include "Types.hpp"

namespace octob {

class AudioBuffer {
 public:
  AudioBuffer() = default;
  explicit AudioBuffer(FrameCount numFrames);

  void resize(FrameCount numFrames);
  void clear();

  Sample* getData() { return data_.data(); }
  const Sample* getData() const { return data_.data(); }

  FrameCount getNumFrames() const { return data_.size(); }

 private:
  std::vector<Sample> data_;
};

}  // namespace octob
