#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "constants.hpp"
#include "core/image.hpp"
#include "images/logo.hpp"
#include "core/events.hpp"
#include "core/sound.hpp"
#include "core/jingle.hpp"
#include "core/menu.hpp"
#include "core/wifi.hpp"
#include "core/logger.hpp"
#include "core/timekeeper.hpp"


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void hlt() {
  vTaskDelete(NULL);
}

void setup() {
  logger::init();
  auto wakeup_cause = esp_sleep_get_wakeup_cause();
  if (wakeup_cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
    // Fresh boot
    timekeeper::first_boot();
    logger::info("WatchMan Starting...");
  } else {
    // Wake from sleep
    timekeeper::wakeup();
    logger::info("WatchMan Restarting from sleep...");
  }
  ledcSetup(0, 5000, 8); // initialize ledc state so it doesn't conflict with i2c
  Wire.begin(SDA_PIN, SCL_PIN);
  logger::info("I2C Initialized.");

  pinMode(A_PIN, INPUT_PULLUP);
  pinMode(B_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);

  pinMode(BAT_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  logger::info("Pins Configured.");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    logger::error("Monitor allocation failed");
    hlt();
  }
  
  display.ssd1306_command(SSD1306_DISPLAYON);
  image::display_image(images::logo, display);
  logger::info("Display Initialized.");

  wifi::connect();
  logger::info("Initiated WiFi Connection.");

  events::enable_events();
  logger::info("Event System Initialized.");

  menu::init();
  logger::info("Menu System Initialized.");

  sound::init();
  logger::info("Sound System Initialized.");

  sound::play_melody(boot_jingle_melody, sizeof(boot_jingle_melody)/sizeof(boot_jingle_melody[0]));
  display.clearDisplay();
  display.display();
  logger::info("Setup Complete.");
}

void loop() {
  menu::main_loop(display);
}
