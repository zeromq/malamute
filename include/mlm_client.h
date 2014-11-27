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
#ifndef MLM_CLIENT_T_DEFINED
typedef struct _mlm_client_t mlm_client_t;
#define MLM_CLIENT_T_DEFINED
#endif

//  @interface
//  Create a new mlm_client
//  Connect to server endpoint, with specified timeout in msecs (zero means wait    
//  forever). Constructor succeeds if connection is successful.                     
MLM_EXPORT mlm_client_t *
    mlm_client_new (const char *endpoint, int timeout);

//  Destroy the mlm_client
MLM_EXPORT void
    mlm_client_destroy (mlm_client_t **self_p);

//  Enable verbose logging of client activity
MLM_EXPORT void
    mlm_client_verbose (mlm_client_t *self);

//  Return message pipe for asynchronous message I/O. In the high-volume case,
//  we send methods and get replies to the actor, in a synchronous manner, and
//  we send/recv high volume message data to a second pipe, the msgpipe. In
//  the low-volume case we can do everything over the actor pipe, if traffic
//  is never ambiguous.
MLM_EXPORT zsock_t *
    mlm_client_msgpipe (mlm_client_t *self);

//  Caller will send messages to this stream exclusively.                           
//  Returns >= 0 if successful, -1 if interrupted.
MLM_EXPORT int
    mlm_client_produce (mlm_client_t *self, const char *stream);

//  Consume messages with a matching addresses. The pattern is a regular expression 
//  using the CZMQ zrex syntax. The most useful elements are: ^ and $ to match the  
//  start and end, . to match any character, \s and \S to match whitespace and      
//  non-whitespace, \d and \D to match a digit and non-digit, \a and \A to match    
//  alphabetic and non-alphabetic, \w and \W to match alphanumeric and              
//  non-alphanumeric, + for one or more repetitions, * for zero or more repetitions,
//  and ( ) to create groups. Returns 0 if subscription was successful, else -1.    
//  Returns >= 0 if successful, -1 if interrupted.
MLM_EXPORT int
    mlm_client_consume (mlm_client_t *self, const char *stream, const char *pattern);

//  Send STREAM SEND message to server
MLM_EXPORT int
    mlm_client_stream_send (mlm_client_t *self, char *subject, zmsg_t **content_p);

//  Send MAILBOX SEND message to server
MLM_EXPORT int
    mlm_client_mailbox_send (mlm_client_t *self, char *address, char *subject, char *tracker, int timeout, zmsg_t **content_p);

//  Send SERVICE SEND message to server
MLM_EXPORT int
    mlm_client_service_send (mlm_client_t *self, char *service, char *subject, char *tracker, int timeout, zmsg_t **content_p);

//  Receive message from server; caller destroys message when done
MLM_EXPORT zmsg_t *
    mlm_client_recv (mlm_client_t *self);

//  Return last received status
MLM_EXPORT int 
    mlm_client_status (mlm_client_t *self);

//  Return last received reason
MLM_EXPORT const char *
    mlm_client_reason (mlm_client_t *self);

//  Return last received command
MLM_EXPORT const char *
    mlm_client_command (mlm_client_t *self);

//  Return last received stream
MLM_EXPORT const char *
    mlm_client_stream (mlm_client_t *self);

//  Return last received sender
MLM_EXPORT const char *
    mlm_client_sender (mlm_client_t *self);

//  Return last received subject
MLM_EXPORT const char *
    mlm_client_subject (mlm_client_t *self);

//  Return last received content
MLM_EXPORT zmsg_t *
    mlm_client_content (mlm_client_t *self);

//  Return last received address
MLM_EXPORT const char *
    mlm_client_address (mlm_client_t *self);

//  Return last received tracker
MLM_EXPORT const char *
    mlm_client_tracker (mlm_client_t *self);

//  Return last received service
MLM_EXPORT const char *
    mlm_client_service (mlm_client_t *self);

//  Self test of this class
MLM_EXPORT void
    mlm_client_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
