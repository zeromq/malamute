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
    This file is part of the Malamute Project.                         
                                                                       
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
    char *protocol;                     //  Constant "MALAMUTE"
    uint16_t version;                   //  Protocol version 1
    char *address;                      //  Client address
    char *stream;                       //  Name of stream
    char *pattern;                      //  Match message subjects
    char *subject;                      //  Message subject
    zmsg_t *content;                    //  Message body frames
    char *sender;                       //  Sending client address
    char *tracker;                      //  Message tracker
    uint32_t timeout;                   //  Timeout, msecs, or zero
    char *service;                      //  Service name
    uint16_t status_code;               //  3-digit status code
    char *status_reason;                //  Printable explanation
    uint16_t amount;                    //  Number of messages
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
        free (self->protocol);
        free (self->address);
        free (self->stream);
        free (self->pattern);
        free (self->subject);
        zmsg_destroy (&self->content);
        free (self->sender);
        free (self->tracker);
        free (self->service);
        free (self->status_reason);

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
    if (signature != (0xAAA0 | 8))
        goto empty;             //  Invalid signature

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case MLM_MSG_CONNECTION_OPEN:
            GET_STRING (self->protocol);
            if (strneq (self->protocol, "MALAMUTE"))
                goto malformed;
            GET_NUMBER2 (self->version);
            if (self->version != 1)
                goto malformed;
            GET_STRING (self->address);
            break;

        case MLM_MSG_CONNECTION_PING:
            break;

        case MLM_MSG_CONNECTION_PONG:
            break;

        case MLM_MSG_CONNECTION_CLOSE:
            break;

        case MLM_MSG_STREAM_WRITE:
            GET_STRING (self->stream);
            break;

        case MLM_MSG_STREAM_READ:
            GET_STRING (self->stream);
            GET_STRING (self->pattern);
            break;

        case MLM_MSG_STREAM_PUBLISH:
            GET_STRING (self->subject);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case MLM_MSG_STREAM_DELIVER:
            GET_STRING (self->stream);
            GET_STRING (self->sender);
            GET_STRING (self->subject);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case MLM_MSG_MAILBOX_SEND:
            GET_STRING (self->address);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            GET_NUMBER4 (self->timeout);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case MLM_MSG_MAILBOX_DELIVER:
            GET_STRING (self->sender);
            GET_STRING (self->address);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case MLM_MSG_SERVICE_SEND:
            GET_STRING (self->service);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            GET_NUMBER4 (self->timeout);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case MLM_MSG_SERVICE_OFFER:
            GET_STRING (self->service);
            GET_STRING (self->pattern);
            break;

        case MLM_MSG_SERVICE_DELIVER:
            GET_STRING (self->sender);
            GET_STRING (self->service);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            //  Get zero or more remaining frames, leaving current
            //  frame untouched
            self->content = zmsg_new ();
            while (zmsg_size (msg))
                zmsg_add (self->content, zmsg_pop (msg));
            break;

        case MLM_MSG_OK:
            GET_NUMBER2 (self->status_code);
            GET_STRING (self->status_reason);
            break;

        case MLM_MSG_ERROR:
            GET_NUMBER2 (self->status_code);
            GET_STRING (self->status_reason);
            break;

        case MLM_MSG_CREDIT:
            GET_NUMBER2 (self->amount);
            break;

        case MLM_MSG_CONFIRM:
            GET_STRING (self->tracker);
            GET_NUMBER2 (self->status_code);
            GET_STRING (self->status_reason);
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
        case MLM_MSG_CONNECTION_OPEN:
            //  protocol is a string with 1-byte length
            frame_size += 1 + strlen ("MALAMUTE");
            //  version is a 2-byte integer
            frame_size += 2;
            //  address is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->address)
                frame_size += strlen (self->address);
            break;
            
        case MLM_MSG_CONNECTION_PING:
            break;
            
        case MLM_MSG_CONNECTION_PONG:
            break;
            
        case MLM_MSG_CONNECTION_CLOSE:
            break;
            
        case MLM_MSG_STREAM_WRITE:
            //  stream is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->stream)
                frame_size += strlen (self->stream);
            break;
            
        case MLM_MSG_STREAM_READ:
            //  stream is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->stream)
                frame_size += strlen (self->stream);
            //  pattern is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->pattern)
                frame_size += strlen (self->pattern);
            break;
            
        case MLM_MSG_STREAM_PUBLISH:
            //  subject is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->subject)
                frame_size += strlen (self->subject);
            break;
            
        case MLM_MSG_STREAM_DELIVER:
            //  stream is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->stream)
                frame_size += strlen (self->stream);
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  subject is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->subject)
                frame_size += strlen (self->subject);
            break;
            
        case MLM_MSG_MAILBOX_SEND:
            //  address is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->address)
                frame_size += strlen (self->address);
            //  subject is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->subject)
                frame_size += strlen (self->subject);
            //  tracker is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->tracker)
                frame_size += strlen (self->tracker);
            //  timeout is a 4-byte integer
            frame_size += 4;
            break;
            
        case MLM_MSG_MAILBOX_DELIVER:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  address is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->address)
                frame_size += strlen (self->address);
            //  subject is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->subject)
                frame_size += strlen (self->subject);
            //  tracker is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->tracker)
                frame_size += strlen (self->tracker);
            break;
            
        case MLM_MSG_SERVICE_SEND:
            //  service is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->service)
                frame_size += strlen (self->service);
            //  subject is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->subject)
                frame_size += strlen (self->subject);
            //  tracker is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->tracker)
                frame_size += strlen (self->tracker);
            //  timeout is a 4-byte integer
            frame_size += 4;
            break;
            
        case MLM_MSG_SERVICE_OFFER:
            //  service is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->service)
                frame_size += strlen (self->service);
            //  pattern is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->pattern)
                frame_size += strlen (self->pattern);
            break;
            
        case MLM_MSG_SERVICE_DELIVER:
            //  sender is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->sender)
                frame_size += strlen (self->sender);
            //  service is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->service)
                frame_size += strlen (self->service);
            //  subject is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->subject)
                frame_size += strlen (self->subject);
            //  tracker is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->tracker)
                frame_size += strlen (self->tracker);
            break;
            
        case MLM_MSG_OK:
            //  status_code is a 2-byte integer
            frame_size += 2;
            //  status_reason is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->status_reason)
                frame_size += strlen (self->status_reason);
            break;
            
        case MLM_MSG_ERROR:
            //  status_code is a 2-byte integer
            frame_size += 2;
            //  status_reason is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->status_reason)
                frame_size += strlen (self->status_reason);
            break;
            
        case MLM_MSG_CREDIT:
            //  amount is a 2-byte integer
            frame_size += 2;
            break;
            
        case MLM_MSG_CONFIRM:
            //  tracker is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->tracker)
                frame_size += strlen (self->tracker);
            //  status_code is a 2-byte integer
            frame_size += 2;
            //  status_reason is a string with 1-byte length
            frame_size++;       //  Size is one octet
            if (self->status_reason)
                frame_size += strlen (self->status_reason);
            break;
            
        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 8);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case MLM_MSG_CONNECTION_OPEN:
            PUT_STRING ("MALAMUTE");
            PUT_NUMBER2 (1);
            if (self->address) {
                PUT_STRING (self->address);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_CONNECTION_PING:
            break;

        case MLM_MSG_CONNECTION_PONG:
            break;

        case MLM_MSG_CONNECTION_CLOSE:
            break;

        case MLM_MSG_STREAM_WRITE:
            if (self->stream) {
                PUT_STRING (self->stream);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_STREAM_READ:
            if (self->stream) {
                PUT_STRING (self->stream);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->pattern) {
                PUT_STRING (self->pattern);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_STREAM_PUBLISH:
            if (self->subject) {
                PUT_STRING (self->subject);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_STREAM_DELIVER:
            if (self->stream) {
                PUT_STRING (self->stream);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->subject) {
                PUT_STRING (self->subject);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_MAILBOX_SEND:
            if (self->address) {
                PUT_STRING (self->address);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->subject) {
                PUT_STRING (self->subject);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->tracker) {
                PUT_STRING (self->tracker);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER4 (self->timeout);
            break;

        case MLM_MSG_MAILBOX_DELIVER:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->address) {
                PUT_STRING (self->address);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->subject) {
                PUT_STRING (self->subject);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->tracker) {
                PUT_STRING (self->tracker);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_SERVICE_SEND:
            if (self->service) {
                PUT_STRING (self->service);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->subject) {
                PUT_STRING (self->subject);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->tracker) {
                PUT_STRING (self->tracker);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER4 (self->timeout);
            break;

        case MLM_MSG_SERVICE_OFFER:
            if (self->service) {
                PUT_STRING (self->service);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->pattern) {
                PUT_STRING (self->pattern);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_SERVICE_DELIVER:
            if (self->sender) {
                PUT_STRING (self->sender);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->service) {
                PUT_STRING (self->service);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->subject) {
                PUT_STRING (self->subject);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            if (self->tracker) {
                PUT_STRING (self->tracker);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_OK:
            PUT_NUMBER2 (self->status_code);
            if (self->status_reason) {
                PUT_STRING (self->status_reason);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_ERROR:
            PUT_NUMBER2 (self->status_code);
            if (self->status_reason) {
                PUT_STRING (self->status_reason);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

        case MLM_MSG_CREDIT:
            PUT_NUMBER2 (self->amount);
            break;

        case MLM_MSG_CONFIRM:
            if (self->tracker) {
                PUT_STRING (self->tracker);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            PUT_NUMBER2 (self->status_code);
            if (self->status_reason) {
                PUT_STRING (self->status_reason);
            }
            else
                PUT_NUMBER1 (0);    //  Empty string
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        mlm_msg_destroy (self_p);
        return NULL;
    }
    //  Now send the message field if there is any
    if (self->id == MLM_MSG_STREAM_PUBLISH) {
        if (self->content) {
            zframe_t *frame = zmsg_pop (self->content);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->content);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == MLM_MSG_STREAM_DELIVER) {
        if (self->content) {
            zframe_t *frame = zmsg_pop (self->content);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->content);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == MLM_MSG_MAILBOX_SEND) {
        if (self->content) {
            zframe_t *frame = zmsg_pop (self->content);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->content);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == MLM_MSG_MAILBOX_DELIVER) {
        if (self->content) {
            zframe_t *frame = zmsg_pop (self->content);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->content);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == MLM_MSG_SERVICE_SEND) {
        if (self->content) {
            zframe_t *frame = zmsg_pop (self->content);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->content);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
    }
    //  Now send the message field if there is any
    if (self->id == MLM_MSG_SERVICE_DELIVER) {
        if (self->content) {
            zframe_t *frame = zmsg_pop (self->content);
            while (frame) {
                zmsg_append (msg, &frame);
                frame = zmsg_pop (self->content);
            }
        }
        else {
            zframe_t *frame = zframe_new (NULL, 0);
            zmsg_append (msg, &frame);
        }
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
//  Encode CONNECTION_OPEN message

zmsg_t * 
mlm_msg_encode_connection_open (
    const char *address)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_OPEN);
    mlm_msg_set_address (self, address);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CONNECTION_PING message

zmsg_t * 
mlm_msg_encode_connection_ping (
)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_PING);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CONNECTION_PONG message

zmsg_t * 
mlm_msg_encode_connection_pong (
)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_PONG);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CONNECTION_CLOSE message

zmsg_t * 
mlm_msg_encode_connection_close (
)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_CLOSE);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode STREAM_WRITE message

zmsg_t * 
mlm_msg_encode_stream_write (
    const char *stream)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_WRITE);
    mlm_msg_set_stream (self, stream);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode STREAM_READ message

zmsg_t * 
mlm_msg_encode_stream_read (
    const char *stream,
    const char *pattern)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_READ);
    mlm_msg_set_stream (self, stream);
    mlm_msg_set_pattern (self, pattern);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode STREAM_PUBLISH message

zmsg_t * 
mlm_msg_encode_stream_publish (
    const char *subject,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_PUBLISH);
    mlm_msg_set_subject (self, subject);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode STREAM_DELIVER message

zmsg_t * 
mlm_msg_encode_stream_deliver (
    const char *stream,
    const char *sender,
    const char *subject,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_DELIVER);
    mlm_msg_set_stream (self, stream);
    mlm_msg_set_sender (self, sender);
    mlm_msg_set_subject (self, subject);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode MAILBOX_SEND message

zmsg_t * 
mlm_msg_encode_mailbox_send (
    const char *address,
    const char *subject,
    const char *tracker,
    uint32_t timeout,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_MAILBOX_SEND);
    mlm_msg_set_address (self, address);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    mlm_msg_set_timeout (self, timeout);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode MAILBOX_DELIVER message

zmsg_t * 
mlm_msg_encode_mailbox_deliver (
    const char *sender,
    const char *address,
    const char *subject,
    const char *tracker,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_MAILBOX_DELIVER);
    mlm_msg_set_sender (self, sender);
    mlm_msg_set_address (self, address);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode SERVICE_SEND message

zmsg_t * 
mlm_msg_encode_service_send (
    const char *service,
    const char *subject,
    const char *tracker,
    uint32_t timeout,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_SERVICE_SEND);
    mlm_msg_set_service (self, service);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    mlm_msg_set_timeout (self, timeout);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode SERVICE_OFFER message

zmsg_t * 
mlm_msg_encode_service_offer (
    const char *service,
    const char *pattern)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_SERVICE_OFFER);
    mlm_msg_set_service (self, service);
    mlm_msg_set_pattern (self, pattern);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode SERVICE_DELIVER message

zmsg_t * 
mlm_msg_encode_service_deliver (
    const char *sender,
    const char *service,
    const char *subject,
    const char *tracker,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_SERVICE_DELIVER);
    mlm_msg_set_sender (self, sender);
    mlm_msg_set_service (self, service);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode OK message

zmsg_t * 
mlm_msg_encode_ok (
    uint16_t status_code,
    const char *status_reason)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_OK);
    mlm_msg_set_status_code (self, status_code);
    mlm_msg_set_status_reason (self, status_reason);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode ERROR message

zmsg_t * 
mlm_msg_encode_error (
    uint16_t status_code,
    const char *status_reason)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_ERROR);
    mlm_msg_set_status_code (self, status_code);
    mlm_msg_set_status_reason (self, status_reason);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CREDIT message

zmsg_t * 
mlm_msg_encode_credit (
    uint16_t amount)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CREDIT);
    mlm_msg_set_amount (self, amount);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode CONFIRM message

zmsg_t * 
mlm_msg_encode_confirm (
    const char *tracker,
    uint16_t status_code,
    const char *status_reason)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONFIRM);
    mlm_msg_set_tracker (self, tracker);
    mlm_msg_set_status_code (self, status_code);
    mlm_msg_set_status_reason (self, status_reason);
    return mlm_msg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the CONNECTION_OPEN to the socket in one step

int
mlm_msg_send_connection_open (
    void *output,
    const char *address)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_OPEN);
    mlm_msg_set_address (self, address);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CONNECTION_PING to the socket in one step

int
mlm_msg_send_connection_ping (
    void *output)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_PING);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CONNECTION_PONG to the socket in one step

