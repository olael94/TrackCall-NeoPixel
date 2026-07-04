// TrackCall: watches a call button and a reset button. Each time the call
// button is pressed, it moves the system to the next "step" and shows a
// different color on an RGB LED. It also briefly turns on a relay (like a
// switch) each time it moves to a new step. The reset button starts it over.
#include "ws2812.pio.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include <stdio.h>

// The three colors the LED can show, used later like urgb_u32(GREEN).
#define GREEN 0xff, 0, 0
#define YELLOW 0x80, 0x80, 0
#define RED 0, 0xff, 0

// Which physical pins on the board the button and relay wires are connected to.
#define TRACKCALL 2 // the call button
#define RESET 6     // the reset button
#define RELAY 7     // the relay output

#define DEBOUNCETIME 50   // how long (ms) to wait and check again before trusting a button press
#define DELAYLOOPTIME 250 // how long (ms) to pause between each pass through the main loop
#define ELEMENTS(x) (sizeof(x) / sizeof((x)[0])) // counts how many items are in a fixed-size list

// This is where we define our pinouts
const int PIN_TX = 3; // the pin wired to the RGB LED's data line

// Sends one color to the RGB LED. This LED type (WS2812) needs its color data
// sent in a very precise, fast pattern, which is handled by the chip's PIO
// hardware (a small built-in helper for timing-sensitive tasks like this).
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Combines separate red, green, and blue values into the single number the
// LED expects. This LED type wants its color bytes in green-red-blue order,
// which is why the bit shifts below don't go in r, g, b order.
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Prepares everything before the main loop runs: turns on USB serial output,
// makes sure the relay starts switched off, and sets up both buttons so they
// can be read as on/off inputs.
void InitalizeLEDsAndButtons()
{
    const uint buttons[] = {RESET, TRACKCALL};
    const uint leds[] = {RELAY};
    stdio_init_all();

    for (int i = 0; i < ELEMENTS(leds); i++) {
        gpio_init(leds[i]);
        gpio_set_dir(leds[i], GPIO_OUT);
        gpio_put(leds[i], 0);
    }

    for (int i = 0; i < ELEMENTS(buttons); i++) {
        gpio_init(buttons[i]);
        gpio_set_dir(buttons[i], GPIO_IN);
        gpio_pull_up(buttons[i]);
    }
}

// Turns the relay on for 1 second and then back off, but only if `changed`
// is true. `changed` is only true for the one loop where the step number
// just moved forward, so the relay only pulses once per step, not repeatedly.
void activateRelay(uint changed)
{
    if (changed) {
        gpio_put(RELAY, 1);
        sleep_ms(1000);
        gpio_put(RELAY, 0);
    }
}

int main()
{
    uint state = 0;  // which step we're currently on (0 = start, green light)
    uint change = 0; // becomes 1 for one loop right after the step number changes
    InitalizeLEDsAndButtons();

    // Get the RGB LED's control program loaded and running on the chip's PIO
    // hardware so put_pixel() can be used to change its color.
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, PIN_TX, 800000, true);

    while (1) {
        // input reading
        // Buttons read as "false" when pressed (they're wired to read HIGH
        // normally and LOW when pushed). After seeing a press, we wait a
        // moment and check again, since a physical button can flicker
        // on/off several times in an instant when it's pressed ("bouncing").
        // Waiting and re-checking avoids counting one press as several.
        if (gpio_get(TRACKCALL) == false) {
            sleep_ms(DEBOUNCETIME);
            if (gpio_get(TRACKCALL) == false) {
                state++;
                change = 1;
            }
        }
        if (gpio_get(RESET) == false) {
            sleep_ms(DEBOUNCETIME);
            if (gpio_get(RESET) == false)
                state = 0;
        }
        // Decide what color to show and whether to pulse the relay, based on
        // which step we're currently on. Pressing the call button moves
        // through: green (start) -> yellow -> red -> off. Pressing reset
        // jumps back to green at any time.
        switch (state) {
        case 0:
            // Start / waiting state.
            put_pixel(urgb_u32(GREEN));
            break;

        case 1:
            // First step after a call: light turns yellow, relay pulses once.
            put_pixel(urgb_u32(YELLOW));
            activateRelay(change);
            break;
        case 2:
            // Second step: light turns red.
            put_pixel(urgb_u32(RED));
            activateRelay(change);
            break;

        case 3:
            // Final step: light turns off.
            put_pixel(urgb_u32(0, 0, 0)); // Black or off
            activateRelay(change);
            break;

        default:
            // This shouldn't happen during normal use, since reset always
            // brings the step count back to 0 before it could get this high.
            // If it ever does, flash cyan then purple so it's obvious
            // something unexpected happened.
            put_pixel(urgb_u32(0, 0xff, 0xff)); // Cyan
            sleep_ms(250);
            put_pixel(urgb_u32(0xff, 0xff, 0)); // Purple
        }
        sleep_ms(DELAYLOOPTIME); // wait a bit before checking the buttons again
        change = 0;              // clear the "just changed" flag until the next press
    }

    return 0;
}