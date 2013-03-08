/*  
 *  Working on the synthesis part of my arduino Tonnetz
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <Ead.h>
#include <tables/cos8192_int8.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type

#define CONTROL_RATE 128
#define NUMBER_OSCS 4

byte newButtons[NUMBER_OSCS];
byte oldButtons[NUMBER_OSCS];
Oscil<COS8192_NUM_CELLS, AUDIO_RATE> *oscs[NUMBER_OSCS];

// Oh god.  I am going to have to do this for the Oscils as well
Ead env1(CONTROL_RATE);
Ead env2(CONTROL_RATE);
Ead env3(CONTROL_RATE);
Ead env4(CONTROL_RATE);

Ead *envs[4] = {&env1, &env2, &env3, &env4};

int gains[NUMBER_OSCS];
unsigned int attack = 125;
unsigned int decay = 250;

void setup(){
  startMozzi(CONTROL_RATE); // a literal control rate here
  Serial.begin(9600);
  Serial.println("Hello World");
  
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
    oldButtons[i] = 0;
    
    Oscil<COS8192_NUM_CELLS, AUDIO_RATE> aCos(COS8192_DATA);
    // cue giant switch statement to give each osc a frequency
    switch (i) {
      case 0:
        aCos.setFreq(float(660.0));
        break;
      default: 
        aCos.setFreq(float(440.0));
        break;
    }
    
    oscs[i] = &aCos;
    gains[i] = 0;
  }
}

void loop(){
  audioHook();
}


void updateControl(){
  // OK, so this is envisioning that I have N oscillators just lying around waiting for their gains to be updated.
  
  //getFromTouchMagic(newButtons);

  // Dummy update function
  for (int i = 0; i < NUMBER_OSCS; i++) {
    if (i == 0) {
      newButtons[i] = 1;
    }
  }
  Serial.println("Read all newButtons");
  

  // Do this very fast
  for (int i = 0; i < NUMBER_OSCS; i++) {
    
    Serial.println(gains[i]);
    
    // If the note is not on, start it
    if (newButtons[i] == 1 && oldButtons[i] == 0) {
      Serial.println("Turn on");
      envs[i]->start(attack,decay);
      gains[i] = (int) envs[i]->next();
    }
    // If the note is still on, keep it on
    if (newButtons[i] == 1 && oldButtons[i] == 1) {
      if (gains[i] == 255){
        Serial.println("Leave alone");
        continue;
      } else {
        Serial.println("Turn up");
        gains[i] = (int) envs[i]->next();
      }
    }
    // If the note was on, stop it
    if (newButtons[i] == 0 && oldButtons[i] == 1) {
      Serial.println("Turn off");
      //gains[i] = (int) envs[i]->next();
    }
    
  oldButtons[i] = newButtons[i];
  }
  
  Serial.println("End of updateControl");
}


int updateAudio(){
  int asig = 0;
  for (int i = 0; i < NUMBER_OSCS; i++) {
    asig = asig + gains[i] * oscs[i]->next();
  }
  return asig >> 3;
}
