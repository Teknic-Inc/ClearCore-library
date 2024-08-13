
#include "ClearCore.h"

/**
 * Reference https://teknic-inc.github.io/ClearCore-library/_microchip_install.html
 * for more information about how this ProjectTemplate should be used.
 */

int main(void) {
    /* Replace with your application code below */
    bool ledState = true;
    while (1)
    {
        ConnectorLed.State(ledState);
        ledState = !ledState;
        Delay_ms(1000);
    }
}