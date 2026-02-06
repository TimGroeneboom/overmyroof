OverMyRoof headless NAP Application
=======================

# Description

This is a headless NAP application that uses flightradar24.com to track flights over a specific location. It uses the napdatabase and naprest module.

At given intervals, the application requests the flightradar24.com API to get the current flights over a specific bounding box location. The application stores the flights in the SQLITE database.

The application also uses the pro6pp API to get the coordinates a location based on the postal code. These coordinates are then cached and refreshed after a certain amount of time. https://www.pro6pp.nl

The application exposes a REST API to get the current and past flights over a specific location. This data is served as a JSON object.

## Requirements

- The application has been tested on Ubuntu 24.04 and 22.04 on x86_64 architecture.
- An internet connection is required to access the flightradar24.com API.
- Enough storage space to store the flight data in the SQLITE database.
- A valid pro6pp API key

## Install and run the application

- Download the latest release from the [releases page]()
- Unzip the downloaded file to a directory of your choice.
- Open a terminal and navigate to the directory where you unzipped the application.
- Replace the line in [data/pro6pp.key](data/pro6pp.key) with your pro6pp API key.
- Type `./overmyroof` to run the application.

## Endpoints

Two endpoints are available, examples:

### Find flights over a location
 
- `/find_flights?streetnumber_and_premise=202&postal_code=1118cp&altitude=10000&radius=4000&begin=20260206000000&end=20260207000000`

Finds flights over the location with the given street number and premise, postal code, altitude, radius, begin and end time. The altitude is in feet, the radius is in meters, the begin and end time are in the format `YYYYMMDDhhmmss`. The response is a JSON object with the following structure:

```json
{
  "status": "ok",
  "data": {
    "flights": [
      {
        "icao": "SWR883P",
        "reg": "SWR",
        "aircraft_type": "BCS3",
        "lat": 52.282,
        "lon": 4.7189,
        "altitude": 297.1799,
        "timestamp": 20260206085937,
        "distance": 3130.7316
      },
      {
        "icao": "EJU32KA",
        "reg": "EZY",
        "aircraft_type": "A20N",
        "lat": 52.283,
        "lon": 4.722,
        "altitude": 281.94,
        "timestamp": 20260206100050,
        "distance": 2917.3666
      }
    ],
    "ms": 0
  }
}
```

Instead of `streetnumber_and_premise` and `postal_code`, the endpoint also accepts `latitude` and `longitude` parameters to specify the location. If both are provided, the application will use the `latitude` and `longitude` parameters and skip the pro6pp API call.

### Find disturbances

- `/find_disturbances?streetnumber_and_premise=202&postal_code=1118cp&altitude=10000&radius=4000&begin=20260206000000&end=20260207000000&period=60&occurrences=2`

Finds disturbances over the location with the given street number and premise, postal code, altitude, radius, begin and end time, period and occurrences. The altitude is in feet, the radius is in meters, the begin and end time are in the format `YYYYMMDDhhmmss`, the period is in minutes and the occurrences is the number of times a flight has hit the given parameters to register as a disturbance. The response is a JSON object with the following structure:

```json
{
  "status": "ok",
  "data": {
    "disturbance_periods": [
      {
        "begin": 20260206100050,
        "end": 20260206100638,
        "flights": [
          {
            "icao": "EJU32KA",
            "reg": "EZY",
            "aircraft_type": "A20N",
            "lat": 52.283,
            "lon": 4.722,
            "altitude": 281.94,
            "timestamp": 20260206100050
          },
          {
            "icao": "KLM95P",
            "reg": "KLM",
            "aircraft_type": "B738",
            "lat": 52.318,
            "lon": 4.7719,
            "altitude": 220.9799,
            "timestamp": 20260206100211
          },
          {
            "icao": "KLM29A",
            "reg": "KLM",
            "aircraft_type": "B738",
            "lat": 52.317,
            "lon": 4.7639,
            "altitude": 198.1199,
            "timestamp": 20260206100333
          },
          {
            "icao": "EFD3T",
            "reg": "EFD",
            "aircraft_type": "C25B",
            "lat": 52.2849,
            "lon": 4.728,
            "altitude": 266.7,
            "timestamp": 20260206100537
          },
          {
            "icao": "KLM59M",
            "reg": "KLM",
            "aircraft_type": "A21N",
            "lat": 52.317,
            "lon": 4.7579,
            "altitude": 205.74,
            "timestamp": 20260206100638
          }
        ],
        "occurrences": 5
      }
    ],
    "ms": 2
  }
}
```

## Build from source

- Clone NAP from [here](https://github.com/TimGroeneboom/nap/) branch `overmyroof` into a directory of your choice.

- Make sure all requirements for NAP are installed. Follow the instructions in the NAP README to install the requirements. Finally, run `./check_build_environment.sh` from the NAP root folder to check if all requirements are installed.

- Create a folder `modules` in the NAP root directory if it does not exist.

- OverMyRoof uses the module `napdatabase` from [here](https://github.com/TimGroeneboom/napdatabase/). Clone `napdatabase` branch `overmyroof` into the directory of `nap/modules` and run `./tools/setup_module.sh napdatabase` from the NAP root folder.

- OverMyRoof uses the module `naprest` from [here](https://github.com/naivisoftware/naprest/). Clone `naprest` branch `overmyroof` into the directory of `nap/modules` and run `./tools/setup_module.sh naprest` from the NAP root folder

- Clone this repository branch `main` into the `apps/overmyroof`  directory. Add `add_subdirectory(apps/overmyrooof)` and `add_subdirectory(apps/overmyrooof/module)` to `CMakeLists.txt` in the NAP root folder.

