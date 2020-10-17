# stmdsp
This is the source code for an STM32-based DSP device. The primary goal of this device is to transform signals in real-time, through a GUI for writing and uploading C++ code to the device.  

The firmware for the device is written in C++, on top of the [ChibiOS](https://www.chibios.org/dokuwiki/doku.php) real-time operating system.

**Features:**
* Read in a signal from the ADC, and either pass-through or apply a filter to the signal before outputting  it over the DAC
* Sampling rate of 96kS/s
* Measuring of filter code in processor clock cycles

**Device features:**
* Read +/- 5V signal(s) off of at least one pin
* Send +/- 5V signal(s) off of at least one pin
* Communicate with a computer program to allow for the reading, writing, and transformation of signals.

See the wiki for more information about components of the device's software and hardware.

**Directory explanation:**
Source code for the device's firmware is in the `source` directory.  
Source code for the accompanying GUI is in `gui`.  
Notebook files for working with the device in Mathematica are in `mathematica`.
