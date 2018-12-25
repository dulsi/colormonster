#ifndef TRAINER_H
#define TRAINER_H

#define MAX_SECRETS 10

class Trainer
{
  public:
    Trainer() : icon(28), location(0), x(136), y(96) {}

    void setSecret(uint8_t s) { secrets[s / 8] |= (((uint8_t)1) << (s % 8)); }
    bool testSecret(uint8_t s) { return ((secrets[s / 8] & (((uint8_t)1) << (s % 8))) != 0); }
    bool load();
    void save();

    uint8_t icon;
    int location;
    int x, y;
    uint8_t dir;
    uint8_t secrets[MAX_SECRETS];
};

extern Trainer pc;

#endif

