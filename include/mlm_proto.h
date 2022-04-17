/*  =========================================================================
    mlm_proto - The Malamute Protocol

    Codec header for mlm_proto.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mlm_proto.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef MLM_PROTO_H_INCLUDED
#define MLM_PROTO_H_INCLUDED

/*  These are the mlm_proto messages:

    CONNECTION_OPEN - Client opens a connection to the server. Client can ask for a mailbox
by specifying an address. If mailbox does not exist, server creates it.
Server replies with OK or ERROR.
        protocol            string      Constant "MALAMUTE"
        version             number 2    Protocol version 1
        address             string      Client address

    CONNECTION_PING - Client pings the server. Server replies with CONNECTION-PONG, or
ERROR with status COMMAND-INVALID if the client is not recognized
(e.g. after a server restart or network recovery).

    CONNECTION_PONG - Server replies to a client connection ping.

    CONNECTION_CLOSE - Client closes the connection. This is polite though not mandatory.
Server will reply with OK or ERROR.

    STREAM_WRITE - Client declares intention to write to a stream. If stream does not
exist, server creates it. A client can write to a single stream at
once. Server replies with OK or ERROR.
        stream              string      Name of stream

    STREAM_READ - Client opens a stream for reading, specifying a pattern to match
message subjects. An empty pattern matches everything. If the stream
does not exist, the server creates it. A client can read from many
streams at once. It can also read from the same stream many times,
with different patterns. Server replies with OK or ERROR.
        stream              string      Name of stream
        pattern             string      Match message subjects

    STREAM_SEND - Client publishes a message to the current stream. A stream message
has a subject, and a content of zero or more frames. Server does not
reply to this message. The subject is used to match messages to
readers. A reader will receive a given message at most once.
        subject             string      Message subject
        content             msg         Message body frames

    STREAM_DELIVER - Server delivers a stream message to a client. The delivered message
has the address of the original sending client, so clients can send
messages back to the original sender's mailbox if they need to.
        address             string      Name of stream
        sender              string      Sending client address
        subject             string      Message subject
        content             msg         Message body frames

    MAILBOX_SEND - Client sends a message to a specified mailbox. Client does not open the
mailbox before sending a message to it. Server replies with OK when it
accepts the message, or ERROR if that failed. If the tracker is not
empty, the sender can expect a CONFIRM at some later stage, for this
message. Confirmations are asynchronous. If the message cannot be
delivered within the specified timeout (zero means infinite), the server
discards it and returns a CONFIRM with a TIMEOUT-EXPIRED status.
        address             string      Mailbox address
        subject             string      Message subject
        tracker             string      Message tracker
        timeout             number 4    Timeout, msecs, or zero
        content             msg         Message body frames

    MAILBOX_DELIVER - Server delivers a mailbox message to client. Note that client does not
open its own mailbox for reading; this is implied in CONNECTION-OPEN.
If tracker is not empty, client must respond with CONFIRM when it
formally accepts delivery of the message, or if the server delivers
the same message a second time.
        sender              string      Sending client address
        address             string      Mailbox address
        subject             string      Message subject
        tracker             string      Message tracker
        content             msg         Message body frames

    SERVICE_SEND - Client sends a service request to a service queue. Server replies with
OK when queued, or ERROR if that failed. If the tracker is not
empty, the client can expect a CONFIRM at some later time.
Confirmations are asynchronous. If the message cannot be delivered
within the specified timeout (zero means infinite), the server
discards it and returns CONFIRM with a TIMEOUT-EXPIRED status.
        address             string      Service address
        subject             string      Message subject
        tracker             string      Message tracker
        timeout             number 4    Timeout, msecs, or zero
        content             msg         Message body frames

    SERVICE_OFFER - Worker client offers a named service, specifying a pattern to match
message subjects. An empty pattern matches anything. A worker can offer
many different services at once. Server replies with OK or ERROR.
        address             string      Service address
        pattern             string      Match message subjects

    SERVICE_DELIVER - Server delivers a service request to a worker client. If tracker
is not empty, worker must respond with CONFIRM when it accepts delivery
of the message. The worker sends replies to the request to the requesting
client's mailbox.
        sender              string      Sending client address
        address             string      Service address
        subject             string      Message subject
        tracker             string      Message tracker
        content             msg         Message body frames

    OK - Server replies with success status. Actual status code provides more
information. An OK always has a 2xx status code.
        status_code         number 2    3-digit status code
        status_reason       string      Printable explanation

    ERROR - Server replies with failure status. Actual status code provides more
information. An ERROR always has a 3xx, 4xx, or 5xx status code.
        status_code         number 2    3-digit status code
        status_reason       string      Printable explanation

    CREDIT - Client sends credit to allow delivery of messages. Until the client
sends credit, the server will not deliver messages. The client can send
further credit at any time. Credit is measured in number of messages.
Server does not reply to this message. Note that credit applies to all
stream, mailbox, and service deliveries.
        amount              number 2    Number of messages

    CONFIRM - Client confirms reception of a message, or server forwards this
confirmation to original sender. If status code is 300 or higher, this
indicates that the message could not be delivered.
        tracker             string      Message tracker
        status_code         number 2    3-digit status code
        status_reason       string      Printable explanation

    STREAM_CANCEL - Cancels and removes all subscriptions to a stream.
Server replies with OK or ERROR.
        stream              string      Name of stream
*/

