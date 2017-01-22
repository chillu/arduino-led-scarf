/**
 * Calculates a magnitude based on accellerometer reads.
 * Smooths out gains and losses to transition between different readings over time.
 *
 * The maxMagnitude value depends on the sensor and setup,
 * some sensors will send down more current based on the same amount of motion.
 * Adjust accordingly.
 */
class AccellerationControl {

  int xPin;
  int yPin;
  int zPin;
  int currentMagnitude;
  int minMagnitude = 0;
  int maxMagnitude = 40; // max difference between two magnitude measurements
  int gainRatePerBeat = 10; // change towards new target magnitude
  int decayRatePerBeat = 1; // move back towards minMagnitude
  int sampleSize = 8; // smooth out accelerometer readings
  int targetMagnitude = maxMagnitude;
  int adjustedMagnitude = maxMagnitude;

  /**
   * Get a magnitude across all vectors.
   * Smooth out result through a rolling average
   */
  int getRawMagnitude() {
    float avgMag = 0;
    for(int x = 0 ; x < sampleSize ; x++) {
      float aX = analogRead(xPin);
      float aY = analogRead(yPin);
      float aZ = analogRead(zPin);

      float magnitude = sqrt((aX * aX) + (aY * aY) + (aZ * aZ));
      avgMag += magnitude;

      // Serial.print(aX);
      // Serial.print(",");
      // Serial.print(aY);
      // Serial.print(",");
      // Serial.print(aZ);
      // Serial.println();
    }
    avgMag /= sampleSize;

    // Serial.println(avgMag);

    return avgMag;
  }

public:
  AccellerationControl(int _xPin, int _yPin, int _zPin):
    xPin(_xPin), yPin(_yPin), zPin(_zPin)
  {
    // no-op
  }

  void setup()
  {
  }

  void update()
  {
    int newMagnitude = getRawMagnitude();
    int magnitudeDiff = abs(currentMagnitude - newMagnitude);

    // Get new target (smoothed out over a couple of readings)
    targetMagnitude = max(
      constrain(magnitudeDiff, minMagnitude, maxMagnitude),
      targetMagnitude
    );

    // Slowly work towards new target (either increasing or decreasing)
    if(adjustedMagnitude <= targetMagnitude) {
      adjustedMagnitude = constrain(
        constrain(
          targetMagnitude + gainRatePerBeat,
          minMagnitude,
          maxMagnitude
        ),
        minMagnitude,
        maxMagnitude
      );
    } else {
      adjustedMagnitude = constrain(
        constrain(
          targetMagnitude - gainRatePerBeat,
          minMagnitude,
          maxMagnitude
        ),
        minMagnitude,
        maxMagnitude
      );
    }

    // Slowly decay max target
    targetMagnitude = targetMagnitude - decayRatePerBeat;
    targetMagnitude = constrain(
      targetMagnitude,
      minMagnitude,
      maxMagnitude
    );

  //  Serial.print(magnitudeDiff);
  //  Serial.print("\t");
  //  Serial.print(targetMagnitude);
  //  Serial.print("\t");
  //  Serial.print(adjustedMagnitude);
  //  Serial.println();

   currentMagnitude = newMagnitude;
  }

  int getAdjustedMagnitude()
  {
    return adjustedMagnitude;
  }

};
