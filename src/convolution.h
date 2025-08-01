//
// Created by Gianni Cappelletti (Test) on 7/31/25.
//

#ifndef CONVOLUTION_H
#define CONVOLUTION_H

#include <vector>

class Convolution {
    std::vector<float> impulseResponse;
    std::vector<float> inputBuffer;
    int impulseLength;
    int bufferSize;
    int writeIndex;
    
    int bufferMask;
    bool isPowerOfTwo;
    
    static int nextPowerOfTwo(int n);
    
public:
    Convolution();
    ~Convolution() = default;
    
    bool initialize(const float* irData, int irLength);
    float process(const float inputSample);
    void processBlock(const float* input, float* output, int blockSize);
    void reset();
    int getImpulseLength() const;
};

#endif //CONVOLUTION_H
