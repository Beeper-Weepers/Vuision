// RCArduino DDS Sinewave by RCArduino is licensed under a Creative Commons Attribution 3.0 Unported License. Based on a work at rcarduino.blogspot.com. 
// Uses the arduino Due (We have 521K flash and 96K ram to play with)

// For helpful background information on Arduino Due Timer Configuration, refer to the following link: http://arduino.cc/forum/index.php?action=post;topic=130423.15;num_replies=20
// For background information on the DDS Technique see: http://interface.khm.de/index.php/lab/experiments/arduino-dds-sinewave-generator/


//SECTION - INPUT & FREQUENCY

// Amount of grains/inputs being used
#define GRAINS 3
#define INTERPOL_AMOUNT 0.6;

//potentiomter model
struct Potentiometer {
  volatile uint32_t ulPhaseAccumulator = 0; // the phase accumulator points to the current sample in our wavetable
  uint32_t ulPhaseIncrement = 0; // the phase increment controls the rate at which we move through the wave table (higher values = higher frequencies)
  uint8_t ulInput = 0; // input from potentiometer (in the loop it is shifted from 12-bit to 7-bit, so this is fine)
  boolean lastPressed = false;
};

// Pot models for each pot - basically a rough model for variables assigned to each pot
struct Potentiometer Pot1;
struct Potentiometer Pot2;
struct Potentiometer Pot3;

bool pressed; // if any of the pots are being pressed



//SECTION - SAMPLE RATE & CLOCK TIMER

// These are the clock frequencies available to the timers /2,/8,/32,/128
// 84Mhz/2 = 42.000 MHz
// 84Mhz/8 = 10.500 MHz
// 84Mhz/32 = 2.625 MHz
// 84Mhz/128 = 656.250 KHz
// 
// 44.1Khz = CD Sample Rate (aim for closest to 44.1Khz)
//
// 42Mhz/44.1Khz = 952.38
// 10.5Mhz/44.1Khz = 238.09 // best fit divide by 8 = TIMER_CLOCK2 and 238 ticks per sample 
// 2.625Hmz/44.1Khz = 59.5
// 656Khz/44.1Khz = 14.88
//
// 84Mhz/44.1Khz = 1904 instructions per tick

// full waveform = 0 to SAMPLES_PER_CYCLE
// Phase Increment for 1 Hz =(SAMPLES_PER_CYCLE_FIXEDPOINT/SAMPLE_RATE) = 1Hz
// Phase Increment for frequency F = (SAMPLES_PER_CYCLE/SAMPLE_RATE)*F
// to represent 600 we need 10 bits
// Our fixed point format will be 10P22 = 32 bits
#define SAMPLE_RATE 44100.0
#define SAMPLES_PER_CYCLE 600
const uint32_t SAMPLES_PER_CYCLE_FIXEDPOINT = (SAMPLES_PER_CYCLE<<20);
const float TICKS_PER_CYCLE = (float)((float)SAMPLES_PER_CYCLE_FIXEDPOINT/(float)SAMPLE_RATE);

//Final output
volatile uint16_t ulOutput;



//MIDI notes, Waveforms, envelopes, etc.
#include "table_header.h"



//SECTION - ENVELOPE (EXT)

struct Envelope Env1;
volatile float fade = 1; //fade envelope
#define attack_add 0.2 //how much higher the attack peak is than the normal peak (like this for performance)
boolean attack = true;



void setup()
{
  Serial.begin(9600); //begins debug connection

  createNoteTable(SAMPLE_RATE); //generates midi notes
 
  //wavetables
  createRampTable();
  createSquareTable();
  createSineTable();

  //envelope wavetable
  envelopeTableFill(&Env1, nSineTable);
  setEnvelopeRate(&Env1, ((pow(2.0,((-2)-69.0)/12.0)) * 440.0) * TICKS_PER_CYCLE);

  //Timer and DAC
  
  /* turn on the timer clock in the power management controller */
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk(ID_TC4);

  /* we want wavesel 01 with RC */
  TC_Configure(/* clock */TC1,/* channel */1, TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC | TC_CMR_TCCLKS_TIMER_CLOCK2);
  TC_SetRC(TC1, 1, 238); // sets <> 44.1 Khz interrupt rate
  TC_Start(TC1, 1);
  
  // enable timer interrupts on the timer 
  TC1->TC_CHANNEL[1].TC_IER=TC_IER_CPCS;
  TC1->TC_CHANNEL[1].TC_IDR=~TC_IER_CPCS;
  
  /* Enable the interrupt in the nested vector interrupt controller */
  /* TC4_IRQn where 4 is the timer number * timer channels (3) + the channel number (=(1*3)+1) for timer1 channel1 */
  NVIC_EnableIRQ(TC4_IRQn);

  // this is a cheat - enable the DAC
  analogWriteResolution(12); 
  analogWrite(DAC1,0);
}

