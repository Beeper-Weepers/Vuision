# Vuision
Arduino/Software based Synthesizer of user provided pitch and waveform. Played like a mix between a guitar, violin and piano.

![alt text](https://raw.githubusercontent.com/Beeper-Weepers/Vuision/main/WIPInstrument.jpg)

The Vuision is a synthesizer that uses an Arduino Due, three potentiometers, a speaker, and a few other components to create dynamic sound. It's closest acoustic relatives are a guitar/violin and a piano, however, it also shares similarities with many other potentiometer-based synthesizers.


# Construction and Usage
The Vuision is comprised of a large wooden slab with a deep cutout fir components in the lower bottom, a small channel for cable routing, and three potentiometers that measure position pressed, an arduinio Due, and breadborad circutry for the potentiometers and an audio output. The cutout has a slab of flat wood mounted onto it where the components are screwed in. Audio and power are housed outside of the instrument, however, a battery can be mounted on for compact usage. The synthesis is entirely software based uses Wavetable DDS (Direct Digital Sytnthesis)+Grain Additive Synthesis combined with the onboard clock timer to generate waveforms. Moving along each potentiometer modulates pitch, and more than one can be played at the same time.

![alt text](https://raw.githubusercontent.com/Beeper-Weepers/Vuision/main/components.jpg)

# Assembly
Materials are:

-Arduino Due 

-3 Softpot Membrane Potentiometers 

-Solderless breadboard 

-Wires, 10uF capacitor, a couple 5.1k resistors, a 500 ohm resistor, etc. [Needs change based off of configuration]

-Screws and nails [Needs changed based off of configuration] 

-USB powered speaker 

-Sparkfun 3.5mm audio jack breakout 

-Power bank 

-A wooden slab

Tools: 

-Soldering Iron 

-Drill 

-Electrical Tape 

-Wood Glue

-Something to carve channels (Routers are better, but we used a chisel)

# Credits
Designed and implemented by:

Maximo P.

Lukas V.

Alex B.

Ibrahim M.

Special thanks to: The Arduino Forums, RCArduino, Arduino, CrossRoads, and ard_newbie
