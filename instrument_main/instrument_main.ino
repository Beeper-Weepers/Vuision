#include <math.h>
#include "waveform.h"

#define rate 36 //Rate at which to add to the i waveform counter; larger is faster, smaller is slower
#define rate2 1 //Rate at which to add to the i2 waveform counter used for envelopes ; larger is faster, smaller is slower
//#define oneHzSample 1000000/maxSamplesNum  // sample for the 1Hz signal expressed in microseconds 
#define oneHzSample 1000000/(maxSamplesNum/rate) //Uncomment this line and comment the above line for a more accurate oneHzSample

#define threshold 2 //Minimal detection range for producing sound

//Defines buttons
#define button0 2
#define button1 3
#define button2 4

uint32_t sinceLast = 0;

uint8_t buttonsPressed = 0; //Number of buttons being pressed, limit of 255 buttons
int16_t DACoutput; //Final output to the speakers after multiplication
int16_t TEMPoutput; //All additives go here, then they are check against the requirments

//If our maxSamplesNum is over 120, AKA one period of the waveform contains more than 255 samples, then move to uint16_t
uint8_t i = 0; //Position in the wave
uint8_t i2 = 0; //Envelope position in the wave

uint16_t sample; //For frequency modification
uint16_t currentRead; //Holds current analog input


void setup() {
  Serial.begin(9600);

  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  //Expands the noise table
  for (uint8_t e = 0; e < 120; e++) {
    waveformsTable[4][i] = map(waveformsTable[4][i], -127, 123, 0, 3000);
    waveformsTable[4][i] += 1000;
  }

  //amount of octaves to create
  for (uint8_t octaves = 0; octaves < maxOctave; octaves++) {
    //number of notes in an octave
    for (uint8_t notes = 0; notes < maxNote; notes++) {
      note_map[(octaves * maxNote) + notes] = 16.35 * pow(1.059463094359, (octaves * maxNote) + notes);
    }
  }

  analogWriteResolution(12);  // set the analog output resolution to 12 bit (4096 levels)
  analogReadResolution(12);   // set the analog input resolution to 12 bit
}

void loop() {
 //Every 17 milliseconds, aka highest it takes to run the loop, run through the body
 if (millis()%6==0) {
  // Read the the potentiometer and map the value  between the maximum and the minimum sample available
  
  // THE COMMENTS BELOW ARE INACCURATE
  // 1 Hz is the minimum freq for the complete wave
  // 170 Hz is the maximum freq for the complete wave. Measured considering the loop and the analogRead() time
  currentRead = constrain(analogRead(A0), 0, 4095);
  sample = (currentRead/4095)*oneHzSample;

  
  //Multiply the waveform
  TEMPoutput = 0;
  buttonsPressed = 0;
  if (digitalRead(button0) == HIGH) { //Multiply by Sine
    TEMPoutput += waveformsTable[0][i];
    buttonsPressed++;
  }
  if (digitalRead(button1) == HIGH) { //Multiply by Triangle
    TEMPoutput += waveformsTable[1][i];
    buttonsPressed++;
  }
  if (digitalRead(button2) == HIGH) { //Multiply by Saw
    TEMPoutput += waveformsTable[2][i];
    buttonsPressed++;
  }
  
  //If activated
  if (buttonsPressed > 0 && currentRead > threshold) {
    i += rate;
    if (i >= maxSamplesNum) { // Reset the counter to repeat the wave
      i = 0;
    }

    DACoutput = TEMPoutput / buttonsPressed; //Gets average of all the waveforms pressed

  } else { //If the pot output is below the threshold, null output
    DACoutput *= 0.9;
    i = 0;
  }

  //Debug information
  Serial.print(sample);
  Serial.print(" ");
  Serial.print(currentRead);
  Serial.print(" ");
  Serial.print(DACoutput);
  Serial.print(" ");

  analogWrite(DAC0, DACoutput);  // write the selected waveform on DAC0
  analogWrite(DAC1, DACoutput);  // write the selected waveform on DAC1

  delayMicroseconds(sample);  // Hold the sample value for the sample time
 }
  Serial.println(millis() - sinceLast);

  sinceLast = millis();
}

float retreiveNoteFreq(int freq) {
  //Works similarly to upper_bound in C++, basically returns lowest note next to frequency
  for (uint8_t a = 0; a < mapSize; a++) {
    if (note_map[a] < freq) {
      return note_map[a];
    }
  }
}
