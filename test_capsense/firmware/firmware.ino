// Cypress touch code via Joe, with deep thanks to Av, Ian, and Hackon for helping me with hardware.  Mozzi synth via Marcello.
// This is the version with onboard audio.  it's a little bit optimistic.  

#include <MozziGuts.h>
#include <Oscil.h>
#include <utils.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type
#include <tables/triangle_warm8192_int8.h>
#include <twi_nonblock.h>

// Synthesis code
#define CONTROL_RATE 64 // 64 seems better than 128, 32 does not work
#define NUMBER_OSCS 3 // Could change this, really
#define NOISE_THRESH 0xA0 // 80 is the minimum, 96 is is about the useful max
#define NUMBER_CHIPS 6

byte newOscs[NUMBER_OSCS];

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
  &osc1, &osc2, &osc3
};

uint8_t chipIndex;
uint8_t masterTouchData[NUMBER_CHIPS * 2];
unsigned int pitchArray[NUMBER_CHIPS * 2][4];


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
uint8_t acc_status;


// some CY8C201xx registers
uint8_t INPUT_PORT0 = 0x00;
uint8_t INPUT_PORT1 = 0x01;
uint8_t I2C_DEV_LOCK = 0x79;
uint8_t I2C_ADDR_DM = 0x7C;
uint8_t CS_ENABLE0 = 0x06;
uint8_t CS_ENABLE1 = 0x07;
uint8_t COMMAND_REG = 0xA0;
uint8_t CS_NOISE_TH = 0x4E;

// Secret codes for locking/unlocking the I2C_DEV_LOCK register
uint8_t I2CDL_KEY_UNLOCK[3] = {0x3C, 0xA5, 0x69};
uint8_t I2CDL_KEY_LOCK[3] = {0x96, 0x5A, 0xC3};

uint8_t blockingWrite(uint8_t address, uint8_t reg, byte value) {
  twowire_beginTransmission(address);
  twowire_send(reg);
  twowire_send(value);
  return twowire_endTransmission();
}

// Set a chip up so we can read its register
void configureChip(uint8_t address) {
  uint8_t error;

  // switch to setup mode
  error = blockingWrite(address, COMMAND_REG, 0x08);
  //Serial.print("Switched to setup mode:  ");
  //Serial.println(error);

  // set CS_ENABLE0 register
  error = blockingWrite(address, CS_ENABLE0, B00001111);
  //Serial.print("Enabled R1:  ");
  //Serial.println(error);

  // setup CS_ENABLE1 register
  error = blockingWrite(address, CS_ENABLE1, B00001111);
  //Serial.print("Enabled R2:  ");
  //Serial.println(error);

  // Increase the noise threshold
  error = blockingWrite(address, CS_NOISE_TH, NOISE_THRESH); // Factory default is 0x28
  //Serial.print("Increased Noise Threshold:  ");
  //Serial.println(error);

  // switch to normal mode
  error = blockingWrite(address, COMMAND_REG, 0x07);
  //Serial.print("Returned to normal mode  :  ");
  //Serial.println(error);
}

// Change the I2C address of a chip
void changeAddress(uint8_t currAddress, uint8_t newAddress) {
  uint8_t error;

  // unlock the I2C_DEV_LOCK register
  twowire_beginTransmission(currAddress);
  twowire_send(I2C_DEV_LOCK);
  twowire_send(I2CDL_KEY_UNLOCK[0]); 
  twowire_send(I2CDL_KEY_UNLOCK[1]); 
  twowire_send(I2CDL_KEY_UNLOCK[2]);
  error = twowire_endTransmission();
  //Serial.print("Unlocked dev register:  ");
  //Serial.println(error);

  //change the I2C_ADDR_DM register to newAddress
  error = blockingWrite(currAddress, I2C_ADDR_DM, newAddress);
  //Serial.print("Changed address:  ");
  //Serial.println(error);

  //lock register again for change to take effect
  twowire_beginTransmission(currAddress);
  twowire_send(I2C_DEV_LOCK);
  twowire_send(I2CDL_KEY_LOCK[0]); 
  twowire_send(I2CDL_KEY_LOCK[1]); 
  twowire_send(I2CDL_KEY_LOCK[2]);
  error = twowire_endTransmission();
  // the I2C address is now newAddress
  //Serial.print("Locked dev register:  ");
  //Serial.println(error);
}



