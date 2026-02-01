#pragma once

#include <cstddef>

namespace octob
{

using SampleRate = double;
using Sample = float;
using FrameCount = size_t;

constexpr int MAX_IR_LENGTH_SECONDS = 10;
constexpr int MIN_SAMPLE_RATE = 8000;
constexpr int MAX_SAMPLE_RATE = 192000;

}  // namespace octob
