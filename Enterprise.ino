// Simple Navigation & Strobe light flashing
// Added Impulse/Warp Mode Fade-out and Fade-in
// Added Double Photon Torpedo Effect and Phaser Effect
// Author: Ostrich Longneck
// Date: 25 August 2016

// Modified by Delta_G 25 June 2023

const int strobeLightPin = 13;
const int navLightPin = 12;
const int bussardLightPins[4] = { 14, 15, 16, 17 };
// Set up variables used in strobe lights
unsigned int strobeOnPeriod = 100;
unsigned int strobeOffPeriod = 900;

// Set up variables used in nav lights
unsigned int navOnPeriod = 1000;
unsigned int navOffPeriod = 500;

// set up the pin numbers used for warp mode activation/deactivation
const int impulseLightPin = 11;  // Pin 10 is a PWM pin, used to fade the yellow impulse LED up and down
const int warpLightPin = 10;     // Pin 11 is a PWM pin, used to fade the blue warp LED up and down
const int warpButtonPin = 8;     // Warpmode will be activated & deactivated by a button on pin 1

//  List of the states the Engines can be in.
enum engineStateEnum { ENGINE_OFF,
                       IMPULSE_ENGAGE,
                       IMPULSE,
                       IMPULSE_DISENGAGE,
                       WARP_ENGAGE,
                       WARP,
                       WARP_DISENGAGE } engineState = IMPULSE_ENGAGE;

// Set up variables used to activate/deactivate warp mode
enum fadeStepDelayEnum { IMPULSE_ON,
                         IMPULSE_OFF,
                         WARP_ON,
                         WARP_OFF };
unsigned int fadeStepDelays[4] = { 5, 5, 5, 5 };
unsigned int longPressTime = 5000;
byte fadeMaxValue = 255;

unsigned int debounceDelay = 50;

// set up pin numbers used for phaser effect
const int phaserLightPin = 7;   // Pin 7 is a digital pin
const int phaserButtonPin = 4;  // phaser effect will be activated by a button on pin 4
// Variables for Phasers
unsigned int phaserOnPeriod = 100;
unsigned int phaserOffPeriod = 50;


// set up pin numbers used for photon torpedo effect
const int photonLightPins[2] = { 5, 6 };  // Pin 5 and 6 are PWM pins, because the photon torpedo effect has a short low-intensity burst, followed by a longer high-intensity burst
const int photonButtonPin = 2;            // photon torpedo effect will be activated by a button on pin 2

// variables for photonTorpedoes function.
// using arrays of 2 so I don't have to duplicate code
byte photonLowFlashLevel = 35;
byte photonHighFlashLevel = 255;

// states for photonTorpedo function.
enum photonStateEnum { LOW_FLASH,
                       BETWEEN,
                       HIGH_FLASH,
                       OFF } photonState[2] = { OFF, OFF };
// These are the ms delays for the low flash, between time, and high flash.
unsigned int photonStateDelays[3] = { 1000, 150, 1000 };

// ms delay between torpedo 1 fires and the torpedo fires again.
// first number is time between volleys from start to start if you hold button long enough
// second number is delay between torpedo 1 and torpedo 2 if you hold button long enough
unsigned long delayBetween = 750;

// ms length of the little delay at that start.
unsigned int startDelayPeriod = 1000;  //time in ms

void setup() {
  pinMode(strobeLightPin, OUTPUT);
  pinMode(navLightPin, OUTPUT);

  pinMode(impulseLightPin, OUTPUT);  // Impulse LED on pin 11
  pinMode(warpLightPin, OUTPUT);     // Warp LED on pin 10
  pinMode(warpButtonPin, INPUT);     // Warp mode button on pin 1

  pinMode(photonLightPins[0], OUTPUT);  // Photon torpedo #1 LED on pin 5
  pinMode(photonLightPins[1], OUTPUT);  // Photon torpedo #2 LED on pin 6
  pinMode(photonButtonPin, INPUT);      // Photon torpedo button on pin 2

  pinMode(phaserLightPin, OUTPUT);  // Phaser LED on pin 7
  pinMode(phaserButtonPin, INPUT);  // Phaser button on pin 4



  // small delay before fade-up of yellow impulse mode LED
  // because it looks kinda cool...
  // delete the next 6 lines if you want to remove this delay
  unsigned long startDelayStart = millis();
  while (millis() - startDelayStart <= startDelayPeriod) {
    navLights();
    strobeLights();
  }  // end of small delay on startup

  for (int i = 0; i < 4; i++) {
    pinMode(bussardLightPins[i], OUTPUT);
  }

}  // end of setup

void loop()  // The main program
{
  // update lights
  strobeLights();
  navLights();
  engines();
  phasers();
  photonTorpedoes(0);
  photonTorpedoes(1);

}  // end of loop


