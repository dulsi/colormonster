#include <TinyScreen.h>
#include "SdFat.h"
#include <SPI.h>
#include <Wire.h>
#include "TinyConfig.h"
#include "battle.h"
#include "dialogcommand.h"
#include "monsters.h"
#include "tileset.h"
#include "portraits.h"
#include "font.h"
#include "colormonster.h"
#include "ui.h"
#include "uiobject.h"
#include "npcdialog.h"

#ifdef TINYARCADE_CONFIG
#include "TinyArcade.h"
#endif
#ifdef TINYSCREEN_GAMEKIT_CONFIG
#include "TinyGameKit.h"
#endif

TinyScreen display = TinyScreen(TinyScreenPlus);

SdFat sd;
SdFile dataFile;

#define BUTTON_COOLDOWN 5
#define JOYSTICK_COOLDOWNSTART 4

#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4
#define DIRECTION_NONE 128
#define DIRECTION_PAUSE 16

#define COLLISION_NPC 2
#define MAX_NPC 30
#define NPC_DISTANCE 16

#define CHOICE_NONE 255
#define CHOICE_BACK 254

#define BASEOPTION_ATTACK 1
#define BASEOPTION_ITEM 2
#define BASEOPTION_SWAP 3
#define BASEOPTION_RUN 4

#define PATTERN_LINES 1
#define PATTERN_SCALE 2
#define PATTERN_COUNT 3

int state = STATE_TITLE;
int prevState = STATE_WORLD;
int turn = 0;
int buttonCoolDown = 0;
int joystickCoolDown = 0;
int joystickCoolDownStart = JOYSTICK_COOLDOWNSTART;

const uint8_t bottomRow[] = { 1, 48, 94, 63};
const uint8_t startMenuRow[] = { 4, 31, 58, 46};
uint8_t nameRow[] = { 28, 37, 94, 44};
const uint8_t portraitLoc[] = { 1, 21, 24, 44};

const char noItems[] = "You have no items.";
const char noSwap[] = "You have no more monsters.";

const uint8_t colorList[16][2] = {
  {0x08,0x54},
  {0x00,0x1f},
  {0x58,0x81},
  {0xf8,0xe0},
  {0xf8,0x77},
  {0x80,0x4c},
  {0x7b,0xcf},
  {0xff,0xff},
  {0x27,0xff},
  {0x04,0xb2},
  {0x05,0xe0},
  {0x13,0x02},
  {0x0a,0x9f},
  {0x0c,0x5f},
  {0x52,0xe1},
  {0xe7,0xe7}
};

const uint8_t tilesetCollision[] = {
  0, 1, 1, 1, 1, 1, 1, 1,
  0, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 0, 1, 1, 1, 1,
  1, 1, 1, 1, 0, 0, 1, 1,
  0, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1
};


void ColorMonster::init(uint8_t bm)
{
  baseMonster = bm;
  const unsigned char *baseImg = monsterType[baseMonster].img;
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
  for (int i = 0; i < 2; i++)
  {
    power[i].power = i;
    power[i].strength = monsterType[baseMonster].power[i].strength;
  }
  hp = maxHp = monsterType[baseMonster].baseHp;
}

void ColorMonster::init(const NPCMonster m)
{
  init(m.baseMonster, m.ruleCount, m.rules);
}

void ColorMonster::init(uint8_t bm, int count, const ColorRule *r)
{
  baseMonster = bm;
  const unsigned char *baseImg = monsterType[baseMonster].img;
  for (int i = 0; i < 64*48; i++)
  {
    if (baseImg[i] == 0)
    {
      img[i * 2] = 0;
      img[i * 2 + 1] = 0;
    }
    else
    {
      bool processed = false;
      for (int k = 0; k < count; k++)
      {
        if (baseImg[i] == r[k].origColor)
        {
          processed = true;
          switch (r[k].instruct)
          {
            case PATTERN_LINES:
              if ((i / 4) % 2 == 1)
              {
                img[i * 2] = r[k].color[1][0];
                img[i * 2 + 1] = r[k].color[1][1];
              }
              else
              {
                img[i * 2] = r[k].color[0][0];
                img[i * 2 + 1] = r[k].color[0][1];
              }
              break;
            case PATTERN_SCALE:
              if ((i / 48) % 2 == 1)
              {
                if (((((i / 96) % 2 == 1) && ((i + 7) % 8) < 2)) || (((i / 96) % 2 == 0) && ((i + 3) % 8) < 2))
                {
                 img[i * 2] = r[k].color[1][0];
                 img[i * 2 + 1] = r[k].color[1][1];
                }
                else
                {
                 img[i * 2] = r[k].color[0][0];
                 img[i * 2 + 1] = r[k].color[0][1];
                }
              }
              else
              {
                int j = i % 8;
                if (((i / 96) % 2 == 1) && ((j == 0) || (j == 3)) || (((i / 96) % 2 == 0) && ((j == 4) || (j == 7))))
                {
                 img[i * 2] = r[k].color[1][0];
                 img[i * 2 + 1] = r[k].color[1][1];
                }
                else
                {
                 img[i * 2] = r[k].color[0][0];
                 img[i * 2 + 1] = r[k].color[0][1];
                }
              }
              break;
            default:
              img[i * 2] = r[k].color[0][0];
              img[i * 2 + 1] = r[k].color[0][1];
              break;
          }
        }
      }
      if (!processed)
      {
        img[i * 2] = 255;
        img[i * 2 + 1] = 255;
      }
    }
  }
  for (int i = 0; i < 2; i++)
  {
    power[i].power = i;
    power[i].strength = monsterType[baseMonster].power[i].strength;
  }
  hp = maxHp = monsterType[baseMonster].baseHp;
}

void ColorMonster::initRandom()
{
  int mon = random(0, COLMONSTERTYPE_COUNT);
  ColorRule rules[20];
  int ruleCount = 0;
  for (int i = 0; i < 64 * 48; i++)
  {
    if (monsterType[mon].img[i] != 0xFF)
    {
      bool found = false;
      for (int j = 0; j < ruleCount; j++)
      {
        if (rules[j].origColor == monsterType[mon].img[i])
          found = true;
      }
      if (!found)
      {
        rules[ruleCount].instruct = random(0, PATTERN_COUNT);
        rules[ruleCount].origColor = monsterType[mon].img[i];
        for (int j = 0; j < 2; j++)
        {
          int c = random(0, 16);
          rules[ruleCount].color[j][0] = colorList[c][0];
          rules[ruleCount].color[j][1] = colorList[c][1];
        }
        ruleCount++;
        if (ruleCount == 20)
          break;
      }
    }
  }
  init(mon, ruleCount, rules);
}

