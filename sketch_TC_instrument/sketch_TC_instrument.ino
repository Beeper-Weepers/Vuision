// RCArduino DDS Sinewave by RCArduino is licensed under a Creative Commons Attribution 3.0 Unported License. Based on a work at rcarduino.blogspot.com. 
// Uses the arduino Due (We have 521K flash and 96K ram to play with)

// For helpful background information on Arduino Due Timer Configuration, refer to the following link: http://arduino.cc/forum/index.php?action=post;topic=130423.15;num_replies=20
// For background information on the DDS Technique see: http://interface.khm.de/index.php/lab/experiments/arduino-dds-sinewave-generator/


//SECTION - INPUT & FREQUENCY

// 1 / grain count for more efficiency, multiplication is faster in the new value mapping
#define grains 3

uint32_t ulPhaseAccumulator = 0; // the phase accumulator points to the current sample in our wavetable
volatile uint32_t ulPhaseIncrement = 0;  // the phase increment controls the rate at which we move through the wave table (higher values = higher frequencies)
uint8_t ulInput = 0; // input from potentiometer (in the loop it is shifted from 12-bit to 7-bit)

//second potentiometer
uint32_t ulPhaseAccumulator2 = 0;
volatile uint32_t ulPhaseIncrement2 = 0; 
uint8_t ulInput2 = 0;

//third potentiometer
uint32_t ulPhaseAccumulator3 = 0;
volatile uint32_t ulPhaseIncrement3 = 0;
uint8_t ulInput3 = 0;

bool pressed;


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
#define SAMPLE_RATE 44100.0
#define SAMPLES_PER_CYCLE 600
#define SAMPLES_PER_CYCLE_FIXEDPOINT (SAMPLES_PER_CYCLE<<20)
#define TICKS_PER_CYCLE (float)((float)SAMPLES_PER_CYCLE_FIXEDPOINT/(float)SAMPLE_RATE)

// to represent 600 we need 10 bits
// Our fixed point format will be 10P22 = 32 bits

uint16_t ulOutput;

#include "table_header.h"


void setup()
{
  Serial.begin(9600);

  createNoteTable(SAMPLE_RATE);
  createSineTable();
  createRampTable();
  
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
  analogWrite(DAC1,0);
}

void loop()
{
  // read analog input 0 drop the range from 0-1024 to 0-127 with a right shift 3 places,
  // then look up the phaseIncrement required to generate the note in our nMidiPhaseIncrement table

  //Get shifted midi note input
  ulInput = analogRead(0)>>3;
  ulInput2 = analogRead(1)>>3;
  ulInput3 = analogRead(2)>>3;

  // identify if the potentiometers are pressed or not (saves on the checks in the interrupt timer)
  pressed = !(ulInput<1 && ulInput2<1 && ulInput3<1);
  Serial.println(pressed);
  Serial.print(ulOutput);
  
  //convert to a frequency
  ulPhaseIncrement = nMidiPhaseIncrement[ulInput]; 
  ulPhaseIncrement2 = nMidiPhaseIncrement[ulInput2];
  ulPhaseIncrement3 = nMidiPhaseIncrement[ulInput3];
}

void TC4_Handler()
{
  // We need to get the status to clear it and allow the interrupt to fire again
  TC_GetStatus(TC1, 1);

  ulPhaseAccumulator += ulPhaseIncrement; // 32 bit accumulator, overflow handling below
  ulPhaseAccumulator2 += ulPhaseIncrement2;
  ulPhaseAccumulator3 += ulPhaseIncrement3;

  // if the phase accumulator over flows - we have been through one cycle at the current pitch, now we need to reset the grains ready for our next cycle
  if(ulPhaseAccumulator > SAMPLES_PER_CYCLE_FIXEDPOINT) {
   // carry the remainder of the phase accumulator
   ulPhaseAccumulator -= SAMPLES_PER_CYCLE_FIXEDPOINT;
  }
  //second grain overflow
  if(ulPhaseAccumulator2 > SAMPLES_PER_CYCLE_FIXEDPOINT) {
   ulPhaseAccumulator2 -= SAMPLES_PER_CYCLE_FIXEDPOINT;
  }
  //third grain overflow
  if(ulPhaseAccumulator3 > SAMPLES_PER_CYCLE_FIXEDPOINT) {
    ulPhaseAccumulator3 -= SAMPLES_PER_CYCLE_FIXEDPOINT;
  }

  // get current samples for grains and add the grains together  
  ulOutput = nSineTable[ulPhaseAccumulator>>20] + nSineTable[(uint16_t) ((ulPhaseAccumulator2>>20) * ((float) nSineTable[ulPhaseAccumulator2>>20]/4095))]; //+ (nSineTable[ulPhaseAccumulator3>>20];
  // only problem is we have three grains so we can't bitshift (result is somewhere within 13-14 bits), and we must convert back to 12-bit
  // what this essentially is the equation (ulOutput / 12285) *4095 but simplified
  ulOutput = (ulOutput/grains)*pressed;
  
  // we cheated and user analogWrite to enable the dac, but here we want to be fast so write directly
  dacc_write_conversion_data(DACC_INTERFACE, ulOutput);
}
