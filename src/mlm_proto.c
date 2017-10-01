/*  =========================================================================
    mlm_proto - The Malamute Protocol

    Codec class for mlm_proto.

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

/*
@header
    mlm_proto - The Malamute Protocol
@discuss
@end
*/

#ifdef NDEBUG
#undef NDEBUG
#endif

#include "mlm_classes.h"
#include "../include/mlm_proto.h"

//  Structure of our class

struct _mlm_proto_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  mlm_proto message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    char address [256];                 //  Client address
    char stream [256];                  //  Name of stream
    char pattern [256];                 //  Match message subjects
    char subject [256];                 //  Message subject
    zmsg_t *content;                    //  Message body frames
    char sender [256];                  //  Sending client address
    char tracker [256];                 //  Message tracker
    uint32_t timeout;                   //  Timeout, msecs, or zero
    uint16_t status_code;               //  3-digit status code
    char status_reason [256];           //  Printable explanation
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
    if (self->needle + size > self->ceiling) { \
        zsys_warning ("mlm_proto: GET_OCTETS failed"); \
        goto malformed; \
    } \
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (byte) (host); \
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
    if (self->needle + 1 > self->ceiling) { \
        zsys_warning ("mlm_proto: GET_NUMBER1 failed"); \
        goto malformed; \
    } \
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) { \
        zsys_warning ("mlm_proto: GET_NUMBER2 failed"); \
        goto malformed; \
    } \
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) { \
        zsys_warning ("mlm_proto: GET_NUMBER4 failed"); \
        goto malformed; \
    } \
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) { \
        zsys_warning ("mlm_proto: GET_NUMBER8 failed"); \
        goto malformed; \
    } \
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
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("mlm_proto: GET_STRING failed"); \
        goto malformed; \
    } \
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
    if (self->needle + string_size > (self->ceiling)) { \
        zsys_warning ("mlm_proto: GET_LONGSTR failed"); \
        goto malformed; \
    } \
    free ((host)); \
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  --------------------------------------------------------------------------
//  bytes cstring conversion macros

// create new array of unsigned char from properly encoded string
// len of resulting array is strlen (str) / 2
// caller is responsibe for freeing up the memory
#define BYTES_FROM_STR(dst, _str) { \
    char *str = (char*) (_str); \
    if (!str || (strlen (str) % 2) != 0) \
        return NULL; \
\
    size_t strlen_2 = strlen (str) / 2; \
    byte *mem = (byte*) zmalloc (strlen_2); \
\
    for (size_t i = 0; i != strlen_2; i++) \
    { \
        char buff[3] = {0x0, 0x0, 0x0}; \
        strncpy (buff, str, 2); \
        unsigned int uint; \
        sscanf (buff, "%x", &uint); \
        assert (uint <= 0xff); \
        mem [i] = (byte) (0xff & uint); \
        str += 2; \
    } \
    dst = mem; \
}

// convert len bytes to hex string
// caller is responsibe for freeing up the memory
#define STR_FROM_BYTES(dst, _mem, _len) { \
    byte *mem = (byte*) (_mem); \
    size_t len = (size_t) (_len); \
    char* ret = (char*) zmalloc (2*len + 1); \
    char* aux = ret; \
    for (size_t i = 0; i != len; i++) \
    { \
        sprintf (aux, "%02x", mem [i]); \
        aux+=2; \
    } \
    dst = ret; \
}

//  --------------------------------------------------------------------------
//  Create a new mlm_proto

mlm_proto_t *
mlm_proto_new (void)
{
    mlm_proto_t *self = (mlm_proto_t *) zmalloc (sizeof (mlm_proto_t));
    return self;
}

//  --------------------------------------------------------------------------
//  Create a new mlm_proto from zpl/zconfig_t *

