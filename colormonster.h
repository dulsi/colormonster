#define POWER_NONE 255

#define MONSTER_PARTYSIZE 1

class ColorRule
{
  public:
    uint8_t instruct;
    unsigned char origColor;
    uint8_t color[2];
};

class NPCMonster
{
  public:
    uint8_t baseMonster;
    uint8_t ruleCount;
    const ColorRule *rules;
};

class ColorMonsterPower
{
  public:
    ColorMonsterPower() : power(POWER_NONE) {}

    uint8_t power;
    int color;
    uint8_t strength;
};

class ColorMonster
{
  public:
    ColorMonster() : baseMonster(255), saved(false) { memset(img, 0, 64*48); }
    void init(uint8_t bm);
    void init(const NPCMonster m);
    void init(uint8_t bm, int count, const ColorRule *r);
    void initRandom();
    void buildChoice(uint8_t &choiceEnd, char **choiceList, char *choiceString);
    void calculateColor();
    void draw(int line, uint8_t *lineBuffer, bool reverse);
    void drawZoom(int line, uint8_t *lineBuffer, uint8_t zoomx, uint8_t zoomy);

    uint8_t baseMonster;
    unsigned char img[64*48];
    int hp, maxHp;
    ColorMonsterPower power[5];
    bool saved;
};

extern ColorMonster party[MONSTER_PARTYSIZE];