#define MLM_PROTO_SUCCESS                   200
#define MLM_PROTO_FAILED                    300
#define MLM_PROTO_COMMAND_INVALID           500
#define MLM_PROTO_NOT_IMPLEMENTED           501
#define MLM_PROTO_INTERNAL_ERROR            502

#define MLM_PROTO_CONNECTION_OPEN           1
#define MLM_PROTO_CONNECTION_PING           2
#define MLM_PROTO_CONNECTION_PONG           3
#define MLM_PROTO_CONNECTION_CLOSE          4
#define MLM_PROTO_STREAM_WRITE              5
#define MLM_PROTO_STREAM_READ               6
#define MLM_PROTO_STREAM_SEND               7
#define MLM_PROTO_STREAM_DELIVER            8
#define MLM_PROTO_MAILBOX_SEND              9
#define MLM_PROTO_MAILBOX_DELIVER           10
#define MLM_PROTO_SERVICE_SEND              11
#define MLM_PROTO_SERVICE_OFFER             12
#define MLM_PROTO_SERVICE_DELIVER           13
#define MLM_PROTO_OK                        14
#define MLM_PROTO_ERROR                     15
#define MLM_PROTO_CREDIT                    16
#define MLM_PROTO_CONFIRM                   17
#define MLM_PROTO_STREAM_CANCEL             18

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
#ifndef MLM_PROTO_T_DEFINED
typedef struct _mlm_proto_t mlm_proto_t;
#define MLM_PROTO_T_DEFINED
#endif

//  @warning THE FOLLOWING @INTERFACE BLOCK IS AUTO-GENERATED BY ZPROJECT
//  @warning Please edit the model at "api/mlm_proto.api" to make changes.
//  @interface
//  This is a stable class, and may not change except for emergencies. It
//  is provided in stable builds.

#define MLM_PROTO_SUCCESS 200

#define MLM_PROTO_FAILED 300

#define MLM_PROTO_COMMAND_INVALID 500

#define MLM_PROTO_NOT_IMPLEMENTED 501

#define MLM_PROTO_INTERNAL_ERROR 502

#define MLM_PROTO_CONNECTION_OPEN 1

#define MLM_PROTO_CONNECTION_PING 2

#define MLM_PROTO_CONNECTION_PONG 3

#define MLM_PROTO_CONNECTION_CLOSE 4

#define MLM_PROTO_STREAM_WRITE 5

#define MLM_PROTO_STREAM_READ 6

#define MLM_PROTO_STREAM_SEND 7

#define MLM_PROTO_STREAM_DELIVER 8

#define MLM_PROTO_MAILBOX_SEND 9

#define MLM_PROTO_MAILBOX_DELIVER 10

#define MLM_PROTO_SERVICE_SEND 11

#define MLM_PROTO_SERVICE_OFFER 12

#define MLM_PROTO_SERVICE_DELIVER 13

#define MLM_PROTO_OK 14

#define MLM_PROTO_ERROR 15

#define MLM_PROTO_CREDIT 16

#define MLM_PROTO_CONFIRM 17

#define MLM_PROTO_STREAM_CANCEL 18

//  Create a new empty mlm_proto
MLM_EXPORT mlm_proto_t *
    mlm_proto_new (void);

//  Destroy a mlm_proto instance
MLM_EXPORT void
    mlm_proto_destroy (mlm_proto_t **self_p);