mlm_proto_t *
    mlm_proto_new_zpl (zconfig_t *config)
{
    assert (config);
    mlm_proto_t *self = NULL;
    char *message = zconfig_get (config, "message", NULL);
    if (!message) {
        zsys_error ("Can't find 'message' section");
        return NULL;
    }

    if (streq ("MLM_PROTO_CONNECTION_OPEN", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_CONNECTION_OPEN);
    }
    else
    if (streq ("MLM_PROTO_CONNECTION_PING", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_CONNECTION_PING);
    }
    else
    if (streq ("MLM_PROTO_CONNECTION_PONG", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_CONNECTION_PONG);
    }
    else
    if (streq ("MLM_PROTO_CONNECTION_CLOSE", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_CONNECTION_CLOSE);
    }
    else
    if (streq ("MLM_PROTO_STREAM_WRITE", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_STREAM_WRITE);
    }
    else
    if (streq ("MLM_PROTO_STREAM_READ", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_STREAM_READ);
    }
    else
    if (streq ("MLM_PROTO_STREAM_SEND", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_STREAM_SEND);
    }
    else
    if (streq ("MLM_PROTO_STREAM_DELIVER", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_STREAM_DELIVER);
    }
    else
    if (streq ("MLM_PROTO_MAILBOX_SEND", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_MAILBOX_SEND);
    }
    else
    if (streq ("MLM_PROTO_MAILBOX_DELIVER", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_MAILBOX_DELIVER);
    }
    else
    if (streq ("MLM_PROTO_SERVICE_SEND", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_SERVICE_SEND);
    }
    else
    if (streq ("MLM_PROTO_SERVICE_OFFER", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_SERVICE_OFFER);
    }
    else
    if (streq ("MLM_PROTO_SERVICE_DELIVER", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_SERVICE_DELIVER);
    }
    else
    if (streq ("MLM_PROTO_OK", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_OK);
    }
    else
    if (streq ("MLM_PROTO_ERROR", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_ERROR);
    }
    else
    if (streq ("MLM_PROTO_CREDIT", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_CREDIT);
    }
    else
    if (streq ("MLM_PROTO_CONFIRM", message)) {
        self = mlm_proto_new ();
        mlm_proto_set_id (self, MLM_PROTO_CONFIRM);
    }
    else
       {
        zsys_error ("message=%s is not known", message);
        return NULL;
       }

    char *s = zconfig_get (config, "routing_id", NULL);
    if (s) {
        byte *bvalue;
        BYTES_FROM_STR (bvalue, s);
        if (!bvalue) {
            mlm_proto_destroy (&self);
            return NULL;
        }
        zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
        free (bvalue);
        self->routing_id = frame;
    }

    zconfig_t *content = NULL;
    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            break;
        case MLM_PROTO_CONNECTION_PING:
            break;
        case MLM_PROTO_CONNECTION_PONG:
            break;
        case MLM_PROTO_CONNECTION_CLOSE:
            break;
        case MLM_PROTO_STREAM_WRITE:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "stream", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->stream, s, 256);
            }
            break;
        case MLM_PROTO_STREAM_READ:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "stream", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->stream, s, 256);
            }
            {
            char *s = zconfig_get (content, "pattern", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->pattern, s, 256);
            }
            break;
        case MLM_PROTO_STREAM_SEND:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "subject", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->subject, s, 256);
            }
            {
            char *s = zconfig_get (content, "content", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                mlm_proto_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->content = msg;
            }
            break;
        case MLM_PROTO_STREAM_DELIVER:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            {
            char *s = zconfig_get (content, "sender", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->sender, s, 256);
            }
            {
            char *s = zconfig_get (content, "subject", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->subject, s, 256);
            }
            {
            char *s = zconfig_get (content, "content", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                mlm_proto_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->content = msg;
            }
            break;
        case MLM_PROTO_MAILBOX_SEND:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            {
            char *s = zconfig_get (content, "subject", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->subject, s, 256);
            }
            {
            char *s = zconfig_get (content, "tracker", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->tracker, s, 256);
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "timeout", NULL);
            if (!s) {
                zsys_error ("content/timeout not found");
                mlm_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/timeout: %s is not a number", s);
                mlm_proto_destroy (&self);
                return NULL;
            }
            self->timeout = uvalue;
            }
            {
            char *s = zconfig_get (content, "content", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                mlm_proto_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->content = msg;
            }
            break;
        case MLM_PROTO_MAILBOX_DELIVER:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "sender", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->sender, s, 256);
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            {
            char *s = zconfig_get (content, "subject", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->subject, s, 256);
            }
            {
            char *s = zconfig_get (content, "tracker", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->tracker, s, 256);
            }
            {
            char *s = zconfig_get (content, "content", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                mlm_proto_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->content = msg;
            }
            break;
        case MLM_PROTO_SERVICE_SEND:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            {
            char *s = zconfig_get (content, "subject", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->subject, s, 256);
            }
            {
            char *s = zconfig_get (content, "tracker", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->tracker, s, 256);
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "timeout", NULL);
            if (!s) {
                zsys_error ("content/timeout not found");
                mlm_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/timeout: %s is not a number", s);
                mlm_proto_destroy (&self);
                return NULL;
            }
            self->timeout = uvalue;
            }
            {
            char *s = zconfig_get (content, "content", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                mlm_proto_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->content = msg;
            }
            break;
        case MLM_PROTO_SERVICE_OFFER:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            {
            char *s = zconfig_get (content, "pattern", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->pattern, s, 256);
            }
            break;
        case MLM_PROTO_SERVICE_DELIVER:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "sender", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->sender, s, 256);
            }
            {
            char *s = zconfig_get (content, "address", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->address, s, 256);
            }
            {
            char *s = zconfig_get (content, "subject", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->subject, s, 256);
            }
            {
            char *s = zconfig_get (content, "tracker", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->tracker, s, 256);
            }
            {
            char *s = zconfig_get (content, "content", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            byte *bvalue;
            BYTES_FROM_STR (bvalue, s);
            if (!bvalue) {
                mlm_proto_destroy (&self);
                return NULL;
            }
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zframe_new (bvalue, strlen (s) / 2);
            zmsg_t *msg = zmsg_decode (frame);
            zframe_destroy (&frame);
#else
            zmsg_t *msg = zmsg_decode (bvalue, strlen (s) / 2);
#endif
            free (bvalue);
            self->content = msg;
            }
            break;
        case MLM_PROTO_OK:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "status_code", NULL);
            if (!s) {
                zsys_error ("content/status_code not found");
                mlm_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/status_code: %s is not a number", s);
                mlm_proto_destroy (&self);
                return NULL;
            }
            self->status_code = uvalue;
            }
            {
            char *s = zconfig_get (content, "status_reason", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->status_reason, s, 256);
            }
            break;
        case MLM_PROTO_ERROR:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "status_code", NULL);
            if (!s) {
                zsys_error ("content/status_code not found");
                mlm_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/status_code: %s is not a number", s);
                mlm_proto_destroy (&self);
                return NULL;
            }
            self->status_code = uvalue;
            }
            {
            char *s = zconfig_get (content, "status_reason", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->status_reason, s, 256);
            }
            break;
        case MLM_PROTO_CREDIT:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "amount", NULL);
            if (!s) {
                zsys_error ("content/amount not found");
                mlm_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/amount: %s is not a number", s);
                mlm_proto_destroy (&self);
                return NULL;
            }
            self->amount = uvalue;
            }
            break;
        case MLM_PROTO_CONFIRM:
            content = zconfig_locate (config, "content");
            if (!content) {
                zsys_error ("Can't find 'content' section");
                mlm_proto_destroy (&self);
                return NULL;
            }
            {
            char *s = zconfig_get (content, "tracker", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->tracker, s, 256);
            }
            {
            char *es = NULL;
            char *s = zconfig_get (content, "status_code", NULL);
            if (!s) {
                zsys_error ("content/status_code not found");
                mlm_proto_destroy (&self);
                return NULL;
            }
            uint64_t uvalue = (uint64_t) strtoll (s, &es, 10);
            if (es != s+strlen (s)) {
                zsys_error ("content/status_code: %s is not a number", s);
                mlm_proto_destroy (&self);
                return NULL;
            }
            self->status_code = uvalue;
            }
            {
            char *s = zconfig_get (content, "status_reason", NULL);
            if (!s) {
                mlm_proto_destroy (&self);
                return NULL;
            }
            strncpy (self->status_reason, s, 256);
            }
            break;
    }
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the mlm_proto

