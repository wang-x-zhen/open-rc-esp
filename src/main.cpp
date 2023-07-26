
/*
 Stepper Motor Control - speed control

 This program drives a unipolar or bipolar stepper motor.
 The motor is attached to digital pins 8 - 11 of the Arduino.
 A potentiometer is connected to analog input 0.

 The motor will rotate in a clockwise direction. The higher the potentiometer value,
 the faster the motor speed. Because setSpeed() sets the delay between steps,
 you may notice the motor is less responsive to changes in the sensor value at
 low speeds.

 Created 30 Nov. 2009
 Modified 28 Oct 2010
 by Tom Igoe

 */
#include <ESP8266WiFi.h>

#include <Stepper.h>

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
// for your motor


// initialize the stepper library on pins 8 through 11:
Stepper stepper(stepsPerRevolution, D0, D1, D2, D3);

int stepCount = 0;  // number of steps the motor has taken

void setup() {
    // nothing to do inside the setup
    stepper.setSpeed(50);
}

int x = 0;

void loop() {

//    myStepper.setSpeed(motorSpeed);
//    // step 1/100 of a revolution:
//    myStepper.step(stepsPerRevolution / 100);

    if (x < 100) {
        stepper.step(-2);

    } else {
        stepper.step(2);
    }
    x++;
    if (x > 200) {
        x = 0;
    }
}


