#include <TinyScreen.h>
#include "SdFat.h"
#include <SPI.h>
#include <Wire.h>
#include "TinyConfig.h"
#include "cateye.h"
#include "tileset.h"
#include "font.h"
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
#define STATE_WORLD 2
#define STATE_BATTLECHOICE 3

#define BUTTON_COOLDOWN 5
#define JOYSTICK_COOLDOWNSTART 4

int state = 0;
int buttonCoolDown = 0;
int joystickCoolDown = 0;
int joystickCoolDownStart = JOYSTICK_COOLDOWNSTART;

#define PART_NONE 255

class ColorMonsterPart
{
  public:
    ColorMonsterPart() : part(PART_NONE) {}

    uint8_t part;
    uint8_t color;
    uint8_t strength;
    uint8_t health;
};

class ColorMonster
{
  public:
    ColorMonster() : saved(false) { memset(img, 0, 64*48*2); }
    void initImage(const unsigned char *bi);

    unsigned char img[64*48*2];
    const unsigned char *baseImg;
    ColorMonsterPart part[5];
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
ColorMonster opponent[1];
ColorMonster *activeOpponent = &opponent[0];

class Trainer
{
  public:
    Trainer() : location(0), icon(28), x(136), y(96) {}

    int location;
    uint8_t icon;
    int x, y;
};

Trainer pc;

class Area
{
  public:
    Area(uint8_t x, uint8_t y, const uint8_t *d) : xSize(x), ySize(y), data(d) {}

