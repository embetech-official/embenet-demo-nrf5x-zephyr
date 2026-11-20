/**
 * @file
 * @brief Constants every device of the demo network has to agree on: the UDP ports, the multicast groups and the keys.
 *
 * The keys are demo values; a product takes them from secure storage.
 */
#ifndef EMBENET_DEMO_NETWORK_H_
#define EMBENET_DEMO_NETWORK_H_

/// @brief Port of the blinky service (the node samples and samples/border_router_integrated), and the group its nodes join.
#define DEMO_BLINKY_UDP_PORT 0xf0bbU
#define DEMO_BLINKY_GROUP 200U

/// @brief Port of the time report the border router logs, and the group its socket is bound to.
#define DEMO_DATAGRAM_UDP_PORT 0xf0bcU
#define DEMO_DATAGRAM_GROUP 100U

/// @brief First byte of the demo K1 and PSK; the rest are zero. K1 must match every device on the network, the PSK the border router's join rule.
#define DEMO_K1_BYTE 0x11
#define DEMO_PSK_BYTE 0x33

#endif // EMBENET_DEMO_NETWORK_H_
