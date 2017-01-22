#include <Bounce2.h>

class BeatControl {
  Bounce button;
  int pin;

public:
  BeatControl(int _pin): pin(_pin)
  {
    // no-op
  }

  void setup()
  {
    pinMode(pin, INPUT_PULLUP);
    button.attach(pin);
    button.interval(50);
  }

  void update()
  {
    button.update();
  }

  bool fell()
  {
    return button.fell();
  }
};
