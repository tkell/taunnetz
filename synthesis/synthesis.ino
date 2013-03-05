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
#define NUMBER_OSCS 48
#define ATTACK 150
#define DECAY 500

byte newButtons[NUMBER_OSCS];
byte oldButtons[NUMBER_OSCS];
Oscil<COS8192_NUM_CELLS, AUDIO_RATE>* oscs[NUMBER_OSCS]; // I doubt it
Ead* envs[NUMBER_OSCS];
int gains[NUMBER_OSCS];

void setup(){
  startMozzi(CONTROL_RATE); // a literal control rate here
  
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
    oldButtons[i] = 0;
    
    Oscil<COS8192_NUM_CELLS, AUDIO_RATE> aCos(COS8192_DATA);
    // cue giant switch statement to give each osc a frequency
    
    oscs[i] = &aCos;
    
    Ead env(CONTROL_RATE);
    envs[i] = &env;
    gains[i] = 0;
  }
  
}


void loop(){
  audioHook();
}


void updateControl(){
  // Gotta set newButtons.
  // OK, so this is envisioning that I have 48 oscillators just lying around waiting for their gains to be updated.
  // this means that I can set my oscillators up in setup, right?
  
  // newButtons = getFromTouchMagic();
  
  // Do this very fast
  for (int i = 0; i < NUMBER_OSCS; i++) {
    // If the note is not on, start it
    if (newButtons[i] == 1 && oldButtons[i] == 0) {
      envs[i]->start(250,500);
      gains[i] = envs[i]->next();
    }
    // If the note is still on, keep it on
    if (newButtons[i] == 1 && oldButtons[i] == 1) {
      if (gains[i] == 127){
        continue;
      } else {
        gains[i] = envs[i]->next();
      }
    }
    // If the note was on, stop it
    if (newButtons[i] == 0 && oldButtons[i] == 1) {
      gains[i] = envs[i]->next();
    }
    
  oldButtons[i] = newButtons[i];
  }
  
}


int updateAudio(){

  int asig = 0;
  for (int i = 0; i < NUMBER_OSCS; i++) {
    asig = asig + oscs[i]->next();
  }

  return asig >> 3;
}
