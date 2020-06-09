#include "Arduino.h"
#include "ClearCore.h"

#include "AllTests.h"

int result = 0;
void setup() {
    
    // Which port should be used for the output messages?
    //TestIO::OutputPort(&ConnectorCOM0);
    //TestIO::OutputPort(&ConnectorCOM1);
    //TestIO::OutputPort(&ConnectorUsb);    // Default
    
    // RunTests will wait for the USB serial port to open
    // then run the specified unit tests and print the results to
    // the USB serial port
    // Returns a count of the number of failures
    result = RunTests();

    // If there weren't any failures, just turn on the LED
    if (!result) {
        digitalWrite(LED_BUILTIN, true);
    }
}

void loop() {
    // Blink the LED to indicate how many tests failed
    for (int i = 0; i < result; i++) {
        digitalWrite(LED_BUILTIN, true);
        delay(500);
        digitalWrite(LED_BUILTIN, false);
        delay(500);
    }
    delay(2000);
}