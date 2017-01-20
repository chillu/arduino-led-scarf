#include <Bounce2.h>
#include "FastLED.h"
#include <EEPROM.h>

/*
 * Heartbeat simulator
 * 
 * Shows a different hue and brightness based on accelerometer activity,
 * with the idea that low movement means colder and less bright colours,
 * with more movement turning it into warmer and brighter colours.
 * 
 * Credit to Ben Nolan for the heart beat LED logic.
 * And for lending me some Neopixels to play around with!
 */
 
#define BAUD_RATE 9600
#define NUM_LEDS_CH1 120
#define LED_PIN_CH1 4
#define NUM_LEDS_CH2 29
#define LED_PIN_CH2 2
#define ACCELX_PIN 0
#define ACCELY_PIN 2
#define ACCELZ_PIN 4
#define PULSE_PIN 0
#define BUTTON_PIN 6

// ui
int mode = 1;
int modeCount = 6;
Bounce modeButton;

// config
int staticBpm = 60; // in case no pulse is found
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
byte bufr[NUM_LEDS_CH1];
byte offset = 0;
int currentMagnitude;
int targetMagnitude = maxMagnitude;
int adjustedMagnitude = maxMagnitude;

// pulse config
static boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 

// pulse state
volatile int pulseBpm; // int that holds raw Analog in 0. updated every 2mS
volatile int pulseSignal; // holds the incoming raw data
volatile int IBI = 600; // int that holds the time interval between beats! Must be seeded! 
volatile boolean pulseFound = false; // "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile boolean pulseBeatFound = false; // becomes true when Arduoino finds a beat.
volatile boolean hasPulse = false; // 

#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120

CRGB leds[2][NUM_LEDS_CH1];

void setup() { 
  FastLED.addLeds<NEOPIXEL, LED_PIN_CH1>(leds[0], NUM_LEDS_CH1);
  FastLED.addLeds<NEOPIXEL, LED_PIN_CH2>(leds[1], NUM_LEDS_CH2);
  Serial.begin(BAUD_RATE);
  interruptSetup();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  modeButton.attach(BUTTON_PIN);
  modeButton.interval(50);

  updateModeFromEEPROM();
}

// other pattern config
uint8_t gHue = 0; // rotating "base color" used by many of the patterns

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

void moveBuffer() {
  int i;
  byte c = (int) beat[offset] * 255 / beatMaxIntensity;
  int midPoint = NUM_LEDS_CH1/2;

  // Move current beat intensity from middle to beginning of strip (backward in buffer)
  bufr[midPoint - 1] = c;
  for (i=0;i<midPoint - 1;i++){
    bufr[i] = bufr[i+1];
  }

  // Move current beat intensity from middle to end of strip (forward in buffer)
  bufr[midPoint] = c;
  for (i=NUM_LEDS_CH1 - 1;i>midPoint;i--){
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
  for (i=0;i<NUM_LEDS_CH1;i++){
      leds[0][i] = CHSV(hue, bufr[i], bufr[i]/(brightnessFactor/100));
  }

  // Channel 2
  int halfPoint = (int)sizeof(bufr)/2;
  for (i=0;i<NUM_LEDS_CH2;i++){
      leds[1][i] = CHSV(hue, bufr[halfPoint], bufr[halfPoint]/(brightnessFactor/100));
  }
  
  FastLED.show();
}

int calcAdjustedMagnitude() {
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

void loopHeartRate() {
  offset = (offset + 1) % beatLength;

//  serialOutput();
  if (pulseFound == true && pulseBpm <= maxBpm) {
//    serialOutputWhenBeatHappens();
    bpm = pulseBpm;
  } else {
    bpm = staticBpm;
  }

  // TODO Fix BPM noise
  bpm = staticBpm;
  
  if (pulseBeatFound == true){
//    offset = 0;
//    serialOutputWhenBeatHappens();
//    Serial.println(bpm);
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

void fadeall() { 
  for(int i = 0; i < NUM_LEDS_CH1; i++) { leds[0][i].nscale8(250); } 
}

void cylon(int ledIndex, int num) {
  static uint8_t hue = 0;

  // First slide the led in one direction
  for(int i = 0; i < num; i++) {
    // Set the i'th led to red 
    leds[ledIndex][i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds[0][i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(5);
  }

  // Now go in the other direction.  
  for(int i = (num)-1; i >= 0; i--) {
    // Set the i'th led to red 
    leds[ledIndex][i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[0][i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(5);
  }
}

void updateModeFromButton() {
  modeButton.update();
  if(modeButton.fell()) {
    mode = (mode % modeCount) + 1;
  }
}

void rainbow(int ledIndex, int num) 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds[ledIndex], num, gHue, 7);
}

void rainbowWithGlitter(int ledIndex, int num) 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow(ledIndex, num);
  addGlitter(80, ledIndex, num);
}

void addGlitter( fract8 chanceOfGlitter, int ledIndex, int num) 
{
  if( random8() < chanceOfGlitter) {
    leds[ledIndex][ random16(num) ] += CRGB::White;
  }
}

void confetti(int ledIndex, int num) 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds[ledIndex], num, 10);
  int pos = random16(num);
  leds[ledIndex][pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon(int ledIndex, int num)
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds[ledIndex], num, 20);
  int pos = beatsin16(13,0,num);
  leds[ledIndex][pos] += CHSV( gHue, 255, 192);
}

void juggle(int ledIndex, int num) {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds[ledIndex], num, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[ledIndex][beatsin16(i+7,0,num)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}


void loop() { 
  // TODO Fix button noise
//  updateModeFromButton();

  // Activate effect based on mode
  if(mode == 1) {
    // Bright pulse meter
    minBrightnessDivisor = 200;
    maxBrightnessDivisor = 400;
    loopHeartRate();
  } else if (mode == 2) {
    // Less bright pulse meter
    minBrightnessDivisor = 300;
    maxBrightnessDivisor = 600;
    loopHeartRate();
  } else if (mode == 3) {
    FastLED.setBrightness(BRIGHTNESS);
    confetti(0, NUM_LEDS_CH1);
    confetti(1, NUM_LEDS_CH2);
    FastLED.show();  
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  } else if (mode == 4) {
    FastLED.setBrightness(BRIGHTNESS);
    sinelon(0, NUM_LEDS_CH1);
    sinelon(1, NUM_LEDS_CH2);
    FastLED.show();  
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  } else if (mode == 5) {
    FastLED.setBrightness(BRIGHTNESS);
    juggle(0, NUM_LEDS_CH1);
    juggle(1, NUM_LEDS_CH2);
    FastLED.show();  
    FastLED.delay(1000/FRAMES_PER_SECOND); 
    EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  }else if (mode == 6) {
    FastLED.setBrightness(BRIGHTNESS);
    cylon(0, NUM_LEDS_CH1);
    cylon(1, NUM_LEDS_CH2);
  }



}
