#include <Bounce2.h>

class BrightnessControl {
  Bounce button;
  int index = 0; // start with lowest brightness
  byte brightnesses[3] = {20,40,60}; // 0 to 255
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
      Serial.print("brightness: ");
      Serial.println(index);
    }
  }

  byte getBrightness()
  {
    return brightnesses[index];
  }
};
