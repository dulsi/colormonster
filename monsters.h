#ifndef MONSTERS_H
#define MONSTERS_H

#include "cateye.h"
#include "weavifly.h"
#include "sharpfin.h"
#include "flacono.h"

#define POWERTYPE_MELEE 0
#define POWERTYPE_RANGED 1
#define POWERTYPE_AREA 2

#define COLMONSTERTYPE_COUNT 4

class ColorMonsterPowerType
{
  public:
    const char *name;
    uint8_t region;
    uint8_t type;
    uint8_t strength;
};

class ColorMonsterType
{
  public:
    const char *name;
    const unsigned char *img;
    int baseHp;
    ColorMonsterPowerType power[5];
};

const ColorMonsterType monsterType[] =
{
  {"Cateye", _image_cateye_data, 13, {{"Slam", 0x49, POWERTYPE_MELEE, 6}, {"EyeZap", 0x6c, POWERTYPE_RANGED, 6}, {"Kick", 0x0b, POWERTYPE_MELEE, 8}, {"Tail", 0x10, POWERTYPE_MELEE, 8}, {"Sonic", 0xe0, POWERTYPE_AREA, 4}}},
  {"Weavifly", _image_weavifly_data, 13, {{"Slam", 0x0b, POWERTYPE_MELEE, 8}, {"Bite", 0x3f, POWERTYPE_MELEE, 4}, {"Wind", 0x10, POWERTYPE_AREA, 6}, {"Sting", 0xe2, POWERTYPE_RANGED, 8}, {"Cocoon", 0xfd, POWERTYPE_MELEE, 4}}},
  {"Sharpfin", _image_sharpfin_data, 13, {{"Slam", 0x0b, POWERTYPE_MELEE, 8}, {"Bite", 0x3f, POWERTYPE_MELEE, 4}, {"Wind", 0x10, POWERTYPE_MELEE, 6}, {"Tail", 0xe2, POWERTYPE_MELEE, 8}, {"Belly", 0xfd, POWERTYPE_MELEE, 4}}},
  {"Flacono", _image_flacono_data, 13, {{"Slam", 0x0b, POWERTYPE_MELEE, 8}, {"Bite", 0x3f, POWERTYPE_MELEE, 4}, {"Wind", 0x10, POWERTYPE_MELEE, 6}, {"Tail", 0xe2, POWERTYPE_MELEE, 8}, {"Belly", 0xfd, POWERTYPE_MELEE, 4}}}
};

#endif

