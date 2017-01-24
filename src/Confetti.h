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
      fadeToBlackBy( state->leds, state->ledsSize, 10);
      int pos = random16(state->ledsSize);
      if (isDropping) {
        // Colorful
        state->leds[pos] += CHSV( gHue + random8(64), 200, 255);
      } else {
        // Default Palette
        state->leds[pos] += ColorFromPalette(*state->palette, gHue, 255);
      }

      EVERY_N_MILLISECONDS( 20 ) { gHue++; }
    }
};
