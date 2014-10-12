/*  =========================================================================
    mlm_msg       - The Malamute Protocol
    
    Codec header for mlm_msg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mlm_msg.xml, or
     * The code generation script that built this file: zproto_codec_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of MALAMUTE, the ZeroMQ broker project.          
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MLM_MSG_H_INCLUDED__
#define __MLM_MSG_H_INCLUDED__

/*  These are the mlm_msg       messages:

    CONNECTION_OPEN - Client opens a connection to the server. Client can ask for a mailbox
by specifying an address. If mailbox does not exist, server creates it.
Server replies with OK or ERROR.
        protocol            string      Constant "MALAMUTE"
        version             number  2   Protocol version 1
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
with different patterns. Server does not reply to this message.
        stream              string      Name of stream
        pattern             string      Match message subjects

    STREAM_PUBLISH - Client publishes a message to the current stream. A stream message
has a subject, and a content of zero or more frames. Server does not
reply to this message. The subject is used to match messages to
readers. A reader will receive a given message at most once.
        subject             string      Message subject
        content             msg         Message body frames

    STREAM_DELIVER - Server delivers a stream message to a client. The delivered message
has the address of the original sending client, so clients can send
messages back to the original sender's mailbox if they need to.
        stream              string      Name of stream
        sender              string      Sending client address
        subject             string      Message subject
        content             msg         Message body frames

    MAILBOX_SEND - Client sends a message to a specified mailbox. Client does not open the
mailbox before sending a message to it. Server replies with OK when it
accepts the message, or ERROR if that failed. If the tracking ID is not
empty, the sender can expect a CONFIRM at some later stage, for this
message. Confirmations are asynchronous. If the message cannot be
delivered within the specified timeout (zero means infinite), the server
discards it and returns a CONFIRM with a TIMEOUT-EXPIRED status.
        address             string      Mailbox address
        subject             string      Message subject
        content             msg         Message body frames
        tracking            string      Message tracking ID
        timeout             number  4   Timeout, msecs, or zero

    MAILBOX_DELIVER - Server delivers a mailbox message to client. Note that client does not 
open its own mailbox for reading; this is implied in CONNECTION-OPEN.
If tracking ID is not empty, client must respond with CONFIRM when it
formally accepts delivery of the message, or if the server delivers
the same message a second time.
        sender              string      Sending client address
        address             string      Mailbox address
        subject             string      Message subject
        content             msg         Message body frames
        tracking            string      Message tracking ID

    SERVICE_SEND - Client sends a service request to a service queue. Server replies with
OK when queued, or ERROR if that failed. If the tracking ID is not
empty, the client can expect a CONFIRM at some later time.
Confirmations are asynchronous. If the message cannot be delivered
within the specified timeout (zero means infinite), the server
discards it and returns CONFIRM with a TIMEOUT-EXPIRED status.
        service             string      Service name
        subject             string      Message subject
        content             msg         Message body frames
        tracking            string      Message tracking ID
        timeout             number  4   Timeout, msecs, or zero

    SERVICE_OFFER - Worker client offers a named service, specifying a pattern to match
message subjects. An empty pattern matches anything. A worker can offer
many different services at once. Server does not reply to this message.
        service             string      Service name
        pattern             string      Match message subjects

    SERVICE_DELIVER - Server delivers a service request to a worker client. If tracking ID
is not empty, worker must respond with CONFIRM when it accepts delivery
of the message. The worker sends replies to the request to the requesting
client's mailbox.
        sender              string      Sending client address
        service             string      Service name
        subject             string      Message subject
        content             msg         Message body frames
        tracking            string      Message tracking ID

    OK      - Server replies with success status. Actual status code provides more
information. An OK always has a 2xx status code.
        status_code         number  2   3-digit status code
        status_reason       string      Printable explanation

    ERROR   - Server replies with failure status. Actual status code provides more
information. An ERROR always has a 3xx, 4xx, or 5xx status code.
        status_code         number  2   3-digit status code
        status_reason       string      Printable explanation

    CREDIT  - Client sends credit to allow delivery of messages. Until the client
sends credit, the server will not deliver messages. The client can send
further credit at any time. Credit is measured in number of messages.
Server does not reply to this message. Note that credit applies to all
stream, mailbox, and service deliveries.
        amount              number  2   Number of messages

    CONFIRM - Client confirms reception of a message, or server forwards this
confirmation to original sender. If status code is 300 or higher, this
indicates that the message could not be delivered.
        tracking            string      Message tracking ID
        status_code         number  2   3-digit status code
        status_reason       string      Printable explanation
*/