void
mlm_proto_destroy (mlm_proto_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        mlm_proto_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zmsg_destroy (&self->content);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}


//  --------------------------------------------------------------------------
//  Create a deep copy of a mlm_proto instance

mlm_proto_t *
mlm_proto_dup (mlm_proto_t *other)
{
    assert (other);
    mlm_proto_t *copy = mlm_proto_new ();

    // Copy the routing and message id
    mlm_proto_set_routing_id (copy, mlm_proto_routing_id (other));
    mlm_proto_set_id (copy, mlm_proto_id (other));

    // Copy the rest of the fields
    mlm_proto_set_address (copy, mlm_proto_address (other));
    mlm_proto_set_stream (copy, mlm_proto_stream (other));
    mlm_proto_set_pattern (copy, mlm_proto_pattern (other));
    mlm_proto_set_subject (copy, mlm_proto_subject (other));
    {
        zmsg_t *dup_msg = zmsg_dup (mlm_proto_content (other));
        mlm_proto_set_content (copy, &dup_msg);
    }
    mlm_proto_set_sender (copy, mlm_proto_sender (other));
    mlm_proto_set_tracker (copy, mlm_proto_tracker (other));
    mlm_proto_set_timeout (copy, mlm_proto_timeout (other));
    mlm_proto_set_status_code (copy, mlm_proto_status_code (other));
    mlm_proto_set_status_reason (copy, mlm_proto_status_reason (other));
    mlm_proto_set_amount (copy, mlm_proto_amount (other));

    return copy;
}

//  --------------------------------------------------------------------------
//  Receive a mlm_proto from the socket. Returns 0 if OK, -1 if
//  the recv was interrupted, or -2 if the message is malformed.
//  Blocks if there is no message waiting.

int
mlm_proto_recv (mlm_proto_t *self, zsock_t *input)
{
    assert (input);
    int rc = 0;
    zmq_msg_t frame;
    zmq_msg_init (&frame);

    if (zsock_type (input) == ZMQ_ROUTER) {
        zframe_destroy (&self->routing_id);
        self->routing_id = zframe_recv (input);
        if (!self->routing_id || !zsock_rcvmore (input)) {
            zsys_warning ("mlm_proto: no routing ID");
            rc = -1;            //  Interrupted
            goto malformed;
        }
    }
    int size;
    size = zmq_msg_recv (&frame, zsock_resolve (input), 0);
    if (size == -1) {
        zsys_warning ("mlm_proto: interrupted");
        rc = -1;                //  Interrupted
        goto malformed;
    }
    //  Get and check protocol signature
    self->needle = (byte *) zmq_msg_data (&frame);
    self->ceiling = self->needle + zmq_msg_size (&frame);

    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 8)) {
        zsys_warning ("mlm_proto: invalid signature");
        rc = -2;                //  Malformed
        goto malformed;
    }
    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
            {
                char protocol [256];
                GET_STRING (protocol);
                if (strneq (protocol, "MALAMUTE")) {
                    zsys_warning ("mlm_proto: protocol is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            {
                uint16_t version;
                GET_NUMBER2 (version);
                if (version != 1) {
                    zsys_warning ("mlm_proto: version is invalid");
                    rc = -2;    //  Malformed
                    goto malformed;
                }
            }
            GET_STRING (self->address);
            break;

        case MLM_PROTO_CONNECTION_PING:
            break;

        case MLM_PROTO_CONNECTION_PONG:
            break;

        case MLM_PROTO_CONNECTION_CLOSE:
            break;

        case MLM_PROTO_STREAM_WRITE:
            GET_STRING (self->stream);
            break;

        case MLM_PROTO_STREAM_READ:
            GET_STRING (self->stream);
            GET_STRING (self->pattern);
            break;

        case MLM_PROTO_STREAM_SEND:
            GET_STRING (self->subject);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case MLM_PROTO_STREAM_DELIVER:
            GET_STRING (self->address);
            GET_STRING (self->sender);
            GET_STRING (self->subject);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case MLM_PROTO_MAILBOX_SEND:
            GET_STRING (self->address);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            GET_NUMBER4 (self->timeout);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case MLM_PROTO_MAILBOX_DELIVER:
            GET_STRING (self->sender);
            GET_STRING (self->address);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case MLM_PROTO_SERVICE_SEND:
            GET_STRING (self->address);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            GET_NUMBER4 (self->timeout);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case MLM_PROTO_SERVICE_OFFER:
            GET_STRING (self->address);
            GET_STRING (self->pattern);
            break;

        case MLM_PROTO_SERVICE_DELIVER:
            GET_STRING (self->sender);
            GET_STRING (self->address);
            GET_STRING (self->subject);
            GET_STRING (self->tracker);
            //  Get zero or more remaining frames
            zmsg_destroy (&self->content);
            if (zsock_rcvmore (input))
                self->content = zmsg_recv (input);
            else
                self->content = zmsg_new ();
            break;

        case MLM_PROTO_OK:
            GET_NUMBER2 (self->status_code);
            GET_STRING (self->status_reason);
            break;

        case MLM_PROTO_ERROR:
            GET_NUMBER2 (self->status_code);
            GET_STRING (self->status_reason);
            break;

        case MLM_PROTO_CREDIT:
            GET_NUMBER2 (self->amount);
            break;

        case MLM_PROTO_CONFIRM:
            GET_STRING (self->tracker);
            GET_NUMBER2 (self->status_code);
            GET_STRING (self->status_reason);
            break;

        default:
            zsys_warning ("mlm_proto: bad message ID");
            rc = -2;            //  Malformed
            goto malformed;
    }
    //  Successful return
    zmq_msg_close (&frame);
    return rc;

    //  Error returns
    malformed:
        zmq_msg_close (&frame);
        return rc;              //  Invalid message
}


