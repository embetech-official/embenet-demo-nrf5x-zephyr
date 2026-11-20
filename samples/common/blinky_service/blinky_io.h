/**
 * @file
 * @brief Blinky service I/O - the 4 DK buttons and 4 DK LEDs, shared by the node and the router side.
 *
 * Buttons and LEDs come from the devicetree aliases sw0..sw3 and led0..led3; button n and LED n are numbered 1..4.
 *
 * A debounced press is latched and taken by @ref blinky_io_button_pressed; holding a button down reports a single
 * press. An LED keeps the state @ref blinky_io_led_set gave it.
 *
 * @note Every function is thread safe; @ref blinky_io_led_set may also be called from interrupts.
 */
#ifndef BLINKY_IO_H_
#define BLINKY_IO_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Number of buttons and of LEDs.
#define BLINKY_IO_COUNT 4U

/**
 * Called from the button interrupt after a press has been latched, to wake the thread that takes it. May be NULL.
 */
typedef void (*blinky_io_wakeup_t)(void);

/**
 * Configures the buttons and LEDs, all LEDs off, and enables the button interrupts.
 *
 * @param wakeup @ref blinky_io_wakeup_t
 *
 * @retval 0         ready
 * @retval -EALREADY already initialized; nothing changed
 * @retval -ENODEV   a GPIO controller is not ready
 * @retval <0        a GPIO call failed; nothing is left configured
 */
int blinky_io_init(blinky_io_wakeup_t wakeup);

/**
 * Turns the LEDs off, disables the button interrupts and disconnects all 8 pins. Latched presses are dropped. Does
 * nothing when not initialized.
 */
void blinky_io_deinit(void);

/**
 * Takes the latched press of button @p button, clearing the latch.
 *
 * The latch is set by a press after a release, so it is set again only once the button has been released and pressed
 * anew. Presses latched before @ref blinky_io_init are not seen; a button held down through init has to be released
 * first.
 *
 * @param button 1..@ref BLINKY_IO_COUNT; other values read false.
 * @return true if the button was pressed since the last call; false otherwise or when not initialized.
 */
bool blinky_io_button_pressed(uint8_t button);

/**
 * Turns LED @p led on or off. Does nothing when not initialized.
 *
 * @param led 1..@ref BLINKY_IO_COUNT; other values are ignored.
 * @param on  true to light it.
 */
void blinky_io_led_set(uint8_t led, bool on);

#ifdef __cplusplus
}
#endif

#endif // BLINKY_IO_H_
