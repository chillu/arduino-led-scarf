#include "Pattern.h"
#include <FastLED.h>

/**
 * Represents a list of patterns that you can switch between
 *
 * @author Sam Minnee
 */
class PatternList {
  private:
    byte _curPattern;
    byte _numPatterns;
    Pattern **_patterns;

  public:
    PatternList(byte numPatterns, Pattern **patterns): _curPattern(0), _numPatterns(numPatterns), _patterns(patterns) {
    }

    void setup() {
      _patterns[_curPattern]->setup();
    }

    void loop(byte fade) {
      _patterns[_curPattern]->loop(fade);
    }

    void setState(int index, PatternState *state)
    {
      for(byte i = 0; i < _numPatterns; i++) {
        _patterns[i]->setState(index, state);
      }
    }

    /**
     * Switch to the next ponattern
     */
    Pattern* next() {
      _curPattern = (_curPattern + 1) % _numPatterns;
      setup();
      return _patterns[_curPattern];
    }

    Pattern* curr()
    {
      return _patterns[_curPattern];
    }

    /**
     * Switch to a random pattern
     */
    void rand() {
      _curPattern = random(_numPatterns);
      setup();
    }
};