//  --------------------------------------------------------------------------
//  Send the mlm_proto to the socket. Does not destroy it. Returns 0 if
//  OK, else -1.

int
mlm_proto_send (mlm_proto_t *self, zsock_t *output)
{
    assert (self);
    assert (output);

    if (zsock_type (output) == ZMQ_ROUTER)
        zframe_send (&self->routing_id, output, ZFRAME_MORE + ZFRAME_REUSE);

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
            frame_size += 1 + strlen ("MALAMUTE");
            frame_size += 2;            //  version
            frame_size += 1 + strlen (self->address);
            break;
        case MLM_PROTO_STREAM_WRITE:
            frame_size += 1 + strlen (self->stream);
            break;
        case MLM_PROTO_STREAM_READ:
            frame_size += 1 + strlen (self->stream);
            frame_size += 1 + strlen (self->pattern);
            break;
        case MLM_PROTO_STREAM_SEND:
            frame_size += 1 + strlen (self->subject);
            break;
        case MLM_PROTO_STREAM_DELIVER:
            frame_size += 1 + strlen (self->address);
            frame_size += 1 + strlen (self->sender);
            frame_size += 1 + strlen (self->subject);
            break;
        case MLM_PROTO_MAILBOX_SEND:
            frame_size += 1 + strlen (self->address);
            frame_size += 1 + strlen (self->subject);
            frame_size += 1 + strlen (self->tracker);
            frame_size += 4;            //  timeout
            break;
        case MLM_PROTO_MAILBOX_DELIVER:
            frame_size += 1 + strlen (self->sender);
            frame_size += 1 + strlen (self->address);
            frame_size += 1 + strlen (self->subject);
            frame_size += 1 + strlen (self->tracker);
            break;
        case MLM_PROTO_SERVICE_SEND:
            frame_size += 1 + strlen (self->address);
            frame_size += 1 + strlen (self->subject);
            frame_size += 1 + strlen (self->tracker);
            frame_size += 4;            //  timeout
            break;
        case MLM_PROTO_SERVICE_OFFER:
            frame_size += 1 + strlen (self->address);
            frame_size += 1 + strlen (self->pattern);
            break;
        case MLM_PROTO_SERVICE_DELIVER:
            frame_size += 1 + strlen (self->sender);
            frame_size += 1 + strlen (self->address);
            frame_size += 1 + strlen (self->subject);
            frame_size += 1 + strlen (self->tracker);
            break;
        case MLM_PROTO_OK:
            frame_size += 2;            //  status_code
            frame_size += 1 + strlen (self->status_reason);
            break;
        case MLM_PROTO_ERROR:
            frame_size += 2;            //  status_code
            frame_size += 1 + strlen (self->status_reason);
            break;
        case MLM_PROTO_CREDIT:
            frame_size += 2;            //  amount
            break;
        case MLM_PROTO_CONFIRM:
            frame_size += 1 + strlen (self->tracker);
            frame_size += 2;            //  status_code
            frame_size += 1 + strlen (self->status_reason);
            break;
    }
    //  Now serialize message into the frame
    zmq_msg_t frame;
    zmq_msg_init_size (&frame, frame_size);
    self->needle = (byte *) zmq_msg_data (&frame);
    PUT_NUMBER2 (0xAAA0 | 8);
    PUT_NUMBER1 (self->id);
    bool have_content = false;
    size_t nbr_frames = 1;              //  Total number of frames to send

    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
            PUT_STRING ("MALAMUTE");
            PUT_NUMBER2 (1);
            PUT_STRING (self->address);
            break;

        case MLM_PROTO_STREAM_WRITE:
            PUT_STRING (self->stream);
            break;

        case MLM_PROTO_STREAM_READ:
            PUT_STRING (self->stream);
            PUT_STRING (self->pattern);
            break;

        case MLM_PROTO_STREAM_SEND:
            PUT_STRING (self->subject);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case MLM_PROTO_STREAM_DELIVER:
            PUT_STRING (self->address);
            PUT_STRING (self->sender);
            PUT_STRING (self->subject);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case MLM_PROTO_MAILBOX_SEND:
            PUT_STRING (self->address);
            PUT_STRING (self->subject);
            PUT_STRING (self->tracker);
            PUT_NUMBER4 (self->timeout);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case MLM_PROTO_MAILBOX_DELIVER:
            PUT_STRING (self->sender);
            PUT_STRING (self->address);
            PUT_STRING (self->subject);
            PUT_STRING (self->tracker);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case MLM_PROTO_SERVICE_SEND:
            PUT_STRING (self->address);
            PUT_STRING (self->subject);
            PUT_STRING (self->tracker);
            PUT_NUMBER4 (self->timeout);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case MLM_PROTO_SERVICE_OFFER:
            PUT_STRING (self->address);
            PUT_STRING (self->pattern);
            break;

        case MLM_PROTO_SERVICE_DELIVER:
            PUT_STRING (self->sender);
            PUT_STRING (self->address);
            PUT_STRING (self->subject);
            PUT_STRING (self->tracker);
            nbr_frames += self->content? zmsg_size (self->content): 1;
            have_content = true;
            break;

        case MLM_PROTO_OK:
            PUT_NUMBER2 (self->status_code);
            PUT_STRING (self->status_reason);
            break;

        case MLM_PROTO_ERROR:
            PUT_NUMBER2 (self->status_code);
            PUT_STRING (self->status_reason);
            break;

        case MLM_PROTO_CREDIT:
            PUT_NUMBER2 (self->amount);
            break;

        case MLM_PROTO_CONFIRM:
            PUT_STRING (self->tracker);
            PUT_NUMBER2 (self->status_code);
            PUT_STRING (self->status_reason);
            break;

    }
    //  Now send the data frame
    zmq_msg_send (&frame, zsock_resolve (output), --nbr_frames? ZMQ_SNDMORE: 0);

    //  Now send the content if necessary
    if (have_content) {
        if (self->content) {
            zframe_t *frame = zmsg_first (self->content);
            while (frame) {
                zframe_send (&frame, output, ZFRAME_REUSE + (--nbr_frames? ZFRAME_MORE: 0));
                frame = zmsg_next (self->content);
            }
        }
        else
            zmq_send (zsock_resolve (output), NULL, 0, 0);
    }
    return 0;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
