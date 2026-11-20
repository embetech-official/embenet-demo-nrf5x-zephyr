/**
 * @file
 * @brief Blinky service, border router side: multicasts button presses to the nodes and switches LEDs on their requests.
 * See samples/node/README.md for the protocol.
 *
 * @note Call every function from the thread that calls EMBENET_BR_Proc().
 */
#ifndef BLINKY_SERVICE_ROUTER_H_
#define BLINKY_SERVICE_ROUTER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sets up the buttons and LEDs and registers the UDP socket. Call from the on_network_started handler.
 *
 * @retval 0         running
 * @retval -EALREADY already running; nothing changed
 * @retval -EIO      registering the socket failed; the buttons and LEDs are released
 * @retval <0        the buttons or LEDs could not be set up, see blinky_io_init()
 */
int blinky_service_router_init(void);

/**
 * Unregisters the socket and releases the buttons and LEDs. Call from the on_network_stopped handler, and before
 * EMBENET_BR_Deinit() if the network is still up.
 */
void blinky_service_router_deinit(void);

/**
 * Multicasts the button presses latched since the last call. Call after every EMBENET_BR_Proc().
 */
void blinky_service_router_proc(void);

#ifdef __cplusplus
}
#endif

#endif // BLINKY_SERVICE_ROUTER_H_
