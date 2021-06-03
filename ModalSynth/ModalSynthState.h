#pragma once

#include "EnvelopePoint.h"
#include "Envelope.h"
#include "EnvelopedFilter.h"
#include <vector>

class ModalSynthState
{
public:
    ModalSynthState();
    void generate(float* outBuffer, unsigned int length);
    void initialize(float samplerate);
    void reset();
    void release();

private:
    void ClearEnevelopedFilters();
    void CreateEnvelopedFilters();
    EnvelopePoint CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(float targetValue, int lengthInMilliseconds);
    EnvelopedFilter* envelopedFilters = nullptr;
    int envelopedFiltersCount = 0;
    int samplerate = 48000;
};