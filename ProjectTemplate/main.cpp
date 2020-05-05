
#include "ClearCore.h"

int main(void) {
    /* Replace with your application code */
    bool ledState = true;
    while (1)
    {
        ConnectorLed.State(ledState);
        ledState = !ledState;
        Delay_ms(1000);
    }
}