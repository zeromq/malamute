/*  =========================================================================
    mlm_client - Malamute Client

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mlm_client.xml, or
     * The code generation script that built this file: zproto_client_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef __MLM_CLIENT_H_INCLUDED__
#define __MLM_CLIENT_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  Opaque class structure
typedef struct _mlm_client_t mlm_client_t;

//  @interface
//  Create a new mlm_client
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful.                     
mlm_client_t *
    mlm_client_new (const char *endpoint, int timeout);

//  Destroy the mlm_client
void
    mlm_client_destroy (mlm_client_t **self_p);

//  Enable verbose logging of client activity
void
    mlm_client_verbose (mlm_client_t *self);

//  Return actor for low-level command control and polling
zactor_t *
    mlm_client_actor (mlm_client_t *self);

//  Attach to specified stream, as publisher.                                       
//  Returns >= 0 if successful, -1 if interrupted.
int
    mlm_client_attach (mlm_client_t *self, const char *stream);

//  Subscribe to all messages sent to matching addresses. The pattern is a regular  
//  expression using the CZMQ zrex syntax. The most useful elements are: ^ and $ to 
//  match the start and end, . to match any character, \s and \S to match whitespace
//  and non-whitespace, \d and \D to match a digit and non-digit, \a and \A to match
//  alphabetic and non-alphabetic, \w and \W to match alphanumeric and              
//  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
//  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
//  Returns >= 0 if successful, -1 if interrupted.
int
    mlm_client_subscribe (mlm_client_t *self, const char *stream, const char *pattern);

//  Send a message to the current stream. The server does not store messages. If a  
//  message is published before subscribers arrive, they will miss it. Currently    
//  only supports string contents. Does not return a status value; send commands are
//  asynchronous and unconfirmed.                                                   
int
    mlm_client_send (mlm_client_t *self, const char *subject, const char *content);

//  Receive next message from server. Returns the message content, as a string, if  
//  any. The caller should not modify or free this string.                          
//  Returns NULL on an interrupt.
char *
    mlm_client_recv (mlm_client_t *self);

//  Return last received status
int 
    mlm_client_status (mlm_client_t *self);

//  Return last received reason
char *
    mlm_client_reason (mlm_client_t *self);

//  Return last received sender
char *
    mlm_client_sender (mlm_client_t *self);

//  Return last received subject
char *
    mlm_client_subject (mlm_client_t *self);

//  Return last received content
char *
    mlm_client_content (mlm_client_t *self);

//  Self test of this class
void
    mlm_client_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
