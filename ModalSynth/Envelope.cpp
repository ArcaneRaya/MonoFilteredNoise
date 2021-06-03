#include "Envelope.h"

Envelope::Envelope()
{
	Reset();
}

void Envelope::AddEnvelopePoint(EnvelopePoint newPoint)
{
	envelopePoints.push_back(newPoint);
}

void Envelope::IncrementWithOneSample()
{
	samplesLeftEnvelopePoint--;
	currentLevel += gainDeltaPerSample;

	while (ShouldMoveToNextPoint()) {
		MoveToNextPoint();
	}
}

float Envelope::GetCurrentLevel()
{
	return currentLevel;
}

void Envelope::Reset()
{
	currentEnvelopePoint = 0;
	currentLevel = 0;
	finishedEnvelope = false;
	UpdateValuesToMatchCurrentPoint();
	while (ShouldMoveToNextPoint()) {
		MoveToNextPoint();
	}
}

bool Envelope::ShouldMoveToNextPoint()
{
	return samplesLeftEnvelopePoint <= 0 && finishedEnvelope == false;
}

bool Envelope::CanMoveToNextPoint()
{
	return (currentEnvelopePoint + 1) < envelopePoints.size();
}


void Envelope::MoveToNextPoint()
{
	if (CanMoveToNextPoint())
	{
		currentLevel = targetLevelEnvelopePoint;
		currentEnvelopePoint++;
		UpdateValuesToMatchCurrentPoint();
	}
	else {
		currentLevel = targetLevelEnvelopePoint;
		gainDeltaPerSample = 0;
		finishedEnvelope = true;
	}
}

void Envelope::UpdateValuesToMatchCurrentPoint()
{
	if (envelopePoints.size() <= currentEnvelopePoint)
	{
		return;
	}

	targetLevelEnvelopePoint = envelopePoints[currentEnvelopePoint].targetValue;
	samplesLeftEnvelopePoint = envelopePoints[currentEnvelopePoint].lengtInSamples;
	gainDeltaPerSample = (targetLevelEnvelopePoint - currentLevel) / samplesLeftEnvelopePoint;
}