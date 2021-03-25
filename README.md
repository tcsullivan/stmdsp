# stmdsp
This is the source code for an STM32-based DSP device. The primary goal of this device is to allow custom C++ algorithms to be applied to signals in real-time.

The firmware for the device is written in C++, on top of the [ChibiOS](https://www.chibios.org/dokuwiki/doku.php) real-time operating system.

**Features:**
* Read in a signal from the ADC, and either pass-through or apply a filter to the signal before outputting it over the DAC.
* Sampling rates of up to 96kS/s.
* Measuring of algorithm performance in processor clock cycles.
* Flexible signal generator for providing source signals.

See the wiki for more information about components of the device's software and hardware.

Source code for the device's firmware is in the `source` directory.  
Source code for the accompanying GUI is in `gui`.  
