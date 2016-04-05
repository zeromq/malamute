/*  =========================================================================
    Malamute Node.js binding header file

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#ifndef MALAMUTE_BINDING_H_INCLUDED
#define MALAMUTE_BINDING_H_INCLUDED

#define MLM_BUILD_DRAFT_API

#include "../../../czmq/bindings/nodejs/binding.h"
#include "malamute.h"
#include "nan.h"

using namespace v8;
using namespace Nan;

class MlmProto: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init);
        explicit MlmProto (void);
        explicit MlmProto (mlm_proto_t *self);
        mlm_proto_t *self;
    private:
        ~MlmProto ();
    static Nan::Persistent <Function> &constructor ();

    static NAN_METHOD (New);
    static NAN_METHOD (destroy);
    static NAN_METHOD (defined);
    static NAN_METHOD (_recv);
    static NAN_METHOD (_send);
    static NAN_METHOD (_routing_id);
    static NAN_METHOD (_id);
    static NAN_METHOD (_command);
    static NAN_METHOD (_address);
    static NAN_METHOD (_stream);
    static NAN_METHOD (_pattern);
    static NAN_METHOD (_subject);
    static NAN_METHOD (_content);
    static NAN_METHOD (_get_content);
    static NAN_METHOD (_sender);
    static NAN_METHOD (_tracker);
    static NAN_METHOD (_timeout);
    static NAN_METHOD (_status_code);
    static NAN_METHOD (_status_reason);
    static NAN_METHOD (_amount);
};

class MlmClient: public Nan::ObjectWrap {
    public:
        static NAN_MODULE_INIT (Init);
        explicit MlmClient (void);
        explicit MlmClient (mlm_client_t *self);
        mlm_client_t *self;
    private:
        ~MlmClient ();
    static Nan::Persistent <Function> &constructor ();

    static NAN_METHOD (New);
    static NAN_METHOD (destroy);
    static NAN_METHOD (defined);
    static NAN_METHOD (_actor);
    static NAN_METHOD (_msgpipe);
    static NAN_METHOD (_connected);
    static NAN_METHOD (_set_plain_auth);
    static NAN_METHOD (_connect);
    static NAN_METHOD (_set_producer);
    static NAN_METHOD (_set_consumer);
    static NAN_METHOD (_set_worker);
    static NAN_METHOD (_send);
    static NAN_METHOD (_sendto);
    static NAN_METHOD (_sendfor);
    static NAN_METHOD (_recv);
    static NAN_METHOD (_command);
    static NAN_METHOD (_status);
    static NAN_METHOD (_reason);
    static NAN_METHOD (_address);
    static NAN_METHOD (_sender);
    static NAN_METHOD (_subject);
    static NAN_METHOD (_content);
    static NAN_METHOD (_tracker);
    static NAN_METHOD (_sendx);
    static NAN_METHOD (_sendtox);
    static NAN_METHOD (_sendforx);
    static NAN_METHOD (_recvx);
};

#endif
