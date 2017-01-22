#include "Pattern.h"

/**
 * Eight colored dots, weaving in and out of sync with each other
 */
class Juggle: public Pattern {
  public:
    void loopForState(PatternState *state, byte fade)
    {
      CRGB *leds = state->getLeds();
      int ledsSize = state->getLedsSize();
      fadeToBlackBy( leds, ledsSize, 20);
      byte dothue = 0;
      for( int i = 0; i < 8; i++) {
        leds[beatsin16(i+7,0,ledsSize)] |= CHSV(dothue, 200, 255);
        dothue += 32;
      }
    }
};
