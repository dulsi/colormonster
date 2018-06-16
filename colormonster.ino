#include <TinyScreen.h>
#include "SdFat.h"
#include <SPI.h>
#include <Wire.h>
#include "TinyConfig.h"
#include "cateye.h"
#include "ui.h"

#ifdef TINYARCADE_CONFIG
#include "TinyArcade.h"
#endif
#ifdef TINYSCREEN_GAMEKIT_CONFIG
#include "TinyGameKit.h"
#endif

TinyScreen display = TinyScreen(TinyScreenPlus);

SdFat sd;
SdFile dataFile;

#define STATE_PAINT 0
#define STATE_PAINTZOOM 1

#define BUTTON_COOLDOWN 5
#define JOYSTICK_COOLDOWNSTART 4

int state = 0;
int buttonCoolDown = 0;
int joystickCoolDown = 0;
int joystickCoolDownStart = JOYSTICK_COOLDOWNSTART;

class ColorMonster
{
  public:
    ColorMonster() : saved(false) { memset(img, 0, 64*48*2); }
    void initImage(const unsigned char *bi);

    unsigned char img[64*48*2];
    const unsigned char *baseImg;
    bool saved;
};

void ColorMonster::initImage(const unsigned char *bi)
{
  baseImg = bi;
  for (int i = 0; i < 64*48; i++)
  {
    if (baseImg[i] == 0)
    {
      img[i * 2] = 0;
      img[i * 2 + 1] = 0;
    }
    else
    {
      img[i * 2] = 255;
      img[i * 2 + 1] = 255;
    }
  }
}

ColorMonster party[1];
ColorMonster *active = &party[0];

#define TOOL_DRAW 0
#define TOOL_FLOOD 1

class Painter
{
  public:
    Painter() : px(0), py(0), tool(0), color1(255), color2(255) {}
    void update();
    void draw();

  private:
    uint8_t px;
    uint8_t py;
    uint8_t zoomx;
    uint8_t zoomy;
    uint8_t tool;
    uint8_t color1;
    uint8_t color2;
};

void Painter::update()
{
  uint8_t btn = checkButton(TAButton1 | TAButton2);
  uint8_t joyDir = checkJoystick(TAJoystickUp | TAJoystickDown | TAJoystickLeft | TAJoystickRight);
  if (joystickCoolDown > 0)
  {
    joystickCoolDown--;
  }
  else
  {
    if (joyDir == 0)
      joystickCoolDownStart = JOYSTICK_COOLDOWNSTART;
    else
    {
      if (joystickCoolDownStart > 0)
        joystickCoolDownStart--;
      buttonCoolDown = 0;
    }
    if ((joyDir & TAJoystickUp) && (py > 0))
    {
      py--;
      joystickCoolDown = joystickCoolDownStart;
    }
    else if ((joyDir & TAJoystickDown) && (py < 63))
    {
      py++;
      joystickCoolDown = joystickCoolDownStart;
    }
    if ((joyDir & TAJoystickLeft) && (px > 0))
    {
      px--;
      joystickCoolDown = joystickCoolDownStart;
    }
    else if ((joyDir & TAJoystickRight) && (px < 95))
    {
      px++;
      joystickCoolDown = joystickCoolDownStart;
    }
  }
  if (buttonCoolDown > 0)
  {
    buttonCoolDown--;
  }
  else
  {
    if ((btn & TAButton1) && (px < 48))
    {
      buttonCoolDown = BUTTON_COOLDOWN;
      active->saved = false;
      uint8_t dx;
      uint8_t dy;
      if (STATE_PAINT == state)
      {
        dx = px;
        dy = py;
      }
      else
      {
        dx = px / 2 + zoomx;
        dy = py / 2 + zoomy;
      }
      if (tool == TOOL_DRAW)
      {
        active->img[dy * 48 * 2 + dx * 2] = color1;
        active->img[dy * 48 * 2 + dx * 2 + 1] = color2;
      }
      else if (tool == TOOL_FLOOD)
      {
        unsigned char fillSpot = active->baseImg[dy * 48 + dx];
        if (fillSpot != 0)
        {
          for (int i = 0; i < 64*48; i++)
          {
            if (active->baseImg[i] == fillSpot)
            {
              active->img[i * 2] = color1;
              active->img[i * 2 + 1] = color2;
            }
          }
        }
      }
    }
    else if ((btn & TAButton1) && (px > 50) && (py > 19) && (py < 62))
    {
      color1 = _image_paint_data[(py * 48 + (px - 48)) * 2];
      color2 = _image_paint_data[(py * 48 + (px - 48)) * 2 + 1];
      buttonCoolDown = BUTTON_COOLDOWN;
    }
    else if ((btn & TAButton1) && (px > 50) && (px < 68) && (py > 1) && (py < 18))
    {
      tool = TOOL_DRAW;
      buttonCoolDown = BUTTON_COOLDOWN;
    }
    else if ((btn & TAButton1) && (px > 70) && (px < 87) && (py > 1) && (py < 18))
    {
      tool = TOOL_FLOOD;
      buttonCoolDown = BUTTON_COOLDOWN;
    }
    else if ((btn & TAButton1) && (px > 88) && (px < 95) && (py > 1) && (py < 18))
    {
      if (!active->saved)
      {
        if (!dataFile.open("colormn1.dat", O_WRITE | O_CREAT | O_TRUNC))
        {
        }
        else
        {
          dataFile.write(active->img, 64*48*2);
          dataFile.sync();
          dataFile.close();
          active->saved = true;
        }
      }
      buttonCoolDown = BUTTON_COOLDOWN;
    }
    if ((btn & TAButton2) && (px < 48))
    {
      if (state == STATE_PAINT)
      {
        state = STATE_PAINTZOOM;
        zoomx = px - 12;
        if (px < 12)
          zoomx = 0;
        else if (zoomx > 24)
          zoomx = 24;
        zoomy = py - 16;
        if (py < 16)
          zoomy = 0;
        else if (zoomy > 32)
          zoomy = 32;
      }
      else
        state = STATE_PAINT;
      buttonCoolDown = BUTTON_COOLDOWN;
    }
  }
}

