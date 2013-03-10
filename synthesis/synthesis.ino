/*  
 *  Working on the synthesis part of my arduino Tonnetz
 */

#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <mozzi_midi.h>
#include <fixedMath.h> // for Q16n16 fixed-point fractional number type
#include <tables/cos8192_int8.h>
#include <tables/sin8192_int8.h>

#define CONTROL_RATE 128
#define NUMBER_OSCS 8

byte newButtons[NUMBER_OSCS];
byte oldButtons[NUMBER_OSCS];

// Amazingly shitty need to declare every os by hand
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc1(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc2(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc3(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc4(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc5(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc6(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc7(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc8(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc9(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc10(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc11(SIN8192_DATA);
Oscil <SIN8192_NUM_CELLS, AUDIO_RATE> osc12(SIN8192_DATA);

Oscil<SIN8192_NUM_CELLS, AUDIO_RATE> *oscs[NUMBER_OSCS] = {
    &osc1, &osc2, &osc3, &osc4, &osc5, &osc6, &osc7, &osc8
  };

EventDelay <CONTROL_RATE> eventDelay;
int noteIndex = 0;
Q16n16 frequency;

void setup() {
  startMozzi(CONTROL_RATE);
  Serial.begin(9600);
  eventDelay.set(1000); // 1 second countdown, within resolution of CONTROL_RATE
  
  
  
  for (int i = 0; i < NUMBER_OSCS; i++) {
    newButtons[i] = 0;
    oldButtons[i] = 0;
    
    // Trying to automate the frequency setting
    frequency = Q16n16_mtof(Q16n0_to_Q16n16(i + 71));
    Serial.println(frequency);
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
    asig = asig + oscs[i]->next() * newButtons[i];
  }

  return asig >> 1;
}