//  Receive a mlm_proto from the socket. Returns 0 if OK, -1 if
//  there was an error. Blocks if there is no message waiting.
MLM_EXPORT int
    mlm_proto_recv (mlm_proto_t *self, zsock_t *input);

//  Send the mlm_proto to the output socket, does not destroy it
MLM_EXPORT int
    mlm_proto_send (mlm_proto_t *self, zsock_t *output);

//  Print contents of message to stdout
MLM_EXPORT void
    mlm_proto_print (mlm_proto_t *self);

//  Get the message routing id, as a frame
MLM_EXPORT zframe_t *
    mlm_proto_routing_id (mlm_proto_t *self);

//  Set the message routing id from a frame
MLM_EXPORT void
    mlm_proto_set_routing_id (mlm_proto_t *self, zframe_t *routing_id);

//  Get the mlm_proto message id
MLM_EXPORT int
    mlm_proto_id (mlm_proto_t *self);

//  Set the mlm_proto message id
MLM_EXPORT void
    mlm_proto_set_id (mlm_proto_t *self, int id);

//  Get the mlm_proto message id as printable text
MLM_EXPORT const char *
    mlm_proto_command (mlm_proto_t *self);

//  Get the address field
MLM_EXPORT const char *
    mlm_proto_address (mlm_proto_t *self);

//  Set the address field
MLM_EXPORT void
    mlm_proto_set_address (mlm_proto_t *self, const char *address);

//  Get the stream field
MLM_EXPORT const char *
    mlm_proto_stream (mlm_proto_t *self);

//  Set the stream field
MLM_EXPORT void
    mlm_proto_set_stream (mlm_proto_t *self, const char *stream);

//  Get the pattern field
MLM_EXPORT const char *
    mlm_proto_pattern (mlm_proto_t *self);

//  Set the pattern field
MLM_EXPORT void
    mlm_proto_set_pattern (mlm_proto_t *self, const char *pattern);

//  Get the subject field
MLM_EXPORT const char *
    mlm_proto_subject (mlm_proto_t *self);

//  Set the subject field
MLM_EXPORT void
    mlm_proto_set_subject (mlm_proto_t *self, const char *subject);

//  Get a copy of the content field
MLM_EXPORT zmsg_t *
    mlm_proto_content (mlm_proto_t *self);

//  Get the content field and transfer ownership to caller
MLM_EXPORT zmsg_t *
    mlm_proto_get_content (mlm_proto_t *self);

//
MLM_EXPORT void
    mlm_proto_set_content (mlm_proto_t *self, zmsg_t **content_p);

//  Get the sender field
MLM_EXPORT const char *
    mlm_proto_sender (mlm_proto_t *self);

//  Set the sender field
MLM_EXPORT void
    mlm_proto_set_sender (mlm_proto_t *self, const char *sender);

//  Get the tracker field
MLM_EXPORT const char *
    mlm_proto_tracker (mlm_proto_t *self);

//  Set the tracker field
MLM_EXPORT void
    mlm_proto_set_tracker (mlm_proto_t *self, const char *tracker);

//  Get the timeout field
MLM_EXPORT uint32_t
    mlm_proto_timeout (mlm_proto_t *self);

//  Set the timeout field
MLM_EXPORT void
    mlm_proto_set_timeout (mlm_proto_t *self, uint32_t timeout);

//  Get the status_code field
MLM_EXPORT uint16_t
    mlm_proto_status_code (mlm_proto_t *self);

//  Set the status_code field
MLM_EXPORT void
    mlm_proto_set_status_code (mlm_proto_t *self, uint16_t status_code);

//  Get the status_reason field
MLM_EXPORT const char *
    mlm_proto_status_reason (mlm_proto_t *self);

//  Set the status_reason field
MLM_EXPORT void
    mlm_proto_set_status_reason (mlm_proto_t *self, const char *status_reason);

//  Get the amount field
MLM_EXPORT uint16_t
    mlm_proto_amount (mlm_proto_t *self);

//  Set the amount field
MLM_EXPORT void
    mlm_proto_set_amount (mlm_proto_t *self, uint16_t amount);

//  Self test of this class.
MLM_EXPORT void
    mlm_proto_test (bool verbose);

//  @end

//  For backwards compatibility with old codecs
#define mlm_proto_dump      mlm_proto_print

#ifdef __cplusplus
}
#endif

#endif
