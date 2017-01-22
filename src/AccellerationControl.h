/**
 * Calculates a magnitude based on accellerometer reads.
 * Smooths out gains and losses to transition between different readings over time.
 */
class AccellerationControl {

  int xPin;
  int yPin;
  int zPin;
  int magnitude;
  int minMagnitude = 0;
  int maxMagnitude = 50; // max difference between two magnitude measurements
  int gainRatePerBeat = 6; // change towards new target magnitude
  int decayRatePerBeat = 2; // move back towards minMagnitude
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
    }
    avgMag /= sampleSize;

    return avgMag;
  }

  int calcMagnitude() {
    int newMagnitude = getRawMagnitude();
    int magnitudeDiff = abs(magnitude - newMagnitude);

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

    return newMagnitude;
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
    magnitude = calcMagnitude();
  }

  int getMagnitude()
  {
    return magnitude;
  }

};
