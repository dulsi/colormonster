#define DIALOG_SAY 1
#define DIALOG_CHOOSE_YESNO 2
#define COMMAND_IF 3
#define COMMAND_JUMP 4
#define COMMAND_BATTLE 5
#define COMMAND_HEALALL 6
#define CONDITION_YES 1

class DialogContext
{
  public:
    void init(const uint8_t* d);
    bool run(uint8_t c = 0);
    bool checkCondition(uint8_t c);

    const uint8_t *dialog;
    int currentCommand;
    bool message;
    bool choose;
};

#define STATE_PAINT 0
#define STATE_PAINTZOOM 1
#define STATE_WORLD 2
#define STATE_BATTLECHOICE 3
#define STATE_BATTLE 4
#define STATE_BATTLELOST 5
#define STATE_BATTLEWON 6
#define STATE_TALKING 7
#define STATE_TITLE 8

extern int state;
extern int prevState;

