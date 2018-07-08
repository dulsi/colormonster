#include <TinyScreen.h>
#include "SdFat.h"
#include <SPI.h>
#include <Wire.h>
#include "TinyConfig.h"
#include "monsters.h"
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
#define STATE_BATTLE 4

#define BUTTON_COOLDOWN 5
#define JOYSTICK_COOLDOWNSTART 4

#define DIRECTION_UP 1
#define DIRECTION_DOWN 2
#define DIRECTION_LEFT 3
#define DIRECTION_RIGHT 4

#define COLLISION_NPC 2

#define CHOICE_NONE 255
#define CHOICE_BACK 254

#define BASEOPTION_ATTACK 1
#define BASEOPTION_ITEM 2
#define BASEOPTION_SWAP 3
#define BASEOPTION_RUN 4

int state = 0;
int buttonCoolDown = 0;
int joystickCoolDown = 0;
int joystickCoolDownStart = JOYSTICK_COOLDOWNSTART;

#define PART_NONE 255

const uint8_t bottomRow[] = { 1, 48, 94, 63};

const char noItems[] = "You have no items.";
const char noSwap[] = "You have no more monsters.";

class ColorMonsterPart
{
  public:
    ColorMonsterPart() : part(PART_NONE) {}

    uint8_t part;
    int color;
    uint8_t strength;
    uint8_t health;
};

class ColorMonster
{
  public:
    ColorMonster() : baseMonster(255), saved(false) { memset(img, 0, 64*48*2); }
    void init(uint8_t bm);
    void buildChoice(bool target, uint8_t &choiceEnd, char **choiceList, char *choiceString);
    void calculateColor();

    uint8_t baseMonster;
    unsigned char img[64*48*2];
    ColorMonsterPart part[5];
    bool saved;
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
    part[i].part = i;
    part[i].strength = part[i].health = monsterType[baseMonster].part[i].strength;
  }
}

void ColorMonster::buildChoice(bool target, uint8_t &choiceEnd, char **choiceList, char *choiceString)
{
  choiceEnd = 0;
  int where = 0;
  for (int i = 0; i < 5; i++)
  {
    if (part[i].part == PART_NONE)
      break;
    const ColorMonsterPartType &partType = monsterType[baseMonster].part[part[i].part];
    if ((target) || (partType.type == PARTTYPE_CORE) || (partType.type == PARTTYPE_WEAPON))
    {
      choiceList[choiceEnd] = choiceString + where;
      if ((i == 0) && (target))
      {
        strcpy(choiceString + where, "Body");
        where += 5;
      }
      else
      {
        strcpy(choiceString + where, partType.name);
        where += strlen(partType.name) + 1;
      }
      choiceEnd++;
    }
  }
}