int
mlm_msg_send_connection_pong (
    void *output)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_PONG);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CONNECTION_CLOSE to the socket in one step

int
mlm_msg_send_connection_close (
    void *output)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONNECTION_CLOSE);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the STREAM_WRITE to the socket in one step

int
mlm_msg_send_stream_write (
    void *output,
    const char *stream)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_WRITE);
    mlm_msg_set_stream (self, stream);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the STREAM_READ to the socket in one step

int
mlm_msg_send_stream_read (
    void *output,
    const char *stream,
    const char *pattern)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_READ);
    mlm_msg_set_stream (self, stream);
    mlm_msg_set_pattern (self, pattern);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the STREAM_PUBLISH to the socket in one step

int
mlm_msg_send_stream_publish (
    void *output,
    const char *subject,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_PUBLISH);
    mlm_msg_set_subject (self, subject);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the STREAM_DELIVER to the socket in one step

int
mlm_msg_send_stream_deliver (
    void *output,
    const char *stream,
    const char *sender,
    const char *subject,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_STREAM_DELIVER);
    mlm_msg_set_stream (self, stream);
    mlm_msg_set_sender (self, sender);
    mlm_msg_set_subject (self, subject);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the MAILBOX_SEND to the socket in one step

int
mlm_msg_send_mailbox_send (
    void *output,
    const char *address,
    const char *subject,
    const char *tracker,
    uint32_t timeout,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_MAILBOX_SEND);
    mlm_msg_set_address (self, address);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    mlm_msg_set_timeout (self, timeout);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the MAILBOX_DELIVER to the socket in one step

