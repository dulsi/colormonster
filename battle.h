class Battle
{
  public:
    Battle() { }
    void init();
    void update();
    void draw();
    void chooseAction();
    void runAction(int a);

    struct {
      int base : 3;
      int subaction : 5;
    } action[2];
    uint8_t choiceEnd;
    char *choiceList[10];
    char choiceString[500];
    bool message;
    bool initiative[2];
    static const char *baseOption[4];
};

extern Battle battle;