void ColorMonster::calculateColor()
{
  for (int i = 0; i < 5; i++)
  {
    if (part[i].part == PART_NONE)
      break;
    uint8_t region = monsterType[baseMonster].part[part[i].part].region;
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
    part[i].color = colors[max][0];
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
    uint8_t dir;
};

Trainer pc;

class NPC
{
  public:
    const char *name;
    uint8_t icon;
    int startX,startY;
};

class Area
{
  public:
    Area(uint8_t x, uint8_t y, const uint8_t *d, const uint8_t *c, uint8_t cNPC, const NPC *n) : xSize(x), ySize(y), data(d), collision(c), countNPC(cNPC), npc(n) {}

    uint8_t xSize, ySize;
    const uint8_t *data;
    const uint8_t *collision;
    uint8_t countNPC;
    const NPC *npc;
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
const uint8_t startTownCollision[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
  0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
  0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
  0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
  0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1,
  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
  0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0,
  0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 0,
  0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
const NPC startTownNPC[] = { { "Jessica", 27, 80, 40 } };
const Area startTown(24, 16, startTownMap, startTownCollision, 1, startTownNPC);

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
    void update();
    void draw();
    uint8_t getCollision(int xWhere, int yWhere);
    uint8_t getTile(int x, int y);
    const uint8_t *getTileData(int tile, int y);
    const uint8_t *getFontData(char c, int y);

    const Area *currentArea;
    uint8_t collision[800];
    NPCInstance npc[30];
};

World world(&startTown);

class MessageBox
{
  public:
    MessageBox(const uint8_t *r) : rect(r) {  }

    void setText(const char *t);
    void draw(int line, uint8_t lineBuffer[96 * 2]);

  private:
    const uint8_t *rect;
    char text[7][16];
};

void MessageBox::setText(const char *t)
{
  const char *where = t;
  int indx = 0;
  int len = strlen(t);
  int letters = (rect[2] - rect[0]) / 6;
  for (int i = 0; i < 7; i++)
    text[i][0] = 0;
  while (*where)
  {
    strncpy(text[indx], where, letters);
    if (len < letters)
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

class Dialog
{
  public:
    Dialog(const uint8_t *r) : rect(r) {  }

    uint8_t process();
    void draw(int line, uint8_t lineBuffer[96 * 2]);
    void setOptions(uint8_t col, uint8_t c, const char **o);

  private:
    const uint8_t *rect;
    uint8_t top;
    uint8_t where;
    uint8_t column;
    uint8_t count;
    const char **option;
};


Dialog choice(bottomRow);
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
  else
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
  column = col;
  count = c;
  option = o;
}

void World::init()
{
  memcpy(collision, currentArea->collision, currentArea->xSize * currentArea->ySize);
  for (int i = 0; i < currentArea->countNPC; i++)
  {
    npc[i].x = currentArea->npc[i].startX;
    npc[i].y = currentArea->npc[i].startY;
    npc[i].dir = 255;
    collision[currentArea->xSize * (npc[i].y / 8) + (npc[i].x / 8)] = COLLISION_NPC;
  }
}

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
          if ((yMod < 3) && (yWhere + 1 < currentArea->ySize))
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
          if ((xMod < 3) && (xWhere < currentArea->xSize - 1))
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
          break;
        default:
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
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
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

class Battle
{
  public:
    Battle() { init(); }
    void init();
    void update();
    void draw();
    void chooseAction();
    void runAction(int a);

    struct {
      int base : 3;
      int subaction : 5;
      int target : 5;
    } action[2];
    uint8_t choiceEnd;
    char *choiceList[10];
    char choiceString[500];
    bool message;
    bool initiative[2];
    static const char *baseOption[4];
};

void Battle::init()
{
  choice.setOptions(2, 4, baseOption);
  action[0].base = 0;
  action[0].subaction = 0;
  action[0].target = 0;
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
          if ((action[0].base == BASEOPTION_ATTACK) && (action[0].subaction != 0))
          {
            action[0].subaction = 0;
            active->buildChoice(false, choiceEnd, choiceList, choiceString);
            choice.setOptions(2, choiceEnd, (const char **)choiceList);
          }
          else
          {
            choice.setOptions(2, 4, baseOption);
            action[0].base = 0;
          }
        }
        else
        {
          if ((action[0].base == BASEOPTION_ATTACK) && (action[0].subaction == 0))
          {
            action[0].subaction = c + 1;
            activeOpponent->buildChoice(true, choiceEnd, choiceList, choiceString);
            choice.setOptions(2, choiceEnd, (const char **)choiceList);
          }
          else if ((action[0].base == BASEOPTION_ATTACK) && (action[0].subaction != 0))
          {
            action[0].target = c + 1;
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
          active->buildChoice(false, choiceEnd, choiceList, choiceString);
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
    else if (state == STATE_BATTLE)
    {
      bottomMessage.draw(lines, lineBuffer);
    }
    display.writeBuffer(lineBuffer,96 * 2);
  }
  display.endTransfer();
}

void Battle::chooseAction()
{
  // Always base attack and body target. AI will be added later.
  action[1].base = BASEOPTION_ATTACK;
  action[1].subaction = 1;
  action[1].target = 1;
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
    int damage = random(1, mon[0]->part[action[a].subaction - 1].strength + 1);
    sprintf(choiceString, "%s hits for %d damage.", monsterType[mon[0]->baseMonster].part[mon[0]->part[action[a].subaction - 1].part].name, damage);
    bottomMessage.setText(choiceString);
  }
}

const char *Battle::baseOption[4] = { "Attack", "Item", "Swap", "Run" };

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
  randomSeed(analogRead(0));
  active->init(0);
  activeOpponent->init(0);
  activeOpponent->calculateColor();
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
  else if ((state == STATE_BATTLECHOICE) || (state == STATE_BATTLE))
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

