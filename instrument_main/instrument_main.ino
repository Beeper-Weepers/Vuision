#include "waveform.h"

#define oneHzSample 1000000/maxSamplesNum  // sample for the 1Hz signal expressed in microseconds 

const int button0 = 2, button1 = 3, button2 = 4; //Button pins
int rate = 36; //Rate at which to add to the i waveform counter; larger is faster, smaller is slower
int rate2 = rate / 3;
int threshold = 1; //Minimal detection range for producing sound
int lowerBound = 10; //Lowest value the potentiometer can produce while pressed. Input is dragged here during extraction of sample.

int buttonsPressed = 0; //Number of buttons being pressed
int DACoutput; //Final output to the speakers after multiplication
int TEMPoutput; //All additives go here, then they are check against the requirments
int i = 0;  //Position in the wave
int i2 = 0; //Envelope position in the wave

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
  TEMPoutput = 0; //Reset multiplication variable
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
  if (buttonsPressed>0 && currentRead>threshold) {
    i+=rate;
    if (i >= maxSamplesNum) { // Reset the counter to repeat the wave
      i = 0;
    }

    //ENVELOPE
    /*
    i2+=rate2;
    if (i2 >= maxSamplesNum) {
      i2=0;
    }
    
    TEMPoutput += waveformsTable[1][i2];
    buttonsPressed += 1; */
    //ENVELOPE END
    
    DACoutput = TEMPoutput / buttonsPressed; //Gets average of all the waveforms pressed
    
  } else { //If the pot output is below the threshold, null output
    DACoutput*=0.9;
    i=0;   
  }
  
  //Debug information
  Serial.print(sample);
  Serial.print(" ");
  Serial.print(currentRead-lowerBound);
  Serial.print(" ");
  Serial.println(DACoutput);
  
  analogWrite(DAC0, DACoutput);  // write the selected waveform on DAC0
  analogWrite(DAC1, DACoutput);  // write the selected waveform on DAC1

  delayMicroseconds(sample);  // Hold the sample value for the sample time
}
