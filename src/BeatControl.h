#include <ArduinoTapTempo.h>

class BeatControl {
  int pin;
  ArduinoTapTempo tapTempo;

public:
  BeatControl(int _pin): pin(_pin)
  {
    // no-op
  }

  void setup()
  {
    pinMode(pin, INPUT_PULLUP);
  }

  void update()
  {
    boolean buttonDown = digitalRead(pin) == LOW;
    tapTempo.update(buttonDown);
  }

  float getBpm()
  {
    return tapTempo.getBPM();
  }

  bool onBeat()
  {
    return tapTempo.onBeat();
  }

  float beatProgress()
  {
    return tapTempo.beatProgress();
  }
};
