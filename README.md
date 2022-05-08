# stmdsp

The `stmdsp` project enables certain STM32 development boards to be used as a digital signal processing (DSP) education tool.

The project consists of two parts: the firmware, which allows users to upload custom DSP algorithms to process signals in real-time; and the GUI currently named [stmdspgui](https://code.bitgloo.com/clyne/stmdspgui), which facilitates algorithm design, upload, and execution, while also providing numerous analysis features.

The firmware for the device is written in C++, building on top of the [ChibiOS](https://www.chibios.org/dokuwiki/doku.php) real-time operating system.

## Features

* Real-time signal processing: the input channel is read from the ADC, and the processed output is sent out over the DAC.
* Signal sampling rates from 8kS/s up to 96kS/s.
* Custom algorithms, uploaded over USB at run-time, can be applied to the input signal.
* The second DAC channel can act as a signal generator, should no other input be available.
* Analysis features, including measuring algorithm execution time and logging input and output samples.

## Supported development boards

This project is aided by a custom add-on board, which is designed to stack on top of STMicroelectronic's NUCLEO line of dev boards.

At the moment, only the NUCLEO-L476RG board is fully supported.

## Programming instructions and more information

See the `doc` folder for further project documentation, including a PDF guide.
