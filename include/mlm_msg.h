/*  =========================================================================
    mlm_msg - The Malamute Protocol
    
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

/*  These are the mlm_msg messages:

    HELLO - Client says hello

    WORLD - Server says world
*/


#define MLM_MSG_HELLO                       1
#define MLM_MSG_WORLD                       2

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _mlm_msg_t mlm_msg_t;

//  @interface
//  Create a new mlm_msg
mlm_msg_t *
    mlm_msg_new (int id);

//  Destroy the mlm_msg
void
    mlm_msg_destroy (mlm_msg_t **self_p);

//  Parse a mlm_msg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.
mlm_msg_t *
    mlm_msg_decode (zmsg_t **msg_p);

//  Encode mlm_msg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.
zmsg_t *
    mlm_msg_encode (mlm_msg_t **self_p);

//  Receive and parse a mlm_msg from the socket. Returns new object, 
//  or NULL if error. Will block if there's no message waiting.
mlm_msg_t *
    mlm_msg_recv (void *input);

//  Receive and parse a mlm_msg from the socket. Returns new object, 
//  or NULL either if there was no input waiting, or the recv was interrupted.
mlm_msg_t *
    mlm_msg_recv_nowait (void *input);

//  Send the mlm_msg to the output, and destroy it
int
    mlm_msg_send (mlm_msg_t **self_p, void *output);

//  Send the mlm_msg to the output, and do not destroy it
int
    mlm_msg_send_again (mlm_msg_t *self, void *output);

//  Encode the HELLO 
zmsg_t *
    mlm_msg_encode_hello (
);

//  Encode the WORLD 
zmsg_t *
    mlm_msg_encode_world (
);


//  Send the HELLO to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_hello (void *output);
    
//  Send the WORLD to the output in one step
//  WARNING, this call will fail if output is of type ZMQ_ROUTER.
int
    mlm_msg_send_world (void *output);
    
//  Duplicate the mlm_msg message
mlm_msg_t *
    mlm_msg_dup (mlm_msg_t *self);

//  Print contents of message to stdout
void
    mlm_msg_print (mlm_msg_t *self);

//  Get/set the message routing id
zframe_t *
    mlm_msg_routing_id (mlm_msg_t *self);
void
    mlm_msg_set_routing_id (mlm_msg_t *self, zframe_t *routing_id);

//  Get the mlm_msg id and printable command
int
    mlm_msg_id (mlm_msg_t *self);
void
    mlm_msg_set_id (mlm_msg_t *self, int id);
const char *
    mlm_msg_command (mlm_msg_t *self);

//  Self test of this class
int
    mlm_msg_test (bool verbose);
//  @end

//  For backwards compatibility with old codecs
#define mlm_msg_dump        mlm_msg_print

#ifdef __cplusplus
}
#endif

#endif
