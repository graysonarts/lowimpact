#include <Adafruit_NeoPixel.h>

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include <Wire.h>

#include <Adafruit_NeoPixel.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_L3GD20_U.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_10DOF.h>

#include "setup.h"
#include "pixel.h"
#include "color.h"
#include "accel.h"

#define PIN 6
#define NUM_PIXELS 8

const float seaLevelPressure PROGMEM = SENSORS_PRESSURE_SEALEVELHPA;
Adafruit_10DOF dof = Adafruit_10DOF();
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(18001);

sensors_event_t accel_event_1, mag_event, bmp_event, accel_event_2;
sensors_vec_t orientation;
bool which = false;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
RGBPixel frame[NUM_PIXELS];


static void fade_pixel(int x) {
  uint32_t color = strip.getPixelColor((x + 1) % NUM_PIXELS);
  avg_color(frame[x], color >> 16 & 0xFF, color >> 8 & 0xFF, color & 0xFF);
  color = strip.getPixelColor((x - 1) % NUM_PIXELS);
  avg_color(frame[x], color >> 16 & 0xFF, color >> 8 & 0xFF, color & 0xFF);
}

void strip_fade() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    fade_pixel(i);
  }
}


void setup() {
  common_setup(strip, accel, mag, bmp);
}

void pulse(int x, sensors_event_t *event, sensors_event_t *previous_event)
{
  int8_t x_delta = abs(event->acceleration.x - previous_event->acceleration.x);
  int8_t y_delta = abs(event->acceleration.y - previous_event->acceleration.y);
  int8_t z_delta = abs(event->acceleration.z - previous_event->acceleration.z);

  rgb_3_pulse(frame[x], frame[0], frame[NUM_PIXELS-1], x_delta, y_delta, z_delta);
}

void loop() {
  sensors_event_t *event = which ? &accel_event_1 : &accel_event_2;
  accel.getEvent(event);
  mag.getEvent(&mag_event);

  if (dof.fusionGetOrientation(event, &mag_event, &orientation)) {

    if (which) {
      pulse(3, &accel_event_1, &accel_event_2);
    } else {
      pulse(4, &accel_event_2, &accel_event_1);
    }

    which = !which;

    bmp.getEvent(&bmp_event);
    if (bmp_event.pressure) {
      float temperature;
      bmp.getTemperature(&temperature);
      bmp.pressureToAltitude(seaLevelPressure, bmp_event.pressure, temperature);
    }
  }
  strip_fade();

  for (int i = 0; i < NUM_PIXELS; i++) {
    RGBPixel * const p = &frame[i];
    strip.setPixelColor(i, p->red, p->green, p->blue);
  }

  strip.show();
  delay(50);
}