#define MLM_MSG_SUCCESS                     200
#define MLM_MSG_STORED                      201
#define MLM_MSG_DELIVERED                   202
#define MLM_MSG_NOT_DELIVERED               300
#define MLM_MSG_CONTENT_TOO_LARGE           301
#define MLM_MSG_TIMEOUT_EXPIRED             302
#define MLM_MSG_CONNECTION_REFUSED          303
#define MLM_MSG_RESOURCE_LOCKED             400
#define MLM_MSG_ACCESS_REFUSED              401
#define MLM_MSG_NOT_FOUND                   404
#define MLM_MSG_COMMAND_INVALID             500
#define MLM_MSG_NOT_IMPLEMENTED             501
#define MLM_MSG_INTERNAL_ERROR              502

#define MLM_MSG_CONNECTION_OPEN             1
#define MLM_MSG_CONNECTION_PING             2
#define MLM_MSG_CONNECTION_PONG             3
#define MLM_MSG_CONNECTION_CLOSE            4
#define MLM_MSG_STREAM_WRITE                5
#define MLM_MSG_STREAM_READ                 6
#define MLM_MSG_STREAM_PUBLISH              7
#define MLM_MSG_STREAM_DELIVER              8
#define MLM_MSG_MAILBOX_SEND                9
#define MLM_MSG_MAILBOX_DELIVER             10
#define MLM_MSG_SERVICE_SEND                11
#define MLM_MSG_SERVICE_OFFER               12
#define MLM_MSG_SERVICE_DELIVER             13
#define MLM_MSG_OK                          14
#define MLM_MSG_ERROR                       15
#define MLM_MSG_CREDIT                      16
#define MLM_MSG_CONFIRM                     17

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _mlm_msg_t       mlm_msg_t;

//  @interface
//  Create a new mlm_msg
mlm_msg_t                            *
    mlm_msg_new       (int id);

//  Destroy the mlm_msg
void
    mlm_msg_destroy       (mlm_msg_t       **self_p);

//  Parse a mlm_msg       from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.
mlm_msg_t                            *
    mlm_msg_decode       (zmsg_t **msg_p);

//  Encode mlm_msg       into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
zmsg_t                      *
    mlm_msg_encode       (mlm_msg_t       **self_p);

//  Receive and parse a mlm_msg       from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
mlm_msg_t                            *
    mlm_msg_recv       (void *input);

//  Receive and parse a mlm_msg       from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
mlm_msg_t                            *
    mlm_msg_recv_nowait       (void *input);

//  Send the mlm_msg       to the output, and destroy it
int
    mlm_msg_send       (mlm_msg_t       **self_p, void *output);

//  Send the mlm_msg       to the output, and do not destroy it
int
    mlm_msg_send_again       (mlm_msg_t       *self, void *output);

//  Encode the CONNECTION_OPEN 
zmsg_t                      *
    mlm_msg_encode_connection_open (
        const char *address);

//  Encode the CONNECTION_PING 
zmsg_t                      *
    mlm_msg_encode_connection_ping (
);

//  Encode the CONNECTION_PONG 
zmsg_t                      *
    mlm_msg_encode_connection_pong (
);

//  Encode the CONNECTION_CLOSE 
zmsg_t                      *
    mlm_msg_encode_connection_close (
);

//  Encode the STREAM_WRITE    
zmsg_t                      *
    mlm_msg_encode_stream_write  (
        const char *stream);

