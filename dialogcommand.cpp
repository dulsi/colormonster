#include <TinyScreen.h>
#include <SPI.h>
#include <cstring>
#include "dialogcommand.h"
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
          currentCommand = *((uint16_t*)(dialog + currentCommand));
        }
        break;
      case COMMAND_BATTLE:
        printf("Battle (Not implemented)\n");
        return false;
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
    default:
      break;
  }
  return false;
}

