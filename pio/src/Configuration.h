#pragma once

// Pins
//
#define PIN_LED_DATA 5     // orig: 10
#define PIN_MOSFET_GATE 4  // orig: 11
#define PIN_MPU_POWER 6    // orig: 9
#define PIN_BUTTON_1 8     // orig: 2
#define PIN_INTERRUPT 3    // orig: 3

#define BUTTON_1_ON (!digitalRead(PIN_BUTTON_1))

// LEDs
//
// How many leds in your strip?
#define NUM_LEDS 72
// Set LEDS brightness
#define HIGH_BRIGHTNESS 50
// Set LEDS brightness
#define LOW_BRIGHTNESS 10

// Power Management
//
// how many milliseconds at idle before moving to Fake_sleep?
#define IDLE_MS 20000
// how many milliseconds at Fake_sleep before moving to Sleep?
#define FAKE_SLEEP_MS 60000
// how many milliseconds at Fake_sleep before moving to Sleep?
#define MAX_SPIRIT_LEVEL_MS 20000
// how many seconds vertical before moving to Sleep? (in case Stecchino is forgotten vertical)
#define MAX_PLAY_SECONDS 60
// how many LEDs are turned on every second when playing?
#define NUM_LEDS_PER_SECONDS 2

// LED Animation
//
#define FRAMES_PER_SECOND 120
// duration of animation when system returns from sleep
#define WAKE_UP_TRANSITION_MS 1000
// duration of animation when game starts
#define START_PLAY_TRANSITION_MS 500
// duration of game over animation
#define GAME_OVER_TRANSITION_MS 1000
// duration of animation to sleep
#define SLEEP_TRANSITION_MS 2000

// lower vcc value when checking battery level
#define LOW_VCC 2700
// higher vcc value when checking battery level
#define HIGH_VCC 3350

// 0, 1 or 2 to set the angle of the joystick
#define ACCELEROMETER_ORIENTATION 2
