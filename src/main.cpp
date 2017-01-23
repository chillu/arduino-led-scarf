#include <Bounce2.h>
#include <FastLED.h>
// #include <EEPROM.h>

#define DEBUG

#ifdef DEBUG
  #define DEBUG_PRINT(msg) (Serial.println(msg))
#else
  #define DEBUG_PRINT(msg) ()
#endif

// Digital PINs
#define LED_PIN_CH1 4
#define LED_PIN_CH2 2
#define MODE_BUTTON_PIN 6
#define BRIGHTNESS_BUTTON_PIN 8
#define BEAT_BUTTON_PIN 10

// Analog PINs
#define ACCELX_PIN A0
#define ACCELY_PIN A2
#define ACCELZ_PIN A4

// Other constants
#define BAUD_RATE 9600
#define NUM_LEDS_CH0 120
#define NUM_LEDS_CH1 29
#define FRAME_LENGTH 33 // 30 fps
#define NUM_STATES 2
#define MAX_MILLIAMPS 500 // should run for ~8h on 2x2000maH 18650

#include <Pattern.h>
#include <PatternList.h>
#include <PatternState.h>
#include <PaletteList.h>

#include <Plasma.h>
#include <Juggle.h>
#include <Sinelon.h>
#include <Confetti.h>
#include <Heartbeat.h>
#include <Bpm.h>

#include <BrightnessControl.h>
#include <ModeControl.h>
#include <BeatControl.h>
#include <AccellerationControl.h>

CRGB ledsCh0[NUM_LEDS_CH0];
PatternState stateCh0(NUM_LEDS_CH0, ledsCh0);

CRGB ledsCh1[NUM_LEDS_CH1];
PatternState stateCh1(NUM_LEDS_CH1, ledsCh1);

Heartbeat *heartbeat = new Heartbeat();
Pattern *patternItems[] = { new Bpm(), heartbeat, new Plasma(), new Juggle(), new Sinelon(), new Confetti() };
PatternList patternList(6, patternItems);

CRGBPalette16 *paletteItems[] = {
  new CRGBPalette16(CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White),
  new CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::White)
};
PaletteList paletteList(2, paletteItems);


BrightnessControl brightnessControl(BRIGHTNESS_BUTTON_PIN);
ModeControl modeControl(MODE_BUTTON_PIN);
BeatControl beatControl(BEAT_BUTTON_PIN);
AccellerationControl accellerationControl(ACCELX_PIN, ACCELY_PIN, ACCELZ_PIN);

// See https://learn.adafruit.com/multi-tasking-the-arduino-part-1/using-millis-for-timing
long previousMillis = 0;

// // Cycle mode in persistent memory on every on switch.
// // More resilient against hardware failures than a button.
// void updateModeFromEEPROM() {
//   mode = EEPROM.read(0);
//   if(!mode) {
//     mode = 1;
//   }
//   mode = (mode % modeCount) + 1;
//
//   EEPROM.write(0, mode);
// }

void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN_CH1>(ledsCh0, NUM_LEDS_CH0);
  FastLED.addLeds<NEOPIXEL, LED_PIN_CH2>(ledsCh1, NUM_LEDS_CH1);

  Serial.begin(BAUD_RATE);

  // https://github.com/FastLED/FastLED/wiki/Power-notes#managing-power-in-fastled
  FastLED.setMaxPowerInVoltsAndMilliamps(5,MAX_MILLIAMPS);

  brightnessControl.setup();
  FastLED.setBrightness(brightnessControl.getBrightness());

  modeControl.setup();

  beatControl.setup();

  stateCh0.palette = paletteList.curr();
  patternList.setState(0, &stateCh0);

  stateCh1.palette = paletteList.curr();
  patternList.setState(1, &stateCh1);

  // updateModeFromEEPROM();
}

void loop() {
  // Timing
  unsigned long currentMillis = millis();

  // Brightness
  brightnessControl.update();
  FastLED.setBrightness(brightnessControl.getBrightness());

  // Mode and Palette
  modeControl.update();
  if(modeControl.fell()) {
    patternList.next();
    // TODO Long press mode
    // CRGBPalette16 *palette = paletteList.next();
    // stateCh0.palette = palette;
    // stateCh1.palette = palette;
  }
  Pattern *currPattern = patternList.curr();

  // Beat
  beatControl.update();
  currPattern->setBpm(beatControl.getBpm());
  currPattern->setOnBeat(beatControl.onBeat());
  currPattern->setBeatProgress(beatControl.beatProgress());

  // Accelleration
  int magnitude;
  EVERY_N_MILLISECONDS(100) {
    // Expensive calc, perform sparingly
    accellerationControl.update();
    magnitude = accellerationControl.getAdjustedMagnitude();
    heartbeat->setMagnitude(magnitude);
  }

  // Patterns
  int frameLength = currPattern->getFrameLength();
  // Should use EVERY_N_MILLISECONDS, but the C++ macro
  // can't seem to change values dynamically
  if(currentMillis - previousMillis > frameLength) {
    previousMillis = currentMillis;
    patternList.loop(0);
    FastLED.show();
  }

}
