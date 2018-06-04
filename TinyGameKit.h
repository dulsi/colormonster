#ifndef TINYGAMEKIT_H
#define TINYGAMEKIT_H

const uint8_t PROGMEM TAJoystickUp  = 1 << 0;       //Mask
const uint8_t PROGMEM TAJoystickDown = 1 << 1;      //Mask
const uint8_t PROGMEM TAJoystickLeft  = 1 << 2;     //Mask
const uint8_t PROGMEM TAJoystickRight = 1 << 3;     //Mask
const uint8_t PROGMEM TAJoystick2Up  = 1 << 4;       //Mask
const uint8_t PROGMEM TAJoystick2Down = 1 << 5;      //Mask
const uint8_t PROGMEM TAJoystick2Left  = 1 << 6;     //Mask
const uint8_t PROGMEM TAJoystick2Right = 1 << 7;     //Mask

const uint8_t TAButton1  = 1 << 0;          //Mask
const uint8_t TAButton2 = 1 << 1;           //Mask 

#define GAMEKIT_HASJOYSTICK 1
#define GAMEKIT_HASBUTTONS 2

byte gamekit_read = 0;
int gamekit_data[4];
byte gamekit_buttons;

void arcadeInit()
{
  Wire.begin();
}

void readJoystickButtons()
{
  Wire.requestFrom(0x22,6);
  for(int i=0;i<4;i++)
  {
    gamekit_data[i]=Wire.read();
  }
  byte lsb=Wire.read();
  for(int i=0;i<4;i++)
  {
    gamekit_data[i]<<=2;
    gamekit_data[i]|= ((lsb>>(i*2))&3);
    gamekit_data[i]-=511;
  }
  gamekit_buttons=~Wire.read();
  gamekit_read = GAMEKIT_HASJOYSTICK | GAMEKIT_HASBUTTONS;
}

uint8_t checkButton(uint8_t btn)
{
  if (0 == (gamekit_read & GAMEKIT_HASBUTTONS))
  {
    readJoystickButtons();
    gamekit_read = gamekit_read & (~GAMEKIT_HASBUTTONS);
  }
  uint8_t answer = 0;
  if ((btn & TAButton1) && (gamekit_buttons & 4))
    answer |= TAButton1;
  if ((btn & TAButton2) && (gamekit_buttons & 8))
    answer |= TAButton2;
  return answer;
}

uint8_t checkJoystick(uint8_t joystickDir)
{
  if (0 == (gamekit_read & GAMEKIT_HASJOYSTICK))
  {
    readJoystickButtons();
    gamekit_read = gamekit_read & (~GAMEKIT_HASJOYSTICK);
  }
  uint8_t answer = 0;
  int RX=gamekit_data[0];
  int RY=gamekit_data[1];
  int LX=gamekit_data[2];
  int LY=gamekit_data[3];
  if ((joystickDir & TAJoystickLeft) && (LX > 100))
  {
    answer |= TAJoystickLeft;
  }
  else if ((joystickDir & TAJoystickRight) && (LX < -100))
  {
    answer |= TAJoystickRight;
  }
  if ((joystickDir & TAJoystickUp) && (LY > 100))
  {
    answer |= TAJoystickUp;
  }
  else if ((joystickDir & TAJoystickDown) && (LY < -100))
  {
    answer |= TAJoystickDown;
  }
  if ((joystickDir & TAJoystick2Left) && (RX > 100))
  {
    answer |= TAJoystick2Left;
  }
  else if ((joystickDir & TAJoystick2Right) && (RX < -100))
  {
    answer |= TAJoystick2Right;
  }
  if ((joystickDir & TAJoystick2Up) && (RY > 100))
  {
    answer |= TAJoystick2Up;
  }
  else if ((joystickDir & TAJoystick2Down) && (RY < -100))
  {
    answer |= TAJoystick2Down;
  }
  return answer;
}

#endif

