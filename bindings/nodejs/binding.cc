/*  =========================================================================
    Malamute Node.js binding implementation

    Copyright (c) the Contributors as noted in the AUTHORS file.       
    This file is part of the Malamute Project.                         
                                                                       
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.           
    =========================================================================
*/

#include "binding.h"

using namespace v8;
using namespace Nan;

NAN_MODULE_INIT (MlmProto::Init) {
    Nan::HandleScope scope;

    // Prepare constructor template
    Local <FunctionTemplate> tpl = Nan::New <FunctionTemplate> (New);
    tpl->SetClassName (Nan::New ("MlmProto").ToLocalChecked ());
    tpl->InstanceTemplate ()->SetInternalFieldCount (1);

    // Prototypes
    Nan::SetPrototypeMethod (tpl, "destroy", destroy);
    Nan::SetPrototypeMethod (tpl, "defined", defined);
    Nan::SetPrototypeMethod (tpl, "recv", _recv);
    Nan::SetPrototypeMethod (tpl, "send", _send);
    Nan::SetPrototypeMethod (tpl, "routingId", _routing_id);
    Nan::SetPrototypeMethod (tpl, "id", _id);
    Nan::SetPrototypeMethod (tpl, "command", _command);
    Nan::SetPrototypeMethod (tpl, "address", _address);
    Nan::SetPrototypeMethod (tpl, "stream", _stream);
    Nan::SetPrototypeMethod (tpl, "pattern", _pattern);
    Nan::SetPrototypeMethod (tpl, "subject", _subject);
    Nan::SetPrototypeMethod (tpl, "content", _content);
    Nan::SetPrototypeMethod (tpl, "getContent", _get_content);
    Nan::SetPrototypeMethod (tpl, "sender", _sender);
    Nan::SetPrototypeMethod (tpl, "tracker", _tracker);
    Nan::SetPrototypeMethod (tpl, "timeout", _timeout);
    Nan::SetPrototypeMethod (tpl, "statusCode", _status_code);
    Nan::SetPrototypeMethod (tpl, "statusReason", _status_reason);
    Nan::SetPrototypeMethod (tpl, "amount", _amount);

    constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
    Nan::Set (target, Nan::New ("MlmProto").ToLocalChecked (),
    Nan::GetFunction (tpl).ToLocalChecked ());
}

MlmProto::MlmProto (void) {
    self = mlm_proto_new ();
}

MlmProto::MlmProto (mlm_proto_t *self_) {
    self = self_;
}

MlmProto::~MlmProto () {
}

NAN_METHOD (MlmProto::New) {
    assert (info.IsConstructCall ());
    MlmProto *mlm_proto = new MlmProto ();
    if (mlm_proto) {
        mlm_proto->Wrap (info.This ());
        info.GetReturnValue ().Set (info.This ());
    }
}

NAN_METHOD (MlmProto::destroy) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    mlm_proto_destroy (&mlm_proto->self);
}


NAN_METHOD (MlmProto::defined) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    info.GetReturnValue ().Set (Nan::New (mlm_proto->self != NULL));
}

