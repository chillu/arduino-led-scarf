#include "Pattern.h"

/**
 * Eight colored dots, weaving in and out of sync with each other
 */
class Juggle: public Pattern {
  public:
    void loopForState(PatternState *state, byte fade)
    {
      fadeToBlackBy( state->leds, state->ledsSize, 20);
      byte dothue = 0;
      for( int i = 0; i < 8; i++) {
        state->leds[beatsin16(i+7,0,state->ledsSize)] |= CHSV(dothue, 200, 255);
        dothue += 32;
      }
    }

    int getFrameLength()
    {
      return 1000 / bpm / 2;
    }
};