mlm_proto_print (mlm_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
            zsys_debug ("MLM_PROTO_CONNECTION_OPEN:");
            zsys_debug ("    protocol=malamute");
            zsys_debug ("    version=1");
            zsys_debug ("    address='%s'", self->address);
            break;

        case MLM_PROTO_CONNECTION_PING:
            zsys_debug ("MLM_PROTO_CONNECTION_PING:");
            break;

        case MLM_PROTO_CONNECTION_PONG:
            zsys_debug ("MLM_PROTO_CONNECTION_PONG:");
            break;

        case MLM_PROTO_CONNECTION_CLOSE:
            zsys_debug ("MLM_PROTO_CONNECTION_CLOSE:");
            break;

        case MLM_PROTO_STREAM_WRITE:
            zsys_debug ("MLM_PROTO_STREAM_WRITE:");
            zsys_debug ("    stream='%s'", self->stream);
            break;

        case MLM_PROTO_STREAM_READ:
            zsys_debug ("MLM_PROTO_STREAM_READ:");
            zsys_debug ("    stream='%s'", self->stream);
            zsys_debug ("    pattern='%s'", self->pattern);
            break;

        case MLM_PROTO_STREAM_SEND:
            zsys_debug ("MLM_PROTO_STREAM_SEND:");
            zsys_debug ("    subject='%s'", self->subject);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case MLM_PROTO_STREAM_DELIVER:
            zsys_debug ("MLM_PROTO_STREAM_DELIVER:");
            zsys_debug ("    address='%s'", self->address);
            zsys_debug ("    sender='%s'", self->sender);
            zsys_debug ("    subject='%s'", self->subject);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case MLM_PROTO_MAILBOX_SEND:
            zsys_debug ("MLM_PROTO_MAILBOX_SEND:");
            zsys_debug ("    address='%s'", self->address);
            zsys_debug ("    subject='%s'", self->subject);
            zsys_debug ("    tracker='%s'", self->tracker);
            zsys_debug ("    timeout=%ld", (long) self->timeout);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case MLM_PROTO_MAILBOX_DELIVER:
            zsys_debug ("MLM_PROTO_MAILBOX_DELIVER:");
            zsys_debug ("    sender='%s'", self->sender);
            zsys_debug ("    address='%s'", self->address);
            zsys_debug ("    subject='%s'", self->subject);
            zsys_debug ("    tracker='%s'", self->tracker);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case MLM_PROTO_SERVICE_SEND:
            zsys_debug ("MLM_PROTO_SERVICE_SEND:");
            zsys_debug ("    address='%s'", self->address);
            zsys_debug ("    subject='%s'", self->subject);
            zsys_debug ("    tracker='%s'", self->tracker);
            zsys_debug ("    timeout=%ld", (long) self->timeout);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case MLM_PROTO_SERVICE_OFFER:
            zsys_debug ("MLM_PROTO_SERVICE_OFFER:");
            zsys_debug ("    address='%s'", self->address);
            zsys_debug ("    pattern='%s'", self->pattern);
            break;

        case MLM_PROTO_SERVICE_DELIVER:
            zsys_debug ("MLM_PROTO_SERVICE_DELIVER:");
            zsys_debug ("    sender='%s'", self->sender);
            zsys_debug ("    address='%s'", self->address);
            zsys_debug ("    subject='%s'", self->subject);
            zsys_debug ("    tracker='%s'", self->tracker);
            zsys_debug ("    content=");
            if (self->content)
                zmsg_print (self->content);
            else
                zsys_debug ("(NULL)");
            break;

        case MLM_PROTO_OK:
            zsys_debug ("MLM_PROTO_OK:");
            zsys_debug ("    status_code=%ld", (long) self->status_code);
            zsys_debug ("    status_reason='%s'", self->status_reason);
            break;

        case MLM_PROTO_ERROR:
            zsys_debug ("MLM_PROTO_ERROR:");
            zsys_debug ("    status_code=%ld", (long) self->status_code);
            zsys_debug ("    status_reason='%s'", self->status_reason);
            break;

        case MLM_PROTO_CREDIT:
            zsys_debug ("MLM_PROTO_CREDIT:");
            zsys_debug ("    amount=%ld", (long) self->amount);
            break;

        case MLM_PROTO_CONFIRM:
            zsys_debug ("MLM_PROTO_CONFIRM:");
            zsys_debug ("    tracker='%s'", self->tracker);
            zsys_debug ("    status_code=%ld", (long) self->status_code);
            zsys_debug ("    status_reason='%s'", self->status_reason);
            break;

    }
}

//  --------------------------------------------------------------------------
//  Export class as zconfig_t*. Caller is responsibe for destroying the instance

