#include "convolution.h"
#include <cstring>
#include <algorithm>

Convolution::Convolution() :
    impulseLength(0), 
    bufferSize(0), 
    writeIndex(0),
    bufferMask(0),
    isPowerOfTwo(false) {}

bool Convolution::initialize(const float* irData, int irLength) {
    if (!irData || irLength <= 0) return false;
    
    impulseLength = irLength;
    
    impulseResponse.resize(impulseLength);
    std::memcpy(impulseResponse.data(), irData, impulseLength * sizeof(float));
    
    int minBufferSize = impulseLength + 64;
    bufferSize = nextPowerOfTwo(minBufferSize);
    
    isPowerOfTwo = (bufferSize & (bufferSize - 1)) == 0;
    if (isPowerOfTwo) {
        bufferMask = bufferSize - 1;
    }
    
    inputBuffer.resize(bufferSize);
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    
    writeIndex = 0;
    
    return true;
}

float Convolution::process(const float inputSample) {
    inputBuffer[writeIndex] = inputSample;
    
    float output = 0.0f;
    int readIndex = writeIndex;
    
    if (impulseLength >= 4) {
        for (int i = 0; i < (impulseLength & ~3); i += 4) {
            output += inputBuffer[readIndex] * impulseResponse[i];
            readIndex = (isPowerOfTwo) ? ((readIndex - 1) & bufferMask) : 
                       ((readIndex - 1 + bufferSize) % bufferSize);
            
            output += inputBuffer[readIndex] * impulseResponse[i + 1];
            readIndex = (isPowerOfTwo) ? ((readIndex - 1) & bufferMask) : 
                       ((readIndex - 1 + bufferSize) % bufferSize);
            
            output += inputBuffer[readIndex] * impulseResponse[i + 2];
            readIndex = (isPowerOfTwo) ? ((readIndex - 1) & bufferMask) : 
                       ((readIndex - 1 + bufferSize) % bufferSize);
            
            output += inputBuffer[readIndex] * impulseResponse[i + 3];
            readIndex = (isPowerOfTwo) ? ((readIndex - 1) & bufferMask) : 
                       ((readIndex - 1 + bufferSize) % bufferSize);
        }
        
        for (int i = (impulseLength & ~3); i < impulseLength; i++) {
            output += inputBuffer[readIndex] * impulseResponse[i];
            readIndex = (isPowerOfTwo) ? ((readIndex - 1) & bufferMask) : 
                       ((readIndex - 1 + bufferSize) % bufferSize);
        }
    } else {
        for (int i = 0; i < impulseLength; i++) {
            output += inputBuffer[readIndex] * impulseResponse[i];
            readIndex = (isPowerOfTwo) ? ((readIndex - 1) & bufferMask) : 
                       ((readIndex - 1 + bufferSize) % bufferSize);
        }
    }
    
    writeIndex = (isPowerOfTwo) ? ((writeIndex + 1) & bufferMask) : 
                ((writeIndex + 1) % bufferSize);

    return output;
}

void Convolution::processBlock(const float* input, float* output, int blockSize) {
    for (int i = 0; i < blockSize; i++) {
        output[i] = process(input[i]);
    }
}

void Convolution::reset() {
    std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
    writeIndex = 0;
}

int Convolution::getImpulseLength() const { 
    return impulseLength; 
}

int Convolution::nextPowerOfTwo(int n) {
    if (n <= 1) return 2;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}
