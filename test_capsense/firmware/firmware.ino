// Cypress touch code via Joe, with deep thanks to Av for helping me with harwdware

#include <Wire.h>

#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <mozzi_midi.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type
#include <tables/smoothsquare8192_int8.h>
#include <tables/triangle_warm8192_int8.h>

// Synthesis code
#define CONTROL_RATE 128
#define NUMBER_OSCS 12

byte newButtons[NUMBER_OSCS];
byte oldButtons[NUMBER_OSCS];

// Amazingly shitty need to declare every osc by hand
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc1(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc2(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc3(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc4(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc5(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc6(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc7(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc8(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc9(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc10(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc11(TRIANGLE_WARM8192_DATA);
Oscil <TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> osc12(TRIANGLE_WARM8192_DATA);

Oscil<TRIANGLE_WARM8192_NUM_CELLS, AUDIO_RATE> *oscs[NUMBER_OSCS] = {
    &osc1, &osc2, &osc3, &osc4, &osc5, &osc6, &osc7, &osc8, &osc9, &osc10, &osc11, &osc12
  };

EventDelay <CONTROL_RATE> eventDelay;
int noteIndex = 0;
Q16n16 frequency;


// Touch code
int xres = 13;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 13

// I2C adresses
#define I2C_ADDR0 0x00

// some CY8C201xx registers
#define INPUT_PORT0 0x00
#define INPUT_PORT1 0x01
#define CS_ENABLE0 0x06
#define CS_ENABLE1 0x07
#define I2C_DEV_LOCK 0x79
#define I2C_ADDR_DM 0x7C
#define COMMAND_REG 0xA0

// Secret codes for locking/unlocking the I2C_DEV_LOCK register
byte I2CDL_KEY_UNLOCK[3] = {0x3C, 0xA5, 0x69};
byte I2CDL_KEY_LOCK[3] = {0x96, 0x5A, 0xC3};


void setup() {
  // start serial interface
  //Serial.begin(9600);
  
  //start I2C bus
  Wire.begin();
  
  // set pin modes
  pinMode(xres, OUTPUT);
  
  // chip #1: put into reset mode
  digitalWrite(xres, HIGH);
  delay(100);

    // let the chip #1 wake up again
    digitalWrite(xres, LOW);
    delay(200);

    // chip #1: switch to setup mode
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(COMMAND_REG);
    Wire.write(0x08);
    Wire.endTransmission();

    // chip #1: setup CS_ENABLE0 register
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(CS_ENABLE0);
    Wire.write(B00001111);
    Wire.endTransmission();

    // chip #1: setup CS_ENABLE1 register
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(CS_ENABLE1);
    Wire.write(B00001111);
    Wire.endTransmission();

    // chip #1: switch to normal mode
    Wire.beginTransmission(I2C_ADDR0);
    Wire.write(COMMAND_REG);
    Wire.write(0x07);
    Wire.endTransmission();
    //Serial.println("Finished touch setup");
  
  
  startMozzi(CONTROL_RATE);
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
    oldButtons[i] = 0;
    // Set frequencies
    frequency = Q16n16_mtof(Q16n0_to_Q16n16(i + 72));
    oscs[i]->setFreq_Q16n16(frequency);
  }
  //Serial.println("Finished Mozzi setup");
  
    
}

void loop() {
   audioHook();
}

void updateControl(){
  byte touchData;
  byte mask;
  // get the touch values from 1 x CY8C201xx chips
  // GP0 Registers are the higher first four bits
  
  
  touchData = readTouch(I2C_ADDR0); 
  //Serial.print("Touch:  ");
  //Serial.println(touchData, BIN);

  // Temp update for a single IC - we'll eventually have six loops to work ou
  int i = 0;
  for (mask = 00000001; mask > 0; mask <<= 1) {
    if (touchData & mask) {
      newButtons[i] = 1;
    } else {
      newButtons[i] = 0;
    }
    //Serial.print(newButtons[i]);
    i++;
  }
  //Serial.println("");
  oldButtons[i] = newButtons[i];
  
  
  
}


int updateAudio(){
  int asig = 0;
  for (int i = 0; i < NUMBER_OSCS; i++) {
    if (newButtons[i] != 0) {
      asig = asig + oscs[i]->next();
    }
  }

  return asig >> 1;
}


// Touch Code
byte readTouch(int address) {
  byte touch = 0;

  // request Register 00h: INPUT_PORT0
  Wire.beginTransmission(address);
  Wire.write(uint8_t(INPUT_PORT0));
  Wire.endTransmission();
      
  Wire.requestFrom(address, 1);
  while (!Wire.available()) {}
  touch = Wire.read() << 4;
  
  // request Register 01h: INPUT_PORT1
  Wire.beginTransmission(address);
  Wire.write(INPUT_PORT1);
  Wire.endTransmission();
  
  Wire.requestFrom(address, 1);
  while (!Wire.available()) {}
  
  touch |= Wire.read();
  return touch;
}

