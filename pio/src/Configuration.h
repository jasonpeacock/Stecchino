#pragma once

// Pins
//
#define PIN_LED_DATA 5     // orig: 10
#define PIN_MOSFET_GATE 4  // orig: 11
#define PIN_MPU_POWER 6    // orig: 9
#define PIN_BUTTON_1 8     // orig: 2
#define PIN_INTERRUPT 3    // orig: 3

#define BUTTON_1_ON (!digitalRead(PIN_BUTTON_1))

// Number of LEDs in the strip.
#define NUM_LEDS 72

// Number of LEDs to turn on every second when playing.
#define NUM_LEDS_PER_SECOND 2

// LED animation speed.
#define FRAMES_PER_SECOND 120

// LED high brightness level.
#define HIGH_BRIGHTNESS 50

// LED low brightness level.
#define LOW_BRIGHTNESS 10

// Minimum Vcc value when reporting battery level.
#define MIN_VCC_MV 2700

// Maximum Vcc value when reporting battery level.
#define MAX_VCC_MV 3350

// 0, 1 or 2 to set the angle of the joystick
#define ACCELEROMETER_ORIENTATION 2

// How long to show the battery level.
#define MAX_SHOW_BATTERY_MS 5000

// How long to be Idle.
#define MAX_IDLE_MS 20000

// How long to be FakeSleep before moving to Sleep.
#define MAX_FAKE_SLEEP_MS 10000 // orig: 60000

// How long to be SpiritLevel before moving to Sleep.
#define MAX_SPIRIT_LEVEL_MS 20000

// How long to be vertical before moving to Sleep? (in case Stecchino is forgotten vertical)
#define MAX_PLAY_MS 60000

// duration of animation when system returns from sleep
#define MAX_WAKE_UP_TRANSITION_MS 1000

// duration of animation when game starts
#define MAX_START_PLAY_TRANSITION_MS 500

// duration of game over animation
#define MAX_GAME_OVER_TRANSITION_MS 1000

// duration of animation to sleep
#define MAX_SLEEP_TRANSITION_MS 2000

