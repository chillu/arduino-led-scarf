#include "Pattern.h"

/**
 * a colored dot sweeping back and forth, with fading trails
 */
class Sinelon: public Pattern {

  /**
   * Rotating "base color" used by many of the patterns
   */
  uint8_t gHue = 0;

  public:
    void loopForState(PatternState *state, byte fade)
    {
      CRGB *leds = state->getLeds();
      int ledsSize = state->getLedsSize();
      fadeToBlackBy( leds, ledsSize, 20);
      int pos = beatsin16(13,0,ledsSize);
      leds[pos] += CHSV( gHue, 255, 192);

    }
};
