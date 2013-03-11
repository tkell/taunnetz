/*  
 *  Working on the synthesis part of my arduino Tonnetz
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <mozzi_midi.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type
#include <tables/sin2048_int8.h>

#define CONTROL_RATE 128
#define NUMBER_OSCS 12

byte newButtons[NUMBER_OSCS];
byte oldButtons[NUMBER_OSCS];

// Amazingly shitty need to declare every osc by hand
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc3(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc4(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc5(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc6(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc7(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc8(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc9(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc10(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc11(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> osc12(SIN2048_DATA);

Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> *oscs[NUMBER_OSCS] = {
    &osc1, &osc2, &osc3, &osc4, &osc5, &osc6, &osc7, &osc8, &osc9, &osc10, &osc11, &osc12
  };

EventDelay <CONTROL_RATE> eventDelay;
int noteIndex = 0;
Q16n16 frequency;

void setup() {
  startMozzi(CONTROL_RATE);
  eventDelay.set(1000); // 1 second countdown, within resolution of CONTROL_RATE
  
  
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
    oldButtons[i] = 0;
    
    // Trying to automate the frequency setting - would be nice to get this in semitones,..
    frequency = Q16n16_mtof(Q16n0_to_Q16n16(i + 71));
    oscs[i]->setFreq_Q16n16(frequency);
   
  }
}

void loop(){
  audioHook();
}

void updateControl(){
  // OK, so this is envisioning that I have N oscillators just lying around waiting for their gains to be updated.
  
  //realGetFromTouchFunction(newButtons);
    
  if(eventDelay.ready()){
      noteIndex = (noteIndex + 1) % NUMBER_OSCS;
      eventDelay.start();
    }
  
 
  // Dummy update function
  for (int i = 0; i < NUMBER_OSCS; i++) {
    if (i == noteIndex) {
        newButtons[i] = 1;
      }
    else {
      newButtons[i] = 0;
    }
    oldButtons[i] = newButtons[i];
  }
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