int
mlm_msg_send_mailbox_deliver (
    void *output,
    const char *sender,
    const char *address,
    const char *subject,
    const char *tracker,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_MAILBOX_DELIVER);
    mlm_msg_set_sender (self, sender);
    mlm_msg_set_address (self, address);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the SERVICE_SEND to the socket in one step

int
mlm_msg_send_service_send (
    void *output,
    const char *service,
    const char *subject,
    const char *tracker,
    uint32_t timeout,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_SERVICE_SEND);
    mlm_msg_set_service (self, service);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    mlm_msg_set_timeout (self, timeout);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the SERVICE_OFFER to the socket in one step

int
mlm_msg_send_service_offer (
    void *output,
    const char *service,
    const char *pattern)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_SERVICE_OFFER);
    mlm_msg_set_service (self, service);
    mlm_msg_set_pattern (self, pattern);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the SERVICE_DELIVER to the socket in one step

int
mlm_msg_send_service_deliver (
    void *output,
    const char *sender,
    const char *service,
    const char *subject,
    const char *tracker,
    zmsg_t *content)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_SERVICE_DELIVER);
    mlm_msg_set_sender (self, sender);
    mlm_msg_set_service (self, service);
    mlm_msg_set_subject (self, subject);
    mlm_msg_set_tracker (self, tracker);
    zmsg_t *content_copy = zmsg_dup (content);
    mlm_msg_set_content (self, &content_copy);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the OK to the socket in one step

