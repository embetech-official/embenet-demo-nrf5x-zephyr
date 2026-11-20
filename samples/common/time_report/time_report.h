/**
 * @file
 * @brief Time report: once a second of network time, sends the node's UID and the network time to the border router
 * on DEMO_DATAGRAM_UDP_PORT.
 *
 * @note Call every function from the thread that drives the stack.
 */
#ifndef TIME_REPORT_H_
#define TIME_REPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/// Time between two reports, in milliseconds of network time.
#define TIME_REPORT_PERIOD_MS 1000U

/**
 * Registers the UDP socket and creates the report task. Call once the stack is initialized, before it joins, and
 * again for every new stack instance.
 *
 * @retval 0    ready
 * @retval -EIO a stack call failed
 */
int time_report_init(void);

/**
 * Starts reporting on the next whole second of network time. Call when the node has joined.
 */
void time_report_on_joined(void);

/**
 * Stops reporting. Call when the node has left the network.
 */
void time_report_on_left(void);

#ifdef __cplusplus
}
#endif

#endif // TIME_REPORT_H_
