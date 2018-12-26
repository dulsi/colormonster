#define BATTLE_WON 1
#define BATTLE_LOST 2
#define BATTLE_RAN 3

class Battle
{
  public:
    Battle() : lastBattle(0) { }
    void init();
    void update();
    void draw();
    void chooseAction();
    void runAction(int a);

    struct {
      int base : 3;
      int subaction : 5;
      unsigned char c;
    } action[2];
    uint8_t choiceEnd;
    char *choiceList[10];
    char choiceString[500];
    bool message;
    bool initiative[2];
    uint8_t lastBattle;
    static const char *baseOption[4];
};

extern Battle battle;

