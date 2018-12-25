#include <algorithm>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>
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

class DialogEntry
{
 public:
  DialogEntry(const std::string &n, int l, uint8_t *bc);
  ~DialogEntry();
  void printHeader();

  std::string name;
  int len;
  uint8_t *bytecode;
};

DialogEntry::DialogEntry(const std::string &n, int l, uint8_t *bc)
: name(n), len(l)
{
 bytecode = new uint8_t[len];
 memcpy(bytecode, bc, len);
}

DialogEntry::~DialogEntry()
{
 delete [] bytecode;
}

void DialogEntry::printHeader()
{
 printf("const uint8_t %s[] = {\n0x%s%x", name.c_str(), ((bytecode[0] < 16) ? "0" : ""), bytecode[0]);
 for (int i = 1; i < len; i++)
 {
  printf(", ");
  if (i % 10 == 0)
   printf("\n");
  printf("0x%s%x", ((bytecode[i] < 16) ? "0" : ""), bytecode[i]);
 }
 printf(", 0x00\n};\n");
}

std::vector<DialogEntry*> dialogList;

std::string parseDialog(std::ifstream &file, uint8_t *dialogResult, int &current)
{
 std::string name;
 std::string line;
 while( std::getline( file, line ) )   
 {
  trim(line);
  if ((line.length() > 0) && (line[0] == '['))
  {
   if (!name.empty())
   {
    dialogList.push_back(new DialogEntry(name, current, dialogResult));
   }
   current = 0;
   name = line.c_str() + 1;
   name.erase(name.length() - 1);
  }
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
   else if (0 == std::strncmp(line.c_str() + 3, "TESTSECRET", 10))
   {
    dialogResult[current] = CONDITION_TESTSECRET;
    current++;
    unsigned int num;
    sscanf(line.c_str() + 14, "%u", &num);
    dialogResult[current++] = num;
   }
   else if (0 == std::strncmp(line.c_str() + 3, "BATTLEWON", 9))
   {
    dialogResult[current] = CONDITION_BATTLEWON;
    current++;
   }
   int jumpLoc = current;
   current += 2;
   std::string endTag = parseDialog(file, dialogResult, current);
   if (endTag == "ELSE")
   {
    dialogResult[current++] = COMMAND_JUMP;
    current += 2;
    *((uint16_t *)(dialogResult + jumpLoc)) = current;
    jumpLoc = current - 2;
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
  else if (0 == std::strncmp(line.c_str(), "HEALALL", 6))
  {
   dialogResult[current] = COMMAND_HEALALL;
   current++;
  }
  else if (0 == std::strncmp(line.c_str(), "PAINT", 6))
  {
   dialogResult[current] = COMMAND_PAINT;
   current++;
  }
  else if (0 == std::strncmp(line.c_str(), "SETSECRET", 9))
  {
   dialogResult[current] = COMMAND_SETSECRET;
   current++;
   unsigned int num;
   sscanf(line.c_str() + 10, "%u", &num);
   dialogResult[current++] = num;
  }
 }
 if (!name.empty())
 {
  dialogList.push_back(new DialogEntry(name, current, dialogResult));
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
 for (int i = 0; i < dialogList.size(); i++)
 {
  dialogList[i]->printHeader();
  delete dialogList[i];
 }
}
