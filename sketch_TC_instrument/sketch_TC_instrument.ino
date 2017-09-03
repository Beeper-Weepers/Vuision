// RCArduino DDS Sinewave by RCArduino is licensed under a Creative Commons Attribution 3.0 Unported License. Based on a work at rcarduino.blogspot.com. 
// Uses the arduino Due (We have 521K flash and 96K ram to play with)

// For helpful background information on Arduino Due Timer Configuration, refer to the following link: http://arduino.cc/forum/index.php?action=post;topic=130423.15;num_replies=20
// For background information on the DDS Technique see: http://interface.khm.de/index.php/lab/experiments/arduino-dds-sinewave-generator/

#define _max(a,b) ((a)>(b)?(a):(b));

/*These are the clock frequencies available to the timers /2,/8,/32,/128
 84Mhz/2 = 42.000 MHz
 84Mhz/8 = 10.500 MHz
 84Mhz/32 = 2.625 MHz
 84Mhz/128 = 656.250 KHz
 
 44.1Khz = CD Sample Rate (aim for closest to 44.1Khz)

 42Mhz/44.1Khz = 952.38
 10.5Mhz/44.1Khz = 238.09 // best fit divide by 8 = TIMER_CLOCK2 and 238 ticks per sample 
 2.625Hmz/44.1Khz = 59.5
 656Khz/44.1Khz = 14.88

 84Mhz/44.1Khz = 1904 instructions per tick

 full waveform = 0 to SAMPLES_PER_CYCLE
 Phase Increment for 1 Hz =(SAMPLES_PER_CYCLE_FIXEDPOINT/SAMPLE_RATE) = 1Hz
 Phase Increment for frequency F = (SAMPLES_PER_CYCLE/SAMPLE_RATE)*F
 to represent 600 we need 10 bits
 Our fixed point format will be 10P22 = 32 bits*/
#define SAMPLE_RATE 44100.0
#define SAMPLES_PER_CYCLE 600
const uint32_t SAMPLES_PER_CYCLE_FIXEDPOINT = (SAMPLES_PER_CYCLE<<20);
const float TICKS_PER_CYCLE = (float)((float)SAMPLES_PER_CYCLE_FIXEDPOINT/(float)SAMPLE_RATE);

//Final output
volatile uint16_t ulOutput;


//SECTION - INPUT & FREQUENCY/

// Amount of grains/inputs being used
#define GRAINS 3
#define INTERPOL_AMOUNT 0.6;

class Oscillator {
  protected:
  volatile uint32_t ulPhaseAccumulator = 0; // the phase accumulator points to the current sample in our wavetable
  uint32_t ulPhaseIncrement = 0; // the phase increment controls the rate at which we move through the wave table (higher values = higher frequencies)

  public:
  void updatePhaseAccumulator() {     // 32 bit accumulator update
    ulPhaseAccumulator += ulPhaseIncrement; 
    
    // if the phase accumulator over flows - we have been through one cycle at the current pitch, now we need to reset the grains ready for our next cycle
    if(ulPhaseAccumulator > SAMPLES_PER_CYCLE_FIXEDPOINT) {
      ulPhaseAccumulator -= SAMPLES_PER_CYCLE_FIXEDPOINT; // carry the remainder of the phase accumulator 
    }  
  }

  uint16_t paValue(uint16_t *table) {
    return table[Oscillator::ulPhaseAccumulator >> 20];
  }
};


//MIDI notes, Waveforms, envelopes, etc.
#include "table_header.h"


//Potentiometer class
class Potentiometer : public Oscillator {
  private:
  uint8_t ulInput = 0; // input from potentiometer (in the loop it is shifted from 12-bit to 7-bit, so this is fine)
  
  public:
  uint8_t rawInput = 0; //Raw, uninterpolated input from pot
  bool lastPressed = false; //if last pressed
  
  void updateInputs(uint8_t pin) {
    // read analog input 0 drop the range from 0-1024 to 0-127 with a right shift 3 places,
    // then look up the phaseIncrement required to generate the note in updatePressed()
    
    //Get shifted midi note input (128 notes)
    rawInput = analogRead(pin)>>3;

    //Interpolate to rawInput amount
    ulInput += (rawInput - ulInput) * INTERPOL_AMOUNT;
  }

  public:
  void updatePressed() {
    // If pressed, update lastPressed and use the interplated value. If not, use the lastPressed value.
    // This works because our ADSR envelope (the variable fade) works independently of the pitch.
    
    ulPhaseIncrement = nMidiPhaseIncrement[ulInput]; //Convert interpolated midi note to a frequency
    lastPressed = rawInput>0; //Update potentiometer pressed
  }

  uint16_t outputValue(uint16_t *table) {
    return paValue(table) * lastPressed;
  }
};


// Pot models for each pot - basically a rough model for variables assigned to each pot
Potentiometer Pot1;
Potentiometer Pot2;
Potentiometer Pot3;

bool pressed; // if any of the pots are being pressed

//Envelope
Envelope Env1;

//ADSR
double fade = 1; //fade envelope
#define attack_add 0.2 //how much higher the attack peak is than the normal peak (like this for performance)
boolean attack = true;

unsigned int delta = millis();

void setup()
{
  Serial.begin(9600); //begins debug connection

  createNoteTable(); //generates midi notes
 
  //wavetables
  createRampTable();
  createSquareTable();
  createSineTable();

  //envelope wavetable
  Env1.constructor(pow(2.0,(-2-69)/12.0) * 440.0 * TICKS_PER_CYCLE, nSineTable);

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
  //analogWriteResolution(12);
  analogWrite(DAC1,0);
}

void loop()
{ 
  Pot1.updateInputs(0);
  Pot2.updateInputs(1);
  Pot3.updateInputs(2);

  // identify if the potentiometers are pressed or not (saves on the checks in the interrupt timer)
  pressed = Pot1.rawInput>0 || Pot2.rawInput>0 || Pot3.rawInput>0;

  if (pressed) {
    //convert to a frequency and push interpolated value to increment
    Pot1.updatePressed();
    Pot2.updatePressed();
    Pot3.updatePressed();
  }

  // ADSR envelope update
  if (pressed) {
   float fadePoint = 0.8 + (attack * attack_add); //Calculation point to interpolate to
   //Transition to Decay and Sustain
   if (fade >= (0.8 + attack_add) - 0.01) {
    attack = false;
   }
   fade += (fadePoint - fade) * 0.2; //Interpolation
  } else { //fade out
   fade *= 1.0 - (12.0 * ((millis() - delta) / 1000.0));
   attack = (fade <= 0.6);
  }
  delta = millis(); //update delta time

  //Debug
  Serial.print(fade);
  Serial.print(" ");
  Serial.println(Pot1.lastPressed);
}


//Where the music is created
void TC4_Handler()
{
  // We need to get the status to clear it and allow the interrupt to fire again
  TC_GetStatus(TC1, 1);

  
  //Envelope 1 update
  Env1.updatePhaseAccumulator();
  float volEnv = 1.0; //Env1.nTable[Env1.getPAIndex()];

  //Update the oscilator's phase accumulators
  Pot1.updatePhaseAccumulator();
  Pot2.updatePhaseAccumulator();
  Pot3.updatePhaseAccumulator();
 
  // get current samples for grains, add the grains together, apply envelopes and apply ADSR
  ulOutput =
    ((Pot1.outputValue(nSineTable) + Pot2.outputValue(nSineTable)) / 2)
      * volEnv * fade;

  // we cheated and use analogWrite to enable the dac, but here we want to be fast so write directly
   dacc_write_conversion_data(DACC_INTERFACE, ulOutput);
}

