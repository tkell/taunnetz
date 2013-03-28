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
int xres1 = 2;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 2
int xres2 = 3;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 3
int xres3 = 4;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 4

// I2C adresses
#define I2C_ADDR0 0x00
#define I2C_ADDR1 0x01
#define I2C_ADDR2 0x02

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

// Set a chip up so we can read its register
void configureChip(int address) {
  
  byte error;
  
  // switch to setup mode
  Wire.beginTransmission(address);
  Wire.write(COMMAND_REG);
  Wire.write(0x08);
  error = Wire.endTransmission();
  Serial.print("Switched to setup mode:  ");
  Serial.println(error);

  // setup CS_ENABLE0 register
  Wire.beginTransmission(address);
  Wire.write(CS_ENABLE0);
  Wire.write(B00001111);
  Wire.endTransmission();

  // setup CS_ENABLE1 register
  Wire.beginTransmission(address);
  Wire.write(CS_ENABLE1);
  Wire.write(B00001111);
  Wire.endTransmission();

  // switch to normal mode
  Wire.beginTransmission(address);
  Wire.write(COMMAND_REG);
  Wire.write(0x07);
  Wire.endTransmission();
}

// Change the I2C address of a chip
void changeAddress(int currAddress, int newAddress) {
    byte error;
     // unlock the I2C_DEV_LOCK register
    Wire.beginTransmission(currAddress);
    Wire.write(I2C_DEV_LOCK);
    Wire.write(I2CDL_KEY_UNLOCK, 3);
    Wire.endTransmission();
    
    //change the I2C_ADDR_DM register to newAddress
    Wire.beginTransmission(currAddress);
    Wire.write(I2C_ADDR_DM);
    Wire.write(newAddress);
    error = Wire.endTransmission();
    Serial.print("Changed address:  ");
    Serial.println(error);
    
    //lock register again for change to take effect
    Wire.beginTransmission(currAddress);
    Wire.write(I2C_DEV_LOCK);
    Wire.write(I2CDL_KEY_LOCK, 3);
    Wire.endTransmission();
    // the I2C address is now newAddress
}


void setup() {
  // start serial interface
  Serial.begin(9600);
  
  //start I2C bus
  Wire.begin();
  
  // set reset pin modes
  pinMode(xres1, OUTPUT);
  pinMode(xres2, OUTPUT);
  pinMode(xres3, OUTPUT);
  delay(100);

  // put all into reset mode
  digitalWrite(xres1, HIGH);
  delay(100);
  digitalWrite(xres2, HIGH);
  delay(100);
  digitalWrite(xres3, HIGH);
  delay(100);
  
  // wake up chip 2 and change its address
  digitalWrite(xres3, LOW);
  delay(200);
  configureChip(I2C_ADDR0);
  changeAddress(I2C_ADDR0, I2C_ADDR2);

  // wake up chip 2 and change its address
  digitalWrite(xres2, LOW);
  delay(200);
  configureChip(I2C_ADDR0);
  changeAddress(I2C_ADDR0, I2C_ADDR1);
  
  // wake up chip 1
  digitalWrite(xres1, LOW);
  delay(200);
  configureChip(I2C_ADDR0);

  Serial.println("Finished touch setup");
  
  
  //startMozzi(CONTROL_RATE);
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
  }
  //Serial.println("Finished Mozzi setup");
  
    
}




void loop() {
  
  byte touchData;

  touchData = readTouch(I2C_ADDR0); 
  Serial.print("Touch 1:  ");
  Serial.println(touchData, BIN);
  
  touchData = readTouch(I2C_ADDR1); 
  Serial.print("Touch 2:  ");
  Serial.println(touchData, BIN);
  
  // This might crash
  touchData = readTouch(I2C_ADDR2); 
  Serial.print("Touch 3:  ");
  Serial.println(touchData, BIN);
}

void updateControl(){
  byte touchData;
  byte mask;
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

