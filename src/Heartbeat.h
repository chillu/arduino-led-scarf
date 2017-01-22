#include "Pattern.h"

#define HEARTBEAT_MODE_SPLIT 0
#define HEARTBEAT_MODE_FULL 1

/**
 * random colored speckles that blink in and fade smoothly
 */
class Heartbeat: public Pattern {

  // config
  byte beat[22]  = {2,2,2,2,3,4,3,2,1,0,10,2,2,3,4,6,8,5,3,3,3,3}; // From http://ecg.utah.edu/img/items/Normal%2012_Lead%20ECG.jpg
  byte beatLength = 22;
  int beatMaxIntensity = 10; // max value in beat array
  int movesPerBeat = 4;
  int minBrightnessDivisor= 150; // lower = brighter
  int maxBrightnessDivisor = 300; // higher = dimmer
  int minHue = 160; // hue value under low motion, as defined through minMagnitude (from 0-255)
  int maxHue = 255; // hue value under high motion, as defined through maxMagnitude (from 0-255)
  int minMagnitude = 0;
  int maxMagnitude = 50; // max difference between two magnitude measurements

  // state
  int bpm = 60;
  byte bufr[NUM_LEDS_CH0]; // TODO Remove hardcoding, relies on first channel being "split" mode
  byte offset = 0;
  int magnitude = 25;
  int mode = HEARTBEAT_MODE_FULL;
  int hue = 0;
  int brightnessFactor = 0;

  void moveBuffer(int ledsSize) {
    int i;
    byte c = (int) beat[offset] * 255 / beatMaxIntensity;
    int midPoint = ledsSize/2;

    // Move current beat intensity from middle to beginning of strip (backward in buffer)
    bufr[midPoint - 1] = c;
    for (i=0;i<midPoint - 1;i++){
      bufr[i] = bufr[i+1];
    }

    // Move current beat intensity from middle to end of strip (forward in buffer)
    bufr[midPoint] = c;
    for (i=ledsSize - 1;i>midPoint;i--){
      bufr[i] = bufr[i-1];
    }
  }

  void updateParameters() {
    // Go from a cool color on low to a warm color on high activity
    hue = map(
      magnitude,
      minMagnitude,
      maxMagnitude,
      minHue,
      maxHue
    );

    // Lower magnitude means more brightness reduction
    brightnessFactor = map(
      magnitude,
      minMagnitude,
      maxMagnitude,
      maxBrightnessDivisor,
      minBrightnessDivisor
    );
  }

  public:
    void loop(byte fade)
    {
      offset = (offset + 1) % beatLength;
      int i;
      for (i=0;i<movesPerBeat;i++){
        // Delay in milliseconds. BPM are measured by minute, so divide accordingly.
        delay((float) 1 / (bpm * 60 * 1000) / beatLength / movesPerBeat);
        updateParameters();

        for(int i = 0 ; i < NUM_STATES ; i++) {
          loopForState(_states[i], fade);
        }

        FastLED.show();
        // Don't use FastLED.delay() here, we've got custom timings
      }
    }

    void loopForState(PatternState *state, byte fade)
    {
      CRGB *leds = state->getLeds();
      int ledsSize = state->getLedsSize();

      int i;
      if (mode == HEARTBEAT_MODE_SPLIT) {
        // TODO Should have separate buffer
        int halfPoint = (int)sizeof(bufr)/2;
        for (i=0;i<ledsSize;i++){
            leds[i] = CHSV(hue, bufr[halfPoint], bufr[halfPoint]/(brightnessFactor/100));
        }
      } else if(mode == HEARTBEAT_MODE_FULL) {
        moveBuffer(ledsSize);
        for (i=0;i<ledsSize;i++){
            leds[i] = CHSV(hue, bufr[i], bufr[i]/(brightnessFactor/100));
        }
      } else {
        // TODO Exception
      }
    }

    void setMagnitude(int _magnitude)
    {
      magnitude = _magnitude;
    }
};
