#include "FastLED.h"

/*
 * Heartbeat simulator
 * 
 * Shows a different hue and brightness based on accelerometer activity,
 * with the idea that low movement means colder and less bright colours,
 * with more movement turning it into warmer and brighter colours.
 * 
 * TODO Integrate pluse meter
 * 
 * Credit to Ben Nolan for the heart beat LED logic.
 * And for lending me some Neopixels to play around with!
 */
 
#define BAUD_RATE 57600
#define NUM_LEDS 120
#define DATA_PIN 12
#define ACCELX_PIN 0
#define ACCELY_PIN 2
#define ACCELZ_PIN 4
#define PULSE_PIN 6

// config
int staticBpm = 35; // From http://ecg.utah.edu/img/items/Normal%2012_Lead%20ECG.jpg
byte beat[]  = {3,2,2,2,2,3,4,3,2,1,0,15,2,2,3,4,6,8,5,3,3,3};
byte beatLength = 22;
int movesPerBeat = 8;
int minMagnitude = 80;
int maxMagnitude = 150;
int minBrightness = 10; // from 0-255
int maxBrightness = 100; // from 0-255
int minHue = 160; // hue value under low motion
int maxHue = 255; // hue value under high motion
int gainRatePerBeat = 200; // value change towards new target value (per beat)
int decayRatePerBeat = 1; // percent change towards new target per beat
int sampleSize = 8; // smooth out accelerometer readings

// state
int bpm = staticBpm;
byte bufr[NUM_LEDS];
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

CRGB leds[NUM_LEDS];

void setup() { 
      FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
      Serial.begin(BAUD_RATE);
      interruptSetup();
}

void moveBuffer() {
  int i;
  byte c = (int) beat[offset] * 255 / beatLength;

  // Move current beat intensity backwards through buffer
  bufr[NUM_LEDS - 1] = c;
  for (i=0;i<NUM_LEDS - 1;i++){
    bufr[i] = bufr[i+1];
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
  
  for (i=0;i<NUM_LEDS;i++){
      leds[i] = CHSV(hue, bufr[i], bufr[i]/brightnessFactor);
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

void loop() { 
  offset = (offset + 1) % beatLength;

//  serialOutput();
  if (pulseFound == true) {
//    serialOutputWhenBeatHappens();
    bpm = pulseBpm;
  } else {
    bpm = staticBpm;
  }

  if (pulseBeatFound == true){
//    serialOutputWhenBeatHappens();
    pulseBeatFound = false;
    Serial.println(bpm);
  }

  int i;
  for (i=0;i<movesPerBeat;i++){
    delay((float) 1 / bpm * 60 * 1000 / beatLength / movesPerBeat);
    moveBuffer();
    draw();
  }

  if(offset % round(beatLength/4) == 0) {
    calcAdjustedMagnitude(); 
  }
}
