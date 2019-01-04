#ifndef PHPROBE_H
#define PHPROBE_H

#include <stdint.h>

class PhProbe {

	public:

		struct CalibrationValues {
			// Gain of the instrumentation amplifier connected to the analog input
			// Negative values for an inverting amplifier
			float ampGain = -4.91f;
			// The amplifier offset; since pH voltages can go negative, this should be never be 0
			float ampOffset = 2.0f;
			// Calibration values for the probe
			float probeSlope = 1.0f;
			float probeOffset = 0.0f;
			// isoelectricPh of the probe.  This is almost always 7; no calibration routine
			// is provided.
			float isoelectricPh = 7.0f;
		};

		// Current calibration values to use for reading pH
		CalibrationValues calibrationValues;

		// Number of samples to take and average when reading pH
		uint8_t numSamples = 5;
		uint8_t numCalibrationSamples = 10;
		// Wait time (milliseconds) between each sample
		uint8_t sampleInterval = 1;
		// Seconds to wait for value to stabilize when enabled
		uint16_t stabilizeDelaySecs = 6;

		PhProbe(uint8_t pin);

		// Reads and returns the current pH
		float readPh(bool stabilize = false, float temperature = 23);

		// Reset any temporary calibration data
		void resetCalibrateProbe();

		// Reads the current probe voltage and performs a calibration to the given pH
		// This needs to be called twice at two different pH's to complete calibration
		// It returns 0 on success, 1 on need more calibration points, 2 on error
		uint8_t calibrateProbe(float pH, bool stabilize = false, float temperature = 23);

		// Automatically guesses which pH is being calibrated to.
		// Returns the nominal pH standard value that was recognized (4, 7, 10) or 0 on error
		// Like calibrateProbe(), this needs to be called twice
		uint8_t autoCalibrateProbe(bool stabilize = false, float temperature = 23);

		// Calibrates the amplifier offset.
		// This should be called while the amplifier is fed an input voltage of 0
		void calibrateAmpOffset();

		// Calibrates the amplifier gain.
		// The amp offset must be set before this is called.
		// Call this while the amplifier is being supplied a specific test voltage (in the range of 100mV or so)
		void calibrateAmpGain(float testVoltage);

		// Returns the "ideal" amp gain value
		float getIdealAmpGain();

	private:

		uint8_t probePin;

		uint16_t lastCalReading = 0;
		float lastCalPh = 0;
		float lastCalTemp = 0;

		uint16_t readRaw(uint8_t samples);
		uint16_t readStableValue(uint8_t samples);
		float getNIST4TempOffset(float temp);
		float getNIST7TempOffset(float temp);
		float getNIST10TempOffset(float temp);
};


#endif