void ColorMonster::buildChoice(uint8_t &choiceEnd, char **choiceList, char *choiceString)
{
  choiceEnd = 0;
  int where = 0;
  for (int i = 0; i < 5; i++)
  {
    if (power[i].power == POWER_NONE)
      break;
    const ColorMonsterPowerType &powerType = monsterType[baseMonster].power[power[i].power];
    choiceList[choiceEnd] = choiceString + where;
    strcpy(choiceString + where, powerType.name);
    where += strlen(powerType.name) + 1;
    choiceEnd++;
  }
}

void ColorMonster::calculateColor()
{
  for (int i = 0; i < 5; i++)
  {
    if (power[i].power == POWER_NONE)
      break;
    uint8_t region = monsterType[baseMonster].power[power[i].power].region;
    const unsigned char *baseImg = monsterType[baseMonster].img;
    uint8_t endColor(0);
    int colors[100][2];
    for (int l = 0; l < 96*64; l++)
    {
      if (baseImg[l] == region)
      {
        bool found = false;
        for (int k = 0; k < endColor; k++)
        {
          if (colors[k][0] == ((img[l * 2 + 1] << 8) + img[l * 2]))
          {
            colors[k][1]++;
            found = true;
            break;
          }
        }
        if ((!found) && (endColor < 100))
        {
          colors[endColor][0] = (img[l * 2 + 1] << 8) + img[l * 2];
          colors[endColor][1] = 1;
          endColor++;
        }
      }
    }
    int max = 0;
    for (int k = 1; k < endColor; k++)
    {
      if (colors[k][1] > colors[max][1])
        max = k;
    }
    power[i].color = colors[max][0];
  }
}

ColorMonster party[MONSTER_PARTYSIZE];
ColorMonster *active = &party[0];
ColorMonster opponent[MONSTER_PARTYSIZE];
ColorMonster *activeOpponent = &opponent[0];

class Trainer
{
  public:
    Trainer() : location(0), icon(28), x(136), y(96) {}

    int location;
    uint8_t icon;
    int x, y;
    uint8_t dir;
    uint8_t secrets[10];
};

Trainer pc;

class NPC
{
  public:
    const char *name;
    uint8_t icon;
    uint8_t portrait;
    int startX,startY;
    const uint8_t *dialog;
    uint8_t monsterCount;
    const NPCMonster *monsters;
};

class Portal
{
  public:
    int locX, locY;
    uint8_t area;
    int startX, startY;
};

class Area
{
  public:
    Area(uint8_t x, uint8_t y, const uint8_t *d, uint8_t cNPC, const NPC *n, uint8_t cPortal, const Portal *p) : xSize(x), ySize(y), data(d), countNPC(cNPC), npc(n), countPortal(cPortal), portals(p) {}

    uint8_t xSize, ySize;
    const uint8_t *data;
    uint8_t countNPC;
    const NPC *npc;
    uint8_t countPortal;
    const Portal *portals;
};

const NPC hospitalNPC[] = {
  { "Dr. Ava", 27, 5, 8 * 8, 5 * 8, doctorDialog, 0, NULL },
  { "Kirk", 28, 0, 4 * 8, 6 * 8, worriedDialog, 0, NULL }
};
Portal hospitalPortal[] = { { 8 * 8, 9 * 8, 0, 17 * 8, 12 * 8} };
const uint8_t hospitalMap[] = {
 31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,  31,
 31,   0,  40,   0,  31,   0,  38,   0,   0,   0,  38,   0,  31,   0,  40,   0,  31,
 31,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  31,
 31,   0,   0,   0,  31,   0,   0,   0,   0,   0,   0,   0,  31,   0,   0,   0,  31,
 31,  31,  31,  31,  31,   0,   0,   0,   0,   0,   0,   0,  31,  31,  31,  31,  31,
 31,   0,  48,   0,  31,   0,   0,  41,   0,  35,  41,   0,  31,   0,  40,   0,  31,
 31,   0,   0,   0,   0,   0,   0,  41,  41,  41,  41,   0,   0,   0,   0,   0,  31,
 31,   0,   0,   0,  31,   0,   0,   0,   0,   0,   0,   0,  31,   0,   0,   0,  31,
 31,  31,  31,  31,  31,   0,   0,   0,   0,   0,   0,   0,  31,  31,  31,  31,  31,
255, 255, 255, 255,  31,  31,  31,  31,   0,  39,  31,  31,  31, 255, 255, 255, 255
};
const Area hospital(17, 10, hospitalMap, 2, hospitalNPC, 1, hospitalPortal);

Portal startTownPortal[] = { { 17 * 8, 11 * 8, 1, 8 * 8, 8 * 8} };
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
const ColorRule jessCateyeRule[] = {
 { 0, 0x49, { { 0x08, 0x54 }, { 0, 0 } } },
 { PATTERN_LINES, 0x03, { { 0x05, 0xe0 }, { 0x13, 0x02 } } }
};
const NPCMonster jessCateye[] = {
  {0, 2, jessCateyeRule}
};
const NPC startTownNPC[] = { { "Jessica", 27, 1, 80, 40, sampleDialog, 1, jessCateye } };
const Area startTown(24, 16, startTownMap, 1, startTownNPC, 1, startTownPortal);

