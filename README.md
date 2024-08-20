OverMyRoof headless NAP Application
=======================

# Description

This is a headless NAP application that uses flightradar24.com to track flights over a specific location. It uses the napdatabase and naprest module.

The application exposes a REST API to get the current and past flights over a specific location.

At given intervals, the application requests the flightradar24.com API to get the current flights over a specific bounding box location. The application stores the flights in the SQLITE databas.

## Build

Clone or extract the source of NAP (> 0.7).

OverMyRoof uses the module `napdatabase` from [here](https://github.com/TimGroeneboom/napdatabase/). Clone `napdatabase` into the directory of `nap/modules` and run `./tools/setup_module.sh napdatabase` from the NAP root folder

OverMyRoof uses the module `naprest` from [here](https://github.com/naivisoftware/naprest/). Clone `naprest` into the directory of `nap/modules` and run `./tools/setup_module.sh naprest` from the NAP root folder

Clone this repo into the apps/overmyroof directory. Add `add_subdirectory(apps/overmyrooof)` and `add_subdirectory(apps/overmyrooof/module)` to `CMakeLists.txt` in the NAP root folder.

