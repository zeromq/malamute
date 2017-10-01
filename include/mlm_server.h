/*  =========================================================================
    mlm_server - Malamute Server

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: mlm_server.xml, or
     * The code generation script that built this file: zproto_server_c
    ************************************************************************
    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef MLM_SERVER_H_INCLUDED
#define MLM_SERVER_H_INCLUDED

#include <czmq.h>

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with mlm_server, use the CZMQ zactor API:
//
//  Create new mlm_server instance, passing logging prefix:
//
//      zactor_t *mlm_server = zactor_new (mlm_server, "myname");
//
//  Destroy mlm_server instance
//
//      zactor_destroy (&mlm_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (mlm_server, "VERBOSE");
//
//  Bind mlm_server to specified endpoint. TCP endpoints may specify
//  the port number as "*" to aquire an ephemeral port:
//
//      zstr_sendx (mlm_server, "BIND", endpoint, NULL);
//
//  Return assigned port number, specifically when BIND was done using an
//  an ephemeral port:
//
//      zstr_sendx (mlm_server, "PORT", NULL);
//      char *command, *port_str;
//      zstr_recvx (mlm_server, &command, &port_str, NULL);
//      assert (streq (command, "PORT"));
//
//  Specify configuration file to load, overwriting any previous loaded
//  configuration file or options:
//
//      zstr_sendx (mlm_server, "LOAD", filename, NULL);
//
//  Set configuration path value:
//
//      zstr_sendx (mlm_server, "SET", path, value, NULL);
//
//  Save configuration data to config file on disk:
//
//      zstr_sendx (mlm_server, "SAVE", filename, NULL);
//
//  Send zmsg_t instance to mlm_server:
//
//      zactor_send (mlm_server, &msg);
//
//  Receive zmsg_t instance from mlm_server:
//
//      zmsg_t *msg = zactor_recv (mlm_server);
//
//  This is the mlm_server constructor as a zactor_fn:
//
MLM_EXPORT void
    mlm_server (zsock_t *pipe, void *args);

//  Self test of this class
MLM_EXPORT void
    mlm_server_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
