#include <algorithm>
#include <string>
#include <cstring>
#include <fstream>
#include "../dialogcommand.h"

static inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
    ltrim(s);
    rtrim(s);
}

std::string parseDialog(std::ifstream &file, uint8_t *dialogResult, int &current)
{
 std::string line;
 while( std::getline( file, line ) )   
 {
  trim(line);
  if (0 == std::strncmp(line.c_str(), "SAY", 3))
  {
   dialogResult[current] = DIALOG_SAY;
   current++;
   std::strcpy((char *)(dialogResult + current), line.c_str() + 4);
   current += line.length() - 4 + 1;
  }
  else if (0 == std::strncmp(line.c_str(), "CHOOSE_YESNO", 12))
  {
   dialogResult[current] = DIALOG_CHOOSE_YESNO;
   current++;
  }
  else if (0 == std::strncmp(line.c_str(), "IF", 2))
  {
   dialogResult[current] = COMMAND_IF;
   current++;
   if (0 == std::strncmp(line.c_str() + 3, "yes", 3))
   {
    dialogResult[current] = CONDITION_YES;
    current++;
   }
   int jumpLoc = current;
   current += 2;
   std::string endTag = parseDialog(file, dialogResult, current);
   if (endTag == "ELSE")
   {
    *((uint16_t *)(dialogResult + jumpLoc)) = current;
    dialogResult[current] = COMMAND_JUMP;
    jumpLoc = current;
    current += 2;
    endTag = parseDialog(file, dialogResult, current);
   }
   *((uint16_t *)(dialogResult + jumpLoc)) = current;
   if (endTag != "FI")
   {
    printf("Failed end if\n");
   }
  }
  else if ((0 == std::strncmp(line.c_str(), "FI", 2)) || (0 == std::strncmp(line.c_str(), "ELSE", 4)))
  {
   return line;
  }
  else if (0 == std::strncmp(line.c_str(), "BATTLE", 6))
  {
   dialogResult[current] = COMMAND_BATTLE;
   current++;
  }
 }
 return "";
}

int main(int argc, char *argv[])
{
 std::ifstream file(argv[1]);
 uint8_t dialogResult[1000];

 std::string line;
 int current = 0;
 parseDialog(file, dialogResult, current);
 printf("{\n0x%s%x", ((dialogResult[0] < 16) ? "0" : ""), dialogResult[0]);
 for (int i = 1; i < current; i++)
 {
  printf(", ");
  if (i % 10 == 0)
   printf("\n");
  printf("0x%s%x", ((dialogResult[i] < 16) ? "0" : ""), dialogResult[i]);
 }
 printf(", 0x00\n}\n");
}