NAN_METHOD (MlmProto::_recv) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    Zsock *input = Nan::ObjectWrap::Unwrap<Zsock>(info [0].As<Object>());
    int result = mlm_proto_recv (mlm_proto->self, input->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmProto::_send) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    Zsock *output = Nan::ObjectWrap::Unwrap<Zsock>(info [0].As<Object>());
    int result = mlm_proto_send (mlm_proto->self, output->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmProto::_routing_id) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    zframe_t *result = mlm_proto_routing_id (mlm_proto->self);
    Zframe *zframe_result = new Zframe (result);
    if (zframe_result) {
    //  Don't yet know how to return a new object
    //      zframe->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmProto::_id) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    int result = mlm_proto_id (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmProto::_command) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_command (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_address) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_address (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_stream) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_stream (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_pattern) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_pattern (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_subject) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_subject (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_content) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    zmsg_t *result = mlm_proto_content (mlm_proto->self);
    Zmsg *zmsg_result = new Zmsg (result);
    if (zmsg_result) {
    //  Don't yet know how to return a new object
    //      zmsg->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmProto::_get_content) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    zmsg_t *result = mlm_proto_get_content (mlm_proto->self);
    Zmsg *zmsg_result = new Zmsg (result);
    if (zmsg_result) {
    //  Don't yet know how to return a new object
    //      zmsg->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmProto::_sender) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_sender (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_tracker) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_tracker (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_timeout) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    uint32_t result = mlm_proto_timeout (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmProto::_status_code) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    uint16_t result = mlm_proto_status_code (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmProto::_status_reason) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    char *result = (char *) mlm_proto_status_reason (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmProto::_amount) {
    MlmProto *mlm_proto = Nan::ObjectWrap::Unwrap <MlmProto> (info.Holder ());
    uint16_t result = mlm_proto_amount (mlm_proto->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

Nan::Persistent <Function> &MlmProto::constructor () {
    static Nan::Persistent <Function> my_constructor;
    return my_constructor;
}


NAN_MODULE_INIT (MlmClient::Init) {
    Nan::HandleScope scope;

    // Prepare constructor template
    Local <FunctionTemplate> tpl = Nan::New <FunctionTemplate> (New);
    tpl->SetClassName (Nan::New ("MlmClient").ToLocalChecked ());
    tpl->InstanceTemplate ()->SetInternalFieldCount (1);

    // Prototypes
    Nan::SetPrototypeMethod (tpl, "destroy", destroy);
    Nan::SetPrototypeMethod (tpl, "defined", defined);
    Nan::SetPrototypeMethod (tpl, "actor", _actor);
    Nan::SetPrototypeMethod (tpl, "msgpipe", _msgpipe);
    Nan::SetPrototypeMethod (tpl, "connected", _connected);
    Nan::SetPrototypeMethod (tpl, "setPlainAuth", _set_plain_auth);
    Nan::SetPrototypeMethod (tpl, "connect", _connect);
    Nan::SetPrototypeMethod (tpl, "setConsumer", _set_consumer);
    Nan::SetPrototypeMethod (tpl, "setWorker", _set_worker);
    Nan::SetPrototypeMethod (tpl, "send", _send);
    Nan::SetPrototypeMethod (tpl, "sendto", _sendto);
    Nan::SetPrototypeMethod (tpl, "sendfor", _sendfor);
    Nan::SetPrototypeMethod (tpl, "recv", _recv);
    Nan::SetPrototypeMethod (tpl, "command", _command);
    Nan::SetPrototypeMethod (tpl, "status", _status);
    Nan::SetPrototypeMethod (tpl, "reason", _reason);
    Nan::SetPrototypeMethod (tpl, "address", _address);
    Nan::SetPrototypeMethod (tpl, "sender", _sender);
    Nan::SetPrototypeMethod (tpl, "subject", _subject);
    Nan::SetPrototypeMethod (tpl, "content", _content);
    Nan::SetPrototypeMethod (tpl, "tracker", _tracker);
    Nan::SetPrototypeMethod (tpl, "sendx", _sendx);
    Nan::SetPrototypeMethod (tpl, "sendtox", _sendtox);
    Nan::SetPrototypeMethod (tpl, "sendforx", _sendforx);
    Nan::SetPrototypeMethod (tpl, "recvx", _recvx);

    constructor ().Reset (Nan::GetFunction (tpl).ToLocalChecked ());
    Nan::Set (target, Nan::New ("MlmClient").ToLocalChecked (),
    Nan::GetFunction (tpl).ToLocalChecked ());
}

MlmClient::MlmClient (void) {
    self = mlm_client_new ();
}

MlmClient::MlmClient (mlm_client_t *self_) {
    self = self_;
}

MlmClient::~MlmClient () {
}

NAN_METHOD (MlmClient::New) {
    assert (info.IsConstructCall ());
    MlmClient *mlm_client = new MlmClient ();
    if (mlm_client) {
        mlm_client->Wrap (info.This ());
        info.GetReturnValue ().Set (info.This ());
    }
}

NAN_METHOD (MlmClient::destroy) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    mlm_client_destroy (&mlm_client->self);
}


NAN_METHOD (MlmClient::defined) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    info.GetReturnValue ().Set (Nan::New (mlm_client->self != NULL));
}

NAN_METHOD (MlmClient::_actor) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    zactor_t *result = mlm_client_actor (mlm_client->self);
    Zactor *zactor_result = new Zactor (result);
    if (zactor_result) {
    //  Don't yet know how to return a new object
    //      zactor->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmClient::_msgpipe) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    zsock_t *result = mlm_client_msgpipe (mlm_client->self);
    Zsock *zsock_result = new Zsock (result);
    if (zsock_result) {
    //  Don't yet know how to return a new object
    //      zsock->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmClient::_connected) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    bool result = mlm_client_connected (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New<Boolean>(result));
}

NAN_METHOD (MlmClient::_set_plain_auth) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *username;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `username`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`username` must be a string");
    else {
        Nan::Utf8String username_utf8 (info [0].As<String>());
        username = *username_utf8;
    }
    char *password;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `password`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`password` must be a string");
    else {
        Nan::Utf8String password_utf8 (info [1].As<String>());
        password = *password_utf8;
    }
    int result = mlm_client_set_plain_auth (mlm_client->self, (const char *)username, (const char *)password);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_connect) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *endpoint;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `endpoint`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`endpoint` must be a string");
    else {
        Nan::Utf8String endpoint_utf8 (info [0].As<String>());
        endpoint = *endpoint_utf8;
    }
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `timeout`");
    else
    if (!info [1]->IsNumber ())
        return Nan::ThrowTypeError ("`timeout` must be a number");
    uint32_t timeout = Nan::To<uint32_t>(info [1]).FromJust ();

    char *address;
    if (info [2]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `address`");
    else
    if (!info [2]->IsString ())
        return Nan::ThrowTypeError ("`address` must be a string");
    else {
        Nan::Utf8String address_utf8 (info [2].As<String>());
        address = *address_utf8;
    }
    int result = mlm_client_connect (mlm_client->self, (const char *)endpoint, (uint32_t)timeout, (const char *)address);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_set_consumer) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *stream;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `stream`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`stream` must be a string");
    else {
        Nan::Utf8String stream_utf8 (info [0].As<String>());
        stream = *stream_utf8;
    }
    char *pattern;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `pattern`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`pattern` must be a string");
    else {
        Nan::Utf8String pattern_utf8 (info [1].As<String>());
        pattern = *pattern_utf8;
    }
    int result = mlm_client_set_consumer (mlm_client->self, (const char *)stream, (const char *)pattern);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_set_worker) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *address;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `address`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`address` must be a string");
    else {
        Nan::Utf8String address_utf8 (info [0].As<String>());
        address = *address_utf8;
    }
    char *pattern;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `pattern`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`pattern` must be a string");
    else {
        Nan::Utf8String pattern_utf8 (info [1].As<String>());
        pattern = *pattern_utf8;
    }
    int result = mlm_client_set_worker (mlm_client->self, (const char *)address, (const char *)pattern);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_send) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *subject;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`subject` must be a string");
    else {
        Nan::Utf8String subject_utf8 (info [0].As<String>());
        subject = *subject_utf8;
    }
    Zmsg *content = Nan::ObjectWrap::Unwrap<Zmsg>(info [1].As<Object>());
    int result = mlm_client_send (mlm_client->self, (const char *)subject, &content->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_sendto) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *address;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `address`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`address` must be a string");
    else {
        Nan::Utf8String address_utf8 (info [0].As<String>());
        address = *address_utf8;
    }
    char *subject;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`subject` must be a string");
    else {
        Nan::Utf8String subject_utf8 (info [1].As<String>());
        subject = *subject_utf8;
    }
    char *tracker;
    if (info [2]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `tracker`");
    else
    if (!info [2]->IsString ())
        return Nan::ThrowTypeError ("`tracker` must be a string");
    else {
        Nan::Utf8String tracker_utf8 (info [2].As<String>());
        tracker = *tracker_utf8;
    }
    if (info [3]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `timeout`");
    else
    if (!info [3]->IsNumber ())
        return Nan::ThrowTypeError ("`timeout` must be a number");
    uint32_t timeout = Nan::To<uint32_t>(info [3]).FromJust ();

    Zmsg *content = Nan::ObjectWrap::Unwrap<Zmsg>(info [4].As<Object>());
    int result = mlm_client_sendto (mlm_client->self, (const char *)address, (const char *)subject, (const char *)tracker, (uint32_t)timeout, &content->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_sendfor) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *address;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `address`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`address` must be a string");
    else {
        Nan::Utf8String address_utf8 (info [0].As<String>());
        address = *address_utf8;
    }
    char *subject;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`subject` must be a string");
    else {
        Nan::Utf8String subject_utf8 (info [1].As<String>());
        subject = *subject_utf8;
    }
    char *tracker;
    if (info [2]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `tracker`");
    else
    if (!info [2]->IsString ())
        return Nan::ThrowTypeError ("`tracker` must be a string");
    else {
        Nan::Utf8String tracker_utf8 (info [2].As<String>());
        tracker = *tracker_utf8;
    }
    if (info [3]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `timeout`");
    else
    if (!info [3]->IsNumber ())
        return Nan::ThrowTypeError ("`timeout` must be a number");
    uint32_t timeout = Nan::To<uint32_t>(info [3]).FromJust ();

    Zmsg *content = Nan::ObjectWrap::Unwrap<Zmsg>(info [4].As<Object>());
    int result = mlm_client_sendfor (mlm_client->self, (const char *)address, (const char *)subject, (const char *)tracker, (uint32_t)timeout, &content->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_recv) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    zmsg_t *result = mlm_client_recv (mlm_client->self);
    Zmsg *zmsg_result = new Zmsg (result);
    if (zmsg_result) {
    //  Don't yet know how to return a new object
    //      zmsg->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmClient::_command) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *result = (char *) mlm_client_command (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmClient::_status) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    int result = mlm_client_status (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_reason) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *result = (char *) mlm_client_reason (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmClient::_address) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *result = (char *) mlm_client_address (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmClient::_sender) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *result = (char *) mlm_client_sender (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmClient::_subject) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *result = (char *) mlm_client_subject (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmClient::_content) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    zmsg_t *result = mlm_client_content (mlm_client->self);
    Zmsg *zmsg_result = new Zmsg (result);
    if (zmsg_result) {
    //  Don't yet know how to return a new object
    //      zmsg->Wrap (info.This ());
    //      info.GetReturnValue ().Set (info.This ());
        info.GetReturnValue ().Set (Nan::New<Boolean>(true));
    }
}

NAN_METHOD (MlmClient::_tracker) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *result = (char *) mlm_client_tracker (mlm_client->self);
    info.GetReturnValue ().Set (Nan::New (result).ToLocalChecked ());
}

NAN_METHOD (MlmClient::_sendx) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *subject;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`subject` must be a string");
    else {
        Nan::Utf8String subject_utf8 (info [0].As<String>());
        subject = *subject_utf8;
    }
    char *content;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `content`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`content` must be a string");
    else {
        Nan::Utf8String content_utf8 (info [1].As<String>());
        content = *content_utf8;
    }
    int result = mlm_client_sendx (mlm_client->self, (const char *)subject, (const char *)content);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_sendtox) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *address;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `address`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`address` must be a string");
    else {
        Nan::Utf8String address_utf8 (info [0].As<String>());
        address = *address_utf8;
    }
    char *subject;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`subject` must be a string");
    else {
        Nan::Utf8String subject_utf8 (info [1].As<String>());
        subject = *subject_utf8;
    }
    char *content;
    if (info [2]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `content`");
    else
    if (!info [2]->IsString ())
        return Nan::ThrowTypeError ("`content` must be a string");
    else {
        Nan::Utf8String content_utf8 (info [2].As<String>());
        content = *content_utf8;
    }
    int result = mlm_client_sendtox (mlm_client->self, (const char *)address, (const char *)subject, (const char *)content);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_sendforx) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *address;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `address`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`address` must be a string");
    else {
        Nan::Utf8String address_utf8 (info [0].As<String>());
        address = *address_utf8;
    }
    char *subject;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`subject` must be a string");
    else {
        Nan::Utf8String subject_utf8 (info [1].As<String>());
        subject = *subject_utf8;
    }
    char *content;
    if (info [2]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `content`");
    else
    if (!info [2]->IsString ())
        return Nan::ThrowTypeError ("`content` must be a string");
    else {
        Nan::Utf8String content_utf8 (info [2].As<String>());
        content = *content_utf8;
    }
    int result = mlm_client_sendforx (mlm_client->self, (const char *)address, (const char *)subject, (const char *)content);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

NAN_METHOD (MlmClient::_recvx) {
    MlmClient *mlm_client = Nan::ObjectWrap::Unwrap <MlmClient> (info.Holder ());
    char *subject_p;
    if (info [0]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `subject_p`");
    else
    if (!info [0]->IsString ())
        return Nan::ThrowTypeError ("`subject_p` must be a string");
    else {
        Nan::Utf8String subject_p_utf8 (info [0].As<String>());
        subject_p = *subject_p_utf8;
    }
    char *string_p;
    if (info [1]->IsUndefined ())
        return Nan::ThrowTypeError ("method requires a `string_p`");
    else
    if (!info [1]->IsString ())
        return Nan::ThrowTypeError ("`string_p` must be a string");
    else {
        Nan::Utf8String string_p_utf8 (info [1].As<String>());
        string_p = *string_p_utf8;
    }
    int result = mlm_client_recvx (mlm_client->self, (char **)&subject_p, (char **)&string_p);
    info.GetReturnValue ().Set (Nan::New<Number>(result));
}

Nan::Persistent <Function> &MlmClient::constructor () {
    static Nan::Persistent <Function> my_constructor;
    return my_constructor;
}


extern "C" NAN_MODULE_INIT (malamute_initialize)
{
    MlmProto::Init (target);
    MlmClient::Init (target);
}

NODE_MODULE (malamute, malamute_initialize)
