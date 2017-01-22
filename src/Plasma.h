#include "Pattern.h"

/**
 * Sine wave (single factor plasma) that moves up the strip
 */
class Plasma: public Pattern {
  public:
    void loopForState(PatternState *state, byte fade)
    {
      CRGBPalette16 *palette = state->getPalette();
      CRGB *leds = state->getLeds();
      int ledsSize = state->getLedsSize();

      for (int i = 0; i < ledsSize; i++) {
        byte c = sin8((long) i * 30 - millis() / 2);
        byte colorindex = scale8(c, 200);
        leds[i] = ColorFromPalette(*palette, colorindex);
      }
    }
};
