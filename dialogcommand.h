#define DIALOG_SAY 1
#define DIALOG_CHOOSE_YESNO 2
#define COMMAND_IF 3
#define COMMAND_JUMP 4
#define COMMAND_BATTLE 5
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

