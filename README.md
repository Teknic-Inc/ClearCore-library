# ClearCore-library 

This repository contains the ClearCore Motion and I/O Library, providing a foundation to build ClearCore applications. Also included are Atmel Studio example programs that demonstrate various features of the ClearCore, and an Atmel Studio template project that can be used to start building your own application.

#### Atmel Studio requirements

The included Atmel Studio projects require Atmel Studio version 7.0.1645 or later (latest version is recommended).

From the Atmel Studio Tools menu, open the Device Pack Manager. Ensure the following packs are installed:
* SAME53_DFP version 1.1.118
* CMSIS version 4.5.0

#### Installers and Resources

https://www.teknic.com/downloads/

### libClearCore

libClearCore provides a C++ object oriented API to interface with the ClearCore hardware. Each connector of the ClearCore has an associated object to use in your application. A Doxygen reference manual for the libClearCore API is available at https://teknic-inc.github.io/ClearCore-library/.

There is an Atmel Studio project file (*.cppproj) included to load and compile this library in Atmel Studio.

### LwIP

The ClearCore Ethernet implementation is based off of the LwIP stack. Ethernet applications should be developed using the ethernet API provided by libClearCore. The LwIP source code is included for completeness.

There is an Atmel Studio project file (*.cppproj) included to load and compile this library in Atmel Studio.

### Atmel_Examples

This folder contains example applications for a variety of ClearCore features. To run a provided example, first choose which subdirectory describes the feature that you want to run. Within each subdirectory is an Atmel solution file (*.atsln) that contains various examples related to that feature, as well as the required interface libraries. After the solution is loaded in Atmel Studio, browse for the project with the example that you wish to run within the solution explorer panel. Right click on the project and select "Set as Startup Project". 

The example programs are configured with a custom firmware loading script that will search for a connected ClearCore USB port and load the example programs on the ClearCore hardware. Simply click "Start Without Debugging (Ctrl+Alt+F5)" and the example program will compile, load the firmware, and start executing.

### Project Template
The Project Template directory is included as a starting point for writing your own application. Simply open the Atmel Studio solution file (*.atsln), and put your application code in main.cpp.

### Tools

We have included Windows tools for loading the firmware onto the ClearCore using the USB connector. 

**bossac** A command line flashing application

**flash_clearcore.cmd** A script that searches for a connected ClearCore USB port and uses bossac to load the firmware

**uf2-builder** Converts the compiled firmware binary file into a UF2 file that allows drag and drop flashing onto the bootloader's mass storage drive.
