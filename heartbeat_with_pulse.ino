#include <Bounce2.h>
#include "FastLED.h"

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
 
#define BAUD_RATE 57600
#define NUM_LEDS_CH1 120
#define LED_PIN_CH1 12
#define NUM_LEDS_CH2 29
#define LED_PIN_CH2 7
#define ACCELX_PIN 0
#define ACCELY_PIN 2
#define ACCELZ_PIN 4
#define PULSE_PIN 6
#define CTRL_LED_RED_PIN 9
#define CTRL_LED_GREEN_PIN 4
#define CTRL_LED_BLUE_PIN 3
#define BUTTON_PIN 2

// ui
int mode = 1;
Bounce modeButton;

// config
int staticBpm = 70; // From http://ecg.utah.edu/img/items/Normal%2012_Lead%20ECG.jpg
byte beat[]  = {3,2,2,2,2,3,4,3,2,1,0,15,2,2,3,4,6,8,5,3,3,3};
byte beatLength = 22;
int movesPerBeat = 8;
int minMagnitude = 80;
int maxMagnitude = 150;
int minBrightness = 180; // from 0-255
int maxBrightness = 255; // from 0-255
int minHue = 160; // hue value under low motion
int maxHue = 255; // hue value under high motion
int gainRatePerBeat = 200; // value change towards new target value (percent*100 per beat, e.g 200 = 20%)
int decayRatePerBeat = 3  ; // percent change towards new target per beat
int sampleSize = 8; // smooth out accelerometer readings
int samplePerBeat = 2;

// state
int bpm = staticBpm;
byte bufr[NUM_LEDS_CH1];
byte offset = 0;
int targetMagnitude = minMagnitude;
int adjustedMagnitude = minMagnitude;

// pulse config
static boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 

// pulse state
volatile int pulseBpm; // int that holds raw Analog in 0. updated every 2mS
volatile int pulseSignal; // holds the incoming raw data
volatile int IBI = 600; // int that holds the time interval between beats! Must be seeded! 
volatile boolean pulseFound = false; // "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile boolean pulseBeatFound = false; // becomes true when Arduoino finds a beat.
volatile boolean hasPulse = false; // 

CRGB leds1[NUM_LEDS_CH1];
CRGB leds2[NUM_LEDS_CH2];

void setup() { 
      FastLED.addLeds<NEOPIXEL, LED_PIN_CH1>(leds1, NUM_LEDS_CH1);
      FastLED.addLeds<NEOPIXEL, LED_PIN_CH2>(leds2, NUM_LEDS_CH2);
      Serial.begin(BAUD_RATE);
      interruptSetup();

      pinMode(CTRL_LED_RED_PIN, OUTPUT);
      pinMode(CTRL_LED_GREEN_PIN, OUTPUT);
      pinMode(CTRL_LED_BLUE_PIN, OUTPUT);
      pinMode(BUTTON_PIN, INPUT_PULLUP);
      
      modeButton.attach(BUTTON_PIN);
      modeButton.interval(5);
}

void moveBuffer() {
  int i;
  byte c = (int) beat[offset] * 255 / beatLength;
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
    3, 
    1
  );

  // Channel 1
  for (i=0;i<NUM_LEDS_CH1;i++){
      leds1[i] = CHSV(hue, bufr[i], bufr[i]/brightnessFactor);
  }

  // Channel 2
  for (i=0;i<NUM_LEDS_CH2;i++){
      leds2[i] = CHSV(hue, bufr[0], bufr[0]/brightnessFactor);
  }
  
  FastLED.show();
}

int calcAdjustedMagnitude() {
  // Get new target (smoothed out over a couple of readings)
  targetMagnitude = max(
    constrain(getMagnitude(), minMagnitude, maxMagnitude), 
    targetMagnitude
  );
  
  // Slowly work towards new target
  if(adjustedMagnitude <= targetMagnitude) {
    adjustedMagnitude = constrain(
      (targetMagnitude*100 + gainRatePerBeat) / 100, 
      minMagnitude, 
      maxMagnitude
    );
  } else {
    adjustedMagnitude = constrain(
      (targetMagnitude*100 - gainRatePerBeat) / 100, 
      minMagnitude, 
      maxMagnitude
    );
  }

  // Slowly decay max target
  targetMagnitude = constrain(
    (targetMagnitude*100 - decayRatePerBeat) / 100, 
    minMagnitude, 
    maxMagnitude
  );
}

// Get a magnitude across all vectors.
// Smooth out result through a rolling average
int getMagnitude() {
  float avgMag = 0;
  for(int x = 0 ; x < sampleSize ; x++) {
    int aX = analogRead(ACCELX_PIN);
    int aY = analogRead(ACCELY_PIN);
    int aZ = analogRead(ACCELZ_PIN);

    float magnitude = sqrt((aX * aX) + (aY * aY) + (aZ * aZ));
    avgMag += magnitude;
  }
  avgMag /= sampleSize;
  
  return avgMag;
}

void loopHeartRate() {
  offset = (offset + 1) % beatLength;

//  serialOutput();
  if (pulseFound == true) {
//    serialOutputWhenBeatHappens();
    bpm = pulseBpm;
    digitalWrite(CTRL_LED_RED_PIN, HIGH);
    digitalWrite(CTRL_LED_GREEN_PIN, LOW);
    digitalWrite(CTRL_LED_BLUE_PIN, HIGH);
  } else {
    bpm = staticBpm;
  }

  if (pulseBeatFound == true){
//    serialOutputWhenBeatHappens();
    pulseBeatFound = false;
//    Serial.println(bpm);
  }

  int i;
  for (i=0;i<movesPerBeat;i++){
    // Delay in milliseconds. BPM are measured by minute, so divide accordingly.
    delay((float) 1 / (bpm * 60 * 1000) / beatLength / movesPerBeat);
    moveBuffer();
    draw();
  }

  // Mesure only a few times per beat to avoid slowdowns
  if(offset % round(beatLength/samplePerBeat) == 0) {
    calcAdjustedMagnitude(); 
  }
}

void fadeall() { 
  for(int i = 0; i < NUM_LEDS_CH1; i++) { leds1[i].nscale8(250); } 
}

void loopCylon() {
  static uint8_t hue = 0;

  // First slide the led in one direction
  for(int i = 0; i < NUM_LEDS_CH1; i++) {
    // Set the i'th led to red 
    leds1[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show(); 
    // now that we've shown the leds, reset the i'th led to black
    // leds1[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }

  // Now go in the other direction.  
  for(int i = (NUM_LEDS_CH1)-1; i >= 0; i--) {
    // Set the i'th led to red 
    leds1[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds1[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}

void loop() { 
  // Check for mode changes
  modeButton.update();
  int modeButtonVal = modeButton.read();
  if(modeButtonVal == LOW) {
    mode = (mode % 2) + 1;
  }

  // default to all off (high)
  digitalWrite(CTRL_LED_GREEN_PIN, HIGH);
  digitalWrite(CTRL_LED_BLUE_PIN, HIGH);
  digitalWrite(CTRL_LED_RED_PIN, HIGH);

  // Activate effect based on mode
  if(mode == 1) {
    loopHeartRate();
  } else if (mode == 2) {
    loopCylon();
  }

}
