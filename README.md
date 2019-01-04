# Arduino pH Probe Library

## Features

- 2-Point calibration
- Temperature correction
- Automatic calibration standard detection
- Instrumentation amplifier calibration
- Wait for reading stabilization

## Requirements & Setup

A pH probe generates a small positive or negative voltage in response to the pH of the solution it is
immersed in.  This voltage is small (about 58 mV per unit pH) and has very little current, so it should
not be connected directly to the arduino analog input.  Some kind of instrumentation amplifier needs to
be used between the probe and the arduino.

This instrumentation amplifier can be defined by two properties; its offset and its gain.  Because the
pH probe can produce negative voltages, and the arduino can only read voltages from 0V to 5V, the amplifier
needs to have an offset of around 2V-2.5V.  The gain should be sufficient to map the desired pH range to
within the range of the arduino ADC.  At an offset of 2V, a gain of 4.9 is about right to represent the
whole range of 0pH to 14pH.  The gain can also be negative (ie, inverting amplifiers are supported).

To connect the probe, connect it to the instrumentation amplifier, then the amplifier to one of the
analog in ports on the arduino.  The amplifier parameters need to be set to their correct values in the
library (see below).

## Example

```cpp
#include <PhProbe.h>

// Create a PhProbe instance and supply the pin it's connected to
PhProbe phprobe(A0);

void setup() {
  pinMode(A0, INPUT);
  Serial.begin(9600);
  // Configure amplifier parameters
  phprobe.calibrationValues.ampOffset = 2.0f;
  phprobe.calibrationValues.ampGain = 4.9f;
}

void loop() {
  // Delay for bootup and serial connection
  Serial.println("Waiting 10s");
  delay(10000);

  // Allow time to rinse probe and place in pH standard
  Serial.println("Prepare to calibrate first standard in 10s");
  delay(10000);
  Serial.println("Calibrating ...");

  // Begin 2-point calibration
  phprobe.resetCalibrateProbe();

  // Record the first calibration sample, autodetecting a 4pH, 7pH, or 10pH standard
  uint8_t standard;
  standard = phprobe.autoCalibrateProbe(true);
  Serial.print("Calibrated to standard ");
  Serial.println(standard);

  Serial.println("Prepare to calibrate second standard in 20s");
  delay(20000);
  Serial.println("Calibrating ...");

  // Record the second calibration sample
  standard = phprobe.autoCalibrateProbe(true);
  Serial.print("Calibrated to standard ");
  Serial.println(standard);

  // Print the calibration values determined
  Serial.println("Slope");
  Serial.println(phprobe.calibrationValues.probeSlope);
  Serial.println("Offset");
  Serial.println(phprobe.calibrationValues.probeOffset);

  // Print pH measurements in a loop
  Serial.println("Looping measurement");
  for (;;) {
    float ph = phprobe.readPh();
    Serial.println(ph);
    delay(2000);
  }
}
```

## Configuring Amplifier Parameters

The amplifier parameters for offset and gain are accessible at `phprobe.calibrationValues.ampOffset`
and `phprobe.calibrationValues.ampGain`.  They default to 2.0 and -4.91, respectively.  These values
can be set at any time; although probe calibration should be performed every time the values are changed.
Usually these values are set once in the application (according to the amplifier's hardware characteristics)
and do not need to be changed again or recalibrated.

If you are working with an amplifier with unknown parameters, methods are provided to determine them.
To determine the amp offset, apply 0V to the input of the amplifier (ie, just a wire connected through
a resistor across the terminals) and call `phprobe.calibrateAmpOffset()`.  To determine the gain, apply
a small known voltage (roughly in the range of 100mV) to the input of the amplifier and call
`phprobe.calibrateAmpGain(0.1f)`, supplying the test voltage at the amplifier input.  The offset must
be set correctly before calibrating gain.

If your amplifier has adjustable gain, you may want to set the gain to a value optimized for range and
resolution.  Such a value of gain can be determined by calling `phprobe.getIdealAmpGain()`.


## Calibrating Probe

This library supports 2-point probe calibration, where two different standard pH solutions are measured
to determine the calibration parameters.  To calibrate, immerse the probe in one standard solution (eg. 4pH)
and call `phprobe.calibrateProbe(4.0f)`.  Then immerse the probe in a different standard solution and call
`calibrateProbe()` again with that pH value.  After the second call, the probe calibration parameters will
be set.

The `calibrateProbe()` function also takes two additional parameters:

`uint8_t calibrateProbe(float pH, bool stabilize = false, float temperature = 23)`

If `stabilize` is set to true, then the function will wait until the reading stabilizes before recording
the calibration values.  The `temperature` parameter is the temperature of the sample in Celsius.

It returns 0 upon successful calibration, 1 after the first datapoint (and a second is needed), or 2 on error.

An additional function `resetCalibrateProbe()` is provided that can reset the calibration process after
the first reading is taken, or in case of an error.


## Automatic Calibration

In addition to `calibrateProbe()`, the method `autoCalibrateProbe()` is provided.  It works similarly, but does
not take a pH parameter.  Instead, it guesses which of the standard NIST samples (pH 4, pH 7, or pH 10) is
being used.  It also performs standard temperature correction based on the temperature tables for the NIST
standards.  This function returns an int 4, 7, or 10, indicating which standard was recognized.  0 is returned
on error.


## Reading pH

After the amplifier is configured, and the probe is calibrated, the pH can be read using the `phprobe.readPh()`
method.  It accepts `stabilize` and `temperature` parameters and returns the pH.


## Saving Calibration Values

Calibration values can be saved and restored just by saving the contents of the `phprobe.calibrationValues`
struct to EEPROM.



