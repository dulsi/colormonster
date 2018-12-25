#include <TinyScreen.h>
#include <SPI.h>
#include <cstring>
#include "battle.h"
#include "colormonster.h"
#include "dialogcommand.h"
#include "trainer.h"
#include "uiobject.h"

const char *yesNoOption[2] = { "Yes", "No" };
const uint8_t yesNoRow[] = { 70, 48, 94, 63};

void DialogContext::init(const uint8_t *d)
{
  dialog = d;
  currentCommand = 0;
  message = false;
  choose = false;
}

bool DialogContext::run(uint8_t c)
{
  while (dialog[currentCommand])
  {
    switch (dialog[currentCommand++])
    {
      case DIALOG_SAY:
        bottomMessage.setText((const char *)(dialog + currentCommand));
        currentCommand += std::strlen((const char *)(dialog + currentCommand)) + 1;
        message = true;
        choose = false;
        return true;
        break;
      case DIALOG_CHOOSE_YESNO:
        choice.setRect(yesNoRow);
        choice.setOptions(1, 2, yesNoOption);
        choose = true;
        return true;
        break;
      case COMMAND_IF:
        if (checkCondition(c))
        {
          currentCommand += 2;
        }
        else
        {
          int tmp = (dialog[currentCommand + 1] << 8) + dialog[currentCommand];
          currentCommand = tmp;
        }
        break;
      case COMMAND_BATTLE:
        choose = false;
        message = false;
        prevState = state;
        state = STATE_BATTLECHOICE;
        battle.init();
        return true;
        break;
      case COMMAND_JUMP:
      {
        int tmp = (dialog[currentCommand + 1] << 8) + dialog[currentCommand];
        currentCommand = tmp;
        break;
      }
      case COMMAND_HEALALL:
        for (int i = 0; i < MONSTER_PARTYSIZE; i++)
        {
          party[i].hp = party[i].maxHp;
        }
        break;
      case COMMAND_PAINT:
        choose = false;
        message = false;
        prevState = state;
        state = STATE_PAINT;
        return true;
        break;
      case COMMAND_SETSECRET:
        pc.setSecret(dialog[currentCommand++]);
        break;
      default:
        return false;
        break;
    }
  }
  return false;
}

bool DialogContext::checkCondition(uint8_t c)
{
  switch (dialog[currentCommand++])
  {
    case CONDITION_YES:
      if (c == 0)
        return true;
      break;
    case CONDITION_TESTSECRET:
     if (pc.testSecret(dialog[currentCommand++]))
      return true;
     break;
    default:
      break;
  }
  return false;
}

