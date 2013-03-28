// --------------------------------------
// i2c_scanner
//
// Version 1
//    This program (or code that looks like it)
//    can be found in many places.
//    For example on the Arduino.cc forum.
//    The original author is not know.
// Version 2, Juni 2012, Using Arduino 1.0.1
//     Adapted to be as simple as possible by Arduino.cc user Krodal
// Version 3, Feb 26  2013
//    V3 by louarnold
// Version 4, March 3, 2013, Using Arduino 1.0.3
//    by Arduino.cc user Krodal.
//    Changes by louarnold removed.
//    Scanning addresses changed from 0...127 to 1...119,
//    according to the i2c scanner by Nick Gammon
//    http://www.gammon.com.au/forum/?id=10896
// 
//
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.
//


#include <Wire.h>


// Joe's code
// MIDDLE CHIP's XRES pin is bad. Must fix tomorrow

int xres1 = 2;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 12
int xres2 = 3;  // XRES pin on one of the CY8C201xx chips is connected to Arduino pin 8

// I2C adresses
#define I2C_ADDR0 0x00
#define I2C_ADDR1 0x01

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


void configureChip(int address) {
  byte error;
  Serial.println("Configuring chip");
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
    Serial.print("Changing address:  ");
    Serial.println(error);
    
    //lock register again for change to take effect
    Wire.beginTransmission(currAddress);
    Wire.write(I2C_DEV_LOCK);
    Wire.write(I2CDL_KEY_LOCK, 3);
    Wire.endTransmission();
    // the I2C address is now newAddress
}

void setup()
{
  Wire.begin();
  Serial.begin(9600);
  Serial.println("\nI2C Scanner");

  // set reset pin modes
  pinMode(xres1, OUTPUT);
  pinMode(xres2, OUTPUT);
  delay(100);

  // put both reset mode
  digitalWrite(xres1, HIGH);
  delay(100);
  digitalWrite(xres2, HIGH);
  delay(100);

  // wake up chip 2 and change its address
  digitalWrite(xres2, LOW);
  delay(200);
  configureChip(I2C_ADDR0);
  changeAddress(I2C_ADDR0, I2C_ADDR1);
  
  // wake up chip 1
  digitalWrite(xres1, LOW);
  delay(200);
  configureChip(I2C_ADDR0);



  
  
}


void loop()
{
  byte error, address;
  int nDevices;
  
  
  
  nDevices = 0;
  for(address = 0; address < 128; address++ ) 
  {
    // Serial.println(address);
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(2000);           // wait 5 seconds for next scan
}
