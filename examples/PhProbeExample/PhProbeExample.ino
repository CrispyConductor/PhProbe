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
