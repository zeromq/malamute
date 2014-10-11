/*  =========================================================================
    mlm_msg - The Malamute Protocol

    Codec class for mlm_msg.

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

/*
@header
    mlm_msg - The Malamute Protocol
@discuss
@end
*/

#include "../include/malamute.h"
#include "../include/mlm_msg.h"

//  Structure of our class

struct _mlm_msg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  mlm_msg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) \
        goto malformed; \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
        goto malformed; \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
        goto malformed; \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
        goto malformed; \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
        goto malformed; \
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
        goto malformed; \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new mlm_msg

mlm_msg_t *
mlm_msg_new (int id)
{
    mlm_msg_t *self = (mlm_msg_t *) zmalloc (sizeof (mlm_msg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the mlm_msg

void
mlm_msg_destroy (mlm_msg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mlm_msg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Parse a mlm_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.

mlm_msg_t *
mlm_msg_decode (zmsg_t **msg_p)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;
        
    mlm_msg_t *self = mlm_msg_new (0);
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case MLM_MSG_HELLO:
            break;

        case MLM_MSG_WORLD:
            break;

        default:
            goto malformed;
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        zsys_error ("malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        mlm_msg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Encode mlm_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.

zmsg_t *
mlm_msg_encode (mlm_msg_t **self_p)
{
    assert (self_p);
    assert (*self_p);
    
    mlm_msg_t *self = *self_p;
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case MLM_MSG_HELLO:
            break;
            
        case MLM_MSG_WORLD:
            break;
            
        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case MLM_MSG_HELLO:
            break;

        case MLM_MSG_WORLD:
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        mlm_msg_destroy (self_p);
        return NULL;
    }
    //  Destroy mlm_msg object
    mlm_msg_destroy (self_p);
    return msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a mlm_msg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

mlm_msg_t *
mlm_msg_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    mlm_msg_t *mlm_msg = mlm_msg_decode (&msg);
    if (mlm_msg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        mlm_msg->routing_id = routing_id;

    return mlm_msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a mlm_msg from the socket. Returns new object,
//  or NULL either if there was no input waiting, or the recv was interrupted.

mlm_msg_t *
mlm_msg_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    if (!msg)
        return NULL;            //  Interrupted
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    mlm_msg_t *mlm_msg = mlm_msg_decode (&msg);
    if (mlm_msg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        mlm_msg->routing_id = routing_id;

    return mlm_msg;
}


//  --------------------------------------------------------------------------
//  Send the mlm_msg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
mlm_msg_send (mlm_msg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    //  Save routing_id if any, as encode will destroy it
    mlm_msg_t *self = *self_p;
    zframe_t *routing_id = self->routing_id;
    self->routing_id = NULL;

    //  Encode mlm_msg message to a single zmsg
    zmsg_t *msg = mlm_msg_encode (self_p);
    
    //  If we're sending to a ROUTER, send the routing_id first
    if (zsocket_type (zsock_resolve (output)) == ZMQ_ROUTER) {
        assert (routing_id);
        zmsg_prepend (msg, &routing_id);
    }
    else
        zframe_destroy (&routing_id);
        
    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the mlm_msg to the output, and do not destroy it

int
mlm_msg_send_again (mlm_msg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = mlm_msg_dup (self);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Encode HELLO message

zmsg_t * 
mlm_msg_encode_hello (
)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_HELLO);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode WORLD message

zmsg_t * 
mlm_msg_encode_world (
)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_WORLD);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the HELLO to the socket in one step

int
mlm_msg_send_hello (
    void *output)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_HELLO);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the WORLD to the socket in one step

int
mlm_msg_send_world (
    void *output)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_WORLD);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the mlm_msg message

mlm_msg_t *
mlm_msg_dup (mlm_msg_t *self)
{
    if (!self)
        return NULL;
        
    mlm_msg_t *copy = mlm_msg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case MLM_MSG_HELLO:
            break;

        case MLM_MSG_WORLD:
            break;

    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
mlm_msg_print (mlm_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case MLM_MSG_HELLO:
            zsys_debug ("MLM_MSG_HELLO:");
            break;
            
        case MLM_MSG_WORLD:
            zsys_debug ("MLM_MSG_WORLD:");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
mlm_msg_routing_id (mlm_msg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
mlm_msg_set_routing_id (mlm_msg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the mlm_msg id

int
mlm_msg_id (mlm_msg_t *self)
{
    assert (self);
    return self->id;
}

void
mlm_msg_set_id (mlm_msg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
mlm_msg_command (mlm_msg_t *self)
{
    assert (self);
    switch (self->id) {
        case MLM_MSG_HELLO:
            return ("HELLO");
            break;
        case MLM_MSG_WORLD:
            return ("WORLD");
            break;
    }
    return "?";
}


//  --------------------------------------------------------------------------
//  Selftest

int
mlm_msg_test (bool verbose)
{
    printf (" * mlm_msg: ");

    //  @selftest
    //  Simple create/destroy test
    mlm_msg_t *self = mlm_msg_new (0);
    assert (self);
    mlm_msg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-mlm_msg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-mlm_msg");

    //  Encode/send/decode and verify each message type
    int instance;
    mlm_msg_t *copy;
    self = mlm_msg_new (MLM_MSG_HELLO);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_WORLD);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        mlm_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
