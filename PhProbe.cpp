#include "PhProbe.h"
#include <Arduino.h>

// MaxVoltage / 2^10
#define ADC_FACTOR (5.0f / 1024.0f)

PhProbe::PhProbe(uint8_t pin) {
	probePin = pin;
}

uint16_t PhProbe::readRaw(uint8_t samples) {
	uint16_t total = 0;
	for (uint8_t i = 0; i < samples; ++i) {
		total += analogRead(probePin);
		if (i != samples - 1) delay(sampleInterval);
	}
	return total / samples;
}

uint16_t PhProbe::readStableValue(uint8_t samples) {
	//uint16_t countdown = stabilizeDelaySecs * 1000;
	//uint16_t cycleTime = (samples - 1) * sampleInterval + 30;
	unsigned long endTime = millis() + stabilizeDelaySecs * 1000;
	int8_t lastDirection = 0;
	uint16_t lastValue = 0;
	for (;;) {
		uint16_t value = readRaw(samples);
		if (value == lastValue) {
			/*
			if (countdown <= cycleTime) {
				return value;
			} else {
				countdown -= cycleTime;
			}
			*/
			if (millis() >= endTime) {
				return value;
			}
		} else {
			//countdown = stabilizeDelaySecs * 1000;
			endTime = millis() + stabilizeDelaySecs * 1000;
			int8_t direction = (value > lastValue) ? 1 : -1;
			if (lastDirection != 0 && direction != lastDirection) {
				delay(stabilizeDelaySecs * 1000);
				return readRaw(samples);
			}
			if (lastValue != 0) {
				lastDirection = direction;
			}
			lastValue = value;
		}
	}
}

float PhProbe::readPh(bool stabilize, float temperature) {
	uint16_t adcReading = stabilize ? readStableValue(numSamples) : readRaw(numSamples);
	float adcVoltage = (float)adcReading * ADC_FACTOR;
	float probeVoltage = (adcVoltage - calibrationValues.ampOffset) / calibrationValues.ampGain;
	float C = 2.303f*8.314f/96490.0f;
	float pH = (calibrationValues.probeOffset - probeVoltage) / calibrationValues.probeSlope / C / (temperature + 273.15f) + calibrationValues.isoelectricPh;
	return pH;
}

void PhProbe::resetCalibrateProbe() {
	lastCalReading = 0;
}

uint8_t PhProbe::calibrateProbe(float pH, bool stabilize, float temperature) {
	uint16_t calReading = stabilize ? readStableValue(numCalibrationSamples) : readRaw(numCalibrationSamples);
	if (calReading == 0) {
		resetCalibrateProbe();
		return 2;
	}
	if (lastCalReading == 0) {
		lastCalReading = calReading;
		lastCalPh = pH;
		lastCalTemp = temperature;
		return 1;
	}
	uint16_t calDiffThreshold = 50;
	if (
			(calReading >= lastCalReading && calReading - lastCalReading < calDiffThreshold) ||
			(calReading < lastCalReading && lastCalReading - calReading < calDiffThreshold)
	) {
		// two calibration readings aren't far enough apart
		resetCalibrateProbe();
		return 2;
	}
	float C = 2.303f*8.314f/96490.0f;
	float probeVoltage1 = ((float)lastCalReading * ADC_FACTOR - calibrationValues.ampOffset) / calibrationValues.ampGain;
	float probeVoltage2 = ((float)calReading * ADC_FACTOR - calibrationValues.ampOffset) / calibrationValues.ampGain;
	float slope = (probeVoltage1 - probeVoltage2) / C / ((temperature + 273.15f) * (pH - calibrationValues.isoelectricPh) - (lastCalTemp + 273.15f) * (lastCalPh - calibrationValues.isoelectricPh));
	float offset = probeVoltage2 + slope * C * (temperature + 273.15f) * (pH - calibrationValues.isoelectricPh);
	calibrationValues.probeSlope = slope;
	calibrationValues.probeOffset = offset;
	resetCalibrateProbe();
	return 0;
}


float PhProbe::getNIST4TempOffset(float temp) {
	int8_t tableStartTemp = 5;
	int8_t tableIncTemp = 5;
	int8_t tableLen = 12;
	// This is a table of pH offsets from the nominal value based on temperature.
	// The key into the table is floor(temp / tableIncTemp) - floor(tableStartTemp / tableIncTemp)
	// The values in the table are in hundredths of a pH unit
	int8_t table[] = { 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 9 };
	int8_t key = (temp / tableIncTemp) - (tableStartTemp / tableIncTemp);
	int8_t idx = (key < 0) ? 0 : ((key >= tableLen) ? (tableLen - 1) : key);
	return (float)table[idx] * 0.01f;
}

float PhProbe::getNIST7TempOffset(float temp) {
	int8_t tableStartTemp = 5;
	int8_t tableIncTemp = 5;
	int8_t tableLen = 12;
	int8_t table[] = { 9, 6, 4, 2, 0, -1, -2, -3, -3, -4, -4, -3 };
	int8_t key = (temp / tableIncTemp) - (tableStartTemp / tableIncTemp);
	int8_t idx = (key < 0) ? 0 : ((key >= tableLen) ? (tableLen - 1) : key);
	return (float)table[idx] * 0.01f;
}

float PhProbe::getNIST10TempOffset(float temp) {
	int8_t tableStartTemp = 5;
	int8_t tableIncTemp = 5;
	int8_t tableLen = 12;
	int8_t table[] = { 25, 18, 12, 6, 1, -3, -7, -11, -14, -17, -19, -22 };
	int8_t key = (temp / tableIncTemp) - (tableStartTemp / tableIncTemp);
	int8_t idx = (key < 0) ? 0 : ((key >= tableLen) ? (tableLen - 1) : key);
	return (float)table[idx] * 0.01f;
}

uint8_t PhProbe::autoCalibrateProbe(bool stabilize, float temperature) {
	// Read a guessed value based on current parameters to try to detect the calibration standard
	uint8_t oldNumSamples = numSamples;
	numSamples = 1;
	float initialGuessPh = readPh(false, temperature);
	numSamples = oldNumSamples;

	// Determine which standard (4, 7, or 10) is closest to the guess
	uint8_t nominalStandardValue;
	if (initialGuessPh >= 8.5f) nominalStandardValue = 10;
	else if (initialGuessPh <= 5.5f) nominalStandardValue = 4;
	else nominalStandardValue = 7;

	// Adjust standard value by temperature
	float standardValue;
	if (nominalStandardValue == 10) standardValue = 10.0f + getNIST10TempOffset(temperature);
	else if (nominalStandardValue == 4) standardValue = 4.0f + getNIST4TempOffset(temperature);
	else standardValue = 7.0f + getNIST7TempOffset(temperature);

	// Perform the calibration
	uint8_t res = calibrateProbe(standardValue, stabilize, temperature);
	if (res == 2) return 0;
	return nominalStandardValue;
}

void PhProbe::calibrateAmpOffset() {
	uint16_t reading = readRaw(numCalibrationSamples);
	float voltage = (float)reading * ADC_FACTOR;
	calibrationValues.ampOffset = voltage;
}

void PhProbe::calibrateAmpGain(float testVoltage) {
	uint16_t reading = readRaw(numCalibrationSamples);
	float voltage = (float)reading * ADC_FACTOR - calibrationValues.ampOffset;
	calibrationValues.ampGain = voltage / testVoltage;
}

float PhProbe::getIdealAmpGain() {
	return calibrationValues.ampOffset / (0.058f * 7.0f);
}


