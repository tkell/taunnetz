/*  
 *  Working on the synthesis part of my arduino Tonnetz
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <Ead.h>
#include <tables/cos8192_int8.h>
#include <tables/sin2048_int8.h>

#define CONTROL_RATE 128
#define NUMBER_OSCS 5

byte newButtons[NUMBER_OSCS];
byte oldButtons[NUMBER_OSCS];

// Amazingly shitty need to declare everything by hand

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc4(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc5(SIN2048_DATA);

Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> *oscs[NUMBER_OSCS] = {&osc1, &osc2, &osc3, &osc4, &osc5};


void setup(){
  startMozzi(CONTROL_RATE);
  
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
    oldButtons[i] = 0;

    // cue giant switch statement to give each osc a frequency
    switch (i) {
      case 0:
        oscs[i]->setFreq(522u);
        break;
      case 1:
        oscs[i]->setFreq(586u);
        break;
      case 2:
        oscs[i]->setFreq(660u);
        break;
      case 3:
        oscs[i]->setFreq(784u);
        break;
      default: 
        oscs[i]->setFreq(880u);
        break;
    }
    
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
    if (i == 2) {
        newButtons[i] = 1;
      }      
  }
}


int updateAudio(){
  int asig = oscs[0]->next() * newButtons[0]
           + oscs[1]->next() * newButtons[1]
           + oscs[2]->next() * newButtons[2]
           + oscs[3]->next() * newButtons[3];
           + oscs[4]->next() * newButtons[4];

  return asig >> 1;
}
