#include <PubSubClient.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <time.h>
#include "custom_fonts.h"

String getLocalTime() {

  struct tm timeinfo;
  char timeNow[6];
  if (!getLocalTime(&timeinfo)) {
    return "Err";
  }
  strftime(timeNow, 6, "%H:%M", &timeinfo);
  return timeNow;
}

void fillArc(TFT_eSprite &sprite, int x, int y, int start_angle, int seg_count, int rx, int ry, int w, unsigned int colour)
{

  byte seg = 6; // Segments are 3 degrees wide = 120 segments for 360 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring

  // Calculate first pair of coordinates for segment start
  float sx = cos((start_angle - 90) * DEG2RAD);
  float sy = sin((start_angle - 90) * DEG2RAD);
  uint16_t x0 = sx * (rx - w) + x;
  uint16_t y0 = sy * (ry - w) + y;
  uint16_t x1 = sx * rx + x;
  uint16_t y1 = sy * ry + y;

  // Draw colour blocks every inc degrees
  for (int i = start_angle; i < start_angle + seg * seg_count; i += inc) {

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * DEG2RAD);
    float sy2 = sin((i + seg - 90) * DEG2RAD);
    int x2 = sx2 * (rx - w) + x;
    int y2 = sy2 * (ry - w) + y;
    int x3 = sx2 * rx + x;
    int y3 = sy2 * ry + y;

    sprite.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
    sprite.fillTriangle(x1, y1, x2, y2, x3, y3, colour);

    // Copy segment end to segement start for next segment
    x0 = x2;
    y0 = y2;
    x1 = x3;
    y1 = y3;

    if(i == start_angle + seg * seg_count - inc) {
      int xc = (x2 + x3) / 2;
      int yc = (y2 + y3) / 2;
      // int rad = sqrt(pow(x2-xc,2) + pow(y2-yc,2));
      sprite.fillCircle(xc, yc, w / 2, colour);
    }
  }
}

void initDisplay(TFT_eSprite &background, TFT_eSprite &arcSprite, TFT_eSprite &timeSprite, TFT_eSprite &tempSprite, TFT_eSprite &humSprite) {

  background.createSprite(240, 240);
  arcSprite.createSprite(240, 240);
  humSprite.createSprite(105, 48);
  timeSprite.createSprite(92, 27);
  tempSprite.createSprite(113, 55);
}

void updateStatic(TFT_eSprite &sprite) {

  sprite.fillCircle(120, 120, 120, CLOCK_BG);
  sprite.fillCircle(120, 120, 115, BACKGROUND);

  sprite.setFreeFont(&Roboto_Black_50);
  sprite.setTextColor(TEXT_COLOUR, BACKGROUND);
  sprite.setTextSize(2);
  sprite.drawString("`", 148, 70, 4);
  sprite.setTextSize(1);
  sprite.drawString("C", 168, 70);

  int rect_x = 72;
  int rect_y = 190;
  int t_offset = 32;
  sprite.fillRoundRect(rect_x, rect_y, 240 - 2 * rect_x, 240 - rect_y, 8, CLOCK_BG);
  sprite.fillTriangle(rect_x - t_offset, 240, rect_x + 3, rect_y + 2, rect_x, 240, CLOCK_BG);
  sprite.fillTriangle(240 - rect_x, 240, 240 - rect_x - 3, rect_y + 2, 240 - rect_x + t_offset, 240, CLOCK_BG);
}

void updateTempDial(TFT_eSprite &sprite, float temperature) {

    int max_seg = 47;
    float temp_range = 20.0;

    sprite.fillSprite(TFT_TRANSPARENT);
    int arc = (int)(temperature - 10) * max_seg/ temp_range;
    if( arc > max_seg ) {
      arc = max_seg;
    }
    unsigned int colour = TFT_GREEN;
    if (arc < 14) {
      colour = TFT_BLUE;
    }
    else if (arc > 36 ) {
      colour = TFT_RED;
    }
    fillArc(sprite, 120, 120, 216, arc, 110, 110, 10, colour);
}

void updateTemp(TFT_eSprite &sprite, float temperature) {

    sprite.fillSprite(TFT_TRANSPARENT);  // Clear

    sprite.setTextSize(1);
    sprite.setFreeFont(&Roboto_Black_50);
    sprite.setTextColor(TEXT_COLOUR, TFT_TRANSPARENT);

    sprite.drawString(String(temperature, 1), 3, 3);
}

void updateHumidity(TFT_eSprite &sprite, String humidity) {

    sprite.fillSprite(TFT_TRANSPARENT);  // Clear

    sprite.setTextSize(1);
    sprite.setFreeFont(&Roboto_Black_50);
    sprite.setTextColor(TFT_RED, TFT_TRANSPARENT);
    
    sprite.drawString(humidity + "%", 3, 0);
}

void updateTime(TFT_eSprite &sprite) {

    sprite.fillSprite(TFT_TRANSPARENT);
    sprite.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
    sprite.setFreeFont(&DSEG7_Classic_Mini_Regular_26);

    String timeNow = getLocalTime();
    sprite.drawString(timeNow, 0, 0);
}

void updateScreen(TFT_eSprite &background, TFT_eSprite &arcSprite, TFT_eSprite &timeSprite, TFT_eSprite &tempSprite, TFT_eSprite &humSprite) {
  
  background.fillSprite(BACKGROUND);
  updateStatic(background);

  tempSprite.pushToSprite(&background, 38, 70, TFT_TRANSPARENT);
  humSprite.pushToSprite(&background, 67, 125, TFT_TRANSPARENT);
  timeSprite.pushToSprite(&background, 73, 200, TFT_TRANSPARENT);
  arcSprite.pushToSprite(&background, 0, 0, TFT_TRANSPARENT);

  background.pushSprite(0, 0);
}

void backlightToggle(PubSubClient &client, const char* topic, String state) {

  if (state == "ON") {
    digitalWrite(TFT_BL, HIGH);
  }
  if (state == "OFF") {
    digitalWrite(TFT_BL, LOW);
  }
  client.publish(topic, state.c_str());
}