int
mlm_msg_send_ok (
    void *output,
    uint16_t status_code,
    const char *status_reason)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_OK);
    mlm_msg_set_status_code (self, status_code);
    mlm_msg_set_status_reason (self, status_reason);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the ERROR to the socket in one step

int
mlm_msg_send_error (
    void *output,
    uint16_t status_code,
    const char *status_reason)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_ERROR);
    mlm_msg_set_status_code (self, status_code);
    mlm_msg_set_status_reason (self, status_reason);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CREDIT to the socket in one step

int
mlm_msg_send_credit (
    void *output,
    uint16_t amount)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CREDIT);
    mlm_msg_set_amount (self, amount);
    return mlm_msg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the CONFIRM to the socket in one step

int
mlm_msg_send_confirm (
    void *output,
    const char *tracker,
    uint16_t status_code,
    const char *status_reason)
{
    mlm_msg_t *self = mlm_msg_new (MLM_MSG_CONFIRM);
    mlm_msg_set_tracker (self, tracker);
    mlm_msg_set_status_code (self, status_code);
    mlm_msg_set_status_reason (self, status_reason);
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
        case MLM_MSG_CONNECTION_OPEN:
            copy->protocol = self->protocol? strdup (self->protocol): NULL;
            copy->version = self->version;
            copy->address = self->address? strdup (self->address): NULL;
            break;

        case MLM_MSG_CONNECTION_PING:
            break;

        case MLM_MSG_CONNECTION_PONG:
            break;

        case MLM_MSG_CONNECTION_CLOSE:
            break;

        case MLM_MSG_STREAM_WRITE:
            copy->stream = self->stream? strdup (self->stream): NULL;
            break;

        case MLM_MSG_STREAM_READ:
            copy->stream = self->stream? strdup (self->stream): NULL;
            copy->pattern = self->pattern? strdup (self->pattern): NULL;
            break;

        case MLM_MSG_STREAM_PUBLISH:
            copy->subject = self->subject? strdup (self->subject): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case MLM_MSG_STREAM_DELIVER:
            copy->stream = self->stream? strdup (self->stream): NULL;
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->subject = self->subject? strdup (self->subject): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case MLM_MSG_MAILBOX_SEND:
            copy->address = self->address? strdup (self->address): NULL;
            copy->subject = self->subject? strdup (self->subject): NULL;
            copy->tracker = self->tracker? strdup (self->tracker): NULL;
            copy->timeout = self->timeout;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case MLM_MSG_MAILBOX_DELIVER:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->address = self->address? strdup (self->address): NULL;
            copy->subject = self->subject? strdup (self->subject): NULL;
            copy->tracker = self->tracker? strdup (self->tracker): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case MLM_MSG_SERVICE_SEND:
            copy->service = self->service? strdup (self->service): NULL;
            copy->subject = self->subject? strdup (self->subject): NULL;
            copy->tracker = self->tracker? strdup (self->tracker): NULL;
            copy->timeout = self->timeout;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case MLM_MSG_SERVICE_OFFER:
            copy->service = self->service? strdup (self->service): NULL;
            copy->pattern = self->pattern? strdup (self->pattern): NULL;
            break;

        case MLM_MSG_SERVICE_DELIVER:
            copy->sender = self->sender? strdup (self->sender): NULL;
            copy->service = self->service? strdup (self->service): NULL;
            copy->subject = self->subject? strdup (self->subject): NULL;
            copy->tracker = self->tracker? strdup (self->tracker): NULL;
            copy->content = self->content? zmsg_dup (self->content): NULL;
            break;

        case MLM_MSG_OK:
            copy->status_code = self->status_code;
            copy->status_reason = self->status_reason? strdup (self->status_reason): NULL;
            break;

        case MLM_MSG_ERROR:
            copy->status_code = self->status_code;
            copy->status_reason = self->status_reason? strdup (self->status_reason): NULL;
            break;

        case MLM_MSG_CREDIT:
            copy->amount = self->amount;
            break;

        case MLM_MSG_CONFIRM:
            copy->tracker = self->tracker? strdup (self->tracker): NULL;
            copy->status_code = self->status_code;
            copy->status_reason = self->status_reason? strdup (self->status_reason): NULL;
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
        case MLM_MSG_CONNECTION_OPEN:
            zsys_debug ("MLM_MSG_CONNECTION_OPEN:");
            zsys_debug ("    protocol=malamute");
            zsys_debug ("    version=1");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            break;
            
        case MLM_MSG_CONNECTION_PING:
            zsys_debug ("MLM_MSG_CONNECTION_PING:");
            break;
            
        case MLM_MSG_CONNECTION_PONG:
            zsys_debug ("MLM_MSG_CONNECTION_PONG:");
            break;
            
        case MLM_MSG_CONNECTION_CLOSE:
            zsys_debug ("MLM_MSG_CONNECTION_CLOSE:");
            break;
            
        case MLM_MSG_STREAM_WRITE:
            zsys_debug ("MLM_MSG_STREAM_WRITE:");
            if (self->stream)
                zsys_debug ("    stream='%s'", self->stream);
            else
                zsys_debug ("    stream=");
            break;
            
        case MLM_MSG_STREAM_READ:
            zsys_debug ("MLM_MSG_STREAM_READ:");
            if (self->stream)
                zsys_debug ("    stream='%s'", self->stream);
            else
                zsys_debug ("    stream=");
            if (self->pattern)
                zsys_debug ("    pattern='%s'", self->pattern);
            else
                zsys_debug ("    pattern=");
            break;
            
        case MLM_MSG_STREAM_PUBLISH:
            zsys_debug ("MLM_MSG_STREAM_PUBLISH:");
            if (self->subject)
                zsys_debug ("    subject='%s'", self->subject);
            else
                zsys_debug ("    subject=");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MLM_MSG_STREAM_DELIVER:
            zsys_debug ("MLM_MSG_STREAM_DELIVER:");
            if (self->stream)
                zsys_debug ("    stream='%s'", self->stream);
            else
                zsys_debug ("    stream=");
            if (self->sender)
                zsys_debug ("    sender='%s'", self->sender);
            else
                zsys_debug ("    sender=");
            if (self->subject)
                zsys_debug ("    subject='%s'", self->subject);
            else
                zsys_debug ("    subject=");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MLM_MSG_MAILBOX_SEND:
            zsys_debug ("MLM_MSG_MAILBOX_SEND:");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            if (self->subject)
                zsys_debug ("    subject='%s'", self->subject);
            else
                zsys_debug ("    subject=");
            if (self->tracker)
                zsys_debug ("    tracker='%s'", self->tracker);
            else
                zsys_debug ("    tracker=");
            zsys_debug ("    timeout=%ld", (long) self->timeout);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MLM_MSG_MAILBOX_DELIVER:
            zsys_debug ("MLM_MSG_MAILBOX_DELIVER:");
            if (self->sender)
                zsys_debug ("    sender='%s'", self->sender);
            else
                zsys_debug ("    sender=");
            if (self->address)
                zsys_debug ("    address='%s'", self->address);
            else
                zsys_debug ("    address=");
            if (self->subject)
                zsys_debug ("    subject='%s'", self->subject);
            else
                zsys_debug ("    subject=");
            if (self->tracker)
                zsys_debug ("    tracker='%s'", self->tracker);
            else
                zsys_debug ("    tracker=");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MLM_MSG_SERVICE_SEND:
            zsys_debug ("MLM_MSG_SERVICE_SEND:");
            if (self->service)
                zsys_debug ("    service='%s'", self->service);
            else
                zsys_debug ("    service=");
            if (self->subject)
                zsys_debug ("    subject='%s'", self->subject);
            else
                zsys_debug ("    subject=");
            if (self->tracker)
                zsys_debug ("    tracker='%s'", self->tracker);
            else
                zsys_debug ("    tracker=");
            zsys_debug ("    timeout=%ld", (long) self->timeout);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MLM_MSG_SERVICE_OFFER:
            zsys_debug ("MLM_MSG_SERVICE_OFFER:");
            if (self->service)
                zsys_debug ("    service='%s'", self->service);
            else
                zsys_debug ("    service=");
            if (self->pattern)
                zsys_debug ("    pattern='%s'", self->pattern);
            else
                zsys_debug ("    pattern=");
            break;
            
        case MLM_MSG_SERVICE_DELIVER:
            zsys_debug ("MLM_MSG_SERVICE_DELIVER:");
            if (self->sender)
                zsys_debug ("    sender='%s'", self->sender);
            else
                zsys_debug ("    sender=");
            if (self->service)
                zsys_debug ("    service='%s'", self->service);
            else
                zsys_debug ("    service=");
            if (self->subject)
                zsys_debug ("    subject='%s'", self->subject);
            else
                zsys_debug ("    subject=");
            if (self->tracker)
                zsys_debug ("    tracker='%s'", self->tracker);
            else
                zsys_debug ("    tracker=");
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;
            
        case MLM_MSG_OK:
            zsys_debug ("MLM_MSG_OK:");
            zsys_debug ("    status_code=%ld", (long) self->status_code);
            if (self->status_reason)
                zsys_debug ("    status_reason='%s'", self->status_reason);
            else
                zsys_debug ("    status_reason=");
            break;
            
        case MLM_MSG_ERROR:
            zsys_debug ("MLM_MSG_ERROR:");
            zsys_debug ("    status_code=%ld", (long) self->status_code);
            if (self->status_reason)
                zsys_debug ("    status_reason='%s'", self->status_reason);
            else
                zsys_debug ("    status_reason=");
            break;
            
        case MLM_MSG_CREDIT:
            zsys_debug ("MLM_MSG_CREDIT:");
            zsys_debug ("    amount=%ld", (long) self->amount);
            break;
            
        case MLM_MSG_CONFIRM:
            zsys_debug ("MLM_MSG_CONFIRM:");
            if (self->tracker)
                zsys_debug ("    tracker='%s'", self->tracker);
            else
                zsys_debug ("    tracker=");
            zsys_debug ("    status_code=%ld", (long) self->status_code);
            if (self->status_reason)
                zsys_debug ("    status_reason='%s'", self->status_reason);
            else
                zsys_debug ("    status_reason=");
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
        case MLM_MSG_CONNECTION_OPEN:
            return ("CONNECTION_OPEN");
            break;
        case MLM_MSG_CONNECTION_PING:
            return ("CONNECTION_PING");
            break;
        case MLM_MSG_CONNECTION_PONG:
            return ("CONNECTION_PONG");
            break;
        case MLM_MSG_CONNECTION_CLOSE:
            return ("CONNECTION_CLOSE");
            break;
        case MLM_MSG_STREAM_WRITE:
            return ("STREAM_WRITE");
            break;
        case MLM_MSG_STREAM_READ:
            return ("STREAM_READ");
            break;
        case MLM_MSG_STREAM_PUBLISH:
            return ("STREAM_PUBLISH");
            break;
        case MLM_MSG_STREAM_DELIVER:
            return ("STREAM_DELIVER");
            break;
        case MLM_MSG_MAILBOX_SEND:
            return ("MAILBOX_SEND");
            break;
        case MLM_MSG_MAILBOX_DELIVER:
            return ("MAILBOX_DELIVER");
            break;
        case MLM_MSG_SERVICE_SEND:
            return ("SERVICE_SEND");
            break;
        case MLM_MSG_SERVICE_OFFER:
            return ("SERVICE_OFFER");
            break;
        case MLM_MSG_SERVICE_DELIVER:
            return ("SERVICE_DELIVER");
            break;
        case MLM_MSG_OK:
            return ("OK");
            break;
        case MLM_MSG_ERROR:
            return ("ERROR");
            break;
        case MLM_MSG_CREDIT:
            return ("CREDIT");
            break;
        case MLM_MSG_CONFIRM:
            return ("CONFIRM");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the address field

const char *
mlm_msg_address (mlm_msg_t *self)
{
    assert (self);
    return self->address;
}

void
mlm_msg_set_address (mlm_msg_t *self, const char *format, ...)
{
    //  Format address from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->address);
    self->address = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the stream field

const char *
mlm_msg_stream (mlm_msg_t *self)
{
    assert (self);
    return self->stream;
}

void
mlm_msg_set_stream (mlm_msg_t *self, const char *format, ...)
{
    //  Format stream from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->stream);
    self->stream = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the pattern field

const char *
mlm_msg_pattern (mlm_msg_t *self)
{
    assert (self);
    return self->pattern;
}

void
mlm_msg_set_pattern (mlm_msg_t *self, const char *format, ...)
{
    //  Format pattern from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->pattern);
    self->pattern = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the subject field

const char *
mlm_msg_subject (mlm_msg_t *self)
{
    assert (self);
    return self->subject;
}

void
mlm_msg_set_subject (mlm_msg_t *self, const char *format, ...)
{
    //  Format subject from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->subject);
    self->subject = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get the content field without transferring ownership

zmsg_t *
mlm_msg_content (mlm_msg_t *self)
{
    assert (self);
    return self->content;
}

//  Get the content field and transfer ownership to caller

zmsg_t *
mlm_msg_get_content (mlm_msg_t *self)
{
    zmsg_t *content = self->content;
    self->content = NULL;
    return content;
}

//  Set the content field, transferring ownership from caller

void
mlm_msg_set_content (mlm_msg_t *self, zmsg_t **msg_p)
{
    assert (self);
    assert (msg_p);
    zmsg_destroy (&self->content);
    self->content = *msg_p;
    *msg_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the sender field

const char *
mlm_msg_sender (mlm_msg_t *self)
{
    assert (self);
    return self->sender;
}

void
mlm_msg_set_sender (mlm_msg_t *self, const char *format, ...)
{
    //  Format sender from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->sender);
    self->sender = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the tracker field

const char *
mlm_msg_tracker (mlm_msg_t *self)
{
    assert (self);
    return self->tracker;
}

void
mlm_msg_set_tracker (mlm_msg_t *self, const char *format, ...)
{
    //  Format tracker from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->tracker);
    self->tracker = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the timeout field

uint32_t
mlm_msg_timeout (mlm_msg_t *self)
{
    assert (self);
    return self->timeout;
}

void
mlm_msg_set_timeout (mlm_msg_t *self, uint32_t timeout)
{
    assert (self);
    self->timeout = timeout;
}


//  --------------------------------------------------------------------------
//  Get/set the service field

const char *
mlm_msg_service (mlm_msg_t *self)
{
    assert (self);
    return self->service;
}

void
mlm_msg_set_service (mlm_msg_t *self, const char *format, ...)
{
    //  Format service from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->service);
    self->service = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the status_code field

uint16_t
mlm_msg_status_code (mlm_msg_t *self)
{
    assert (self);
    return self->status_code;
}

void
mlm_msg_set_status_code (mlm_msg_t *self, uint16_t status_code)
{
    assert (self);
    self->status_code = status_code;
}


//  --------------------------------------------------------------------------
//  Get/set the status_reason field

const char *
mlm_msg_status_reason (mlm_msg_t *self)
{
    assert (self);
    return self->status_reason;
}

void
mlm_msg_set_status_reason (mlm_msg_t *self, const char *format, ...)
{
    //  Format status_reason from provided arguments
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    free (self->status_reason);
    self->status_reason = zsys_vprintf (format, argptr);
    va_end (argptr);
}


//  --------------------------------------------------------------------------
//  Get/set the amount field

uint16_t
mlm_msg_amount (mlm_msg_t *self)
{
    assert (self);
    return self->amount;
}

void
mlm_msg_set_amount (mlm_msg_t *self, uint16_t amount)
{
    assert (self);
    self->amount = amount;
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
    self = mlm_msg_new (MLM_MSG_CONNECTION_OPEN);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_address (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_address (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_CONNECTION_PING);
    
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
    self = mlm_msg_new (MLM_MSG_CONNECTION_PONG);
    
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
    self = mlm_msg_new (MLM_MSG_CONNECTION_CLOSE);
    
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
    self = mlm_msg_new (MLM_MSG_STREAM_WRITE);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_stream (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_stream (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_STREAM_READ);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_stream (self, "Life is short but Now lasts for ever");
    mlm_msg_set_pattern (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_stream (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_pattern (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_STREAM_PUBLISH);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_subject (self, "Life is short but Now lasts for ever");
    zmsg_t *stream_publish_content = zmsg_new ();
    mlm_msg_set_content (self, &stream_publish_content);
    zmsg_addstr (mlm_msg_content (self), "Hello, World");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_subject (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_msg_content (self)) == 1);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_STREAM_DELIVER);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_stream (self, "Life is short but Now lasts for ever");
    mlm_msg_set_sender (self, "Life is short but Now lasts for ever");
    mlm_msg_set_subject (self, "Life is short but Now lasts for ever");
    zmsg_t *stream_deliver_content = zmsg_new ();
    mlm_msg_set_content (self, &stream_deliver_content);
    zmsg_addstr (mlm_msg_content (self), "Hello, World");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_stream (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_subject (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_msg_content (self)) == 1);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_MAILBOX_SEND);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_address (self, "Life is short but Now lasts for ever");
    mlm_msg_set_subject (self, "Life is short but Now lasts for ever");
    mlm_msg_set_tracker (self, "Life is short but Now lasts for ever");
    mlm_msg_set_timeout (self, 123);
    zmsg_t *mailbox_send_content = zmsg_new ();
    mlm_msg_set_content (self, &mailbox_send_content);
    zmsg_addstr (mlm_msg_content (self), "Hello, World");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_tracker (self), "Life is short but Now lasts for ever"));
        assert (mlm_msg_timeout (self) == 123);
        assert (zmsg_size (mlm_msg_content (self)) == 1);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_MAILBOX_DELIVER);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_sender (self, "Life is short but Now lasts for ever");
    mlm_msg_set_address (self, "Life is short but Now lasts for ever");
    mlm_msg_set_subject (self, "Life is short but Now lasts for ever");
    mlm_msg_set_tracker (self, "Life is short but Now lasts for ever");
    zmsg_t *mailbox_deliver_content = zmsg_new ();
    mlm_msg_set_content (self, &mailbox_deliver_content);
    zmsg_addstr (mlm_msg_content (self), "Hello, World");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_tracker (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_msg_content (self)) == 1);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_SERVICE_SEND);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_service (self, "Life is short but Now lasts for ever");
    mlm_msg_set_subject (self, "Life is short but Now lasts for ever");
    mlm_msg_set_tracker (self, "Life is short but Now lasts for ever");
    mlm_msg_set_timeout (self, 123);
    zmsg_t *service_send_content = zmsg_new ();
    mlm_msg_set_content (self, &service_send_content);
    zmsg_addstr (mlm_msg_content (self), "Hello, World");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_service (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_tracker (self), "Life is short but Now lasts for ever"));
        assert (mlm_msg_timeout (self) == 123);
        assert (zmsg_size (mlm_msg_content (self)) == 1);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_SERVICE_OFFER);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_service (self, "Life is short but Now lasts for ever");
    mlm_msg_set_pattern (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_service (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_pattern (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_SERVICE_DELIVER);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_sender (self, "Life is short but Now lasts for ever");
    mlm_msg_set_service (self, "Life is short but Now lasts for ever");
    mlm_msg_set_subject (self, "Life is short but Now lasts for ever");
    mlm_msg_set_tracker (self, "Life is short but Now lasts for ever");
    zmsg_t *service_deliver_content = zmsg_new ();
    mlm_msg_set_content (self, &service_deliver_content);
    zmsg_addstr (mlm_msg_content (self), "Hello, World");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_service (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_msg_tracker (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_msg_content (self)) == 1);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_OK);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_status_code (self, 123);
    mlm_msg_set_status_reason (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (mlm_msg_status_code (self) == 123);
        assert (streq (mlm_msg_status_reason (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_ERROR);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_status_code (self, 123);
    mlm_msg_set_status_reason (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (mlm_msg_status_code (self) == 123);
        assert (streq (mlm_msg_status_reason (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_CREDIT);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_amount (self, 123);
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (mlm_msg_amount (self) == 123);
        mlm_msg_destroy (&self);
    }
    self = mlm_msg_new (MLM_MSG_CONFIRM);
    
    //  Check that _dup works on empty message
    copy = mlm_msg_dup (self);
    assert (copy);
    mlm_msg_destroy (&copy);

    mlm_msg_set_tracker (self, "Life is short but Now lasts for ever");
    mlm_msg_set_status_code (self, 123);
    mlm_msg_set_status_reason (self, "Life is short but Now lasts for ever");
    //  Send twice from same object
    mlm_msg_send_again (self, output);
    mlm_msg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = mlm_msg_recv (input);
        assert (self);
        assert (mlm_msg_routing_id (self));
        
        assert (streq (mlm_msg_tracker (self), "Life is short but Now lasts for ever"));
        assert (mlm_msg_status_code (self) == 123);
        assert (streq (mlm_msg_status_reason (self), "Life is short but Now lasts for ever"));
        mlm_msg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
