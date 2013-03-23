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
Q16n16 frequency;
byte gain;

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
  }
  //Serial.println("Finished Mozzi setup");
  
    
}

void loop() {
   audioHook();
}

void updateControl(){
  byte touchData;
  byte mask;
  int oscIndex = 0;
  
  // get the touch values from 1 x CY8C201xx chips - GP0 are the higher bits, GP1 the lower bits
  touchData = readTouch(I2C_ADDR0); 
  //Serial.print("Touch:  ");
  //Serial.println(touchData, BIN);

  // Temp update for a single IC - we'll eventually have six loops to work out
  // We're now changing the frequency with each press
  // Put in a for 0 - 5 loop here, for each IC
  // increment oscIndex every time we actually turn a thing on.
  // once oscIndex goes over 11, we break out
  // if, once we're done, oscIndex is less than 11, set oscIndex to 11 to zeroint oscIndex = 0;

  
  for (mask = 00000001; mask > 0; mask <<= 1) {
    if (oscIndex >= NUMBER_OSCS) { 
      break;
    }
    if (touchData & mask) {
      //Serial.println(mask);
      switch (mask) {
          case 1:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(72));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 2:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(73));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 4:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(74));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 8:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(75));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 16:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(76));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 32:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(77));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 64:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(78));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;
        case 128:
          frequency = Q16n16_mtof(Q16n0_to_Q16n16(79));
          oscs[oscIndex]->setFreq_Q16n16(frequency);
          break;          
        default:
          break;
      }
      newButtons[oscIndex] = 1;
      oscIndex++;
    } 
  }
  
  // Turn off any unused oscillators
  for (oscIndex; oscIndex < NUMBER_OSCS; oscIndex++) {
    newButtons[oscIndex] = 0;
  }
}

int updateAudio(){
  int asig = 0;
  for (int i = 0; i < NUMBER_OSCS; i++) {
    if (newButtons[i] != 0) {
      asig = asig + oscs[i]->next();
    }
  }
  //  >> 3 works for 1-3 oscs.  
  // >> 4 should work for 4-6
  // >> ? should work for 7-9
  // >> ? should work for 10-12
  return asig >> 3;
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

