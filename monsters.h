#ifndef MONSTERS_H
#define MONSTERS_H

#include "cateye.h"
#include "weavifly.h"

#define PARTTYPE_CORE 0
#define PARTTYPE_WEAPON 1
#define PARTTYPE_SHIELD 2

class ColorMonsterPartType
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
    ColorMonsterPartType part[5];
};

const ColorMonsterType monsterType[] =
{
  {"Cateye", _image_cateye_data, {{"Slam", 0x49, PARTTYPE_CORE, 6}, {"EyeZap", 0x6c, PARTTYPE_WEAPON, 6}, {"Kick", 0x0b, PARTTYPE_WEAPON, 8}, {"Tail", 0x10, PARTTYPE_WEAPON, 8}, {"Hearing", 0xe0, PARTTYPE_SHIELD, 4}}},
  {"Weavifly", _image_weavifly_data, {{"Slam", 0x0b, PARTTYPE_CORE, 8}, {"Bite", 0x3f, PARTTYPE_WEAPON, 4}, {"Wind", 0x10, PARTTYPE_WEAPON, 6}, {"Tail", 0xe2, PARTTYPE_WEAPON, 8}, {"Belly", 0xfd, PARTTYPE_SHIELD, 4}}}
};

#endif

