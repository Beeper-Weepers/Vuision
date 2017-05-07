#include "waveform.h"

#define oneHzSample 4000000/maxSamplesNum  // sample for the 1Hz signal expressed in microseconds 

const int button0 = 2, button1 = 5, button2 = 7; //Button pins
int rate = 18.35; //Rate at which to add to the i waveform counter; larger is faster, smaller is slower
int threshold = 45; //Minimal detection range for producing sound
int lowerBound = 195; //Lowest value the potentiometer can produce while pressed. Input is dragged here during extraction of sample.

int buttonsPressed = 0; //Number of buttons being pressed
float DACoutput; //Final output to the speakers after multiplication
int i = 0;  //Position in the wave
int sample; //For frequency modification
int currentRead; //Holds current analog input


void setup() {
  Serial.begin(9600);

  pinMode(button0, INPUT);
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);

  //Expands the noise table
  for (int i = 0; i < 120; i++) {
   waveformsTable[4][i] = map(waveformsTable[4][i],-127,123,0,1000);
   waveformsTable[4][i] += 3000;
  }
  
  analogWriteResolution(12);  // set the analog output resolution to 12 bit (4096 levels)
  analogReadResolution(12);   // set the analog input resolution to 12 bit 
}

void loop() {
  // Read the the potentiometer and map the value  between the maximum and the minimum sample available
  // 1 Hz is the minimum freq for the complete wave
  // 170 Hz is the maximum freq for the complete wave. Measured considering the loop and the analogRead() time

  currentRead = analogRead(A0);
  sample = map(currentRead-lowerBound, 0, 4095, 0, oneHzSample);
  sample = constrain(sample, 0, oneHzSample);

  
  //Multiply the waveform
  buttonsPressed = 0; //Reset button counter
  DACoutput = 0; //Reset audio output
  if (digitalRead(button0) == HIGH) { //Multiply by Sine
    DACoutput += waveformsTable[0][i];
    buttonsPressed++;
  }
  if (digitalRead(button1) == HIGH) { //Multiply by Triangle 
    DACoutput += waveformsTable[1][i];
    buttonsPressed++;
  }
  if (digitalRead(button2) == HIGH) { //Multiply by Saw
    DACoutput += waveformsTable[2][i];
    buttonsPressed++;
  }
  DACoutput /= max(buttonsPressed,1); //Can't divide by zero, so we just divide by one instead. Gets average of all the waveforms pressed

  //If the pot output is below the threshold, null output
  if (currentRead<threshold) {
    DACoutput=0;
  }
  
  //Debug information
  Serial.print(sample);
  Serial.print(" ");
  Serial.print(currentRead);
  Serial.print(" ");
  Serial.print(DACoutput);
  Serial.println(" ");
  
  analogWrite(DAC0, DACoutput);  // write the selected waveform on DAC0
  analogWrite(DAC1, DACoutput);  // write the selected waveform on DAC1

  i+=rate;
  if(i >= maxSamplesNum)  // Reset the counter to repeat the wave
    i = 0;

  delayMicroseconds(sample);  // Hold the sample value for the sample time
}
