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
    float fFrequency = ((pow(2.0,(unMidiNote-69.0)/12.0)) * 440.0);
    nMidiPhaseIncrement[unMidiNote] = fFrequency*TICKS_PER_CYCLE;
  }
}


//SECTION - WAVETABLES

// Create a table to hold pre computed sinewave, the table has a resolution of 600 samples
#define WAVE_SAMPLES 600
// default int is 32 bit, in most cases its best to use uint32_t but for large arrays its better to use smaller
// data types if possible, here we are storing 12 bit samples in 16 bit ints
uint16_t nSineTable[WAVE_SAMPLES];

uint16_t nRampTable[WAVE_SAMPLES];
#define LIFT 300

// create the individual samples for our sinewave table
void createSineTable()
{
  for(uint16_t nIndex = 0;nIndex < WAVE_SAMPLES;nIndex++)
  {
    // normalised to 12 bit range 0-4095
    nSineTable[nIndex] = (uint16_t)  (((1+sin(((2.0*PI)/WAVE_SAMPLES)*nIndex))*4095.0)/2);
    Serial.println(nSineTable[nIndex]);
  }
}

void createRampTable() {
  for(uint16_t nIndex = 0;nIndex < WAVE_SAMPLES;nIndex++)
  {
    // normalised to 12 bit range 0-4095
    nRampTable[nIndex] = (uint16_t) (((float) nIndex/WAVE_SAMPLES) * (4095.0 - LIFT)) + LIFT;
    Serial.println(nRampTable[nIndex]);
  }
}


#endif 