    uint8_t xSize, ySize;
    const uint8_t *data;
};

const uint8_t startTownMap[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,31,21,31,21,21,21,31,21,31,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,31, 9, 0, 9, 0, 0, 9, 9,31,
  0, 0, 0,14, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0,31, 0,22, 0,22, 0,29, 9,31,
  0, 0, 0,13, 5, 5, 5,13, 0, 0, 0, 0, 0, 0, 0,31, 9, 0, 0, 0, 0, 0, 0,31,
  0, 0, 0,13,12, 4,12,13, 0, 0, 0, 0, 0, 0, 0,31, 0,22, 0,25, 9,25, 0,31,
  0, 0, 0,13, 4, 1, 4,13, 0, 0, 0, 0, 0, 0, 0,31, 0, 0, 0, 0, 0, 9, 0,31,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,31,21,31,21,20,21,31,21,31,
 15,15, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 16,17,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,10, 0, 0,
  0,18,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0,10, 0, 0, 0,
  0,18,17, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,11,11,11,11, 0,10, 0, 0, 0,
  0,23,17,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 1, 2, 0, 0, 0, 0, 0,
  0, 0,18,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0,18,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0,18,17,15,15, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0,
  0, 0,18,17,17,17,19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
const Area startTown(24, 16, startTownMap);

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
        if (!dataFile.open("colormonster/colormn1.dat", O_WRITE | O_CREAT | O_TRUNC))
        {
        }
        else
        {
          dataFile.write(active->img, 64*48*2);
          dataFile.sync();
          dataFile.close();
          active->saved = true;
          state = STATE_WORLD;
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

class World
{
  public:
    void update();
    void draw();
    uint8_t getTile(int x, int y);
    const uint8_t *getTileData(int tile, int y);
    const uint8_t *getFontData(char c, int y);
};

void World::update()
{
  uint8_t btn = checkButton(TAButton1 | TAButton2);
  uint8_t joyDir = checkJoystick(TAJoystickUp | TAJoystickDown | TAJoystickLeft | TAJoystickRight);
  if (joystickCoolDown > 0)
  {
    joystickCoolDown--;
  }
  else
  {
    if ((joyDir & TAJoystickUp) && (pc.y > 0))
    {
      pc.y--;
    }
    else if ((joyDir & TAJoystickDown) && (pc.y < startTown.ySize * 8))
    {
      pc.y++;
    }
    if ((joyDir & TAJoystickLeft) && (pc.x > 0))
    {
      pc.x--;
    }
    else if ((joyDir & TAJoystickRight) && (pc.x < startTown.xSize * 8))
    {
      pc.x++;
    }
  }
  if (buttonCoolDown > 0)
  {
    buttonCoolDown--;
  }
  else
  {
  }
}

void World::draw()
{
  int startX(0), startY(0);

  startX = pc.x - 46;
  startY = pc.y - 30;
  display.goTo(0,0);
  display.startData();

  uint8_t lineBuffer[96 * 2];

  for(int lines = 0; lines < 64; ++lines)
  {
    int currentY = startY + lines;
    if ((currentY < 0) || (currentY >= (startTown.ySize * 8)))
    {
      memset(lineBuffer, 0, 96 * 2);
    }
    else
    {
      int x = 0;
      if (startX < 0)
      {
        x = startX * -1;
        memset(lineBuffer, 0, x * 2);
      }
      int currentTile = getTile(startX + x, currentY);
      if (x == 0)
      {
        int init = (x + startX) % 8;
        if (init != 0)
        {
          if (currentTile == 0)
            memset(lineBuffer + x * 2, 255, (8 - init) * 2);
          else
            memcpy(lineBuffer + x * 2, getTileData(currentTile, currentY % 8) + (init * 2), (8 - init) * 2);
          x += 8 - init;
        }
      }
      while (x <= 88)
      {
        if (startX + x > ((startTown.xSize - 1) * 8))
        {
          memset(lineBuffer + x * 2, 0, (96 - x) * 2);
          x = 96;
          break;
        }
        currentTile = getTile(startX + x, currentY);
        if (currentTile == 0)
          memset(lineBuffer + x * 2, 255, 8 * 2);
        else
          memcpy(lineBuffer + x * 2, getTileData(currentTile, currentY % 8), 8 * 2);
        x += 8;
      }
      if (x != 96)
      {
        int init = 96 - x;
        if (startX + x > ((startTown.xSize - 1) * 8))
        {
          memset(lineBuffer + x * 2, 0, init * 2);
          x = 96;
        }
        else
        {
          currentTile = getTile(startX + x, currentY);
          if (currentTile == 0)
            memset(lineBuffer + x * 2, 255, init * 2);
          else
            memcpy(lineBuffer + x * 2, getTileData(currentTile, currentY % 8), init * 2);
        }
      }
      if ((currentY >= pc.y) && (currentY < pc.y + 8))
      {
        const uint8_t *data = getTileData(pc.icon, currentY - pc.y);
        for (int i = 0; i < 8; i++)
        {
          if ((data[i * 2] != 255) || (data[i * 2 + 1] != 255))
          {
            lineBuffer[(46 + i) * 2] = data[i * 2];
            lineBuffer[(46 + i) * 2 + 1] = data[i * 2 + 1];
          }
        }
      }
    }
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

uint8_t World::getTile(int x, int y)
{
  return startTown.data[x / 8 + ((y / 8) * startTown.xSize)];
}

const uint8_t *World::getTileData(int tile, int y)
{
  y += ((tile - 1) / 8) * 8;
  int x = ((tile - 1) % 8) * 8;
  return _image_tileset_data + (x + y * (8 * 8 )) * 2;
}

const uint8_t *World::getFontData(char c, int y)
{
  y += ((c - 32) / 18) * 7;
  int x = ((c - 32) % 18) * 6;
  return _image_charmap_oldschool_white_data + (x + y * (18 * 6 )) * 2;
}

World world;

class Battle
{
  public:
    Battle() : action(0) {}
    void update();
    void draw();

    uint8_t action;
    static const char *baseOption[4];
    static uint8_t optionRect[4][4];
};


void Battle::update()
{
  uint8_t btn = checkButton(TAButton1 | TAButton2);
  uint8_t joyDir = checkJoystick(TAJoystickUp | TAJoystickDown | TAJoystickLeft | TAJoystickRight);
  if (joystickCoolDown > 0)
  {
    joystickCoolDown--;
  }
  else
  {
    if (joyDir & TAJoystickUp)
    {
      if (action & 0x01)
        action -= 1;
      joystickCoolDown = joystickCoolDownStart;
    }
    if (joyDir & TAJoystickDown)
    {
      if ((action & 0x01) == 0)
        action += 1;
      joystickCoolDown = joystickCoolDownStart;
    }
    if (joyDir & TAJoystickLeft)
    {
      if (action & 0x02)
        action -= 2;
      joystickCoolDown = joystickCoolDownStart;
    }
    if (joyDir & TAJoystickRight)
    {
      if ((action & 0x02) == 0)
        action += 2;
      joystickCoolDown = joystickCoolDownStart;
    }
  }
}

void Battle::draw()
{
  display.goTo(0,0);
  display.startData();

  uint8_t lineBuffer[96 * 2];

  for(int lines = 0; lines < 64; ++lines)
  {
    memcpy(lineBuffer, active->img + (lines * 48 * 2), 48 * 2);
    memcpy(lineBuffer + (48 *2), activeOpponent->img + (lines * 48 * 2), 48 * 2);
    if (state == STATE_BATTLECHOICE)
    {
      for (int rect = 0; rect < 4; rect++)
      {
        if (rect == (action & 0xFF))
        {
          if ((lines == optionRect[rect][1]) || (lines == optionRect[rect][3]))
          {
            memset(lineBuffer + (optionRect[rect][0] * 2), 0xFF, ((optionRect[rect][2] - optionRect[rect][0] + 1) * 2));
          }
          if ((lines > optionRect[rect][1]) && (lines < optionRect[rect][3]))
          {
            memset(lineBuffer + (optionRect[rect][0] * 2), 0xFF, 2);
            memset(lineBuffer + (optionRect[rect][2] * 2), 0xFF, 2);
          }
        }
        if ((lines > optionRect[rect][1]) && (lines < optionRect[rect][3]))
        {
          memset(lineBuffer + ((optionRect[rect][0] + 1) * 2), 0x00, ((optionRect[rect][2] - optionRect[rect][0] - 1) * 2));
          for (int i = 0; baseOption[rect][i] != 0; i++)
          {
            const uint8_t *fontData = world.getFontData(baseOption[rect][i], lines - optionRect[rect][1] - 1);
            memcpy(lineBuffer + ((optionRect[rect][0] + 1 + (i * 6)) * 2), fontData, 6 * 2);
          }
        }
      }
    }
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

const char *Battle::baseOption[4] = { "Attack", "Item", "Swap", "Run" };
uint8_t Battle::optionRect[4][4] = { {0, 46, 47, 54}, {0, 55, 47, 63}, {48, 46, 95, 54}, {48, 55, 95, 63}, };

Battle battle;

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
  activeOpponent->initImage(_image_cateye_data);
  if (!sd.begin(10,SPI_FULL_SPEED)) {
    SerialUSB.println("Card failed");
    while(1);
  }
  lastTime = millis();
  if (sd.exists("colormonster/colormn1.dat"))
  {
    if (!dataFile.open("colormonster/colormn1.dat", O_READ))
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
  else if (state == STATE_WORLD)
  {
    world.update();
    world.draw();
  }
  else if (state == STATE_BATTLECHOICE)
  {
    battle.update();
    battle.draw();
  }
  unsigned long oldTime = lastTime;
  lastTime = millis();
  if ((lastTime > oldTime) && (lastTime - oldTime < 70))
  {
    delay(70 - (lastTime - oldTime));
  }
}