zconfig_t *
mlm_proto_zpl (mlm_proto_t *self, zconfig_t *parent)
{
    assert (self);

    zconfig_t *root = zconfig_new ("mlm_proto", parent);

    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
        {
            zconfig_put (root, "message", "MLM_PROTO_CONNECTION_OPEN");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "protocol", "%s", "malamute");
            zconfig_putf (config, "version", "%s", "1");
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            break;
            }
        case MLM_PROTO_CONNECTION_PING:
        {
            zconfig_put (root, "message", "MLM_PROTO_CONNECTION_PING");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            break;
            }
        case MLM_PROTO_CONNECTION_PONG:
        {
            zconfig_put (root, "message", "MLM_PROTO_CONNECTION_PONG");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            break;
            }
        case MLM_PROTO_CONNECTION_CLOSE:
        {
            zconfig_put (root, "message", "MLM_PROTO_CONNECTION_CLOSE");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            break;
            }
        case MLM_PROTO_STREAM_WRITE:
        {
            zconfig_put (root, "message", "MLM_PROTO_STREAM_WRITE");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->stream)
                zconfig_putf (config, "stream", "%s", self->stream);
            break;
            }
        case MLM_PROTO_STREAM_READ:
        {
            zconfig_put (root, "message", "MLM_PROTO_STREAM_READ");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->stream)
                zconfig_putf (config, "stream", "%s", self->stream);
            if (self->pattern)
                zconfig_putf (config, "pattern", "%s", self->pattern);
            break;
            }
        case MLM_PROTO_STREAM_SEND:
        {
            zconfig_put (root, "message", "MLM_PROTO_STREAM_SEND");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->subject)
                zconfig_putf (config, "subject", "%s", self->subject);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->content);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->content, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case MLM_PROTO_STREAM_DELIVER:
        {
            zconfig_put (root, "message", "MLM_PROTO_STREAM_DELIVER");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            if (self->sender)
                zconfig_putf (config, "sender", "%s", self->sender);
            if (self->subject)
                zconfig_putf (config, "subject", "%s", self->subject);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->content);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->content, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case MLM_PROTO_MAILBOX_SEND:
        {
            zconfig_put (root, "message", "MLM_PROTO_MAILBOX_SEND");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            if (self->subject)
                zconfig_putf (config, "subject", "%s", self->subject);
            if (self->tracker)
                zconfig_putf (config, "tracker", "%s", self->tracker);
            zconfig_putf (config, "timeout", "%ld", (long) self->timeout);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->content);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->content, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case MLM_PROTO_MAILBOX_DELIVER:
        {
            zconfig_put (root, "message", "MLM_PROTO_MAILBOX_DELIVER");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->sender)
                zconfig_putf (config, "sender", "%s", self->sender);
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            if (self->subject)
                zconfig_putf (config, "subject", "%s", self->subject);
            if (self->tracker)
                zconfig_putf (config, "tracker", "%s", self->tracker);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->content);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->content, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case MLM_PROTO_SERVICE_SEND:
        {
            zconfig_put (root, "message", "MLM_PROTO_SERVICE_SEND");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            if (self->subject)
                zconfig_putf (config, "subject", "%s", self->subject);
            if (self->tracker)
                zconfig_putf (config, "tracker", "%s", self->tracker);
            zconfig_putf (config, "timeout", "%ld", (long) self->timeout);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->content);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->content, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case MLM_PROTO_SERVICE_OFFER:
        {
            zconfig_put (root, "message", "MLM_PROTO_SERVICE_OFFER");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            if (self->pattern)
                zconfig_putf (config, "pattern", "%s", self->pattern);
            break;
            }
        case MLM_PROTO_SERVICE_DELIVER:
        {
            zconfig_put (root, "message", "MLM_PROTO_SERVICE_DELIVER");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->sender)
                zconfig_putf (config, "sender", "%s", self->sender);
            if (self->address)
                zconfig_putf (config, "address", "%s", self->address);
            if (self->subject)
                zconfig_putf (config, "subject", "%s", self->subject);
            if (self->tracker)
                zconfig_putf (config, "tracker", "%s", self->tracker);
            {
            char *hex = NULL;
#if CZMQ_VERSION_MAJOR == 4
            zframe_t *frame = zmsg_encode (self->content);
            STR_FROM_BYTES (hex, zframe_data (frame), zframe_size (frame));
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            zframe_destroy (&frame);
#else
            byte *buffer;
            size_t size = zmsg_encode (self->content, &buffer);
            STR_FROM_BYTES (hex, buffer, size);
            zconfig_putf (config, "content", "%s", hex);
            zstr_free (&hex);
            free (buffer); buffer= NULL;
#endif
            }
            break;
            }
        case MLM_PROTO_OK:
        {
            zconfig_put (root, "message", "MLM_PROTO_OK");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "status_code", "%ld", (long) self->status_code);
            if (self->status_reason)
                zconfig_putf (config, "status_reason", "%s", self->status_reason);
            break;
            }
        case MLM_PROTO_ERROR:
        {
            zconfig_put (root, "message", "MLM_PROTO_ERROR");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "status_code", "%ld", (long) self->status_code);
            if (self->status_reason)
                zconfig_putf (config, "status_reason", "%s", self->status_reason);
            break;
            }
        case MLM_PROTO_CREDIT:
        {
            zconfig_put (root, "message", "MLM_PROTO_CREDIT");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            zconfig_putf (config, "amount", "%ld", (long) self->amount);
            break;
            }
        case MLM_PROTO_CONFIRM:
        {
            zconfig_put (root, "message", "MLM_PROTO_CONFIRM");

            if (self->routing_id) {
                char *hex = NULL;
                STR_FROM_BYTES (hex, zframe_data (self->routing_id), zframe_size (self->routing_id));
                zconfig_putf (root, "routing_id", "%s", hex);
                zstr_free (&hex);
            }

            zconfig_t *config = zconfig_new ("content", root);
            if (self->tracker)
                zconfig_putf (config, "tracker", "%s", self->tracker);
            zconfig_putf (config, "status_code", "%ld", (long) self->status_code);
            if (self->status_reason)
                zconfig_putf (config, "status_reason", "%s", self->status_reason);
            break;
            }
    }
    return root;
}

//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
mlm_proto_routing_id (mlm_proto_t *self)
{
    assert (self);
    return self->routing_id;
}

