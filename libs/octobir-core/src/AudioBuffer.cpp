#include "octobir-core/AudioBuffer.hpp"

#include <algorithm>

namespace octob {

AudioBuffer::AudioBuffer(FrameCount numFrames) { resize(numFrames); }

void AudioBuffer::resize(FrameCount numFrames) { data_.resize(numFrames); }

void AudioBuffer::clear() { std::fill(data_.begin(), data_.end(), 0.0f); }

}  // namespace octob
