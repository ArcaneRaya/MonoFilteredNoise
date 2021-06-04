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
    if (data != nullptr && dataLength != 0) {
        CreateModesFromData();
    }
//    CreateEnvelopedFilters();

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
    ClearData();
}

void ModalSynthState::setData(void* data, unsigned int length)
{
    this->data = data;
    dataLength = length;

    ClearEnevelopedFilters();
    CreateModesFromData();
}

void ModalSynthState::getData(void** data, unsigned int* length)
{
    *data = data;
    *length = dataLength;
}

void ModalSynthState::ClearData()
{
    data = nullptr;
    dataLength = 0;
}

void ModalSynthState::CreateModesFromData()
{
    int numberOfModes = ParseDataForNumberOfModes();
    envelopedFiltersCount = numberOfModes;
    envelopedFilters = new EnvelopedFilter[numberOfModes];

    char* startPoint = (char*)data;
    int position = 1;
    int currentModeIndex = 0;

    while (position < dataLength && currentModeIndex < envelopedFiltersCount) {
        EnvelopedFilter* currenEnvelopedFilter = envelopedFilters + currentModeIndex;
        InitializeModeFromData(startPoint, &position, currenEnvelopedFilter);
        position++;
        currentModeIndex++;
    }
}

int ModalSynthState::ParseDataForNumberOfModes()
{
    char* currentCharacter = (char*)data;
    int dataLengthLeft = dataLength;
    int numberOfModes = 0;

    while (dataLengthLeft > 0) {
        if (*currentCharacter == '#') {
            numberOfModes++;
        }

        currentCharacter++;
        dataLengthLeft--;
    }

    return numberOfModes;
}

void ModalSynthState::InitializeModeFromData(char* fileStart, int* position, EnvelopedFilter* mode)
{
    mode->SetSamplerate(samplerate);

    int frequency = ParseDataForFrequency(fileStart, position);
    float randomValue = (((float)(rand() % 32768) / 16384.0f) - 1.0f);
    frequency += (frequency / 15) * randomValue;

    mode->SetFrequency(frequency);

    bool envelopeEndReached = false;
    while (envelopeEndReached == false) {
        (*position)++;
        ParseForNextEnvelopePoint(fileStart, position, mode);
        char currentChar = *(fileStart + *position);
        envelopeEndReached = currentChar == '#' || *position >= dataLength;
    }

    mode->Reset();

}

int ModalSynthState::ParseDataForFrequency(char* fileStart, int* position)
{
    char numberAsCharArray[7];
    numberAsCharArray[0] = 0;

    int numberAsCharArrayPosition = 0;
    while (*position < dataLength) {
        numberAsCharArray[numberAsCharArrayPosition] = *(fileStart + *position);
        
        numberAsCharArrayPosition++;
        (*position)++;

        if (*(fileStart + *position) == ';') {
            break;
        }
    }
    numberAsCharArray[numberAsCharArrayPosition] = 0;

    return std::atoi(numberAsCharArray);
}

void ModalSynthState::ParseForNextEnvelopePoint(char* fileStart, int* position, EnvelopedFilter* mode)
{
    int intPosition = *position;
    int lengthInMilliseconds = ParseForLength(fileStart, position);
    char currentChar = *(fileStart + *position);
    intPosition = *position;
    (*position)++;
    float targetValue = ParseForTargetValue(fileStart, position);

    intPosition = *position;

    mode->AddEnvelopePoint(CreateEnvelopePointWithTargetValueAndLengthInMilliseconds(targetValue, lengthInMilliseconds));
}

int ModalSynthState::ParseForLength(char* fileStart, int* position)
{
    char numberAsCharArray[7];
    numberAsCharArray[0] = 0;

    int numberAsCharArrayPosition = 0;
    while (*position < dataLength) {
        numberAsCharArray[numberAsCharArrayPosition] = *(fileStart + *position);

        numberAsCharArrayPosition++;
        (*position)++;

        if (*(fileStart + *position) == '-') {
            break;
        }
    }
    numberAsCharArray[numberAsCharArrayPosition] = 0;

    return std::atoi(numberAsCharArray);
}

float ModalSynthState::ParseForTargetValue(char* fileStart, int* position)
{
    char numberAsCharArray[7];
    numberAsCharArray[0] = 0;

    int numberAsCharArrayPosition = 0;
    while (*position < dataLength) {
        numberAsCharArray[numberAsCharArrayPosition] = *(fileStart + *position);

        numberAsCharArrayPosition++;
        (*position)++;

        if (*(fileStart + *position) == ';' || *(fileStart + *position) == '#') {
            break;
        }
    }
    numberAsCharArray[numberAsCharArrayPosition] = 0;

    return std::atof(numberAsCharArray);
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
