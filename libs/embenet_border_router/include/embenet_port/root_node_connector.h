/**
 * @file
 * @license   See LICENSE.txt
 * @copyright Embetech sp. z o.o.
 * @version   1.1.1
 * @brief     Provides Port interface for the Root Node connection.
 *
 * A port implements ONLY the transport (open/close/read/write/link-state) for reaching the Root Node. Framing
 * (HDLC or COBS) and the RCP protocol are entirely owned by the library and are NOT the port's concern - this
 * header's contract does not change based on which encoding is selected by the configuration key (see the key
 * grammar below).
 *
 * ## Key grammar
 *
 * The Root Node key passed to @ref EMBENET_BR_PORT_RootNodeConnectorCreate() has the form
 * `contract[+encoding]:port_specific_key`. `contract` and `encoding` are two independent, closed-dictionary
 * tokens consumed entirely by the library; the port never sees them. Everything after the first `:` is the
 * `port_specific_key`, passed through byte-for-byte (it may itself contain `:` or `+`).
 *
 * `contract` (always required) describes how THIS port delivers data - it is the piece of information the
 * port implementation needs to know about itself:
 *
 * | Token     | Meaning                                                                                       |
 * |-----------|------------------------------------------------------------------------------------------------|
 * | `stream`  | Data-received callback may deliver arbitrary byte-stream fragments (see the STREAM contract below). |
 * | `message` | Each data-received callback / write() carries exactly one complete frame (see the MESSAGE contract below). |
 *
 * `encoding` (optional, defaults to `none`) selects the framing algorithm the library applies on top of
 * whatever `contract` the port provides - purely a library-side concern, listed here only so port authors
 * can recognize valid keys in configuration/logs:
 *
 * | Token   | Meaning                                                                       |
 * |---------|--------------------------------------------------------------------------------|
 * | `none`  | Passthrough (default when `encoding` is omitted); only valid with `message`.   |
 * | `hdlc`  | Flags + byte-stuffing + CRC + resync.                                          |
 * | `cobs`  | Consistent Overhead Byte Stuffing; a zero byte terminates each frame.          |
 *
 * `stream+none` (explicit or via omission, i.e. bare `stream:...`) is REJECTED by the library at
 * configuration time: a byte stream has no message boundaries of its own, so it can never be decoded without
 * some encoding. Every other `contract`+`encoding` pairing is accepted, even unusual ones like `message+hdlc`
 * - the library does not judge whether a pairing is a good idea, only whether it parses.
 *
 * Example keys: `stream+hdlc:/dev/ttyUSB0`, `message:memory_q1` (encoding defaults to `none`),
 * `message+none:memory_q1` (equivalent, explicit).
 *
 * ## Transport contracts
 *
 * A port implementation follows exactly one of these two contracts, matching the shape of its underlying
 * transport - this is exactly the `contract` token from the key grammar above:
 *
 * - **STREAM contract** (`stream`): the data-received callback may deliver arbitrary fragments of the byte
 *   stream - a single call may contain zero, one, or several partial or complete frames, split at arbitrary
 *   byte boundaries. @ref EMBENET_BR_PORT_RootNodeConnectorWrite() is likewise a raw stream write; it carries
 *   no framing guarantee of its own.
 * - **MESSAGE contract** (`message`): each data-received callback invocation MUST deliver exactly one
 *   complete RCP frame, no more and no less. Symmetrically, each @ref EMBENET_BR_PORT_RootNodeConnectorWrite()
 *   call MUST send exactly one frame atomically.
 */
#ifndef EMBENET_BORDER_ROUTER_PORT_ROOT_NODE_CONNECTOR_H
#define EMBENET_BORDER_ROUTER_PORT_ROOT_NODE_CONNECTOR_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Type alias to port-defined structure representing a root node connection.
 */
typedef struct EMBENET_BR_PORT_RootNodeConnector EMBENET_BR_PORT_RootNodeConnector;

/**
 * Returns handle to a root node connector instance with given key.
 * The key is used to identify the connector instance.
 * If the requested connector does not exist and cannot be created, the function SHALL return nullptr.
 *
 * The stack WILL call @ref EMBENET_BR_PORT_RootNodeConnectorDestroy() when the instance is no longer needed.
 *
 * @param[in] key Port-specific root node connector identifier (the part after the first `:` - see the key
 *                grammar in this file's header comment; the library has already parsed and consumed the
 *                `contract[+encoding]:` prefix).
 *
 * @return pointer to the valid Root Node Connector instance in DOWN state, or nullptr if the instance cannot be acquired.
 */
EMBENET_BR_PORT_RootNodeConnector *EMBENET_BR_PORT_RootNodeConnectorCreate(char const *key);

/**
 * Releases the root node connector instance.
 *
 * @param[in] connector Instance to be released.
 */
void EMBENET_BR_PORT_RootNodeConnectorDestroy(EMBENET_BR_PORT_RootNodeConnector *connector);

/**
 * Sets the callback to be called when data is received from the root node.
 *
 * @param[in] connector Instance to set the callback for.
 * @param[in] callback Callback to be called when data is received on the network adapter.
 * @param[in] context User-provided context to be passed to the callback.
 *
 * @pre connector was obtained from @ref EMBENET_BR_PORT_NetworkAdapterCreate().
 */
void EMBENET_BR_PORT_RootNodeConnectorSetDataReceivedCallback(EMBENET_BR_PORT_RootNodeConnector *connector,
                                                              void (*callback)(void *context, void const *data, size_t data_length), void *context);

/**
 *  Sets the callback to be called when the state of the link to the root node changes.
 *
 * @param connector Instance to set the callback for.
 * @param callback Callback to be called when the state of the link changes.
 * @param context User-provided context to be passed to the callback.
 */
void EMBENET_BR_PORT_RootNodeConnectorSetLinkStateChangedCallback(EMBENET_BR_PORT_RootNodeConnector *connector,
                                                                  void (*callback)(void *context, bool is_up), void *context);
/**
 * Brings the network adapter instance to the UP state.
 * By UP state we mean that the network adapter is ready to send and receive data.
 *
 * @param[in] connector Instance to be brought UP.
 *
 * @return true when after the call the network adapter is UP and ready to send and receive data, false otherwise.
 */
bool EMBENET_BR_PORT_RootNodeConnectorUp(EMBENET_BR_PORT_RootNodeConnector *connector);

/**
 * Brings the network adapter instance to the DOWN state.
 * By DOWN state we mean that the network adapter MUST NOT receive data, and WILL NOT send one.
 *
 * @param[in] connector Instance to be brought DOWN.
 */
void EMBENET_BR_PORT_RootNodeConnectorDown(EMBENET_BR_PORT_RootNodeConnector *connector);

/**
 * Returns the name of the Root Node Connector Instance.
 *
 * @param[in] connector Root Node Connector Instance.
 * @return C-string containing the name of the Root Node Connector Instance.
 */
char const *EMBENET_BR_PORT_RootNodeConnectorGetName(EMBENET_BR_PORT_RootNodeConnector const *connector);

/**
 * Writes the data to the root node connector.
 * The function SHALL return false if the connector is DOWN or if the data cannot be sent for any reason.
 *
 * @param[in] connector Root Node Connector instance.
 * @param[in] data Pointer to the data to be sent.
 * @param[in] data_length Length of the data to be sent.
 *
 * @return true if the data was sent successfully, false otherwise.
 */
bool EMBENET_BR_PORT_RootNodeConnectorWrite(EMBENET_BR_PORT_RootNodeConnector *connector, void const *data, size_t data_length);

#ifdef __cplusplus
}
#endif

#endif