void photonTorpedoes(int tNum) {

  // If we enforce a few ms between photon torpedoes then we don't need debounce here
  // IIRC the Enterprise always fired two torpedoes with a short delay between.

  // We need separate time stamps for the two torpedoes.
  static unsigned int lastChange[2] = { millis(), millis() };
  unsigned int currentTime = millis();

  static unsigned long torpedoOneLastFireTime = millis();

  // Since delay times are in an array we can check them all in once piece of code:
  if ((photonState[tNum] != OFF) && (currentTime - lastChange[tNum] >= photonStateDelays[photonState[tNum]])) {
    // if it is time for a state change then increase the state andsave the timestamp
    photonState[tNum] = photonState[tNum] + 1;
    lastChange[tNum] = currentTime;
  }

  switch (photonState[tNum]) {

    case LOW_FLASH:
      {
        analogWrite(photonLightPins[tNum], photonLowFlashLevel);
        break;
      }
    case BETWEEN:
      {
        digitalWrite(photonLightPins[tNum], LOW);
        break;
      }
    case HIGH_FLASH:
      {
        analogWrite(photonLightPins[tNum], photonHighFlashLevel);
        break;
      }
    case OFF:
      {
        // lights off
        digitalWrite(photonLightPins[tNum], LOW);

        // Only let torpedo 1 fire if no torpedos are firing.
        if (tNum == 0 && photonState[1] == OFF) {
          if (digitalRead(photonButtonPin) == HIGH) {
            //Fire the torpedo!
            photonState[tNum] = LOW_FLASH;
            //and save the timestamp
            lastChange[tNum] = currentTime;
            // save the start of torpedo one so we can time against it
            torpedoOneLastFireTime = currentTime;
          }
        } else {
          // torpedo 2 should always fire after torpedo 1
          if (photonState[0] != OFF && (currentTime - torpedoOneLastFireTime >= delayBetween)) {
            //Fire the torpedo!
            photonState[tNum] = LOW_FLASH;
            //and save the timestamp
            lastChange[tNum] = currentTime;
          }
        }

        break;
      }
  }
}

void phasers() {

  // No edge detection for button.  If it is pressed then we are firing.  Otherwise no.
  // No debounce either.  Once it fires there will be 150ms before another blast.
  // That's longer than switch bounce so I'll just read the pin when it's time.

  static boolean lightOn = false;
  static unsigned long lastChangeTime = millis();
  unsigned long currentTime = millis();

  if (lightOn && currentTime - lastChangeTime >= phaserOnPeriod) {
    // Don't bother with button while phaser is firing.  Let it finish the flash
    lightOn = false;
    digitalWrite(phaserLightPin, LOW);
    // timestamp of when we changed
    lastChangeTime = currentTime;
  } else if (!lightOn && currentTime - lastChangeTime >= phaserOffPeriod) {
    // Only fire another phaser blast if the button is still down.
    if (digitalRead(phaserButtonPin) == HIGH) {
      lightOn = true;
      digitalWrite(phaserLightPin, HIGH);
      // timestamp of when we changed
      lastChangeTime = currentTime;
    }
  }
}

void strobeLights() {
  static boolean lightOn = false;
  static unsigned long lastChangeTime = millis();
  unsigned long currentTime = millis();

  // if the lights are on and have been for the full on period
  if (lightOn && currentTime - lastChangeTime >= strobeOnPeriod) {
    lightOn = false;
    digitalWrite(strobeLightPin, LOW);
    // timestamp of when we changed
    lastChangeTime = currentTime;
  }
  // else if they are currently off and have been for the full off period
  else if (!lightOn && currentTime - lastChangeTime >= strobeOffPeriod) {
    lightOn = true;
    digitalWrite(strobeLightPin, HIGH);
    // timestamp of when we changed
    lastChangeTime = currentTime;
  }
}

void navLights() {
  static boolean lightOn = false;
  static unsigned long lastChangeTime = millis();
  unsigned long currentTime = millis();

  if (lightOn && currentTime - lastChangeTime >= navOnPeriod) {
    lightOn = false;
    digitalWrite(navLightPin, LOW);
    // timestamp of when we changed
    lastChangeTime = currentTime;
  } else if (!lightOn && currentTime - lastChangeTime >= navOffPeriod) {
    lightOn = true;
    digitalWrite(navLightPin, HIGH);
    // timestamp of when we changed
    lastChangeTime = currentTime;
  }
}

