#ifndef Pattern_h
#define Pattern_h

#include "PatternState.h"

/**
 * Abstract base class for all patterns
 *
 * @author Sam Minnee
 */
class Pattern {

  protected:
    PatternState *_states[NUM_STATES];
    int bpm = 120;
    bool onBeat = false;
    float beatProgress = 0;
    bool isDropping = false;

  public:
    /**
     * Prepare the pattern to start running.
     * May be called multiple times, e.g. when switching between patterns
     * @param _state->leds The LED strip to draw to
     */
    virtual void setup()
    {
    }

    /**
     * Run a single frame of the pattern
     * @param _state->leds The LED strip to draw to
     * @param fade The activation level of this pattern. Use this to fade patterns in and out
     */
    virtual void loopForState(PatternState *state, byte fade) = 0;

    /**
     * Link a PatternState to this pattern.
     */
    virtual void setState(int index, PatternState *state)
    {
      // TODO Out of bounds check
      _states[index] = state;
    };

    virtual void loop(byte fade)
    {
      for(int i = 0 ; i < NUM_STATES ; i++) {
        loopForState(_states[i], fade);
      }
    }

    virtual int getFrameLength()
    {
      return FRAME_LENGTH;
    }

    void setBpm(int _bpm)
    {
      bpm = _bpm;
    }

    void setOnBeat(bool _onBeat)
    {
      onBeat = _onBeat;
    }

    void setBeatProgress(float _beatProgress)
    {
      beatProgress = _beatProgress;
    }

    void setIsDropping(bool _isDropping)
    {
      isDropping = _isDropping;
    }

};

#endif