//  Encode the STREAM_READ     
zmsg_t                      *
    mlm_msg_encode_stream_read   (
        const char *stream,
        const char *pattern);

//  Encode the STREAM_PUBLISH  
zmsg_t                      *
    mlm_msg_encode_stream_publish (
        const char *subject,
        zmsg_t *content);

//  Encode the STREAM_DELIVER  
zmsg_t                      *
    mlm_msg_encode_stream_deliver (
        const char *stream,
        const char *sender,
        const char *subject,
        zmsg_t *content);

//  Encode the MAILBOX_SEND    
zmsg_t                      *
    mlm_msg_encode_mailbox_send  (
        const char *address,
        const char *subject,
        zmsg_t *content,
        const char *tracking,
        uint32_t timeout);

//  Encode the MAILBOX_DELIVER 
zmsg_t                      *
    mlm_msg_encode_mailbox_deliver (
        const char *sender,
        const char *address,
        const char *subject,
        zmsg_t *content,
        const char *tracking);

//  Encode the SERVICE_SEND    
zmsg_t                      *
    mlm_msg_encode_service_send  (
        const char *service,
        const char *subject,
        zmsg_t *content,
        const char *tracking,
        uint32_t timeout);

//  Encode the SERVICE_OFFER   
zmsg_t                      *
    mlm_msg_encode_service_offer (
        const char *service,
        const char *pattern);

//  Encode the SERVICE_DELIVER 
zmsg_t                      *
    mlm_msg_encode_service_deliver (
        const char *sender,
        const char *service,
        const char *subject,
        zmsg_t *content,
        const char *tracking);

//  Encode the OK              
zmsg_t                      *
    mlm_msg_encode_ok            (
        uint16_t status_code,
        const char *status_reason);

//  Encode the ERROR           
zmsg_t                      *
    mlm_msg_encode_error         (
        uint16_t status_code,
        const char *status_reason);

//  Encode the CREDIT          
zmsg_t                      *
    mlm_msg_encode_credit        (
        uint16_t amount);

//  Encode the CONFIRM         
zmsg_t                      *
    mlm_msg_encode_confirm       (
        const char *tracking,
        uint16_t status_code,
        const char *status_reason);


//  Send the CONNECTION_OPEN to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_connection_open (void *output,
        const char *address);
    
//  Send the CONNECTION_PING to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_connection_ping (void *output);
    
//  Send the CONNECTION_PONG to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_connection_pong (void *output);
    
//  Send the CONNECTION_CLOSE to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_connection_close (void *output);
    
//  Send the STREAM_WRITE    to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_stream_write  (void *output,
        const char *stream);
    
//  Send the STREAM_READ     to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_stream_read   (void *output,
        const char *stream,
        const char *pattern);
    
//  Send the STREAM_PUBLISH  to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_stream_publish (void *output,
        const char *subject,
        zmsg_t     *content);
    
//  Send the STREAM_DELIVER  to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_stream_deliver (void *output,
        const char *stream,
        const char *sender,
        const char *subject,
        zmsg_t     *content);
    
//  Send the MAILBOX_SEND    to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_mailbox_send  (void *output,
        const char *address,
        const char *subject,
        zmsg_t     *content,
        const char *tracking,
        uint32_t timeout);
    
//  Send the MAILBOX_DELIVER to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_mailbox_deliver (void *output,
        const char *sender,
        const char *address,
        const char *subject,
        zmsg_t     *content,
        const char *tracking);
    
//  Send the SERVICE_SEND    to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_service_send  (void *output,
        const char *service,
        const char *subject,
        zmsg_t     *content,
        const char *tracking,
        uint32_t timeout);
    
//  Send the SERVICE_OFFER   to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_service_offer (void *output,
        const char *service,
        const char *pattern);
    
//  Send the SERVICE_DELIVER to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_service_deliver (void *output,
        const char *sender,
        const char *service,
        const char *subject,
        zmsg_t     *content,
        const char *tracking);
    
