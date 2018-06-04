#include <TinyScreen.h>
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

#define STATE_PAINT 0

#define BUTTON_COOLDOWN 5
#define JOYSTICK_COOLDOWN 3

int state = 0;
int buttonCoolDown = 0;
int joystickCoolDown = 0;

class ColorMonster
{
  public:
    ColorMonster() { memset(img, 0, 64*48*2); }
    void initImage(const unsigned char *bi);

    unsigned char img[64*48*2];
    const unsigned char *baseImg;
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
    if ((joyDir & TAJoystickUp) && (py > 0))
    {
      py--;
      joystickCoolDown = JOYSTICK_COOLDOWN;
    }
    else if ((joyDir & TAJoystickDown) && (py < 63))
    {
      py++;
      joystickCoolDown = JOYSTICK_COOLDOWN;
    }
    if ((joyDir & TAJoystickLeft) && (px > 0))
    {
      px--;
      joystickCoolDown = JOYSTICK_COOLDOWN;
    }
    else if ((joyDir & TAJoystickRight) && (px < 95))
    {
      px++;
      joystickCoolDown = JOYSTICK_COOLDOWN;
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
      if (tool == TOOL_DRAW)
      {
        active->img[py * 48 * 2 + px * 2] = color1;
        active->img[py * 48 * 2 + px * 2 + 1] = color2;
      }
      else if (tool == TOOL_FLOOD)
      {
        unsigned char fillSpot = active->baseImg[py * 48 + px];
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
    else if ((btn & TAButton1) && (px > 51) && (py > 19) && (py < 62))
    {
      color1 = _image_paint_data[(py * 48 + (px - 48)) * 2];
      color2 = _image_paint_data[(py * 48 + (px - 48)) * 2 + 1];
      buttonCoolDown = BUTTON_COOLDOWN;
    }
    if (btn & TAButton2)
    {
      if (++tool >= 2)
        tool = TOOL_DRAW;
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
    memcpy(lineBuffer, active->img + (lines * 48 * 2), 48 * 2);
    memcpy(lineBuffer + 48 * 2, _image_paint_data + lines * 48 * 2,48*2);
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
    if ((lines == 1) || (lines == 18))
    {
      if (tool == TOOL_DRAW)
      {
        for (int i = (48 + 3) * 2; i < (48 + 20) * 2; i += 2)
        {
          lineBuffer[i] = color1;
          lineBuffer[i + 1] = color2;
        }
      }
      else if (tool == TOOL_FLOOD)
      {
        for (int i = (48 + 27) * 2; i < (48 + 45) * 2; i += 2)
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
        lineBuffer[(48 + 3) * 2] = color1;
        lineBuffer[(48 + 3) * 2 + 1] = color2;
        lineBuffer[(48 + 19) * 2] = color1;
        lineBuffer[(48 + 19) * 2 + 1] = color2;
      }
      else if (tool == TOOL_FLOOD)
      {
        lineBuffer[(48 + 27) * 2] = color1;
        lineBuffer[(48 + 27) * 2 + 1] = color2;
        lineBuffer[(48 + 44) * 2] = color1;
        lineBuffer[(48 + 44) * 2 + 1] = color2;
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
  lastTime = millis();
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (state == STATE_PAINT)
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
