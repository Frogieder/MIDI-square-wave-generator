/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
// #include <string.h>
#include <math.h>

#include "bsp/board.h"
#include "tusb.h"
#include "pico/stdlib.h"

#include "hardware/pwm.h"
// #include "hardware/clocks.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define NOTE packet[2]

void midi_task(void);

void init_task(void);

// uint32_t clock_freq;
// uint slice_num = 2;
float duty_cycle = 0.25f;
int active_notes = 0;

int notes[] = {-1, -1, -1, -1, -1, -1, -1, -1}; // one for each slice

/*------------- MAIN -------------*/
int main(void) {
    tusb_init();
    init_task();

    while (1) {
        tud_task(); // tinyusb device task
        midi_task();
    }

    return 0;
}
// JINDO BACHAAAAA !!!!

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {

}

// Invoked when device is unmounted
void tud_umount_cb(void) {

}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {

}

//--------------------------------------------------------------------+
// MIDI Task
//--------------------------------------------------------------------+


void init_task(void) {
    /*
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);
    gpio_init(3);
    gpio_set_dir(3, GPIO_OUT);
    */
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    // initialize all PWMs
    for (int slice_num = 0; slice_num < 8; slice_num++) {
        gpio_set_function(slice_num * 2, GPIO_FUNC_PWM);
        pwm_set_wrap(slice_num, (uint16_t) (5000));
        pwm_set_clkdiv_int_frac(slice_num, 250, 0);
        pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
        pwm_set_enabled(slice_num, true);
    }
    // clock_freq = (uint32_t)clock_get_hz(clk_sys);
}

int find_first(int value, const int *array, int size) {
    for (int i = 0; i < size; i++) {
        if (array[i] == value)
            return i;
    }
    return -1;
}

void midi_task(void) {
    /*
  uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
  uint8_t const channel = 0;   // 0 for channel 1
     */

    uint8_t packet[4];

    while (tud_midi_available()) {
        tud_midi_packet_read(packet);
        /*
        for (int i = 0; i < 4; i++)
        {
          for (int j = 7; j >= 0; j--)
          {
            gpio_put(3, 1);
            gpio_put(2, ((packet[i] & (1 << j)) > 0));
            sleep_us(500);
            tud_task();
            gpio_put(3, 0);
            sleep_us(500);
            tud_task();
          }
          gpio_put(2, 0);
          sleep_us(500);
          tud_task();
          sleep_us(500);
          tud_task();
        }
        gpio_put(2, 0);
      */

        // NoteOn
        if (packet[0] == 9) {
            int32_t new_frequency = (int32_t) (440 * exp2f((float) (NOTE - 69) / 12.f));
            uint slice_num = find_first(-1, notes, 8);
            if (slice_num != -1) {
                notes[slice_num] = packet[2];
                active_notes++;
                if (active_notes >= 2)
                    gpio_put(25, 1);
                pwm_set_wrap(slice_num, (uint16_t) ((500000.f / (float) new_frequency) + 0.5));
                pwm_set_chan_level(slice_num, PWM_CHAN_A,
                                   (uint16_t) ((duty_cycle * 500000) / (float) new_frequency + 0.5));
            }
        }

            // Note Off
        else if (packet[0] == 0x8) {
            uint slice_num = find_first(NOTE, notes, 8);
            if (slice_num != -1) {
                notes[slice_num] = -1;
                active_notes--;
                if (active_notes <= 1)
                    gpio_put(25, 0);
                pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
            }
        }
    }

    /*
    // Send Note On for current position at full velocity (127) on channel 1.
    uint8_t note_on[3] = { 0x90 | channel, note_sequence[note_pos], 127 };
    tud_midi_stream_write(cable_num, note_on, 3);

    // Send Note Off for previous note.
    uint8_t note_off[3] = { 0x80 | channel, note_sequence[previous], 0};
    tud_midi_stream_write(cable_num, note_off, 3);

    // Increment position
    note_pos++;

    // If we are at the end of the sequence, start over.
    if (note_pos >= sizeof(note_sequence)) note_pos = 0;
    */
}

#pragma clang diagnostic pop