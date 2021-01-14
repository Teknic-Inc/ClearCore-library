/*
 * Title: PlayTone
 *
 * Objective:
 *    This example demonstrates how to play a sequence of tones on the
 *    ClearCore's H-Bridge connectors using the musical frequency values defined
 *    in the pitches.h header file.
 *
 * Description:
 *    This example sets up an H-Bridge connector into output mode, sets the
 *    volume, and plays a melodic sequence of tones of equal duration.
 *
 * Requirements:
 * ** A speaker or other audio output device connected to IO-4
 *
 * Links:
 * ** ClearCore Documentation: https://teknic-inc.github.io/ClearCore-library/
 * ** ClearCore Manual: https://www.teknic.com/files/downloads/clearcore_user_manual.pdf
 *
 * 
 * Copyright (c) 2020 Teknic Inc. This work is free to use, copy and distribute under the terms of
 * the standard MIT permissive software license which can be found at https://opensource.org/licenses/MIT
 */

#include "ClearCore.h"
#include "pitches.h"

// Notes to be played in sequence as part of a melody.
// The maximum tone frequency is 1/4 of the tone interrupt rate, i.e. 5512 Hz.
// Any commanded frequency above 5512 Hz will get clipped to 5512 Hz.
// See the "pitches.h" file for the frequency definitions of these notes.
const uint16_t melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,
                           NOTE_CS4, NOTE_FS4, NOTE_AS4, NOTE_CS5,
                           NOTE_E4, NOTE_G4, NOTE_C5, NOTE_E5,
                           NOTE_FS4, NOTE_AS4, NOTE_CS5, NOTE_FS5
                          };
const uint32_t noteCount = sizeof(melody) / sizeof(melody[0]);

const uint32_t toneDuration = 200;              // in milliseconds
const int16_t toneAmplitude = INT16_MAX / 100;  // max volume is INT16_MAX

// Tone output is supported on connectors IO-4 and IO-5 only.
#define tonePin ConnectorIO4

int main() {
    // Set the tone connector into output mode.
    tonePin.Mode(Connector::OUTPUT_TONE);

    // Set the volume of the tone connector to the value specified
    // by toneAmplitude.
    tonePin.ToneAmplitude(toneAmplitude);

    while (true) {
        // Play the melody in order with equal note durations.
        for (uint8_t note = 0; note < noteCount; note++) {
            tonePin.ToneContinuous(melody[note]);
            Delay_ms(toneDuration);
        }

        // Stop the tone generation.
        tonePin.ToneStop();

        // Wait a second, then repeat...
        Delay_ms(1000);
    }
}
