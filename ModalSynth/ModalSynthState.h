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
    void setData(void* data, unsigned int length);
    void getData(void** data, unsigned int* length);

private:
    void ClearData();
    void CreateModesFromData();
    int ParseDataForNumberOfModes();
    void InitializeModeFromData(char* fileStart, int* position, EnvelopedFilter* mode);
    int ParseDataForFrequency(char* fileStart, int* position);
    void ParseForNextEnvelopePoint(char* fileStart, int* position, EnvelopedFilter* mode);
    int ParseForLength(char* filestart, int* position);
    float ParseForTargetValue(char* fileStart, int* position);
    void ClearEnevelopedFilters();
    void CreateEnvelopedFilters();
    EnvelopePoint CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(float targetValue, int lengthInMilliseconds);
    EnvelopedFilter* envelopedFilters = nullptr;
    int envelopedFiltersCount = 0;
    int samplerate = 48000;
    void* data = nullptr;
    unsigned int dataLength = 0;
};