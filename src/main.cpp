#include <Bounce2.h>
#include <FastLED.h>
#include <EEPROM.h>

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
#define FRAMES_PER_SECOND 30
#define NUM_STATES 2

#include <Pattern.h>
#include <PatternList.h>
#include <PatternState.h>

#include <Plasma.h>
#include <Juggle.h>
#include <Sinelon.h>
#include <Confetti.h>

#include <BrightnessControl.h>

// ui
int mode = 1;
int modeCount = 2;
Bounce modeButton;
Bounce beatButton;

CRGB ledsCh0[NUM_LEDS_CH0];
PatternState stateCh0(NUM_LEDS_CH0, ledsCh0);

CRGB ledsCh1[NUM_LEDS_CH1];
PatternState stateCh1(NUM_LEDS_CH1, ledsCh1);

CRGBPalette16 palette(CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
Pattern *patternItems[] = { new Plasma(), new Juggle(), new Sinelon(), new Confetti() };
PatternList patternList(4, patternItems);

BrightnessControl brightnessControl(BRIGHTNESS_BUTTON_PIN);

// config
int staticBpm = 60; // in case no pulse is found
int pulseBpm = 0; // actually detected value
int maxBpm = 180; // avoids the system going crazy
byte beat[]  = {2,2,2,2,3,4,3,2,1,0,10,2,2,3,4,6,8,5,3,3,3,3}; // From http://ecg.utah.edu/img/items/Normal%2012_Lead%20ECG.jpg
byte beatLength = 22;
int beatMaxIntensity = 10; // max value in beat array
int movesPerBeat = 4;
int minMagnitude = 0;
int maxMagnitude = 50; // max difference between two magnitude measurements from accelerometer at which we show maxHue
int minBrightnessDivisor= 150; // lower = brighter
int maxBrightnessDivisor = 300; // higher = dimmer
int minHue = 160; // hue value under low motion, as defined through minMagnitude (from 0-255)
int maxHue = 255; // hue value under high motion, as defined through maxMagnitude (from 0-255)
int gainRatePerBeat = 6; // change towards new target magnitude
int decayRatePerBeat = 2; // move back towards minMagnitude
int sampleSize = 8; // smooth out accelerometer readings
int samplePerBeat = 1;

// state
int bpm = staticBpm;
byte bufr[NUM_LEDS_CH0];
byte offset = 0;
int currentMagnitude;
int targetMagnitude = maxMagnitude;
int adjustedMagnitude = maxMagnitude;

// pulse state
volatile boolean pulseFound = false; // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean pulseBeatFound = false; // becomes true when Arduoino finds a beat.
volatile boolean hasPulse = false; //

CRGB leds[2][NUM_LEDS_CH0];

// Cycle mode in persistent memory on every on switch.
// More resilient against hardware failures than a button.
void updateModeFromEEPROM() {
  mode = EEPROM.read(0);
  if(!mode) {
    mode = 1;
  }
  mode = (mode % modeCount) + 1;

  EEPROM.write(0, mode);
}

void updateModeFromButton() {
  modeButton.update();
  if(modeButton.fell()) {
    patternList.next();
  }
}

void moveBuffer() {
  int i;
  byte c = (int) beat[offset] * 255 / beatMaxIntensity;
  int midPoint = NUM_LEDS_CH0/2;

  // Move current beat intensity from middle to beginning of strip (backward in buffer)
  bufr[midPoint - 1] = c;
  for (i=0;i<midPoint - 1;i++){
    bufr[i] = bufr[i+1];
  }

  // Move current beat intensity from middle to end of strip (forward in buffer)
  bufr[midPoint] = c;
  for (i=NUM_LEDS_CH0 - 1;i>midPoint;i--){
    bufr[i] = bufr[i-1];
  }
}

void draw() {
  int i;

  // Go from a cool color on low to a warm color on high activity
  int hue = map(
    adjustedMagnitude,
    minMagnitude,
    maxMagnitude,
    minHue,
    maxHue
  );

  // Lower magnitude means more brightness reduction
  int brightnessFactor = map(
    adjustedMagnitude,
    minMagnitude,
    maxMagnitude,
    maxBrightnessDivisor,
    minBrightnessDivisor
  );

  // Channel 1
  for (i=0;i<NUM_LEDS_CH0;i++){
      leds[0][i] = CHSV(hue, bufr[i], bufr[i]/(brightnessFactor/100));
  }

  // Channel 2
  int halfPoint = (int)sizeof(bufr)/2;
  for (i=0;i<NUM_LEDS_CH1;i++){
      leds[1][i] = CHSV(hue, bufr[halfPoint], bufr[halfPoint]/(brightnessFactor/100));
  }

  FastLED.show();
}

// Get a magnitude across all vectors.
// Smooth out result through a rolling average
int getMagnitude() {
  float avgMag = 0;
  for(int x = 0 ; x < sampleSize ; x++) {
    float aX = analogRead(ACCELX_PIN);
    float aY = analogRead(ACCELY_PIN);
    float aZ = analogRead(ACCELZ_PIN);

    float magnitude = sqrt((aX * aX) + (aY * aY) + (aZ * aZ));
    avgMag += magnitude;
  }
  avgMag /= sampleSize;

  return avgMag;
}

void calcAdjustedMagnitude() {
  int newMagnitude = getMagnitude();
  int magnitudeDiff = abs(currentMagnitude - newMagnitude);

  // Get new target (smoothed out over a couple of readings)
  targetMagnitude = max(
    constrain(magnitudeDiff, minMagnitude, maxMagnitude),
    targetMagnitude
  );

  // Slowly work towards new target (either increasing or decreasing)
  if(adjustedMagnitude <= targetMagnitude) {
    adjustedMagnitude = constrain(
      constrain(
        targetMagnitude + gainRatePerBeat,
        minMagnitude,
        maxMagnitude
      ),
      minMagnitude,
      maxMagnitude
    );
  } else {
    adjustedMagnitude = constrain(
      constrain(
        targetMagnitude - gainRatePerBeat,
        minMagnitude,
        maxMagnitude
      ),
      minMagnitude,
      maxMagnitude
    );
  }

  // Slowly decay max target
  targetMagnitude = targetMagnitude - decayRatePerBeat;
  targetMagnitude = constrain(
    targetMagnitude,
    minMagnitude,
    maxMagnitude
  );

//  Serial.print(magnitudeDiff);
//  Serial.print("\t");
//  Serial.print(targetMagnitude);
//  Serial.print("\t");
//  Serial.print(adjustedMagnitude);
//  Serial.println();

  currentMagnitude = newMagnitude;
}

void loopHeartRate() {
  offset = (offset + 1) % beatLength;

  if (pulseFound == true && pulseBpm <= maxBpm) {
    bpm = pulseBpm;
  } else {
    bpm = staticBpm;
  }

  // TODO Fix BPM noise
  bpm = staticBpm;

  if (pulseBeatFound == true){
    pulseBeatFound = false;
  }

  int i;
  for (i=0;i<movesPerBeat;i++){
    // Delay in milliseconds. BPM are measured by minute, so divide accordingly.
    delay((float) 1 / (bpm * 60 * 1000) / beatLength / movesPerBeat);
    moveBuffer();
    draw();
  }

  // Measure only a few times per beat to avoid slowdowns
  if(offset % round(beatLength/samplePerBeat) == 0) {
    calcAdjustedMagnitude();
  }
}


void setup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN_CH1>(ledsCh0, NUM_LEDS_CH0);
  FastLED.addLeds<NEOPIXEL, LED_PIN_CH2>(ledsCh1, NUM_LEDS_CH1);

  Serial.begin(BAUD_RATE);

  brightnessControl.setup();

  pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
  modeButton.attach(MODE_BUTTON_PIN);
  modeButton.interval(50);

  pinMode(BEAT_BUTTON_PIN, INPUT_PULLUP);
  beatButton.attach(BEAT_BUTTON_PIN);
  beatButton.interval(50);

  stateCh0.setPalette(&palette);
  patternList.setState(0, &stateCh0);

  stateCh1.setPalette(&palette);
  patternList.setState(1, &stateCh1);

  updateModeFromEEPROM();
}

void loop() {
  updateModeFromButton();

  brightnessControl.update();
  FastLED.setBrightness(brightnessControl.getBrightness());

  byte fade = 0;
  patternList.loop(fade);

  FastLED.show();

  // Run loop() once per frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}
