#include "Pattern.h"

/**
 * colored stripes pulsing at a defined Beats-Per-Minute (BPM)
 */
class Bpm: public Pattern {

  /**
   * Rotating "base color" used by many of the patterns
   */
  uint8_t gHue = 0;

  /**
   * "Frame" in which a single LED will glitter.
   * Smaller frames means more glitter
   */
  int glitterFrame = 20;

  void loopDefault(PatternState *state)
  {
    int halfPoint = (int)state->ledsSize/2;
    uint8_t beat = beat8( bpm);

    for( int i = 0; i <= halfPoint; i++) {
      CRGB color = ColorFromPalette(*state->palette, gHue+(i*2), beat-gHue+(i*10));
      state->leds[i] = color;
      state->leds[state->ledsSize - i - 1] = color;
    }

    // Add some glitter on parts the first beat (of four)
    if (beatProgress < 0.15) {
      CRGB firstColor = ColorFromPalette(*state->palette, gHue, 255);
      for ( int j = 0; j < (int)(state->ledsSize/glitterFrame); j++ ) {
        int min = glitterFrame * j;
        int max = glitterFrame * (j+1);
        if (max >= state->ledsSize) {
          max = state->ledsSize - 1;
        }
        // state->leds[ min ] = firstColor;
        state->leds[ random16(min,max) ] = CRGB::White;
      }
    }

    EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  }

  void loopDrop(PatternState *state)
  {
    // flash for the first beat (of four)
    if( beatProgress < 0.25 ) {
      for( int i = 0; i < state->ledsSize; i++) {
        // One in three chance of lighting up
        state->leds[i] = (random8(3) == 0) ? CRGB::White : CRGB::Black;
      }
    } else {
      for( int i = 0; i < state->ledsSize; i++) {
        state->leds[i] = CRGB::Black;
      }
    }
  }

  public:
    void loopForState(PatternState *state, byte fade)
    {
      if(isDropping) {
        loopDrop(state);
      } else {
        loopDefault(state);
      }
    }

};
