#include "Pattern.h"

/**
 * Sine wave (single factor plasma) that moves up the strip
 */
class Plasma: public Pattern {
  public:
    void loop(byte fade)
    {
      CRGBPalette16 *palette = _state->getPalette();

      for (int i = 0; i < NUM_LEDS; i++) {
        byte c = sin8((long) i * 30 - millis() / 2);
        byte colorindex = scale8(c, 200);
        _state->leds[i] = ColorFromPalette(*palette, colorindex);
      }
    }
};
