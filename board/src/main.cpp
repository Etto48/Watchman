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


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void hlt() {
  while(true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  Serial.begin(MONITOR_SPEED);
  Serial.println("WatchMan Starting...");
  ledcSetup(0, 5000, 8); // initialize ledc state so it doesn't conflict with i2c
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("I2C Initialized.");

  pinMode(A_PIN, INPUT_PULLUP);
  pinMode(B_PIN, INPUT_PULLUP);
  pinMode(UP_PIN, INPUT_PULLUP);
  pinMode(DOWN_PIN, INPUT_PULLUP);
  pinMode(LEFT_PIN, INPUT_PULLUP);
  pinMode(RIGHT_PIN, INPUT_PULLUP);

  pinMode(BAT_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Pins Configured.");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Monitor allocation failed");
    hlt();
  }
  
  display.ssd1306_command(SSD1306_DISPLAYON);
  display_image(images::logo, display);
  
  Serial.println("Display Initialized.");

  wifi::connect();
  Serial.println("Initiated WiFi Connection.");

  events::enable_events();
  Serial.println("Event System Initialized.");

  menu::init();
  Serial.println("Menu System Initialized.");

  sound::init();
  Serial.println("Sound System Initialized.");

  sound::play_melody(boot_jingle_melody, sizeof(boot_jingle_melody)/sizeof(boot_jingle_melody[0]));
  display.clearDisplay();
  display.display();
  Serial.println("Setup Complete.");
}

void loop() {
  menu::main_loop(display);
}
