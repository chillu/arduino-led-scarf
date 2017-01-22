#include "Pattern.h"

/**
 * random colored speckles that blink in and fade smoothly
 */
class Confetti: public Pattern {

  /**
   * Rotating "base color" used by many of the patterns
   */
  uint8_t gHue = 0;

  public:
    void loopForState(PatternState *state, byte fade)
    {
      CRGB *leds = state->getLeds();
      int ledsSize = state->getLedsSize();
      fadeToBlackBy( leds, ledsSize, 10);
      int pos = random16(ledsSize);
      leds[pos] += CHSV( gHue + random8(64), 200, 255);

      EVERY_N_MILLISECONDS( 20 ) { gHue++; }
    }
};
