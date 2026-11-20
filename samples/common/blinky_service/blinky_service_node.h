/**
 * @file
 * @brief Blinky service, node side: sends button presses to the border router, switches LEDs on its messages, shows the
 * joined state on LED3 and blinks LED4 on network time. See samples/node/README.md for the protocol.
 *
 * @note Call every function from the thread that drives the stack.
 */
#ifndef BLINKY_SERVICE_NODE_H_
#define BLINKY_SERVICE_NODE_H_

#ifdef __cplusplus
extern "C" {
#endif

/// LED lit while the node is joined.
#define BLINKY_SERVICE_NODE_JOINED_LED 3U
/// LED that blinks on network time while the node is joined.
#define BLINKY_SERVICE_NODE_BLINK_LED 4U
/// Time between two toggles of LED4, in milliseconds of network time: a 1000 ms blink period.
#define BLINKY_SERVICE_NODE_BLINK_HALF_PERIOD_MS 500U

/**
 * Called from interrupt context when a button press latches, to wake the thread that drives the stack.
 */
typedef void (*blinky_service_node_wakeup_t)(void);

/**
 * Sets up the buttons and LEDs, registers the UDP socket and joins the group. Call once the stack is initialized,
 * before it joins, and again for every new stack instance.
 *
 * @param wakeup @ref blinky_service_node_wakeup_t, so the stack's thread calls @ref blinky_service_node_proc without
 *               delay. May be NULL.
 *
 * @retval 0       ready
 * @retval -EIO    a stack call failed; nothing is left registered
 * @retval -ENODEV a GPIO controller is not ready
 * @retval <0      a GPIO call failed; nothing is left configured
 */
int blinky_service_node_init(blinky_service_node_wakeup_t wakeup);

/**
 * Destroys the LED4 blink task, leaves the group, unregisters the socket and releases the buttons and LEDs. Call while
 * the stack is still initialized.
 */
void blinky_service_node_deinit(void);

/**
 * Lights LED3 and starts the LED4 blink on network time. Call when the node has joined.
 */
void blinky_service_node_on_joined(void);

/**
 * Turns LED3 and LED4 off and stops the blink. Call when the node has left the network.
 */
void blinky_service_node_on_left(void);

/**
 * Sends the button presses latched since the last call. Call after every EMBENET_NODE_Proc().
 */
void blinky_service_node_proc(void);

#ifdef __cplusplus
}
#endif

#endif // BLINKY_SERVICE_NODE_H_
