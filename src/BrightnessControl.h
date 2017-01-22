#include <Bounce2.h>

class BrightnessControl {
  Bounce button;
  int index = 1; // start with medium brightness
  byte brightnesses[3] = {30,60,90};
  int pin;

public:
  BrightnessControl(int _pin): pin(_pin)
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
    if(button.fell()) {
      index = ((index + 1) % 3);
      Serial.print("index: ");
      Serial.println(index);
    }
  }

  byte getBrightness()
  {
    return brightnesses[index];
  }
};
