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
      fadeToBlackBy( state->leds, state->ledsSize, 20);
      int pos = beatsin16(13, 0, state->ledsSize);
      state->leds[pos] += CHSV( gHue, 255, 192);

      EVERY_N_MILLISECONDS( 20 ) { gHue++; }
    }

    int getFrameLength()
    {
      return 1000 / bpm / 2;
    }
};
