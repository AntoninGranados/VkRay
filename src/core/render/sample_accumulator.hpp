#pragma once

#include <cstdint>

class SampleAccumulator {
public:
    static constexpr int kUnboundedSamples = -1;

    bool     isRenderFinished() const { return targetSampleCount != kUnboundedSamples && sampleCount >= static_cast<uint32_t>(targetSampleCount); }
    uint32_t getSampleCount()   const { return sampleCount; }
    uint32_t increment()              { return ++sampleCount; }
    void     setTargetSampleCount(int n) { targetSampleCount = n; }
    void     restart()                { sampleCount = 0; }

private:
    uint32_t sampleCount = 0;
    int      targetSampleCount = kUnboundedSamples;
};