void
mlm_proto_set_routing_id (mlm_proto_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the mlm_proto id

int
mlm_proto_id (mlm_proto_t *self)
{
    assert (self);
    return self->id;
}

void
mlm_proto_set_id (mlm_proto_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
mlm_proto_command (mlm_proto_t *self)
{
    assert (self);
    switch (self->id) {
        case MLM_PROTO_CONNECTION_OPEN:
            return ("CONNECTION_OPEN");
            break;
        case MLM_PROTO_CONNECTION_PING:
            return ("CONNECTION_PING");
            break;
        case MLM_PROTO_CONNECTION_PONG:
            return ("CONNECTION_PONG");
            break;
        case MLM_PROTO_CONNECTION_CLOSE:
            return ("CONNECTION_CLOSE");
            break;
        case MLM_PROTO_STREAM_WRITE:
            return ("STREAM_WRITE");
            break;
        case MLM_PROTO_STREAM_READ:
            return ("STREAM_READ");
            break;
        case MLM_PROTO_STREAM_SEND:
            return ("STREAM_SEND");
            break;
        case MLM_PROTO_STREAM_DELIVER:
            return ("STREAM_DELIVER");
            break;
        case MLM_PROTO_MAILBOX_SEND:
            return ("MAILBOX_SEND");
            break;
        case MLM_PROTO_MAILBOX_DELIVER:
            return ("MAILBOX_DELIVER");
            break;
        case MLM_PROTO_SERVICE_SEND:
            return ("SERVICE_SEND");
            break;
        case MLM_PROTO_SERVICE_OFFER:
            return ("SERVICE_OFFER");
            break;
        case MLM_PROTO_SERVICE_DELIVER:
            return ("SERVICE_DELIVER");
            break;
        case MLM_PROTO_OK:
            return ("OK");
            break;
        case MLM_PROTO_ERROR:
            return ("ERROR");
            break;
        case MLM_PROTO_CREDIT:
            return ("CREDIT");
            break;
        case MLM_PROTO_CONFIRM:
            return ("CONFIRM");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the address field

const char *
mlm_proto_address (mlm_proto_t *self)
{
    assert (self);
    return self->address;
}

void
mlm_proto_set_address (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->address)
        return;
    strncpy (self->address, value, 255);
    self->address [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the stream field

const char *
mlm_proto_stream (mlm_proto_t *self)
{
    assert (self);
    return self->stream;
}

void
mlm_proto_set_stream (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->stream)
        return;
    strncpy (self->stream, value, 255);
    self->stream [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the pattern field

const char *
mlm_proto_pattern (mlm_proto_t *self)
{
    assert (self);
    return self->pattern;
}

void
mlm_proto_set_pattern (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->pattern)
        return;
    strncpy (self->pattern, value, 255);
    self->pattern [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the subject field

const char *
mlm_proto_subject (mlm_proto_t *self)
{
    assert (self);
    return self->subject;
}

void
mlm_proto_set_subject (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->subject)
        return;
    strncpy (self->subject, value, 255);
    self->subject [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get the content field without transferring ownership

zmsg_t *
mlm_proto_content (mlm_proto_t *self)
{
    assert (self);
    return self->content;
}

//  Get the content field and transfer ownership to caller

zmsg_t *
mlm_proto_get_content (mlm_proto_t *self)
{
    zmsg_t *content = self->content;
    self->content = NULL;
    return content;
}

//  Set the content field, transferring ownership from caller

void
mlm_proto_set_content (mlm_proto_t *self, zmsg_t **msg_p)
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
mlm_proto_sender (mlm_proto_t *self)
{
    assert (self);
    return self->sender;
}

void
mlm_proto_set_sender (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->sender)
        return;
    strncpy (self->sender, value, 255);
    self->sender [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the tracker field

const char *
mlm_proto_tracker (mlm_proto_t *self)
{
    assert (self);
    return self->tracker;
}

void
mlm_proto_set_tracker (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->tracker)
        return;
    strncpy (self->tracker, value, 255);
    self->tracker [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the timeout field

uint32_t
mlm_proto_timeout (mlm_proto_t *self)
{
    assert (self);
    return self->timeout;
}

void
mlm_proto_set_timeout (mlm_proto_t *self, uint32_t timeout)
{
    assert (self);
    self->timeout = timeout;
}


//  --------------------------------------------------------------------------
//  Get/set the status_code field

uint16_t
mlm_proto_status_code (mlm_proto_t *self)
{
    assert (self);
    return self->status_code;
}

void
mlm_proto_set_status_code (mlm_proto_t *self, uint16_t status_code)
{
    assert (self);
    self->status_code = status_code;
}


//  --------------------------------------------------------------------------
//  Get/set the status_reason field

const char *
mlm_proto_status_reason (mlm_proto_t *self)
{
    assert (self);
    return self->status_reason;
}

void
mlm_proto_set_status_reason (mlm_proto_t *self, const char *value)
{
    assert (self);
    assert (value);
    if (value == self->status_reason)
        return;
    strncpy (self->status_reason, value, 255);
    self->status_reason [255] = 0;
}


//  --------------------------------------------------------------------------
//  Get/set the amount field

uint16_t
mlm_proto_amount (mlm_proto_t *self)
{
    assert (self);
    return self->amount;
}

void
mlm_proto_set_amount (mlm_proto_t *self, uint16_t amount)
{
    assert (self);
    self->amount = amount;
}



//  --------------------------------------------------------------------------
//  Selftest

void
mlm_proto_test (bool verbose)
{
    printf (" * mlm_proto: ");

    if (verbose)
        printf ("\n");

    //  @selftest
    //  Simple create/destroy test
    zconfig_t *config;
    mlm_proto_t *self = mlm_proto_new ();
    assert (self);
    mlm_proto_destroy (&self);
    //  Create pair of sockets we can send through
    //  We must bind before connect if we wish to remain compatible with ZeroMQ < v4
    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    int rc = zsock_bind (output, "inproc://selftest-mlm_proto");
    assert (rc == 0);

    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    rc = zsock_connect (input, "inproc://selftest-mlm_proto");
    assert (rc == 0);


    //  Encode/send/decode and verify each message type
    int instance;
    self = mlm_proto_new ();
    mlm_proto_set_id (self, MLM_PROTO_CONNECTION_OPEN);

    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_CONNECTION_PING);

    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_CONNECTION_PONG);

    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_CONNECTION_CLOSE);

    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_STREAM_WRITE);

    mlm_proto_set_stream (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_stream (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_STREAM_READ);

    mlm_proto_set_stream (self, "Life is short but Now lasts for ever");
    mlm_proto_set_pattern (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_stream (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_pattern (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_STREAM_SEND);

    mlm_proto_set_subject (self, "Life is short but Now lasts for ever");
    zmsg_t *stream_send_content = zmsg_new ();
    mlm_proto_set_content (self, &stream_send_content);
    zmsg_addstr (mlm_proto_content (self), "Captcha Diem");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_subject (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_proto_content (self)) == 1);
        char *content = zmsg_popstr (mlm_proto_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&stream_send_content);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_STREAM_DELIVER);

    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    mlm_proto_set_sender (self, "Life is short but Now lasts for ever");
    mlm_proto_set_subject (self, "Life is short but Now lasts for ever");
    zmsg_t *stream_deliver_content = zmsg_new ();
    mlm_proto_set_content (self, &stream_deliver_content);
    zmsg_addstr (mlm_proto_content (self), "Captcha Diem");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_subject (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_proto_content (self)) == 1);
        char *content = zmsg_popstr (mlm_proto_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&stream_deliver_content);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_MAILBOX_SEND);

    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    mlm_proto_set_subject (self, "Life is short but Now lasts for ever");
    mlm_proto_set_tracker (self, "Life is short but Now lasts for ever");
    mlm_proto_set_timeout (self, 123);
    zmsg_t *mailbox_send_content = zmsg_new ();
    mlm_proto_set_content (self, &mailbox_send_content);
    zmsg_addstr (mlm_proto_content (self), "Captcha Diem");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_tracker (self), "Life is short but Now lasts for ever"));
        assert (mlm_proto_timeout (self) == 123);
        assert (zmsg_size (mlm_proto_content (self)) == 1);
        char *content = zmsg_popstr (mlm_proto_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&mailbox_send_content);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_MAILBOX_DELIVER);

    mlm_proto_set_sender (self, "Life is short but Now lasts for ever");
    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    mlm_proto_set_subject (self, "Life is short but Now lasts for ever");
    mlm_proto_set_tracker (self, "Life is short but Now lasts for ever");
    zmsg_t *mailbox_deliver_content = zmsg_new ();
    mlm_proto_set_content (self, &mailbox_deliver_content);
    zmsg_addstr (mlm_proto_content (self), "Captcha Diem");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_tracker (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_proto_content (self)) == 1);
        char *content = zmsg_popstr (mlm_proto_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&mailbox_deliver_content);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_SERVICE_SEND);

    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    mlm_proto_set_subject (self, "Life is short but Now lasts for ever");
    mlm_proto_set_tracker (self, "Life is short but Now lasts for ever");
    mlm_proto_set_timeout (self, 123);
    zmsg_t *service_send_content = zmsg_new ();
    mlm_proto_set_content (self, &service_send_content);
    zmsg_addstr (mlm_proto_content (self), "Captcha Diem");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_tracker (self), "Life is short but Now lasts for ever"));
        assert (mlm_proto_timeout (self) == 123);
        assert (zmsg_size (mlm_proto_content (self)) == 1);
        char *content = zmsg_popstr (mlm_proto_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&service_send_content);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_SERVICE_OFFER);

    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    mlm_proto_set_pattern (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_pattern (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_SERVICE_DELIVER);

    mlm_proto_set_sender (self, "Life is short but Now lasts for ever");
    mlm_proto_set_address (self, "Life is short but Now lasts for ever");
    mlm_proto_set_subject (self, "Life is short but Now lasts for ever");
    mlm_proto_set_tracker (self, "Life is short but Now lasts for ever");
    zmsg_t *service_deliver_content = zmsg_new ();
    mlm_proto_set_content (self, &service_deliver_content);
    zmsg_addstr (mlm_proto_content (self), "Captcha Diem");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_sender (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_address (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_subject (self), "Life is short but Now lasts for ever"));
        assert (streq (mlm_proto_tracker (self), "Life is short but Now lasts for ever"));
        assert (zmsg_size (mlm_proto_content (self)) == 1);
        char *content = zmsg_popstr (mlm_proto_content (self));
        assert (streq (content, "Captcha Diem"));
        zstr_free (&content);
        if (instance == 1)
            zmsg_destroy (&service_deliver_content);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_OK);

    mlm_proto_set_status_code (self, 123);
    mlm_proto_set_status_reason (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (mlm_proto_status_code (self) == 123);
        assert (streq (mlm_proto_status_reason (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_ERROR);

    mlm_proto_set_status_code (self, 123);
    mlm_proto_set_status_reason (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (mlm_proto_status_code (self) == 123);
        assert (streq (mlm_proto_status_reason (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_CREDIT);

    mlm_proto_set_amount (self, 123);
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (mlm_proto_amount (self) == 123);
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }
    mlm_proto_set_id (self, MLM_PROTO_CONFIRM);

    mlm_proto_set_tracker (self, "Life is short but Now lasts for ever");
    mlm_proto_set_status_code (self, 123);
    mlm_proto_set_status_reason (self, "Life is short but Now lasts for ever");
    // convert to zpl
    config = mlm_proto_zpl (self, NULL);
    if (verbose)
        zconfig_print (config);
    //  Send twice
    mlm_proto_send (self, output);
    mlm_proto_send (self, output);

    for (instance = 0; instance < 3; instance++) {
        mlm_proto_t *self_temp = self;
        if (instance < 2)
            mlm_proto_recv (self, input);
        else {
            self = mlm_proto_new_zpl (config);
            assert (self);
            zconfig_destroy (&config);
        }
        if (instance < 2)
            assert (mlm_proto_routing_id (self));
        assert (streq (mlm_proto_tracker (self), "Life is short but Now lasts for ever"));
        assert (mlm_proto_status_code (self) == 123);
        assert (streq (mlm_proto_status_reason (self), "Life is short but Now lasts for ever"));
        if (instance == 2) {
            mlm_proto_destroy (&self);
            self = self_temp;
        }
    }

    mlm_proto_destroy (&self);
    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
}