void Painter::draw()
{
  display.goTo(0,0);
  display.startData();
  
  uint8_t lineBuffer[96 * 2];

  for(int lines = 0; lines < 64; ++lines)
  {
    if (state == STATE_PAINT)
      memcpy(lineBuffer, active->img + (lines * 48 * 2), 48 * 2);
    else
    {
      int curLine = zoomy + lines / 2;
      for (int i = 0; i < 24; ++i)
      {
        lineBuffer[i * 2 * 2] = active->img[curLine * 48 * 2 + (zoomx + i) * 2];
        lineBuffer[i * 2 * 2 + 1] = active->img[curLine * 48 * 2 + (zoomx + i) * 2 + 1];
        lineBuffer[i * 2 * 2 + 2] = active->img[curLine * 48 * 2 + (zoomx + i) * 2];
        lineBuffer[i * 2 * 2 + 3] = active->img[curLine * 48 * 2 + (zoomx + i) * 2 + 1];
      }
    }
    memcpy(lineBuffer + 48 * 2, _image_paint_data + lines * 48 * 2,48*2);
    if ((lines == 1) || (lines == 18))
    {
      if (tool == TOOL_DRAW)
      {
        for (int i = (48 + 2) * 2; i < (48 + 20) * 2; i += 2)
        {
          lineBuffer[i] = color1;
          lineBuffer[i + 1] = color2;
        }
      }
      else if (tool == TOOL_FLOOD)
      {
        for (int i = (48 + 22) * 2; i < (48 + 40) * 2; i += 2)
        {
          lineBuffer[i] = color1;
          lineBuffer[i + 1] = color2;
        }
      }
    }
    else if ((lines > 1) && (lines < 18))
    {
      if (tool == TOOL_DRAW)
      {
        lineBuffer[(48 + 2) * 2] = color1;
        lineBuffer[(48 + 2) * 2 + 1] = color2;
        lineBuffer[(48 + 19) * 2] = color1;
        lineBuffer[(48 + 19) * 2 + 1] = color2;
      }
      else if (tool == TOOL_FLOOD)
      {
        lineBuffer[(48 + 22) * 2] = color1;
        lineBuffer[(48 + 22) * 2 + 1] = color2;
        lineBuffer[(48 + 39) * 2] = color1;
        lineBuffer[(48 + 39) * 2 + 1] = color2;
      }
    }
    if ((lines >= py) && (lines < py + 8))
    {
      for (int x = 0; x < 8; x++)
      {
        if (x + px >= 96)
          break;
        unsigned char pointerCol1 = _image_pointer_data[(x + (lines - py) * 8) * 2];
        unsigned char pointerCol2 = _image_pointer_data[(x + (lines - py) * 8) * 2 + 1];
        if ((pointerCol1 != 0) || (pointerCol2 != 31))
        {
          lineBuffer[(px + x) * 2] = pointerCol1;
          lineBuffer[(px + x) * 2 + 1] = pointerCol2;
        }
      }
    }
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

Painter paint;

unsigned long lastTime;

void setup()
{
  arcadeInit();
  display.begin();
  display.setBitDepth(TSBitDepth16);
  display.setBrightness(15);
  display.setFlip(false);

#ifdef TINYARCADE_CONFIG
  USBDevice.init();
  USBDevice.attach();
#endif
  SerialUSB.begin(9600);
  active->initImage(_image_cateye_data);
  if (!sd.begin(10,SPI_FULL_SPEED)) {
    SerialUSB.println("Card failed");
    while(1);
  }
  lastTime = millis();
  if (sd.exists("colormn1.dat"))
  {
    if (!dataFile.open("colormn1.dat", O_READ))
    {
    }
    else
    {
      int sz = dataFile.write(active->img, 64*48*2);
      dataFile.read(active->img, 64*48*2);
      dataFile.sync();
      dataFile.close();
    }
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  if ((state == STATE_PAINT) || (state == STATE_PAINTZOOM))
  {
    paint.update();
    paint.draw();
  }
  unsigned long oldTime = lastTime;
  lastTime = millis();
  if ((lastTime > oldTime) && (lastTime - oldTime < 70))
  {
    delay(70 - (lastTime - oldTime));
  }
}
