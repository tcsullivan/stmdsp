# stmdsp

The *stmdsp* project enables certain STM32 development boards to be used as a digital signal processing (DSP) education tool. The solution is portable, and can be used without any external tools or lab equipment.

The project consists of three parts: the firmware, which allows users to upload custom DSP algorithms to process signals in real-time; the hardware, which provides the necessary circuitry for interfacing with signals and the host computer; and the computer software, a program currently named [stmdspgui](https://code.bitgloo.com/clyne/stmdspgui) that facilitates algorithm design and execution while also providing numerous analysis features.

## Features

* Real-time signal processing: signal readings from the ADC are streamed through the loaded algorithm and outputted over the DAC.
* Supports signal sampling rates from 8kS/s up to 96kS/s.
* Supports signals between -3.3V and +3.3V, with adequate protection for the development board.
* Custom algorithms are uploaded to the hardware at run-time, enabling a faster design and test process.
* Parameter knobs allow for algorithm adjustments while the algorithm is running.
* An on-board signal generator eliminates the need for external generators.
* Numerous analysis features, including signal visualization and measuring algorithm execution time, eliminate the need for other external tools.

## Learn more

See the [project's wiki](https://code.bitgloo.com/clyne/stmdsp/wiki/Home) for more details. The `doc` folder also contains add-on board schematics and a work-in-progress PDF guide (which may later be abandoned in favor of the wiki).

### Licensing

The *stmdsp*, *stmdspgui*, and ChibiOS projects are all licensed under version three of the GNU General Public License.
