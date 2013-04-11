// Cypress touch code via Joe, with deep thanks to Av, Ian, and Hackon for helping me with hardware.  Mozzi synth via Marcello.

#include <MozziGutsT2.h>
#include <Oscil.h>
#include <utils.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type
#include <tables/triangle_warm8192_int8.h>
#include <twi_nonblock.h>

// Synthesis code
#define CONTROL_RATE 64 // 64 seems better than 128, 32 does not work
#define NUMBER_OSCS 6 // cuold change this, really
#define NUMBER_CONDITIONS 4 // 4 works well.  More than 16 ruins my day.  May cut this totally?
#define NOISE_THRESH 0x96  // 0x28 is factory default, 0x50 was working well, 0x72 was working great, 0xAA felt a little slow
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
    &osc1, &osc2, &osc3, &osc4, &osc5, &osc6
  };

Q16n16 frequency;
int pitchArray[8];

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

// Non-blocking globals
#define ACC_IDLE 0
#define ACC_READING 1
#define ACC_WRITING 2
uint8_t tempBuffer[1];
uint8_t acc_status;

// some CY8C201xx registers.  These may all have to be re-declared as uint8_t
#define CS_ENABLE0 0x06
#define CS_ENABLE1 0x07
#define COMMAND_REG 0xA0
#define CS_NOISE_TH 0x4E

uint8_t INPUT_PORT0 = 0x00;
uint8_t INPUT_PORT1 = 0x01;
uint8_t I2C_DEV_LOCK = 0x7;
uint8_t I2C_ADDR_DM = 0x7C;

// Secret codes for locking/unlocking the I2C_DEV_LOCK register
uint8_t I2CDL_KEY_UNLOCK[3] = {0x3C, 0xA5, 0x69};
uint8_t I2CDL_KEY_LOCK[3] = {0x96, 0x5A, 0xC3};

// Set a chip up so we can read its register
void configureChip(int address) {
  uint8_t error;

  // switch to setup mode
  twowire_beginTransmission(address);
  twowire_send(COMMAND_REG);
  twowire_send(0x08);
  error = twowire_endTransmission();
  Serial.print("Switched to setup mode:  ");
  Serial.println(error);

  // setup CS_ENABLE0 register
  twowire_beginTransmission(address);
  twowire_send(CS_ENABLE0);
  twowire_send(B00001111);
  error = twowire_endTransmission();
  Serial.print("Enabled R1:  ");
  Serial.println(error);

  // setup CS_ENABLE1 register
  twowire_beginTransmission(address);
  twowire_send(CS_ENABLE1);
  twowire_send(B00001111);
  error = twowire_endTransmission();
  Serial.print("Enabled R2:  ");
  Serial.println(error);
    
  // Increase the noise threshold
  twowire_beginTransmission(address);
  twowire_send(CS_NOISE_TH);
  twowire_send(NOISE_THRESH); // Factory default is 0x28
  error = twowire_endTransmission();
  Serial.print("Increased Noise Threshold:  ");
  Serial.println(error);

  // switch to normal mode
  twowire_beginTransmission(address);
  twowire_send(COMMAND_REG);
  twowire_send(0x07);
  twowire_endTransmission();
}

// Change the I2C address of a chip
void changeAddress(int currAddress, int newAddress) {
    uint8_t error;
     // unlock the I2C_DEV_LOCK register
    twowire_beginTransmission(currAddress);
    twowire_send(I2C_DEV_LOCK);
    twowire_send(I2CDL_KEY_UNLOCK[0]); twowire_send(I2CDL_KEY_UNLOCK[1]); twowire_send(I2CDL_KEY_UNLOCK[2]);
    twowire_endTransmission();
    
    //change the I2C_ADDR_DM register to newAddress
    twowire_beginTransmission(currAddress);
    twowire_send(I2C_ADDR_DM);
    twowire_send(newAddress);
    error = twowire_endTransmission();
    Serial.print("Changed address:  ");
    Serial.println(error);
    
    //lock register again for change to take effect
    twowire_beginTransmission(currAddress);
    twowire_send(I2C_DEV_LOCK);
    twowire_send(I2CDL_KEY_LOCK[0]); twowire_send(I2CDL_KEY_LOCK[1]); twowire_send(I2CDL_KEY_LOCK[2]);
    twowire_endTransmission();
    // the I2C address is now newAddress
}