//  Send the OK              to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_ok            (void *output,
        uint16_t status_code,
        const char *status_reason);
    
//  Send the ERROR           to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_error         (void *output,
        uint16_t status_code,
        const char *status_reason);
    
//  Send the CREDIT          to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_credit        (void *output,
        uint16_t amount);
    
//  Send the CONFIRM         to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_confirm       (void *output,
        const char *tracking,
        uint16_t status_code,
        const char *status_reason);
    
//  Duplicate the mlm_msg       message
mlm_msg_t                            *
    mlm_msg_dup       (mlm_msg_t       *self);

//  Print contents of message to stdout
void
    mlm_msg_print       (mlm_msg_t       *self);

//  Get/set the message routing id
zframe_t                      *
    mlm_msg_routing_id       (mlm_msg_t       *self);
void
    mlm_msg_set_routing_id       (mlm_msg_t       *self, zframe_t *routing_id);

//  Get the mlm_msg       id and printable command
int
    mlm_msg_id       (mlm_msg_t       *self);
void
    mlm_msg_set_id       (mlm_msg_t       *self, int id);
const                      char *
    mlm_msg_command       (mlm_msg_t       *self);

//  Get/set the address field
const                      char *
    mlm_msg_address       (mlm_msg_t       *self);
void
    mlm_msg_set_address       (mlm_msg_t       *self, const char *format, ...);

//  Get/set the stream  field
const                      char *
    mlm_msg_stream        (mlm_msg_t       *self);
void
    mlm_msg_set_stream        (mlm_msg_t       *self, const char *format, ...);

//  Get/set the pattern field
const                      char *
    mlm_msg_pattern       (mlm_msg_t       *self);
void
    mlm_msg_set_pattern       (mlm_msg_t       *self, const char *format, ...);

//  Get/set the subject field
const                      char *
    mlm_msg_subject       (mlm_msg_t       *self);
void
    mlm_msg_set_subject       (mlm_msg_t       *self, const char *format, ...);

//  Get a copy of the content field
zmsg_t                          *
    mlm_msg_content       (mlm_msg_t       *self);
//  Get the content field and transfer ownership to caller
zmsg_t                          *
    mlm_msg_get_content       (mlm_msg_t       *self);
//  Set the content field, transferring ownership from caller
void
    mlm_msg_set_content       (mlm_msg_t       *self, zmsg_t     **msg_p);

//  Get/set the sender  field
const                      char *
    mlm_msg_sender        (mlm_msg_t       *self);
void
    mlm_msg_set_sender        (mlm_msg_t       *self, const char *format, ...);

//  Get/set the tracking field
const                      char *
    mlm_msg_tracking      (mlm_msg_t       *self);
void
    mlm_msg_set_tracking      (mlm_msg_t       *self, const char *format, ...);

//  Get/set the timeout field
uint32_t
    mlm_msg_timeout       (mlm_msg_t       *self);
void
    mlm_msg_set_timeout       (mlm_msg_t       *self, uint32_t timeout);

//  Get/set the service field
const                      char *
    mlm_msg_service       (mlm_msg_t       *self);
void
    mlm_msg_set_service       (mlm_msg_t       *self, const char *format, ...);

//  Get/set the status_code field
uint16_t
    mlm_msg_status_code   (mlm_msg_t       *self);
void
    mlm_msg_set_status_code   (mlm_msg_t       *self, uint16_t status_code);

//  Get/set the status_reason field
const                      char *
    mlm_msg_status_reason (mlm_msg_t       *self);
void
    mlm_msg_set_status_reason (mlm_msg_t       *self, const char *format, ...);

//  Get/set the amount  field
uint16_t
    mlm_msg_amount        (mlm_msg_t       *self);
void
    mlm_msg_set_amount        (mlm_msg_t       *self, uint16_t amount);

//  Self test of this class
int
    mlm_msg_test       (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define mlm_msg_dump        mlm_msg_print

#ifdef __cplusplus
}
#endif

#endif
