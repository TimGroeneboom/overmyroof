OverMyRoof headless NAP Application
=======================

# Description

This is a headless NAP application that uses flightradar24.com to track flights over a specific location. It uses the napdatabase and naprest module.

The application exposes a REST API to get the current and past flights over a specific location.

At given intervals, the application requests the flightradar24.com API to get the current flights over a specific bounding box location. The application stores the flights in the SQLITE database.

## Requirements

- The application has been tested on Ubuntu 24.04 and 22.04 on x86_64 architecture.
- An internet connection is required to access the flightradar24.com API.
- Enough storage space to store the flight data in the SQLITE database.
- A valid pro6pp API key

## Install and run the application

- Download the latest release from the [releases page]()
- Unzip the downloaded file to a directory of your choice.
- Open a terminal and navigate to the directory where you unzipped the application.
- Type `./overmyroof` to run the application.

## Build from source

- Clone NAP from [here](https://github.com/TimGroeneboom/nap/) branch `overmyroof` into a directory of your choice.

- Make sure all requirements for NAP are installed. Follow the instructions in the NAP README to install the requirements. Finally, run `./check_build_environment.sh` from the NAP root folder to check if all requirements are installed.

- Create a folder `modules` in the NAP root directory if it does not exist.

- OverMyRoof uses the module `napdatabase` from [here](https://github.com/TimGroeneboom/napdatabase/). Clone `napdatabase` branch `overmyroof` into the directory of `nap/modules` and run `./tools/setup_module.sh napdatabase` from the NAP root folder.

- OverMyRoof uses the module `naprest` from [here](https://github.com/naivisoftware/naprest/). Clone `naprest` branch `overmyroof` into the directory of `nap/modules` and run `./tools/setup_module.sh naprest` from the NAP root folder

- Clone this repository branch `main` into the `apps/overmyroof`  directory. Add `add_subdirectory(apps/overmyrooof)` and `add_subdirectory(apps/overmyrooof/module)` to `CMakeLists.txt` in the NAP root folder.

