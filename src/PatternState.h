#ifndef PatternState_h
#define PatternState_h

/**
 * Provides shared state between patterns.
 * State for a pattern can be memory intensive and with only 2KB of SRAM availble,
 * it's not practical to have separate state for each pattern.
 *
 * Rather than use globals, we make use of a PatternState object.
 *
 * @author Sam Minnee
 */
class PatternState {
  protected:
    /**
     * A byte that can be used to track 'activation level' per LED
     * Patterns can use this as they wish
     */
    byte *activation;

    /**
     * A palette to for the patterns to use.
     */
    CRGBPalette16 *_palette;

    /**
     * The LEDs to write to
     */
    CRGB *_leds;

    byte _ledsSize;

  public:
    PatternState(byte ledsSize, CRGB *leds): activation(0)
    {
      _ledsSize = ledsSize;
      
      _leds = leds;
      for(byte i = 0; i < _ledsSize; i++) {
        _leds[i] = CRGB::Black;
      }
    };

    byte *getActivation()
    {
      // Allocate the byte array if it hasn't realy been allocated
      if ((unsigned int)activation == 0) {
        Serial.println("Allocating activation");
        activation = new byte[_ledsSize];
        for(byte i = 0; i < _ledsSize; i++) {
          activation[i] = 0;
        }
      }

      return activation;
    };

    void setPalette(CRGBPalette16 *palette)
    {
      _palette = palette;
    };

    CRGBPalette16 *getPalette()
    {
      return _palette;
    };

    CRGB *getLeds()
    {
      return _leds;
    }

    int getLedsSize()
    {
      return _ledsSize;
    }

};

#endif
