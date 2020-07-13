# stmdsp
Code for an STM32-based DSP device.

**Goals:**
* Read +/- 5V signal(s) off of at least one pin
* Send +/- 5V signal(s) off of at least one pin
* Perform DSP calculations (e.g. filtering) on-device
* Communicate with a computer program to allow for the reading, writing, and transformation of signals.

See the wiki for pages on each of these goals.

Source code for the device's firmware is in the `source` directory. `gui` contains code for a custom GUI for the device, in C++.

Code for using the device with Mathematica will be added soon.
