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
#define DROP_BUTTON_PIN 7
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
#include <DropControl.h>
#include <AccellerationControl.h>

CRGB ledsCh0[NUM_LEDS_CH0];
PatternState stateCh0(NUM_LEDS_CH0, ledsCh0);

CRGB ledsCh1[NUM_LEDS_CH1];
PatternState stateCh1(NUM_LEDS_CH1, ledsCh1);

Heartbeat *heartbeat = new Heartbeat();
Pattern *patternItems[] = {
  new Bpm(),
  heartbeat,
  new Plasma(),
  new Juggle(),
  new Sinelon(),
  new Confetti()
};
PatternList patternList(6, patternItems);

CRGBPalette16 *paletteItems[] = {
  new CRGBPalette16(
    CRGB::Blue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::DarkBlue,
    CRGB::Blue,
    CRGB::DarkBlue,
    CRGB::SkyBlue,
    CRGB::SkyBlue,
    CRGB::LightBlue,
    CRGB::White,
    CRGB::LightBlue,
    CRGB::SkyBlue
  ),
  // Lava
  new CRGBPalette16(
    CRGB::Black,
    CRGB::Maroon,
    CRGB::Black,
    CRGB::Maroon,
    CRGB::DarkRed,
    CRGB::Maroon,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::DarkRed,
    CRGB::Red,
    CRGB::Orange,
    CRGB::White,
    CRGB::Orange,
    CRGB::Red,
    CRGB::DarkRed,
    CRGB::Black
  ),
  // Rainbow
  new CRGBPalette16(
    0xFF0000, 0xD52A00, 0xAB5500, 0xAB7F00,
    0xABAB00, 0x56D500, 0x00FF00, 0x00D52A,
    0x00AB55, 0x0056AA, 0x0000FF, 0x2A00D5,
    0x5500AB, 0x7F0081, 0xAB0055, 0xD5002B
  )
};
PaletteList paletteList(3, paletteItems);


BrightnessControl brightnessControl(BRIGHTNESS_BUTTON_PIN);
ModeControl modeControl(MODE_BUTTON_PIN);
BeatControl beatControl(BEAT_BUTTON_PIN);
DropControl dropControl(DROP_BUTTON_PIN);
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

  dropControl.setup();

  stateCh0.palette = paletteList.curr();
  patternList.setState(0, &stateCh0);

  stateCh1.palette = paletteList.curr();
  patternList.setState(1, &stateCh1);

  // updateModeFromEEPROM();
}

void loop() {
  // Timing
  unsigned long currentMillis = millis();

  // Mode and Palette
  modeControl.update();
  if(modeControl.rose()) {
    if(modeControl.wasLongPress()) {
      CRGBPalette16 *palette = paletteList.next();
      stateCh0.palette = palette;
      stateCh1.palette = palette;
    } else {
      patternList.next();
    }
  }
  Pattern *currPattern = patternList.curr();

  // Beat
  beatControl.update();
  currPattern->setBpm(beatControl.getBpm());
  currPattern->setOnBeat(beatControl.onBeat());
  currPattern->setBeatProgress(beatControl.beatProgress());

  // Drop
  dropControl.update();
  currPattern->setIsDropping((dropControl.read() == LOW));

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

    // Brightness
    brightnessControl.update();
    FastLED.setBrightness(brightnessControl.getBrightness());

    FastLED.show();
  }

}
