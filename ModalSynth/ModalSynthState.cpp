#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fmod.hpp"
#include "Iir.h"
#include "ModalSynthState.h"

ModalSynthState::ModalSynthState()
{

}

void ModalSynthState::generate(float* outBuffer, unsigned int length)
{
    float generatedValue = 0;
    float totalFilteredValue = 0;
    while (length--)
    {
        totalFilteredValue = 0;
        generatedValue = (((float)(rand() % 32768) / 16384.0f) - 1.0f);

        if (envelopedFiltersCount > 0) 
        {
            for (int i = 0; i < envelopedFiltersCount; i++)
            {
                totalFilteredValue += (envelopedFilters + i)->ProcessSource(generatedValue);
                (envelopedFilters + i)->IncrementWithOneSample();
            }
            *outBuffer = totalFilteredValue / envelopedFiltersCount;
        }
        else 
        {
            *outBuffer = 0;
        }

        outBuffer++;
    }
}

void ModalSynthState::initialize(float samplerate)
{
    this->samplerate = samplerate;

    ClearEnevelopedFilters();
    CreateEnvelopedFilters();

    float someValue = 0;
}

void ModalSynthState::reset()
{
    for (int i = envelopedFiltersCount - 1; i >= 0; i--)
    {
        (envelopedFilters + i)->Reset();
    }
}

void ModalSynthState::release()
{
    ClearEnevelopedFilters();
}

void ModalSynthState::ClearEnevelopedFilters()
{
    if (envelopedFilters != nullptr) {
        delete [] envelopedFilters;
        envelopedFilters = nullptr;
    }
    if (envelopedFiltersCount > 0) {
        envelopedFiltersCount = 0;
    }
}

void ModalSynthState::CreateEnvelopedFilters()
{
    float baseFrequency = 150;
    float frequencyIncrement = 200;

    envelopedFiltersCount = 15;
    envelopedFilters = new EnvelopedFilter[envelopedFiltersCount];

    for (int i = 0; i < envelopedFiltersCount; i++)
    {
        envelopedFilters[i].SetSamplerate(samplerate);
        envelopedFilters[i].SetFrequency(baseFrequency + frequencyIncrement* i);
        envelopedFilters[i].AddEnvelopePoint(CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(0, 0));
        envelopedFilters[i].AddEnvelopePoint(CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(1, 50));
        envelopedFilters[i].AddEnvelopePoint(CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(0, 50 + (frequencyIncrement / 3 * envelopedFiltersCount - frequencyIncrement / 3 * i)));
        envelopedFilters[i].Reset();
    }
}

EnvelopePoint ModalSynthState::CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(float targetValue, int lengthInMilliseconds)
{
    int lengthInSamples = (((float)lengthInMilliseconds) / 1000) * samplerate;
    return EnvelopePoint(targetValue, lengthInSamples);
}