void setup() {
  // start serial interface
  Serial.begin(9600);

  //start twowire
  initialize_twi_nonblock();
  acc_status = ACC_IDLE;
  chipIndex = 0x00;
  
  // We'll keep the init for the touch array in this loop, and then replace
  //  the pitch array with bespoke things
  for (int i = 0; i < NUMBER_CHIPS * 2; i++) {
      masterTouchData[i] = 0;
  }
  
  // Initialize the pitch array.  Because I wired for short distance, these are out of order!  Sorry!!
  pitchArray[0] = {440u, 554u, 698u, 440u}; pitchArray[1] = {554u, 698u, 659u, 659u};
  pitchArray[2] = {831u, 523u, 622u, 494u}; pitchArray[3] = {831u, 523u, 622u, 784u};
  pitchArray[4] = {587u, 740u, 740u, 466u}; pitchArray[5] = {784u, 494u, 466u, 587u};
  
  // 69 - 80
  pitchArray[6] = {880u, 1397u, 1661u, 1319u}; pitchArray[7] = {880u, 1397u, 1109u, 1109u};
  pitchArray[8] = {1568u, 1661u, 1568u, 1245u}; pitchArray[9] = {1047u, 1047u, 1319u, 1245u};
  pitchArray[10] = {1175u, 932u, 932u, 1480u}; pitchArray[11] = {988u, 988u, 1480u, 1175u};

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

  //Serial.println("Finished touch setup");
  for (int i = 0; i < NUMBER_OSCS; i++) {
    oscs[i]->setFreq(440u);
    newOscs[i] = 0;
  }

  delay(250);
  startMozzi(CONTROL_RATE);
  //Serial.println("Finished Mozzi setup");
}



int playNotes(byte touchData, int oscIndex, unsigned int frequencies[]) {
  byte mask;
  int freqIndex = 0;
  // Going to try to scope this so it is always less than 10000
  for (mask = 00000001; mask > 0; mask <<= 1) {
    if (oscIndex >= NUMBER_OSCS || mask >= 10000) { 
      break;
    }
    if (touchData & mask) {
      //Serial.println(freqIndex);
      //Serial.println(frequencies[freqIndex]);
      oscs[oscIndex]->setFreq(frequencies[freqIndex]);  
      newOscs[oscIndex] = 1;
      oscIndex++;
    }
    freqIndex++;
  }
  return oscIndex;
}



// The three non-blocking read functions
void initiateTouchRead(uint8_t address, uint8_t reg) {
  // Looks like we may have to use these
  txAddress = address;
  txBufferIndex = 0;
  txBufferLength = 0;  
  txBuffer[txBufferIndex] = reg; // the register that we want to read.  
  ++txBufferIndex;
  txBufferLength = txBufferIndex;

  twi_initiateWriteTo(txAddress, txBuffer, txBufferLength);
  acc_status = ACC_WRITING;
}

void initiateTouchRequest(uint8_t address) {
  txBufferIndex = 0;
  txBufferLength = 0;
  uint8_t read = twi_initiateReadFrom(address, 1);
  acc_status = ACC_READING;
}

uint8_t finaliseTouchRequest() {
  uint8_t data;
  uint8_t read = twi_readMasterBuffer(rxBuffer, 1);
  rxBufferIndex = 0;
  rxBufferLength = read;  
  
  uint8_t i = 0;
  while (rxBufferLength - rxBufferIndex > 0) {         // device may send less than requested (abnormal)
    //Serial.println(rxBuffer[rxBufferIndex], BIN);
    data = rxBuffer[rxBufferIndex];
    ++rxBufferIndex;
    i++;
  }
  acc_status = ACC_IDLE;
  
  return data;
}

void updateControl() {
  uint8_t address;
  uint8_t port;
  int oscIndex = 0; 
  
  // Reset our index
  if (chipIndex == 0x0C) { // should be NUMBER_CHIPS *2 which is 0x0C
    chipIndex = 0x00;
  }
  
  address = chipIndex >> 1;
  port = INPUT_PORT0;
  if (chipIndex % 2 == 1) {
    port = INPUT_PORT1;
  } 
  
  switch(acc_status) {
    case ACC_IDLE:
      //Serial.print("The address:  ");
      //Serial.println(address);
      //Serial.print("The port:  ");
      //Serial.println(port);
      initiateTouchRead(address, port);
      break;
    case ACC_WRITING:
      if (twi_state != TWI_MRX) {
        initiateTouchRequest(address);
      }
      break;
    case ACC_READING:
      if (twi_state != TWI_MRX) {
        //Serial.print("The chip index:  ");
        //Serial.println(chipIndex);
        masterTouchData[chipIndex] = finaliseTouchRequest();
        Serial.println(masterTouchData[chipIndex], BIN);
        //masterTouchData[chipIndex] = conditionTouchData(masterTouchData[chipIndex], chipIndex); // this is WAY too slow now.  SIgh.  
        //Serial.println(masterTouchData[chipIndex], BIN);
        chipIndex = chipIndex + 0x01;
        }
      break;
    }

  // Play all the chips
  for (int i = 0; i < NUMBER_CHIPS * 2; i++) {
   oscIndex = playNotes(masterTouchData[i], oscIndex, pitchArray[i]);
  }

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
  // Debug audio test:
  //return oscs[0]->next() >> 3;
}

void loop() {
  audioHook();
}
