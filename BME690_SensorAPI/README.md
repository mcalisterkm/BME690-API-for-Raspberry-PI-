## Raspberry Pi version of the BME690 API

The Bosch Sensortec BME690 API v 1.0.1 is a C API for the Bosch Sensortec BME690 air quality sensor. This is a minimal port to remove the COINS dependency and replace with standard Raspbian/Linux functions. 

The API files bme69x.h, bme69x_defs.h, bme69x.c files are unchanged, the majority of the changes to remove COINS are in the examples/common folder and the files common.c common.h, which have been replaced by rpi-common.h and rpi-common.c. 
Each of the folders self_test, forced_mode, parallel_mode, sequential_mode is self contained and has its own Makefile. 

To build each example you need to be in the folder and run 'make all' which will produce an executable file in the same folder.

``` C
$ cd  examples/forced_mode
$ make all

$ ./forced_mode
I2C Interface
Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%), Gas resistance(ohm), Status
1, 120569194, 24.59, 100113.88, 50.26, 96458.18, 0xa0
2, 120569397, 25.11, 100103.73, 50.32, 5684.85, 0xb0
3, 120569601, 25.89, 100112.43, 50.31, 5684.85, 0xb0
4, 120569805, 26.38, 100123.62, 50.21, 5684.85, 0xb0
5, 120570009, 26.62, 100125.77, 50.07, 5684.85, 0xb0
6, 120570212, 26.78, 100126.28, 49.87, 5684.85, 0xb0
7, 120570416, 26.89, 100128.86, 49.72, 5684.85, 0xb0
```

The Bus number and I2C address of the sensor are hard coded in rpi-common.c as "/dev/i2c-1" and BME69X_I2C_ADDR_LOW (0x76) respectively.

## Dependencies

1.  A BME690 sensor that can be connected via I2C to the PI.
2.  Raspberry PI
3.  The i2c-tools package is useful to confirm the sensor is visible on the bus.
4.  Visual Studio Code was used to make these changes, but it is not needed to build and run the examples.

## Known Issues

1. Hard coding Bus and device details is not good, a parameter file would make sense.
2. Only limited testing, on a PI5.
3. The BME680/688 is not supported by this API, it has its own API. If you connect a 688 expect -ve pressure and humidity.
4. The Makefiles all build with dubugging in mind: "gcc - g"
