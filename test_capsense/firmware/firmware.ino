// Cypress touch code via Joe, with deep thanks to Av, Ian, and Hackon for helping me with harwdware

#include <Wire.h>

#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <mozzi_midi.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type
#include <tables/triangle_warm8192_int8.h>

// Synthesis code
#define CONTROL_RATE 128
#define NUMBER_OSCS 12
#define NUMBER_CONDITIONS 16 // More than 16 ruins my day
#define NUMBER_CHIPS 6

byte newOscs[NUMBER_OSCS];
byte conditionData[NUMBER_CHIPS][NUMBER_CONDITIONS];

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

Q16n16 frequency;

// Touch code
int xres1 = 2;  
int xres2 = 3;  
int xres3 = 4;
int xres4 = 5;  
int xres5 = 6;  
int xres6 = 7;

// I2C adresses
#define I2C_ADDR0 0x00
#define I2C_ADDR1 0x01
#define I2C_ADDR2 0x02
#define I2C_ADDR3 0x03
#define I2C_ADDR4 0x04
#define I2C_ADDR5 0x05

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
  error = Wire.endTransmission();
  Serial.print("Enabled R1:  ");
  Serial.println(error);

  // setup CS_ENABLE1 register
  Wire.beginTransmission(address);
  Wire.write(CS_ENABLE1);
  Wire.write(B00001111);
  error = Wire.endTransmission();
  Serial.print("Enabled R2:  ");
  Serial.println(error);

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
  pinMode(xres4, OUTPUT);
  pinMode(xres5, OUTPUT);
  pinMode(xres6, OUTPUT);
  delay(100);

  // put all into reset mode
  digitalWrite(xres1, HIGH);
  delay(100);
  digitalWrite(xres2, HIGH);
  delay(100);
  digitalWrite(xres3, HIGH);
  delay(100);
  digitalWrite(xres4, HIGH);
  delay(100);
  digitalWrite(xres5, HIGH);
  delay(100);
  digitalWrite(xres6, HIGH);
  delay(100);
  
   // wake up chip 6 and change its address
  digitalWrite(xres6, LOW);
  delay(200);
  configureChip(I2C_ADDR0);
  changeAddress(I2C_ADDR0, I2C_ADDR5);
  
  // wake up chip 5 and change its address
  digitalWrite(xres5, LOW);
  delay(200);
  configureChip(I2C_ADDR0);
  changeAddress(I2C_ADDR0, I2C_ADDR4);
  
  // wake up chip 4 and change its address
  digitalWrite(xres4, LOW);
  delay(200);
  configureChip(I2C_ADDR0);
  changeAddress(I2C_ADDR0, I2C_ADDR3);
  
  // wake up chip 3 and change its address
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

  // Initialize the conditioning arrays
  for (int i = 0; i < NUMBER_CHIPS; i++) {
    for (int j = 0; j < NUMBER_CONDITIONS; j++) {
      conditionData[i][j] = 0;
    }  
  }
  
  //Serial.println("Finished touch setup");

  startMozzi(CONTROL_RATE);
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newOscs[i] = 0;
  }
  //Serial.println("Finished Mozzi setup");
}

void loop() {
 audioHook();
}


int playNotes(byte touchData, int oscIndex, int frequencies[]) {
  byte mask;
  int freqIndex = 0;
  
  for (mask = 00000001; mask > 0; mask <<= 1) {
    if (oscIndex >= NUMBER_OSCS) { 
      break;
    }
    if (touchData & mask) {
      frequency = Q16n16_mtof(Q16n0_to_Q16n16(frequencies[freqIndex]));
      oscs[oscIndex]->setFreq_Q16n16(frequency);  
      newOscs[oscIndex] = 1;
      oscIndex++;
    }
    freqIndex++;
  }
  
  return oscIndex;
}

void updateControl() {
  byte touchData;
  byte newData;
  byte mask;
  int oscIndex = 0;
  int pitchArray[8];

  // For 6 chips
  touchData = readTouch(I2C_ADDR0); // get the touch values from 1 x CY8C201xx chips - GP0 are the higher bits, GP1 the lower
  touchData = conditionTouchData(touchData, 0);
  // Serial.println(touchData, BIN);
  // So this is GP0:  0, 1, 2, 3 - GP1:  0, 1, 2, 3
  // I am re-writing based on proximity, so EACH of these will be different.  Sorry.
  pitchArray = {57, 61, 65, 57, 61, 65, 64, 68};  // A-C#-F, A-C#-F, E-Ab
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);
  
  touchData = readTouch(I2C_ADDR1);
  touchData = conditionTouchData(touchData, 1);
  // Serial.println(touchData, BIN);
  pitchArray = {60, 64, 68, 60, 59, 63, 67, 59}; // C, E-Ab-C, B-Eb-G, B    
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);
  
  touchData = readTouch(I2C_ADDR2);
  touchData = conditionTouchData(touchData, 2);
  // Serial.println(touchData, BIN);
  pitchArray = {63, 67, 66, 58, 62, 66, 58, 62};  // Eb-G, F#-Bb-D, F#-Bb-D  
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);
   
  touchData = readTouch(I2C_ADDR3);
  touchData = conditionTouchData(touchData, 3);
  //Serial.println(touchData, BIN);
  pitchArray = {69, 73, 77, 69, 73, 77, 76, 80};  //  A-C#-F, A-C#-F, E-Ab    
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);

  touchData = readTouch(I2C_ADDR4);
  touchData = conditionTouchData(touchData, 4);
  //Serial.println(touchData, BIN);
  pitchArray = {72, 76, 80, 72, 71, 75, 79, 71};  // C, E-Ab-C, B-Eb-G, B    
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);

  touchData = readTouch(I2C_ADDR5);
  touchData = conditionTouchData(touchData, 5);
  Serial.println(touchData, BIN);
  pitchArray = {75, 79, 78, 70, 74, 78, 70, 74};  // Eb-G, F#-Bb-D, F#-Bb-D  
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);

  // Turn off any unused oscillators
  for (oscIndex; oscIndex < NUMBER_OSCS; oscIndex++) {
    newOscs[oscIndex] = 0;
  }
}

int updateAudio(){
  int asig = 0;
  for (int i = 0; i < NUMBER_OSCS; i++) {
    if (newOscs[i] != 0) {
      asig = asig + oscs[i]->next();
    }
  }
  //  >> 3 works for 1-3 oscs.  Will need to solve this later
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


byte conditionTouchData(byte touchData, int index) {
  byte newData;
  newData = touchData;
  // AND the data together
  for (int i = 0; i < NUMBER_CONDITIONS; i++) {
   newData = newData & conditionData[index][i];
  }
    
  // Update the data.  WARNING:  REVERSE FOR LOOP
  for (int i = NUMBER_CONDITIONS - 1; i > 0; i--) {
    conditionData[index][i] = conditionData[index][i - 1];
  }
  conditionData[index][0] = touchData;
  
  return newData; 
}
