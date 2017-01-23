#include <Bounce2.h>

class ModeControl {
  Bounce _button;
  int pin;
  unsigned long _millisAtPress = 0;
  int _longPressMillis = 1000;
  bool _wasLongPress = false;

public:
  ModeControl(int _pin): pin(_pin)
  {
    // no-op
  }

  void setup()
  {
    pinMode(pin, INPUT_PULLUP);
    _button.attach(pin);
    _button.interval(50);
  }

  void update()
  {
    _button.update();
    if (_button.fell()) {
      _millisAtPress = millis();
    }
    if (_button.rose()) {
      _wasLongPress = ((millis() - _millisAtPress) > _longPressMillis);
      _millisAtPress = 0;
    }
  }

  bool fell()
  {
    return _button.fell();
  }

  bool rose()
  {
    return _button.rose();
  }

  /**
   * Was the last press a long one?
   */
  bool wasLongPress()
  {
    return _wasLongPress;
  }
};
