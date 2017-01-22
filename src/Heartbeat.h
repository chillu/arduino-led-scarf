#include "Pattern.h"

#define HEARTBEAT_MODE_SPLIT 0
#define HEARTBEAT_MODE_FULL 1

/**
 * random colored speckles that blink in and fade smoothly
 */
class Heartbeat: public Pattern {

  // config
  byte beat[22]  = {2,2,2,2,3,4,3,2,1,0,10,2,2,3,4,6,8,5,3,3,3,3}; // From http://ecg.utah.edu/img/items/Normal%2012_Lead%20ECG.jpg
  int beatLength = 22;
  int beatMaxIntensity = 10; // max value in beat array
  int movesPerBeat = 2; // determines the speed, not the tempo
  int minBrightnessDivisor= 150; // lower = brighter
  int maxBrightnessDivisor = 300; // higher = dimmer
  int minHue = 160; // hue value under low motion, as defined through minMagnitude (from 0-255)
  int maxHue = 255; // hue value under high motion, as defined through maxMagnitude (from 0-255)
  int minMagnitude = 0;
  int maxMagnitude = 600; // max difference between two magnitude measurements

  // state
  int bpm = 60;
  byte bufr[NUM_LEDS_CH0]; // TODO Remove hardcoding, relies on first channel being "split" mode
  byte offset = 0;
  int magnitude = 10;
  int mode = HEARTBEAT_MODE_FULL;
  int hue = 0;
  int brightnessFactor = 220;

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
      updateParameters();

      for (int i=0;i<movesPerBeat;i++){
        // Only move *one* buffer for all states, for performance reasons
        // TODO Remove assumption that first state has more LEDs
        moveBuffer(_states[0]->ledsSize);

        // TODO Switch to NUM_STATES
        for(int j=0; j<NUM_STATES; j++) {
          loopForState(_states[j], fade);
        }
      }
    }

    void loopForState(PatternState *state, byte fade)
    {
      int i;
      if (mode == HEARTBEAT_MODE_SPLIT) {
        int halfPoint = (int)sizeof(bufr)/2;
        for (i=0;i<state->ledsSize;i++){
            state->leds[i] = CHSV(hue, bufr[halfPoint], bufr[halfPoint]/(brightnessFactor/100));
        }
      } else if(mode == HEARTBEAT_MODE_FULL) {
        for (i=0;i<state->ledsSize;i++){
            state->leds[i] = CHSV(hue, bufr[i], bufr[i]/(brightnessFactor/100));
        }
      } else {
        // TODO Exception
      }
    }

    void setMagnitude(int _magnitude)
    {
      magnitude = _magnitude;
    }

    int getFrameLength()
    {
      // Delay in milliseconds. BPM are measured by minute, so divide accordingly.
      long msPerBeat = (unsigned int)((60*1000) / (unsigned int)bpm);
      return (int)((unsigned int)msPerBeat / (int)beatLength);
    }
};
