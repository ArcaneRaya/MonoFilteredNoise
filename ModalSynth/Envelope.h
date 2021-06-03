#pragma once

#include <vector>
#include "EnvelopePoint.h"

class Envelope
{
public:
	Envelope();
	void AddEnvelopePoint(EnvelopePoint newPoint);
	void IncrementWithOneSample();
	float GetCurrentLevel();
	void Reset();
private:
	bool ShouldMoveToNextPoint();
	bool CanMoveToNextPoint();
	void MoveToNextPoint();
	void UpdateValuesToMatchCurrentPoint();
	std::vector<EnvelopePoint> envelopePoints;
	float currentLevel = 0;
	int currentEnvelopePoint = 0;
	unsigned int samplesLeftEnvelopePoint = 0;
	float targetLevelEnvelopePoint = 0;
	float gainDeltaPerSample = 0;
	bool finishedEnvelope = false;
};

