// Cypress touch code via Joe, with deep thanks to Av, Ian, and Hackon for helping me with harwdware
// This is the non-Mozzi version:  it sends serial data to some sort of synthesis program, via a python bridge.


#include <Wire.h>
#define NUMBER_CONDITIONS 4 // 4 works well.  More than 16 ruins my day.  May cut this totally?
#define NOISE_THRESH 0x72 // 0x28 is factory default, 0x50 was working well, 0x72 was working great, 0xAA felt a little slow
#define NUMBER_CHIPS 6
#define NUMBER_TOUCHES 10

int touchCount;
String pitchArray[NUMBER_CHIPS][8];
String touchString = "";

// Touch code
int xres1 = 2;  
int xres2 = 3;  
int xres3 = 4;
int xres4 = 5;  
int xres5 = 6;  
int xres6 = 7;
byte conditionData[NUMBER_CHIPS][NUMBER_CONDITIONS];

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
#define CS_NOISE_TH 0x4E

// Secret codes for locking/unlocking the I2C_DEV_LOCK register
byte I2CDL_KEY_UNLOCK[3] = {0x3C, 0xA5, 0x69};
byte I2CDL_KEY_LOCK[3] = {0x96, 0x5A, 0xC3};

// Set a chip up so we can read its register
void configureChip(int address, int noise_thresh) {
  byte error;

  // switch to setup mode
  Wire.beginTransmission(address);
  Wire.write(COMMAND_REG);
  Wire.write(0x08);
  error = Wire.endTransmission();
  //Serial.print("Switched to setup mode:  ");
  //Serial.println(error);

  // setup CS_ENABLE0 register
  Wire.beginTransmission(address);
  Wire.write(CS_ENABLE0);
  Wire.write(B00001111);
  error = Wire.endTransmission();
  //Serial.print("Enabled R1:  ");
  //Serial.println(error);

  // setup CS_ENABLE1 register
  Wire.beginTransmission(address);
  Wire.write(CS_ENABLE1);
  Wire.write(B00001111);
  error = Wire.endTransmission();
  //Serial.print("Enabled R2:  ");
  //Serial.println(error);
    
  // Increase the noise threshold
  Wire.beginTransmission(address);
  Wire.write(CS_NOISE_TH);
  Wire.write(noise_thresh); // Factory default is 0x28
  error = Wire.endTransmission();
  //Serial.print("Increased Noise Threshold:  ");
  //Serial.println(error);

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
    //Serial.print("Changed address:  ");
    //Serial.println(error);
    
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
  configureChip(I2C_ADDR0, NOISE_THRESH);
  changeAddress(I2C_ADDR0, I2C_ADDR5);
  
  // wake up chip 5 and change its address
  digitalWrite(xres5, LOW);
  delay(200);
  configureChip(I2C_ADDR0, NOISE_THRESH);
  changeAddress(I2C_ADDR0, I2C_ADDR4);
  
  // wake up chip 4 and change its address
  digitalWrite(xres4, LOW);
  delay(200);
  configureChip(I2C_ADDR0, NOISE_THRESH);
  changeAddress(I2C_ADDR0, I2C_ADDR3);
  
  // wake up chip 3 and change its address
  digitalWrite(xres3, LOW);
  delay(200);
  configureChip(I2C_ADDR0, NOISE_THRESH);
  changeAddress(I2C_ADDR0, I2C_ADDR2);

  // wake up chip 2 and change its address
  digitalWrite(xres2, LOW);
  delay(200);
  configureChip(I2C_ADDR0, NOISE_THRESH - 0x40); // For my one chip with two shitty connections.  I think i've lost these
  changeAddress(I2C_ADDR0, I2C_ADDR1);
  
  // wake up chip 1
  digitalWrite(xres1, LOW);
  delay(200);
  configureChip(I2C_ADDR0, NOISE_THRESH);
  
  

  // Initialize the conditioning arrays
  for (int i = 0; i < NUMBER_CHIPS; i++) {
    for (int j = 0; j < NUMBER_CONDITIONS; j++) {
      conditionData[i][j] = 0;
    }  
  }
  
  touchCount = 0;
  // Initialize the pitch array here
  // Because I wired for short distance, these are out of order!  Sorry!!
  // 57 - 68
  pitchArray[0] = {"61", "65", "64", "64", "57", "61", "65", "57"};  // GOLD
  pitchArray[1] = {"68", "60", "63", "67", "68", "60", "63", "59"};  // GOLD
  pitchArray[2] = {"67", "59", "58", "62", "62", "66", "66", "58"};  // GOLD
  // 69 - 80
  pitchArray[3] = {"69", "77", "73", "73", "69", "77", "80", "76"}; // GOLD
  pitchArray[4] = {"72", "72", "76", "75", "79", "80", "79", "75"}; // GOLD
  pitchArray[5] = {"71", "71", "78", "74", "74", "70", "70", "78"}; // GOLD
}

void loop() {
  byte touchData = 0;
  byte mask;
  int index;
  touchCount = 0;
  touchString = "";
  
  // For 6 chips
  for (int i = 0; i < NUMBER_CHIPS; i++) {
    touchData = readTouch(i); // get the touch values from 1 x CY8C201xx chips - GP0 are the higher bits, GP1 the lower
    touchData = conditionTouchData(touchData, i);
    //Serial.println(touchData, BIN);
    
    // mask and send the pitch array midi numbers here
    index = 0;
    for (mask = 00000001; mask > 0; mask <<= 1) {
      //Serial.println(touchCount);
      if (touchCount >= NUMBER_TOUCHES) { 
        break;
      }
      if (touchData & mask) {
        touchString.concat(pitchArray[i][index]);
        touchString.concat(",");
        touchCount++;
        
      }
      index++;
    }
  }
  
  // Fill touchArray with zeros
  if (touchCount < NUMBER_TOUCHES) {
   for (int i = touchCount; i < NUMBER_TOUCHES; i++) {
     touchString.concat("00,");
   }
  }
  // Send over serial
  Serial.println(touchString);

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
