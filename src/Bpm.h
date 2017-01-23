#include "Pattern.h"

/**
 * colored stripes pulsing at a defined Beats-Per-Minute (BPM)
 */
class Bpm: public Pattern {

  /**
   * Rotating "base color" used by many of the patterns
   */
  uint8_t gHue = 0;

  public:
    void loopForState(PatternState *state, byte fade)
    {
      CRGBPalette16 *palette = state->palette;
      uint8_t beat = beatsin8( bpm, 64, 255);
      for( int i = 0; i < state->ledsSize; i++) { //9948
        state->leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
      }

      EVERY_N_MILLISECONDS( 20 ) { gHue++; }
    }
};