void loop()
{
  // read analog input 0 drop the range from 0-1024 to 0-127 with a right shift 3 places,
  // then look up the phaseIncrement required to generate the note in our nMidiPhaseIncrement table

  //Get shifted midi note input
  uint8_t rawIn = analogRead(0)>>3;
  Pot1.ulInput += (rawIn - Pot1.ulInput) * INTERPOL_AMOUNT;
  rawIn = analogRead(1)>>3; //reuse variable
  Pot2.ulInput += (rawIn - Pot2.ulInput) * INTERPOL_AMOUNT;
  rawIn = analogRead(2)>>3; //same thing here
  Pot3.ulInput += (rawIn - Pot3.ulInput) * INTERPOL_AMOUNT;

  // identify if the potentiometers are pressed or not (saves on the checks in the interrupt timer)
  pressed = Pot1.ulInput>1 || Pot2.ulInput>1 || Pot3.ulInput>1;
  if (pressed) {
    Pot1.lastPressed = Pot1.ulInput>1;
    Pot2.lastPressed = Pot2.ulInput>1;
    Pot3.lastPressed = Pot3.ulInput>1;
  }
  
  //convert to a frequency
  Pot1.ulPhaseIncrement = nMidiPhaseIncrement[Pot1.ulInput]; 
  Pot2.ulPhaseIncrement = nMidiPhaseIncrement[Pot2.ulInput];
  Pot3.ulPhaseIncrement = nMidiPhaseIncrement[Pot3.ulInput];

  // ADSR envelope update
  if (pressed) { 
   float fadePoint = 0.8 + (attack * attack_add); //Calculation point to interpolate to
   //Transition to Decay and Sustain
   if (fade >= (0.8+attack_add) - 0.02) {
    attack = false;
   }
   fade += (fadePoint - fade) * 0.2; //Interpolation
  } else { //fade out
   fade *= 0.6;
   attack = (fade <= 0.3);
  }

  Serial.println(attack);
}


//Where the music is created
void TC4_Handler()
{
  // We need to get the status to clear it and allow the interrupt to fire again
  TC_GetStatus(TC1, 1);
  

 //Envelope 1 update
 envelopeUpdate(&Env1);
 float volEnv = Env1.nTable[Env1.ulPhaseAccumulator>>20];

  // 32 bit accumulators update
  Pot1.ulPhaseAccumulator += Pot1.ulPhaseIncrement; 
  Pot2.ulPhaseAccumulator += Pot2.ulPhaseIncrement;
  Pot3.ulPhaseAccumulator += Pot3.ulPhaseIncrement;

   // if the phase accumulator over flows - we have been through one cycle at the current pitch, now we need to reset the grains ready for our next cycle
  if(Pot1.ulPhaseAccumulator > SAMPLES_PER_CYCLE_FIXEDPOINT) {
   Pot1.ulPhaseAccumulator -= SAMPLES_PER_CYCLE_FIXEDPOINT; } // carry the remainder of the phase accumulator 
  if(Pot2.ulPhaseAccumulator > SAMPLES_PER_CYCLE_FIXEDPOINT) { //second grain overflow
   Pot2.ulPhaseAccumulator -= SAMPLES_PER_CYCLE_FIXEDPOINT; }
  if(Pot3.ulPhaseAccumulator > SAMPLES_PER_CYCLE_FIXEDPOINT) { //third grain overflow
    Pot3.ulPhaseAccumulator -= SAMPLES_PER_CYCLE_FIXEDPOINT; }
 
  // get current samples for grains and add the grains together
  ulOutput = ((nSineTable[Pot1.ulPhaseAccumulator>>20]*Pot1.lastPressed + nSquareTable[Pot2.ulPhaseAccumulator>>20]*Pot2.lastPressed) / 2) * volEnv * fade;

  // we cheated and use analogWrite to enable the dac, but here we want to be fast so write directly
   dacc_write_conversion_data(DACC_INTERFACE, ulOutput);
}
