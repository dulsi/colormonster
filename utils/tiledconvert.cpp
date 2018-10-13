#include "expatcpp.h"
#include <algorithm>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <stdio.h>

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

class TiledParser : public ExpatXMLParser
{
 public:
  TiledParser() :  mapCSVNext(true) {}
  virtual void startElement(const XML_Char *name, const XML_Char **atts);
  virtual void endElement(const XML_Char *name);
  virtual void characterData(const XML_Char *s, int len);
  void convert();

 private:
  int blankFirst;
  int tilesetFirst;
  int width;
  int height;
  std::string mapCSV;
  bool mapCSVNext;
};

void TiledParser::startElement(const XML_Char *name, const XML_Char **atts)
{
 if (std::strcmp(name, "data") == 0)
  mapCSVNext = true;
 else if (std::strcmp(name, "layer") == 0)
 {
  for (int i = 0; atts[i]; i += 2)
  {
   if (std::strcmp(atts[i], "height") == 0)
    height = atol(atts[i + 1]);
   else if (std::strcmp(atts[i], "width") == 0)
    width = atol(atts[i + 1]);
  }
 }
 else if (std::strcmp(name, "tileset") == 0)
 {
  int first = 0;
  std::string source;
  for (int i = 0; atts[i]; i += 2)
  {
   if (std::strcmp(atts[i], "source") == 0)
    source = atts[i + 1];
   else if (std::strcmp(atts[i], "firstgid") == 0)
    first = atol(atts[i + 1]);
  }
  if (std::strcmp(source.c_str(), "blank.tsx") == 0)
  {
   blankFirst = first;
  }
  else if (std::strcmp(source.c_str(), "1bit_tileset_8x8.tsx") == 0)
  {
   tilesetFirst = first;
  }
 }
}

void TiledParser::endElement(const XML_Char *name)
{
 if (std::strcmp(name, "data") == 0)
  mapCSVNext = false;
}

void TiledParser::characterData(const XML_Char *s, int len)
{
 if (mapCSVNext)
 {
  std::string line(s, len);
  trim(line);
  mapCSV += line;
 }
}

void TiledParser::convert()
{
 std::vector<int> vect;
 std::stringstream ss(mapCSV);
 int i;

 while (ss >> i)
 {
  vect.push_back(i);
  if (ss.peek() == ',')
   ss.ignore();
 }
 if (vect.size() != height * width)
  printf("Data incorrect\n");
 else
 {
  for (int y = 0; y < height; y++)
  {
   for (int x = 0; x < width; x++)
   {
    int value = vect[y * width + x];
    if (value == blankFirst)
     value = 0;
    else if (value == blankFirst + 1)
     value = 255;
    else
     value = value - tilesetFirst + 1;
    printf("%3d%s", value, ((x + 1 == width) ? ((y + 1 == height) ? "": ",") : ", "));
   }
   printf("\n");
  }
 }
}

int main(int argc, char *argv[])
{
 TiledParser t;
 t.parse(argv[1], false);
 t.convert();
 return 0;
}
