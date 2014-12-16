/*  =========================================================================
    mlm_stream_simple - simple stream engine

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#ifndef __MLM_STREAM_SIMPLE_H_INCLUDED__
#define __MLM_STREAM_SIMPLE_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

//  @interface
//  To work with mlm_stream_simple, use the stream command API:
//
//  Create new mlm_stream_simple server instance, passing logging prefix:
//
//      zactor_t *mlm_stream_simple_server = zactor_new (mlm_stream_simple, "myname");
//
//  Destroy mlm_stream_simple server instance
//
//      zactor_destroy (&mlm_stream_simple_server);
//
//  Enable verbose logging of commands and activity:
//
//      zstr_send (server, "VERBOSE");
//
//  This is the mlm_stream_simple constructor as a zactor_fn:
//
MLM_EXPORT void
    mlm_stream_simple (zsock_t *pipe, void *args);

//  Self test of this class
MLM_EXPORT void
    mlm_stream_simple_test (bool verbose);
//  @end

#ifdef __cplusplus
}
#endif

#endif
