#include <Arduino.h>

#include "config.h"
#include "lcd.h"
#include "ota.h"
#include "i2c.h"
#include "mqtt.h"

// WiFi & MQTT Setup
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
char msg[MSG_BUFFER_SIZE];

// Time Setup
uint32_t targetTime = 0; // to update time every few seconds

// Screen Setup
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite tempSprite = TFT_eSprite(&tft);
TFT_eSprite humSprite = TFT_eSprite(&tft);
TFT_eSprite timeSprite = TFT_eSprite(&tft);
TFT_eSprite arcSprite = TFT_eSprite(&tft);
TFT_eSprite background = TFT_eSprite(&tft);

// I2C Setup
TwoWire I2C = TwoWire(0);
Adafruit_BME280 bme;
BH1750 bh1750;

void MQTTcallback(char* topic, byte* payload, unsigned int length) {

  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  if (strcmp(topic, bklcmdTopic) == 0) {
    backlightToggle(client, message);
  }
}

void setup(void) {

  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(BACKGROUND);

  initWiFi();
  initMQTT(client, MQTTcallback);
  initOTA();
  I2C.begin(38, 39);
  initBME(I2C, bme);
  initBH1750(I2C, bh1750);

  configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);

  backlightToggle(client, "ON");
  initDisplay(background, arcSprite, timeSprite, tempSprite, humSprite);
}

void loop() {

  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnectMQTT(client);
  }
  client.loop();

  if (targetTime < millis()) {
    // Set next update for 3 second later
    targetTime = millis() + 3000;

    updateTime(timeSprite);

    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();

    updateTemp(tempSprite, temperature);
    updateHumidity(humSprite, humidity);
    updateDial(arcSprite, temperature, humidity);
    pushToHA(client, temperature, humidity, bme.readPressure(), bh1750.readLightLevel());
  }

  updateScreen(background, arcSprite, timeSprite, tempSprite, humSprite);
}

