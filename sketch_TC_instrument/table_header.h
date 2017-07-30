#ifndef _Waveforms_h_
#define _Waveforms_h_


//SECTION - MIDI NOTES AND TABLE


// Create a table to hold the phase increments we need to generate midi note frequencies at our 44.1Khz sample rate
#define MIDI_NOTES 128
uint32_t nMidiPhaseIncrement[MIDI_NOTES];

// fill the note table with the phase increment values we require to generate the note
void createNoteTable(float fSampleRate)
{
  for(uint32_t unMidiNote = 0;unMidiNote < MIDI_NOTES;unMidiNote++)
  {
    // Correct calculation for frequency
    float fFrequency = ((pow(2.0,((unMidiNote/2.0)-35.0)/12.0)) * 440.0);
    nMidiPhaseIncrement[unMidiNote] = fFrequency*TICKS_PER_CYCLE;
  }
}


//SECTION - WAVETABLES

#define WAVE_SAMPLES 600 // Create a table to hold pre computed sinewave, the table has a resolution of 600 samples


// storing 12 bit samples in 16 bit int arrays
uint16_t nSineTable[WAVE_SAMPLES];
uint16_t nTriangleTable[WAVE_SAMPLES];
uint16_t nRampTable[WAVE_SAMPLES];
uint16_t nSquareTable[WAVE_SAMPLES];


//Sin
void createSineTable() {
  float chunk = (2.0*PI)/WAVE_SAMPLES; //saves on precomputation by computing this chunk once instead of 120 times
  for(uint16_t nIndex = 0;nIndex < WAVE_SAMPLES;nIndex++) {
    // normalised to 12 bit range 0-4095
    nSineTable[nIndex] = ((1+sin(chunk*nIndex))*3071.0)/2; //fundamental, 3/4ths volume
    nSineTable[nIndex] += ((1+sin(chunk*nIndex*3))*1023.0)/2; //another harmonic, with 3 periods and 1/4th volume
  }
}


//Saw
#define LIFT 100.0

void createRampTable() {
  for(uint16_t nIndex = 0;nIndex < WAVE_SAMPLES ;nIndex++)
  {
    // normalised to 12 bit range 0-4095
     nRampTable[nIndex] =  ((float) nIndex/WAVE_SAMPLES ) * (4095 - LIFT) + LIFT;
  }
}

//Tri
#define MID_POINT (WAVE_SAMPLES/2)

void createTriangleTable() {
  for(uint16_t nIndex = 0;nIndex < WAVE_SAMPLES ;nIndex++)
  {
    // normalised to 12 bit range 0-4095
    if (nIndex < MID_POINT) {
      nTriangleTable[nIndex] = ( (float) nIndex/MID_POINT ) * 4095;
    } else {
      nTriangleTable[nIndex] = ( (float) (MID_POINT-(nIndex-MID_POINT))/MID_POINT ) * 4095;
    }
  }
}

//Square
//We can just reuse MID_POINT
void createSquareTable() {
  for(uint16_t nIndex = 0; nIndex < WAVE_SAMPLES; nIndex++) {
    if (nIndex < MID_POINT) {
      nSquareTable[nIndex] = 0;
    } else {
      nSquareTable[nIndex] = 4095;
    }
  }
}


//SECTION - ENVELOPES
//Basically, I love OOP and I try to fake it


//Envelope model
struct Envelope {
  volatile uint32_t ulPhaseAccumulator = 0; // the phase accumulator points to the current sample in our wavetable
  uint32_t ulPhaseIncrement = 0; // rate control, 64 is middle of the MIDI spectrum (0-128)
  float nTable[WAVE_SAMPLES];
};

//Sets the envelope rate (constructor)
void setEnvelopeRate(Envelope *env, int frequency) {
 env->ulPhaseIncrement = frequency;
}

//Updates an envelope, inline because it is called often
inline void envelopeUpdate(Envelope *env) {
 env->ulPhaseAccumulator += env->ulPhaseIncrement;
 if (env->ulPhaseAccumulator > SAMPLES_PER_CYCLE_FIXEDPOINT) { //envelope overflow
   env->ulPhaseAccumulator -= SAMPLES_PER_CYCLE_FIXEDPOINT; }
}

//Puts the sine wave into the table of the envelope
void envelopeTableFill(Envelope *env,uint16_t *wav) {
 for(uint16_t nIndex = 0;nIndex < WAVE_SAMPLES;nIndex++) {
   //Take value from sine table to construct a normalized table
   env->nTable[nIndex] = (float) wav[nIndex] / 4095.0;
 }
}

#endif 
