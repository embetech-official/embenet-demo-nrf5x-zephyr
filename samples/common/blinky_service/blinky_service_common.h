/**
 * @file
 * @brief Blinky service protocol shared by the node and the router side: a 2-byte datagram {led, state} on
 * DEMO_BLINKY_UDP_PORT, and its receive handler. See samples/node/README.md.
 */
#ifndef BLINKY_SERVICE_COMMON_H_
#define BLINKY_SERVICE_COMMON_H_

#include <embenet/udp.h>
#include <embenet_demo_network.h>

#include <stddef.h>
#include <stdint.h>

/// Size of every datagram, in bytes.
#define BLINKY_SERVICE_MSG_SIZE 2U
/// LEDs a datagram can switch: 1..this.
#define BLINKY_SERVICE_LED_COUNT 2U

/**
 * Builds the message button @p button sends: buttons 1 and 2 turn LED1 on and off, buttons 3 and 4 LED2.
 *
 * @param button 1..BLINKY_IO_COUNT
 * @param[out] msg The message.
 */
void blinky_service_button_msg(uint8_t button, uint8_t msg[BLINKY_SERVICE_MSG_SIZE]);

/**
 * The service socket's receive handler, an EMBENET_UDP_RxDataHandler: switches the LED a datagram names. Runs on
 * the thread that drives the stack.
 */
void blinky_service_on_datagram(EMBENET_UDP_SocketDescriptor const *socket, EMBENET_IPV6 const *source_address, uint16_t source_port,
                                void const *data, size_t data_size);

#endif // BLINKY_SERVICE_COMMON_H_
