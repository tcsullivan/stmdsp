# stmdsp

The *stmdsp* project enables certain [NUCLEO development boards](https://www.st.com/en/evaluation-tools/stm32-nucleo-boards.html) to be used as an educational tool for digital signal processing (DSP). The solution is portable, and can be used without any external tools or lab equipment.

The project consists of three parts:
1. Firmware that allows users to upload custom DSP algorithms to process signals in real-time.
2. A [custom add-on board](https://code.bitgloo.com/clyne/stmdsp/wiki/DSP-add-on-board) which provides the necessary circuitry for interfacing with signals and the host computer.
3. [Computer software](https://code.bitgloo.com/clyne/stmdspgui) that facilitates algorithm design and execution while also providing numerous analysis features.

## Features

* Real-time signal processing: signal readings from the ADC are streamed through the loaded algorithm binary and out the DAC.
* Custom algorithms are uploaded to the hardware at run-time, enabling a fast design and test process.
* Supports signal sampling rates from 8kS/s up to 96kS/s, with buffer size of up to 4,096 samples.
* Supports signals between -3.3V and +3.3V, with adequate protection for the development board.
* Two parameter knobs allow for algorithm adjustments while the algorithm is running.
* An on-board signal generator eliminates the need for inputs from external hardware.
* Numerous analysis features, including signal visualization and algorithm execution time measurement, eliminate the need of other equipment such as oscilloscopes.

## Learn more

See the [project's wiki](https://code.bitgloo.com/clyne/stmdsp/wiki/Home) for more details. The `doc` folder also contains add-on board schematics and a work-in-progress PDF guide (which may later be abandoned in favor of the wiki).

### Licensing

The *stmdsp*, *stmdspgui*, and ChibiOS projects are all licensed under version three of the GNU General Public License.