void engines() {
  unsigned int bussardOnPeriod = 200;
  unsigned int bussardOffPeriod = 150;
  static unsigned long lastBussardUpdate = millis();
  static boolean bussardOn = false;
  static unsigned long lastDebounceTime = millis();
  static byte lastDebounceState = LOW;
  static unsigned long buttonPressStart = millis();
  static byte lastButtonState = LOW;
  byte newButtonState = lastButtonState;

  static boolean killingEngine = false;

  //  read the button state
  boolean buttonRead = digitalRead(warpButtonPin);
  // if the state has changed
  if (buttonRead != lastDebounceState) {
    lastDebounceTime = millis();
    lastDebounceState = buttonRead;
  }
  if (millis() - lastDebounceTime >= debounceDelay) {
    newButtonState = buttonRead;
  }

  if (newButtonState != lastButtonState) {

    // and the new state is HIGH
    if (newButtonState == HIGH) {
      buttonPressStart = millis();
    } else {
      // The button state is low so button has been released
      //  If it ws a long press turn engines off
      if ((engineState != ENGINE_OFF) && (millis() - buttonPressStart >= longPressTime)) {
        killingEngine = true;
      }

      if ((engineState == IMPULSE) || (engineState == IMPULSE_ENGAGE)) {
        engineState = IMPULSE_DISENGAGE;
      } else if ((engineState == WARP) || (engineState == WARP_ENGAGE)) {
        engineState = WARP_DISENGAGE;
      } else if (engineState == ENGINE_OFF) {
        engineState = IMPULSE_ENGAGE;
      }
    }
    lastButtonState = newButtonState;
  }

  static unsigned long lastFadeChange = millis();
  unsigned long currentTime = millis();

  static byte impulseFadeVal = 0;
  static byte warpFadeVal = 0;



  switch (engineState) {

    case ENGINE_OFF:
      {
        digitalWrite(impulseLightPin, LOW);
        digitalWrite(warpLightPin, LOW);
        bussardOn = false;
        break;
      }
    case IMPULSE_ENGAGE:
      {
        // fade up the impulse
        if (currentTime - lastFadeChange >= fadeStepDelays[IMPULSE_ON]) {
          lastFadeChange = millis();
          impulseFadeVal++;
          analogWrite(impulseLightPin, impulseFadeVal);
          if (impulseFadeVal == fadeMaxValue) {
            engineState = IMPULSE;
            bussardOn = true;
            bussardOnPeriod = 250;
            bussardOffPeriod = 5;
          }
        }
        break;
      }
    case IMPULSE:
      {
        bussardOn = true;
        bussardOnPeriod = 200;
        bussardOffPeriod = 150;
        break;
      }
    case IMPULSE_DISENGAGE:
      {
        // fade out impulse
        if (currentTime - lastFadeChange >= fadeStepDelays[IMPULSE_OFF]) {
          lastFadeChange = millis();
          impulseFadeVal--;
          analogWrite(impulseLightPin, impulseFadeVal);
          if (impulseFadeVal == 0) {
            if (killingEngine) {
              engineState = ENGINE_OFF;
              killingEngine = false;
            } else {
              engineState = WARP_ENGAGE;
            }
          }
        }
        break;
      }
    case WARP_ENGAGE:
      {
        // fade up the warp
        if (currentTime - lastFadeChange >= fadeStepDelays[WARP_ON]) {
          lastFadeChange = millis();
          warpFadeVal++;
          analogWrite(warpLightPin, warpFadeVal);
          if (warpFadeVal == fadeMaxValue) {
            engineState = WARP;
            bussardOn = true;
            bussardOnPeriod = 250;
            bussardOffPeriod = 5;
          }
        }
        break;
      }
    case WARP:
      {
        bussardOn = true;
        bussardOnPeriod = 100;
        bussardOffPeriod = 1;
        break;
      }
    case WARP_DISENGAGE:
      {
        // fade out warp
        if (currentTime - lastFadeChange >= fadeStepDelays[WARP_OFF]) {
          lastFadeChange = millis();
          warpFadeVal--;
          analogWrite(warpLightPin, warpFadeVal);
          if (warpFadeVal == 0) {
            if (killingEngine) {
              engineState = ENGINE_OFF;
              killingEngine = false;
            } else {
              engineState = IMPULSE_ENGAGE;
            }
          }
        }
        break;
      }
  }
  static boolean bussardLit = false;
  static byte currentLight = 0;
  // Cycle the Bussard Lights:
  if (bussardOn) {
    // If the light is lit and has been long enough to turn off
    if ((bussardLit) && (currentTime - lastBussardUpdate >= bussardOnPeriod)) {
      bussardLit = false;
      digitalWrite(bussardLightPins[currentLight], LOW);
      lastBussardUpdate = currentTime;
    }
    // if light is off and has been long enough to turn on
    if ((!bussardLit) && (currentTime - lastBussardUpdate >= bussardOffPeriod)) {
      bussardLit = true;
      // Next light
      currentLight = (currentLight + 1) % 4;
      digitalWrite(bussardLightPins[currentLight], HIGH);
      lastBussardUpdate = currentTime;
    }
  } else {
    // bussard lights should be all off
    for (int i = 0; i < 4; i++) {
      digitalWrite(bussardLightPins[i], LOW);
    }
  }
}