const Area *areaList[] = {
  &startTown,
  &hospital
};

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
        const unsigned char *baseImg = monsterType[active->baseMonster].img;
        unsigned char fillSpot = baseImg[dy * 48 + dx];
        if (fillSpot != 0)
        {
          for (int i = 0; i < 64*48; i++)
          {
            if (baseImg[i] == fillSpot)
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
      if (px > 50)
      {
        if ((py >= 20) && ((py - 20) % 11 < 9) && ((px - 50) % 11 < 9))
        {
          int colorIdx = ((py - 20) / 11) * 4 + ((px - 50) / 11);
          if (colorIdx < 16)
          {
            color1 = colorList[colorIdx][0];
            color2 = colorList[colorIdx][1];
            buttonCoolDown = BUTTON_COOLDOWN;
          }
        }
      }
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
          active->calculateColor();
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
  int colorIdx = 0;

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
    if (lines < 2)
      memset(lineBuffer + 48 * 2, 0, 48 * 2);
    else if (lines < 18)
      memcpy(lineBuffer + 48 * 2, _image_paint_data + (lines - 2) * 48 * 2,48*2);
    else
    {
      if ((colorIdx < 16) && (lines >= 20) && ((lines - 20) % 11 < 9))
      {
        memset(lineBuffer + 48 * 2, 0, 3 * 2);
        for (int i = 0; i < 4; ++i)
        {
          for (int x = 0; x < 9; ++x)
          {
            lineBuffer[(i * 11 + x + 51) * 2] = colorList[colorIdx + i][0];
            lineBuffer[(i * 11 + x + 51) * 2 + 1] = colorList[colorIdx + i][1];
          }
          memset(lineBuffer + (i * 11 + 60) * 2, 0, 2 * 2);
        }
        memset(lineBuffer + 95 * 2, 0, 1 * 2);
        if ((lines - 20) % 11 == 8)
          colorIdx += 4;
      }
      else
        memset(lineBuffer + 48 * 2, 0, 48 * 2);
    }
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

class NPCInstance
{
  public:
    int x, y;
    uint8_t dir;
};

class World
{
  public:
    World(const Area *cArea) : currentArea(cArea) { init(); }
    void init();
    void buildCollision();
    void update();
    void updateNPCs();
    void draw();
    uint8_t findNPC(int xWhere, int yWhere);
    uint8_t getCollision(int xWhere, int yWhere);
    uint8_t getTile(int x, int y);
    const uint8_t *getTileData(int tile, int y);
    const uint8_t *getFontData(char c, int y);
    bool isTrainerIn(int xWhere, int yWhere);

    const Area *currentArea;
    uint8_t collision[800];
    NPCInstance npc[MAX_NPC];
};

World world(&startTown);

Battle battle;

void MessageBox::setText(const char *t)
{
  const char *where = t;
  int indx = 0;
  int len = strlen(t);
  int letters = (rect[2] - rect[0] + 1) / 6;
  for (int i = 0; i < 7; i++)
    text[i][0] = 0;
  while (*where)
  {
    strncpy(text[indx], where, letters);
    text[indx][letters] = 0;
    if (len <= letters)
      break;
    int i = letters - 1;
    while ((i > 0) && (text[indx][i] != ' '))
      i--;
    if (i == 0)
    {
      i = letters;
      text[indx][letters - 1] = 0;
      len -= i;
    }
    else
    {
      text[indx][i] = 0;
      if (where[i] != 0)
      {
        i++;
      }
      len -= i;
    }
    where += i;
    indx++;
  }
}

void MessageBox::draw(int line, uint8_t lineBuffer[96 * 2])
{
  if ((line >= rect[1]) && (line <= rect[3]))
  {
    {
      int offset = rect[0] - 2;
      if (offset < 0)
        offset = 0;
      int distance = rect[2] - rect[0] + 5;
      if (distance > 96)
        distance = 96;
      memset(lineBuffer + offset * 2, 0x00, distance * 2);
    }
    memset(lineBuffer + (rect[0] - 1) * 2, 0xFF, 2);
    memset(lineBuffer + (rect[2] + 1) * 2, 0xFF, 2);
    int yOffset = (line - rect[1]) % 8;
    int yWhere = (line - rect[1]) / 8;
    if (yOffset != 7)
    {
      for (int i = 0; text[yWhere][i]; i++)
      {
        const uint8_t *fontData = world.getFontData(text[yWhere][i], yOffset);
        memcpy(lineBuffer + ((rect[0] + 1 + i * 6) * 2), fontData, 6 * 2);
      }
    }
  }
  else if ((line == rect[1] - 1) || (line == rect[3] + 1))
  {
    memset(lineBuffer + (rect[0] - 1) * 2, 0xFF, (rect[2] - rect[0] + 3) * 2);
    if (rect[0] - 2 >= 0)
      memset(lineBuffer + (rect[0] - 2) * 2, 0x00, 2);
    if (rect[2] + 2 >= 0)
      memset(lineBuffer + (rect[2] + 2) * 2, 0x00, 2);
  }
  else if ((line == rect[1] - 2) || (line == rect[3] + 2))
  {
    int offset = rect[0] - 2;
    if (offset < 0)
      offset = 0;
    int distance = rect[2] - rect[0] + 5;
    if (distance > 96)
      distance = 96;
    memset(lineBuffer + offset * 2, 0x00, distance * 2);
  }
}

class Portrait
{
  public:
    Portrait(const uint8_t *r) : rect(r) {  }

    void setPortrait(uint8_t p);
    void draw(int line, uint8_t lineBuffer[96 * 2]);

  private:
    const uint8_t *rect;
    uint8_t portrait;
};

void Portrait::setPortrait(uint8_t p)
{
  portrait = p;
}

void Portrait::draw(int line, uint8_t lineBuffer[96 * 2])
{
  if ((line >= rect[1]) && (line <= rect[3]))
  {
    {
      int offset = rect[0] - 2;
      if (offset < 0)
        offset = 0;
      int distance = rect[2] - rect[0] + 4;
      if (distance > 96)
        distance = 96;
      memset(lineBuffer + offset * 2, 0x00, distance * 2);
    }
    memset(lineBuffer + (rect[0] - 1) * 2, 0xFF, 2);
    memset(lineBuffer + (rect[2] + 1) * 2, 0xFF, 2);
    for (int i = 0; i < 24; i++)
    {
      uint8_t val = _image_portraits_data[portrait * 24 * 24 + (line - rect[1]) * 24 + i];
      lineBuffer[(rect[0] + i) * 2] = val;
      lineBuffer[(rect[0] + i) * 2 + 1] = val;
    }
  }
  else if ((line == rect[1] - 1) || (line == rect[3] + 1))
  {
    memset(lineBuffer + (rect[0] - 1) * 2, 0xFF, (rect[2] - rect[0] + 3) * 2);
    if (rect[0] - 2 >= 0)
      memset(lineBuffer + (rect[0] - 2) * 2, 0x00, 2);
    if (rect[2] + 2 >= 0)
      memset(lineBuffer + (rect[2] + 2) * 2, 0x00, 2);
  }
  else if ((line == rect[1] - 2) || (line == rect[3] + 2))
  {
    int offset = rect[0] - 2;
    if (offset < 0)
      offset = 0;
    int distance = rect[2] - rect[0] + 4;
    if (distance > 96)
      distance = 96;
    memset(lineBuffer + offset * 2, 0x00, distance * 2);
  }
}

Portrait portrait(portraitLoc);

DialogContext dialogContext;
Dialog choice(bottomRow);
MessageBox nameMessage(nameRow);
MessageBox bottomMessage(bottomRow);

uint8_t Dialog::process()
{
  uint8_t btn = checkButton(TAButton1 | TAButton2);
  uint8_t joyDir = checkJoystick(TAJoystickUp | TAJoystickDown | TAJoystickLeft | TAJoystickRight);
  if (joystickCoolDown > 0)
  {
    joystickCoolDown--;
  }
  else
  {
    int rows = count / column;
    int xWhere = where % column;
    int yWhere = where / column;
    if (joyDir & TAJoystickUp)
    {
      if (yWhere > 0)
        yWhere--;
      joystickCoolDown = joystickCoolDownStart;
    }
    if (joyDir & TAJoystickDown)
    {
      if (yWhere < rows - 1)
        yWhere++;
      joystickCoolDown = joystickCoolDownStart;
    }
    if (joyDir & TAJoystickLeft)
    {
      if (xWhere > 0)
        xWhere--;
      joystickCoolDown = joystickCoolDownStart;
    }
    if (joyDir & TAJoystickRight)
    {
      if (xWhere < column - 1)
        xWhere++;
      joystickCoolDown = joystickCoolDownStart;
    }
    where = xWhere + yWhere * column;
    rows = (rect[3] - rect[1] + 1) / 8;
    if (where < top)
    {
      top -= column;
    }
    if (where >= top + (rows * column))
    {
      top += column;
    }
  }
  if (buttonCoolDown > 0)
  {
    buttonCoolDown--;
  }
  else if (btn)
  {
    buttonCoolDown = BUTTON_COOLDOWN;
    if (btn == TAButton1)
    {
      return where;
    }
    else if (btn == TAButton2)
    {
      return CHOICE_BACK;
    }
  }
  return CHOICE_NONE;
}

void Dialog::draw(int line, uint8_t lineBuffer[96 * 2])
{
  int rows = (rect[3] - rect[1] + 1) / 8;
  if ((line >= rect[1]) && (line <= rect[3]))
  {
    int colOffset = (rect[2] - rect[0] + 1) / column;
    {
      int offset = rect[0] - 2;
      if (offset < 0)
        offset = 0;
      int distance = rect[2] - rect[0] + 5;
      if (distance > 96)
        distance = 96;
      memset(lineBuffer + offset * 2, 0x00, distance * 2);
    }
    memset(lineBuffer + (rect[0] - 1) * 2, 0xFF, 2);
    memset(lineBuffer + (rect[2] + 1) * 2, 0xFF, 2);
    int yOffset = (line - rect[1]) % 8;
    int yWhere = (line - rect[1]) / 8;
    if (yOffset != 7)
    {
      for (int col = 0; col < column; col++)
      {
        if (where == top + col + (yWhere * column))
        {
          if (yOffset == 4)
            memset(lineBuffer + (rect[0] + (colOffset * col) + 2) * 2, 0xFF, 3 * 2);
          else if ((yOffset == 3) || (yOffset == 5))
            memset(lineBuffer + (rect[0] + (colOffset * col) + 2) * 2, 0xFF, 2 * 2);
          else if ((yOffset == 2) || (yOffset == 6))
            memset(lineBuffer + (rect[0] + (colOffset * col) + 2) * 2, 0xFF, 1 * 2);
        }
        if (yWhere * column + col < count)
        {
          for (int i = 0; (option[yWhere * column + col][i] != 0) && (12 + (i * 6) < colOffset); i++)
          {
            const uint8_t *fontData = world.getFontData(option[yWhere * column + col][i], yOffset);
            memcpy(lineBuffer + ((rect[0] + (colOffset * col) + 6 + (i * 6)) * 2), fontData, 6 * 2);
          }
        }
      }
    }
  }
  else if ((line == rect[1] - 1) || (line == rect[3] + 1))
  {
    memset(lineBuffer + (rect[0] - 1) * 2, 0xFF, (rect[2] - rect[0] + 3) * 2);
    if (rect[0] - 2 >= 0)
      memset(lineBuffer + (rect[0] - 2) * 2, 0x00, 2);
    if (rect[2] + 2 >= 0)
      memset(lineBuffer + (rect[2] + 2) * 2, 0x00, 2);
  }
  else if ((line == rect[1] - 2) || (line == rect[3] + 2))
  {
    int offset = rect[0] - 2;
    if (offset < 0)
      offset = 0;
    int distance = rect[2] - rect[0] + 5;
    if (distance > 96)
      distance = 96;
    memset(lineBuffer + offset * 2, 0x00, distance * 2);
  }
}

void Dialog::setOptions(uint8_t col, uint8_t c, const char **o)
{
  top = 0;
  where = 0;
  column = col;
  count = c;
  option = o;
}

void World::init()
{
  buildCollision();
  for (int i = 0; i < currentArea->countNPC; i++)
  {
    npc[i].x = currentArea->npc[i].startX;
    npc[i].y = currentArea->npc[i].startY;
    npc[i].dir = DIRECTION_NONE;
    collision[currentArea->xSize * (npc[i].y / 8) + (npc[i].x / 8)] = COLLISION_NPC;
  }
}

void World::buildCollision()
{
  for (int i = 0; i < currentArea->xSize * currentArea->ySize; i++)
  {
    if (currentArea->data[i] == 0)
     collision[i] = 0;
    else if (currentArea->data[i] == 255)
     collision[i] = 1;
    else
     collision[i] = tilesetCollision[currentArea->data[i] - 1];
  }
}

void World::update()
{
  turn++;
  updateNPCs();
  if (state == STATE_TALKING)
  {
    if (dialogContext.choose)
    {
      uint8_t c = choice.process();
      if (c != CHOICE_NONE)
      {
        if (!dialogContext.run(c))
        {
          state = STATE_WORLD;
          buttonCoolDown = BUTTON_COOLDOWN;
          for (int i = 0; i < currentArea->countNPC; i++)
          {
            if ((npc[i].dir & DIRECTION_PAUSE) == DIRECTION_PAUSE)
            {
              npc[i].dir = npc[i].dir & (~DIRECTION_PAUSE);
            }
          }
        }
      }
    }
    else
    {
      uint8_t btn = checkButton(TAButton1 | TAButton2);
      uint8_t joyDir = checkJoystick(TAJoystickUp | TAJoystickDown | TAJoystickLeft | TAJoystickRight);
      if (buttonCoolDown > 0)
      {
        buttonCoolDown--;
      }
      else
      {
        if (btn == TAButton1)
        {
          buttonCoolDown = BUTTON_COOLDOWN;
          if (!dialogContext.run())
          {
            state = STATE_WORLD;
            for (int i = 0; i < currentArea->countNPC; i++)
            {
              if ((npc[i].dir & DIRECTION_PAUSE) == DIRECTION_PAUSE)
              {
                npc[i].dir = npc[i].dir & (~DIRECTION_PAUSE);
              }
            }
          }
        }
      }
    }
  }
  else
  {
    uint8_t btn = checkButton(TAButton1 | TAButton2);
    uint8_t joyDir = checkJoystick(TAJoystickUp | TAJoystickDown | TAJoystickLeft | TAJoystickRight);
    if (joystickCoolDown > 0)
    {
      joystickCoolDown--;
    }
    else
    {
      int origX = pc.x;
      int origY = pc.y;
      if ((joyDir & TAJoystickUp) && (pc.y > 0))
      {
        pc.dir = DIRECTION_UP;
        if ((pc.y % 8) == 0)
        {
          int yWhere = pc.y / 8;
          int xWhere = pc.x / 8;
          int xMod = pc.x % 8;
          bool col1 = getCollision(xWhere, yWhere - 1);
          if (xMod == 0)
          {
            if (col1 == 0)
              pc.y--;
          }
          else
          {
            bool col2 = getCollision(xWhere + 1, yWhere - 1);
            if ((col1 == 0) && (col2 == 0))
              pc.y--;
            else if ((col1 == 0) && (col2 != 0) && (xMod == 1))
            {
              pc.y--;
              pc.x--;
            }
            else if ((col1 != 0) && (col2 == 0) && (xMod == 7))
            {
              pc.y--;
              pc.x++;
            }
          }
        }
        else
          pc.y--;
      }
      else if ((joyDir & TAJoystickDown) && (pc.y < (currentArea->ySize - 1) * 8))
      {
        pc.dir = DIRECTION_DOWN;
        if ((pc.y % 8) == 0)
        {
          int yWhere = pc.y / 8;
          int xWhere = pc.x / 8;
          int xMod = pc.x % 8;
          bool col1 = getCollision(xWhere, yWhere + 1);
          if (xMod == 0)
          {
            if (col1 == 0)
              pc.y++;
          }
          else
          {
            bool col2 = getCollision(xWhere + 1, yWhere + 1);
            if ((col1 == 0) && (col2 == 0))
              pc.y++;
            else if ((col1 == 0) && (col2 != 0) && (xMod == 1))
            {
              pc.y++;
              pc.x--;
            }
            else if ((col1 != 0) && (col2 == 0) && (xMod == 7))
            {
              pc.y++;
              pc.x++;
            }
          }
        }
        else
          pc.y++;
      }
      if ((joyDir & TAJoystickLeft) && (pc.x > 0))
      {
        pc.dir = DIRECTION_LEFT;
        if ((pc.x % 8) == 0)
        {
          int yWhere = pc.y / 8;
          int xWhere = pc.x / 8;
          int yMod = pc.y % 8;
          bool col1 = getCollision(xWhere - 1, yWhere);
          if (yMod == 0)
          {
            if (col1 == 0)
              pc.x--;
          }
          else
          {
            bool col2 = getCollision(xWhere - 1, yWhere + 1);
            if ((col1 == 0) && (col2 == 0))
              pc.x--;
            else if ((col1 == 0) && (col2 != 0) && (yMod == 1))
            {
              pc.y--;
              pc.x--;
            }
            else if ((col1 != 0) && (col2 == 0) && (yMod == 7))
            {
              pc.y++;
              pc.x--;
            }
          }
        }
        else
          pc.x--;
      }
      else if ((joyDir & TAJoystickRight) && (pc.x < (currentArea->xSize - 1) * 8))
      {
        pc.dir = DIRECTION_RIGHT;
        if ((pc.x % 8) == 0)
        {
          int yWhere = pc.y / 8;
          int xWhere = pc.x / 8;
          int yMod = pc.y % 8;
          bool col1 = getCollision(xWhere + 1, yWhere);
          if (yMod == 0)
          {
            if (col1 == 0)
              pc.x++;
          }
          else
          {
            bool col2 = getCollision(xWhere + 1, yWhere + 1);
            if ((col1 == 0) && (col2 == 0))
              pc.x++;
            else if ((col1 == 0) && (col2 != 0) && (yMod == 1))
            {
              pc.y--;
              pc.x++;
            }
            else if ((col1 != 0) && (col2 == 0) && (yMod == 7))
            {
              pc.y++;
              pc.x++;
            }
          }
        }
        else
          pc.x++;
      }
      if ((origX != pc.x) || (origY != pc.y))
      {
        for (int i = 0; i < currentArea->countPortal; i++)
        {
          if ((pc.x == currentArea->portals[i].locX) && (pc.y == currentArea->portals[i].locY))
          {
            pc.x = currentArea->portals[i].startX;
            pc.y = currentArea->portals[i].startY;
            currentArea = areaList[currentArea->portals[i].area];
            init();
            break;
          }
        }
      }
    }
    if (buttonCoolDown > 0)
    {
      buttonCoolDown--;
    }
    else
    {
      if (btn == TAButton2)
      {
        state = STATE_BATTLECHOICE;
        buttonCoolDown = BUTTON_COOLDOWN;
        activeOpponent->init(0, 2, (const ColorRule *)jessCateyeRule);
        activeOpponent->calculateColor();
        battle.init();
      }
      else if (btn == TAButton1)
      {
        int yWhere = pc.y / 8;
        int xWhere = pc.x / 8;
        int yMod = pc.y % 8;
        int xMod = pc.x % 8;
        uint8_t col = 0;
        buttonCoolDown = BUTTON_COOLDOWN;
        switch (pc.dir)
        {
          case DIRECTION_UP:
            if ((yMod < 3) && (yWhere > 0))
            {
              col = getCollision(xWhere, yWhere - 1);
              if (col > 1)
                yWhere--;
              else if (xMod > 0)
              {
                col = getCollision(xWhere + 1, yWhere - 1);
                if (col > 1)
                {
                  xWhere++;
                  yWhere--;
                }
              }
            }
            break;
          case DIRECTION_DOWN:
            if (yMod > 5)
            {
              yMod = 0;
              yWhere++;
            }
            if ((yMod == 0) && (yWhere + 1 < currentArea->ySize))
            {
              col = getCollision(xWhere, yWhere + 1);
              if (col > 1)
                yWhere++;
              else if (xMod > 0)
              {
                col = getCollision(xWhere + 1, yWhere + 1);
                if (col > 1)
                {
                  xWhere++;
                  yWhere++;
                }
              }
            }
            break;
          case DIRECTION_LEFT:
            if ((xMod < 3) && (xWhere > 0))
            {
              col = getCollision(xWhere - 1, yWhere);
              if (col > 1)
                xWhere--;
              else if (yMod > 0)
              {
                col = getCollision(xWhere - 1, yWhere + 1);
                if (col > 1)
                {
                  xWhere--;
                  yWhere++;
                }
              }
            }
            break;
          case DIRECTION_RIGHT:
            if (xMod > 5)
            {
              xMod = 0;
              xWhere++;
            }
            if ((xMod == 0) && (xWhere < currentArea->xSize - 1))
            {
              col = getCollision(xWhere + 1, yWhere);
              if (col > 1)
                xWhere++;
              else if (xMod > 0)
              {
                col = getCollision(xWhere + 1, yWhere + 1);
                if (col > 1)
                {
                  xWhere++;
                  yWhere++;
                }
              }
            }
            break;
          default:
            break;
        }
        switch (col)
        {
          case COLLISION_NPC:
          {
            int who = findNPC(xWhere, yWhere);
            if (who != 255)
            {
              npc[who].dir |= DIRECTION_PAUSE;
              dialogContext.init(currentArea->npc[who].dialog);
              if (dialogContext.run())
              {
                state = STATE_TALKING;
                nameRow[2] = nameRow[0] + strlen(currentArea->npc[who].name) * 6;
                for (int i = 0; i < currentArea->npc[who].monsterCount; i++)
                {
                  opponent[i].init(currentArea->npc[who].monsters[i]);
                  opponent[i].calculateColor();
                }
                activeOpponent = &opponent[0];
                nameMessage.setText(currentArea->npc[who].name);
                portrait.setPortrait(currentArea->npc[who].portrait);
              }
            }
            else
              printf("Error: Should not be here\n");
            break;
          }
          default:
            break;
        }
      }
    }
  }
}

void World::updateNPCs()
{
  for (int i = 0; i < currentArea->countNPC; i++)
  {
    if (npc[i].dir != DIRECTION_NONE)
    {
      switch (npc[i].dir)
      {
        case DIRECTION_UP:
          npc[i].y--;
          if (npc[i].y % 8 == 0)
          {
            npc[i].dir = DIRECTION_NONE;
            collision[currentArea->xSize * ((npc[i].y + 8) / 8) + (npc[i].x / 8)] = 0;
          }
          break;
        case DIRECTION_DOWN:
          npc[i].y++;
          if (npc[i].y % 8 == 0)
          {
            npc[i].dir = DIRECTION_NONE;
            collision[currentArea->xSize * ((npc[i].y - 8) / 8) + (npc[i].x / 8)] = 0;
          }
          break;
        case DIRECTION_LEFT:
          npc[i].x--;
          if (npc[i].x % 8 == 0)
          {
            npc[i].dir = DIRECTION_NONE;
            collision[currentArea->xSize * (npc[i].y / 8) + ((npc[i].x + 8) / 8)] = 0;
          }
          break;
        case DIRECTION_RIGHT:
          npc[i].x++;
          if (npc[i].x % 8 == 0)
          {
            npc[i].dir = DIRECTION_NONE;
            collision[currentArea->xSize * (npc[i].y / 8) + ((npc[i].x - 8) / 8)] = 0;
          }
          break;
        default:
          break;
      }
    }
    else if (turn % 24 == 0)
    {
      if (random(0, 2) == 1)
      {
        if (npc[i].y > currentArea->npc[i].startY + NPC_DISTANCE)
          npc[i].dir = DIRECTION_UP;
        else if ((npc[i].y < currentArea->npc[i].startY - NPC_DISTANCE) || (random(0, 2) == 1))
          npc[i].dir = DIRECTION_DOWN;
        else
          npc[i].dir = DIRECTION_UP;
      }
      else
      {
        if (npc[i].x > currentArea->npc[i].startX + NPC_DISTANCE)
          npc[i].dir = DIRECTION_LEFT;
        else if ((npc[i].x < currentArea->npc[i].startX - NPC_DISTANCE) || (random(0, 2) == 1))
          npc[i].dir = DIRECTION_RIGHT;
        else
          npc[i].dir = DIRECTION_LEFT;
      }
      switch (npc[i].dir)
      {
        case DIRECTION_UP:
        {
          if ((npc[i].y == 0) || (collision[currentArea->xSize * ((npc[i].y - 8) / 8) + (npc[i].x / 8)] != 0) || (isTrainerIn(npc[i].x / 8, (npc[i].y - 8) / 8)))
            npc[i].dir = DIRECTION_NONE;
          else
          {
            collision[currentArea->xSize * ((npc[i].y - 8) / 8) + (npc[i].x / 8)] = COLLISION_NPC;
            npc[i].y--;
          }
          break;
        }
        case DIRECTION_DOWN:
          if ((npc[i].y == (currentArea->ySize - 1) * 8) || (collision[currentArea->xSize * ((npc[i].y + 8) / 8) + (npc[i].x / 8)] != 0) || (isTrainerIn(npc[i].x / 8, (npc[i].y + 8) / 8)))
            npc[i].dir = DIRECTION_NONE;
          else
          {
            collision[currentArea->xSize * ((npc[i].y + 8) / 8) + (npc[i].x / 8)] = COLLISION_NPC;
            npc[i].y++;
          }
          break;
        case DIRECTION_LEFT:
          if ((npc[i].x == 0) || (collision[currentArea->xSize * (npc[i].y / 8) + ((npc[i].x - 8) / 8)] != 0) || (isTrainerIn((npc[i].x - 8) / 8, npc[i].y / 8)))
            npc[i].dir = DIRECTION_NONE;
          else
          {
            collision[currentArea->xSize * (npc[i].y / 8) + ((npc[i].x - 8) / 8)] = COLLISION_NPC;
            npc[i].x--;
          }
          break;
        case DIRECTION_RIGHT:
          if ((npc[i].x == (currentArea->xSize - 1) * 8) || (collision[currentArea->xSize * (npc[i].y / 8) + ((npc[i].x + 8) / 8)] != 0) || (isTrainerIn((npc[i].x + 8) / 8, npc[i].y / 8)))
            npc[i].dir = DIRECTION_NONE;
          else
          {
            collision[currentArea->xSize * (npc[i].y / 8) + ((npc[i].x + 8) / 8)] = COLLISION_NPC;
            npc[i].x++;
          }
          break;
      }
    }
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
    if ((currentY < 0) || (currentY >= (currentArea->ySize * 8)))
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
          else if (currentTile == 255)
            memset(lineBuffer + x * 2, 0, 8 * 2);
          else
            memcpy(lineBuffer + x * 2, getTileData(currentTile, currentY % 8) + (init * 2), (8 - init) * 2);
          x += 8 - init;
        }
      }
      while (x <= 88)
      {
        if (startX + x > ((currentArea->xSize - 1) * 8))
        {
          memset(lineBuffer + x * 2, 0, (96 - x) * 2);
          x = 96;
          break;
        }
        currentTile = getTile(startX + x, currentY);
        if (currentTile == 0)
          memset(lineBuffer + x * 2, 255, 8 * 2);
        else if (currentTile == 255)
          memset(lineBuffer + x * 2, 0, 8 * 2);
        else
          memcpy(lineBuffer + x * 2, getTileData(currentTile, currentY % 8), 8 * 2);
        x += 8;
      }
      if (x != 96)
      {
        int init = 96 - x;
        if (startX + x > ((currentArea->xSize - 1) * 8))
        {
          memset(lineBuffer + x * 2, 0, init * 2);
          x = 96;
        }
        else
        {
          currentTile = getTile(startX + x, currentY);
          if (currentTile == 0)
            memset(lineBuffer + x * 2, 255, init * 2);
        else if (currentTile == 255)
          memset(lineBuffer + x * 2, 0, 8 * 2);
          else
            memcpy(lineBuffer + x * 2, getTileData(currentTile, currentY % 8), init * 2);
        }
      }
      for (int n = 0; n < currentArea->countNPC; n++)
      {
        if ((currentY >= npc[n].y) && (currentY < npc[n].y + 8))
        {
          const uint8_t *data = getTileData(currentArea->npc[n].icon, currentY - npc[n].y);
          for (int i = 0; i < 8; i++)
          {
            if (((data[i * 2] != 255) || (data[i * 2 + 1] != 255)) && (startX <= npc[n].x + i) && (startX + 96 > npc[n].x + i))
            {
              lineBuffer[(npc[n].x + i - startX) * 2] = data[i * 2];
              lineBuffer[(npc[n].x + i - startX) * 2 + 1] = data[i * 2 + 1];
            }
          }
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
    if (state == STATE_TALKING)
    {
      portrait.draw(lines, lineBuffer);
      nameMessage.draw(lines, lineBuffer);
      if (dialogContext.message)
        bottomMessage.draw(lines, lineBuffer);
      if (dialogContext.choose)
        choice.draw(lines, lineBuffer);
    }
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

uint8_t World::findNPC(int xWhere, int yWhere)
{
  for (int i = 0; i < currentArea->countNPC; i++)
  {
    int npcXWhere = npc[i].x / 8;
    int npcYWhere = npc[i].y / 8;
    if ((xWhere == npcXWhere) && (yWhere == npcYWhere))
    {
      return i;
    }
    if (npc[i].x % 8 != 0)
    {
      if ((xWhere == npcXWhere + 1) && (yWhere == npcYWhere))
        return i;
    }
    else if (npc[i].y % 8 != 0)
    {
      if ((xWhere == npcXWhere) && (yWhere == npcYWhere + 1))
        return i;
    }
  }
  return 255;
}

uint8_t World::getCollision(int xWhere, int yWhere)
{
  return collision[xWhere + (yWhere * currentArea->xSize)];
}

uint8_t World::getTile(int x, int y)
{
  return currentArea->data[x / 8 + ((y / 8) * currentArea->xSize)];
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

bool World::isTrainerIn(int xWhere, int yWhere)
{
  int pcXWhere = pc.x / 8;
  int pcYWhere = pc.y / 8;
  int modX = pc.x % 8;
  int modY = pc.y % 8;
  if ((xWhere == pcXWhere) && (yWhere == pcYWhere))
    return true;
  else if ((modX) && (xWhere == pcXWhere + 1) && (yWhere == pcYWhere))
    return true;
  else if ((modY) && (xWhere == pcXWhere) && (yWhere == pcYWhere + 1))
    return true;
  else if ((modX) && (modY) && (xWhere == pcXWhere + 1) && (yWhere == pcYWhere + 1))
    return true;
  return false;
}

void Battle::init()
{
  choice.setRect(bottomRow);
  choice.setOptions(2, 4, baseOption);
  action[0].base = 0;
  action[0].subaction = 0;
  message = false;
  initiative[0] = false;
  initiative[1] = false;
}

void Battle::update()
{
  if (state == STATE_BATTLECHOICE)
  {
    uint8_t c = choice.process();
    if (c != CHOICE_NONE)
    {
      if (action[0].base)
      {
        if (c == CHOICE_BACK)
        {
          choice.setOptions(2, 4, baseOption);
          action[0].base = 0;
        }
        else
        {
          if ((action[0].base == BASEOPTION_ATTACK) && (action[0].subaction == 0))
          {
            action[0].subaction = c + 1;
            message = false;
            initiative[0] = true;
            initiative[1] = true;
            state = STATE_BATTLE;
          }
        }
      }
      else if (c != CHOICE_BACK)
      {
        c++;
        if (c == BASEOPTION_RUN)
        {
          state = STATE_WORLD;
        }
        else if (c == BASEOPTION_ATTACK)
        {
          action[0].base = c;
          active->buildChoice(choiceEnd, choiceList, choiceString);
          choice.setOptions(2, choiceEnd, (const char **)choiceList);
        }
        else if (c == BASEOPTION_ITEM)
        {
          bottomMessage.setText(noItems);
          message = true;
          state = STATE_BATTLE;
        }
        else if (c == BASEOPTION_SWAP)
        {
          bottomMessage.setText(noSwap);
          message = true;
          state = STATE_BATTLE;
        }
      }
    }
  }
  else if (state == STATE_BATTLE)
  {
    if (message)
    {
      uint8_t btn = checkButton(TAButton1 | TAButton2);
      if (buttonCoolDown > 0)
      {
        buttonCoolDown--;
      }
      else
      {
        if ((btn == TAButton1) || (btn == TAButton2))
        {
          if ((!initiative[0]) && (!initiative[1]))
          {
            if (active->hp == 0)
            {
              for (int i = 0; i < MONSTER_PARTYSIZE; i++)
              {
                if (party[i].hp > 0)
                {
                }
              }
            }
            if (active->hp == 0)
            {
              state = STATE_BATTLELOST;
              bottomMessage.setText("You have lost the battle.");
            }
            if (activeOpponent->hp == 0)
            {
              for (int i = 0; i < MONSTER_PARTYSIZE; i++)
              {
                if (opponent[i].hp > 0)
                {
                  activeOpponent = &opponent[i];
                  break;
                }
              }
            }
            if ((state == STATE_BATTLE) && (activeOpponent->hp == 0))
            {
              state = STATE_BATTLEWON;
              bottomMessage.setText("You have won the battle.");
            }
            if (state == STATE_BATTLE)
              state = STATE_BATTLECHOICE;
            init();
          }
          else
            message = false;
          buttonCoolDown = BUTTON_COOLDOWN;
        }
      }
    }
    else
    {
      int choose = 0;
      if ((initiative[0]) && (initiative[1]))
      {
        chooseAction();
        if (random(0, 2) == 1)
          choose = 1;
      }
      else if (initiative[1])
      {
        choose = 1;
      }
      initiative[choose] = false;
      runAction(choose);
    }
  }
  else if ((state == STATE_BATTLEWON) || (state == STATE_BATTLELOST))
  {
    uint8_t btn = checkButton(TAButton1 | TAButton2);
    if (buttonCoolDown > 0)
    {
      buttonCoolDown--;
    }
    else
    {
      if ((btn == TAButton1) || (btn == TAButton2))
      {
        state = prevState;
        prevState = STATE_WORLD;
        if (state == STATE_TALKING)
        {
          if (!dialogContext.run())
          {
            state = STATE_WORLD;
            for (int i = 0; i < world.currentArea->countNPC; i++)
            {
              if ((world.npc[i].dir & DIRECTION_PAUSE) == DIRECTION_PAUSE)
              {
                world.npc[i].dir = world.npc[i].dir & (~DIRECTION_PAUSE);
              }
            }
          }
        }
        buttonCoolDown = BUTTON_COOLDOWN;
      }
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
    for (int i = 0; i < 48; i++)
      memcpy(lineBuffer + ((48 + i) *2), activeOpponent->img + (((lines * 48) + (47 - i)) * 2), 2);
    if (state == STATE_BATTLECHOICE)
    {
      choice.draw(lines, lineBuffer);
    }
    else if ((state == STATE_BATTLE) || (state == STATE_BATTLEWON) || (state == STATE_BATTLELOST))
    {
      bottomMessage.draw(lines, lineBuffer);
    }
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

void Battle::chooseAction()
{
  // Always base attack. AI will be added later.
  action[1].base = BASEOPTION_ATTACK;
  action[1].subaction = 1;
}

void Battle::runAction(int a)
{
  ColorMonster *mon[2];
  message = true;
  if (action[a].base == BASEOPTION_ATTACK)
  {
    if (a == 0)
    {
      mon[0] = active;
      mon[1] = activeOpponent;
    }
    else
    {
      mon[1] = active;
      mon[0] = activeOpponent;
    }
    int damage = random(1, mon[0]->power[action[a].subaction - 1].strength + 1);
    sprintf(choiceString, "%s hits for %d damage.", monsterType[mon[0]->baseMonster].power[mon[0]->power[action[a].subaction - 1].power].name, damage);
    if (mon[1]->hp <= damage)
    {
      mon[1]->hp = 0;
      initiative[1 - a] = false;
    }
    else
    {
      mon[1]->hp -= damage;
    }
    bottomMessage.setText(choiceString);
  }
}

const char *Battle::baseOption[4] = { "Attack", "Item", "Swap", "Run" };

class Title
{
  public:
    Title() {}
    void init();
    void update();
    void draw();

    static const char *baseOption[2];
};

void Title::init()
{
  choice.setRect(startMenuRow);
  choice.setOptions(1, 2, baseOption);
  opponent[0].initRandom();
}

void Title::update()
{
  turn++;
  if (turn > 100)
  {
    turn = 0;
    opponent[0].initRandom();
  }
  uint8_t c = choice.process();
  if (c != CHOICE_NONE)
  {
    switch (c)
    {
      case 0:
      case 1:
        state = STATE_PAINT;
        buttonCoolDown = BUTTON_COOLDOWN;
        break;
    }
  }
}

void Title::draw()
{
  display.goTo(0,0);
  display.startData();

  uint8_t lineBuffer[96 * 2];

  for(int lines = 0; lines < 64; ++lines)
  {
    memcpy(lineBuffer, _image_title_data + (lines * 48 * 2), 48 * 2);
    memcpy(lineBuffer + 48 * 2, opponent[0].img + (lines * 48 * 2), 48 * 2);
    choice.draw(lines, lineBuffer);
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

const char *Title::baseOption[2] = { "Continue", "New Game" };

Title title;

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
  randomSeed(analogRead(0));
  active->init(0);
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
  title.init();
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (state == STATE_TITLE)
  {
    title.update();
  }
  else if ((state == STATE_PAINT) || (state == STATE_PAINTZOOM))
  {
    paint.update();
  }
  else if ((state == STATE_WORLD) || (state == STATE_TALKING))
  {
    world.update();
  }
  else if ((state == STATE_BATTLECHOICE) || (state == STATE_BATTLE) || (state == STATE_BATTLEWON) || (state == STATE_BATTLELOST))
  {
    battle.update();
  }
  if (state == STATE_TITLE)
  {
    title.draw();
  }
  else if ((state == STATE_PAINT) || (state == STATE_PAINTZOOM))
  {
    paint.draw();
  }
  else if ((state == STATE_WORLD) || (state == STATE_TALKING))
  {
    world.draw();
  }
  else if ((state == STATE_BATTLECHOICE) || (state == STATE_BATTLE) || (state == STATE_BATTLEWON) || (state == STATE_BATTLELOST))
  {
    battle.draw();
  }
  unsigned long oldTime = lastTime;
  lastTime = millis();
  if ((lastTime > oldTime) && (lastTime - oldTime < 70))
  {
    delay(70 - (lastTime - oldTime));
  }
}