void setup() {
  // start serial interface
  Serial.begin(9600);
  
  //start twowire
  initialize_twi_nonblock();
  acc_status = ACC_IDLE;
  tempBuffer[0] = 0;
  
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
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newOscs[i] = 0;
  }
  pitchArray = {57, 61, 65, 57, 61, 65, 64, 68};

  delay(250);
  //Serial.println("Finished Mozzi setup");
  startMozziT2(CONTROL_RATE);
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

// Magic non-blocking touch read stuff.
// For now we're barely reading anything.

// This is what we're trying to duplicate
//  Wire.beginTransmission(address);
//  Wire.write(uint8_t(INPUT_PORT0));
//  Wire.endTransmission();
//      
//  Wire.requestFrom(address, 1);
//  while (!Wire.available()) {}
//  touch = Wire.read() << 4;

void initiateTouchRead(uint8_t address) {
  // Looks like we may have to use these
  txBufferIndex = 0;
  txBufferLength = 0;  
  txBuffer[txBufferIndex] = INPUT_PORT0;
  txBufferIndex++;
  txBufferLength = txBufferIndex;
  
  twi_initiateWriteTo(address, txBuffer, txBufferLength);
  Serial.print(address); Serial.print(txBuffer[0]); Serial.println(txBufferLength);
  acc_status = ACC_WRITING;
}


void initiateTouchRequest(uint8_t address) {
  txBufferIndex = 0;
  txBufferLength = 0;
  
  uint8_t read = twi_initiateReadFrom(address, 1);
  acc_status = ACC_READING;
}


void finaliseTouchRequest() {
  uint8_t read = twi_readMasterBuffer(tempBuffer, 1);
  acc_status = ACC_IDLE;
  Serial.println(tempBuffer[0]);
}


void updateControlT2() {
  uint8_t touchData = 0;
  int oscIndex = 0; 
  Serial.print("Status:  ");
  Serial.println(acc_status);
  //Serial.print("TWI state:  ");
  //Serial.println(twi_state);
  
  switch(acc_status) {
      case ACC_IDLE:
	initiateTouchRead(0);
	break;
      case ACC_WRITING:
	if (twi_state != TWI_MRX) {
	  initiateTouchRequest(0);
	}
	break;
      case ACC_READING:
	if (twi_state != TWI_MRX) {
	  finaliseTouchRequest();
	}
	break;
    }

  // For 6 chips
  //touchData = readTouch(I2C_ADDR0); // get the touch values from 1 x CY8C201xx chips - GP0 are the higher bits, GP1 the lower
  //touchData = conditionTouchData(touchData, 0);
  //touchData = 7;
  //Serial.println(touchData, BIN);
  // So this is GP0:  0, 1, 2, 3 - GP1:  0, 1, 2, 3
  // I am re-writing based on proximity, so EACH of these will be different.  Sorry.
  // A-C#-F, A-C#-F, E-Ab
  //oscIndex = playNotes(touchData, oscIndex, pitchArray);
  

  // Turn off any unused oscillators
  for (oscIndex; oscIndex < NUMBER_OSCS; oscIndex++) {
    newOscs[oscIndex] = 0;
  }
}

int updateAudioT2(){
  int asig = 0;
  for (int i = 0; i < NUMBER_OSCS; i++) {
    if (newOscs[i] != 0) {
      asig = asig + oscs[i]->next();
    }
  }
  //  >> 3 works for 1-3 oscs.  Will need to solve this later
  return asig >> 3;
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

void loop() {
 audioHookT2();
}

// Touch Code
//byte readTouch(int address) {
//  uint8_t touch = 0;
//
//  // request Register 00h: INPUT_PORT0
//  Wire.beginTransmission(address);
//  Wire.write(uint8_t(INPUT_PORT0));
//  Wire.endTransmission();
//      
//  Wire.requestFrom(address, 1);
//  while (!Wire.available()) {}
//  touch = Wire.read() << 4;
//  
//  // request Register 01h: INPUT_PORT1
//  Wire.beginTransmission(address);
//  Wire.write(INPUT_PORT1);
//  Wire.endTransmission();
//  
//  Wire.requestFrom(address, 1);
//  while (!Wire.available()) {}
//  
//  touch |= Wire.read();
//  return touch;
//}

