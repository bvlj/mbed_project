#include "mbed.h"
#include "rtos.h"

// Display number digits
int digits[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

// Display Segments pins
BusOut display {p28, p22, p23, p25, p24, p29, p26, p27};

// Display pins
DigitalOut fSide(p21);
DigitalOut sSide(p30);

// Display number
int i = 0;

// Light Sensor
AnalogIn light(p20);

// Potentiometer
AnalogIn potentiometer(p18);

// State pins

DigitalIn reset(p5);
DigitalIn set(p6);
DigitalIn calibration(p7);
DigitalIn count(p8);

// Counter leds
DigitalOut ledGreen(p15);
DigitalOut ledYellow(p16);
DigitalOut ledRed(p17);

// Killer status
int killer = 0;

// Execution status
int state;

// People counter
int people = 0;

// People max value
int max = 1;

// Light level
int lightLevel = 50;

int statusReset       = 1;
int statusSet         = 0;
int statusCalibration = 0;
int statusCount       = 0;

/**
 * Print a number (minor than 100)
 * using a 2 digits display.
 * We cannot use 2 sides at the same time,
 * keep each one of them on just for a few ms
 * so it looks like they're both always on
 */
void screenPrint(void const *args) {
    int side = 0;

    while (1) {
        side++;
        if (side > 75) {
            display = ~digits[ i != 0 ? i/10 : 0];
            fSide = 1;
            sSide = 0;
            if (side > 150) {
                side = 0;
            }
        } else {
            display = ~digits[ i != 0 ? i%10 : 0];
            fSide = 0;
            sSide = 1;
        }
    }
}

/**
 * Reset state
 * Print "00" on display, turn of killer and
 * reset people counter, calibration level and max
 */
void stateReset(void const *args) {
    if (statusReset == 1) {
        killer     = 0;
        people     = 0;
        i          = 0;
        ledRed     = 0;
        max        = 1;
        lightLevel = 50;
        Thread::wait(0.5);
    }
}

/**
 * Set state
 * Read potentiometer value,
 *  display it and set it as max
 */
void stateSet(void const *args) {
    if (statusSet == 1) {
        float tmp = potentiometer;
        max = tmp * 100;
        i = max;
    }
}


/**
 * Calibration state
 */
void stateCalibration(void const *args) {
    if (statusCalibration == 1) {
        lightLevel = light * 100 - 1;
        i          = lightLevel;
    }
}

/**
 * Count state
 * Increase counter if light level is lower than
 * the excepted value.
 * Set the led color accordingly to the counter value
 * If we reach the max enter killer mode
 */
void stateCount(void const *args) {
    if (statusCount == 1) {
        if (light*100 < (lightLevel-4)) {
            people++;
        }
        
        i = people;
        Thread::wait(0.8);

        if (people < max) {
            ledGreen = 1;
        } else if (people == max-1) {
            ledYellow = 1;
        } else if (people >= max) {
            ledRed = 1;
            people = 0;
            killer = 1;
        }
    }
}

/**
 * Choose the threads that will be terminated
 */
void setState(char mode) {
    statusReset       = mode == 'r' ? 1 : 0;
    statusSet         = mode == 's' ? 1 : 0;
    statusCalibration = mode == 'c' ? 1 : 0;
    statusCount       = mode == 'o' ? 1 : 0;
}

int main() {
    // Start with the first side of the display
    fSide = 1;

    // Fire the display thread
    Thread printThread(screenPrint);

    while (1) {
        // Turn off a few of leds
        ledYellow = 0;
        ledGreen = 0;
        
        // If killer mode is on, set display value as 00
        if (killer == 1) {
            i = 11;
        }

        // Select the thread that won't be terminated
        if (reset == 1) {
            setState('r');
        } else if (set == 1) {
            setState('s');
        } else if (calibration == 1) {
            setState('c');
        } else if (count == 1) {
            setState('o');
        }       

        Thread resetThread(stateReset);
        if (statusReset == 0) {
            // If reset is off (we can ignore killer here), terminate resetThread
            resetThread.terminate();
        }

        Thread setThread(stateSet);
        if (statusSet == 0 || killer == 1) {
            // If set is off (or killer is on), terminate setThread
            setThread.terminate();
        }
        
        Thread calibrationThread(stateCalibration);
        if (statusCalibration == 0 || killer == 1) {
            // If calibration is off (or killer is on), terminate calibrationThread
            calibrationThread.terminate();
        }
        
        Thread countThread(stateCount);
        if (statusCount == 0 || killer == 1) {
            // If count is off (or killer is on), terminate countThread
            countThread.terminate();
        }

        // Relax while we make sure the running thread does its job
        wait(0.5);
    }
}
