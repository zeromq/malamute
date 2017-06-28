/*  =========================================================================
    tutorial.c - Malamute tutorial

    Learn how to use the Malamute C APIs in a single breath. Malamute lives
    as a C library, exposing a simple class model. In this tutorial we'll
    take you through those classes, and show with live data how they work.

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/


//  This header file gives us the Malamute APIs plus Zyre, CZMQ, and libzmq:
#include "mlm_classes.h"

int main (int argc, char *argv [])
{
    //  Let's start a new Malamute broker
    zactor_t *broker = zactor_new (mlm_server, NULL);

    //  Switch on verbose tracing... this gets a little overwhelming so you
    //  can comment or delete this when you're bored with it:
    zsock_send (broker, "s", "VERBOSE");

    //  We control the broker by sending it commands. It's a CZMQ actor, and
    //  we can talk to it using the zsock API (or zstr, or zframe, or zmsg).
    //  To get things started, let's tell the broker to bind to an endpoint:
//     zsock_send (broker, "ss", "BIND", "tcp://*:12345");

    //  This is how we configure a server from an external config file, which
    //  is in http://rfc.zeromq.org/spec:4/ZPL format:
    zstr_sendx (broker, "LOAD", "src/malamute.cfg", NULL);

    //  We can also, or alternatively, set server properties by sending it
    //  SET commands like this (see malamute.cfg for details):
    zsock_send (broker, "sss", "SET", "server/timeout", "5000");

    //  For PLAIN authentication, we start a zauth instance. This handles
    //  all client connection requests by checking against a password file
    zactor_t *auth = zactor_new (zauth, NULL);
    assert (auth);

    //  We can switch on verbose tracing to debug authentication errors
    zstr_sendx (auth, "VERBOSE", NULL);
    zsock_wait (auth);

    //  Now specify the password file; each line says 'username=password'
    zstr_sendx (auth, "PLAIN", "src/passwords.cfg", NULL);
    zsock_wait (auth);

    //  The broker is now running. Let's start two clients, one to publish
    //  messages and one to receive them. We're going to test the stream
    //  pattern with some natty wildcard patterns.
    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    int rc = mlm_client_set_plain_auth (reader, "reader", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (reader, "tcp://127.0.0.1:9999", 1000, "reader");
    assert (rc == 0);

    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    rc = mlm_client_set_plain_auth (writer, "writer", "secret");
    assert (rc == 0);
    rc = mlm_client_connect (writer, "tcp://127.0.0.1:9999", 1000, "writer");
    assert (rc == 0);

    //  The reader consumes temperature messages off the "weather" stream
    mlm_client_set_consumer (reader, "weather", "temp.*");

    //  The writer sends a series of messages with various subjects. The
    //  sendx method sends string data to the stream (we send the subject,
    //  then one or more strings):
    mlm_client_sendx (writer, "weather", "temp.moscow", "1", NULL);
    mlm_client_sendx (writer, "weather", "rain.moscow", "2", NULL);
    mlm_client_sendx (writer, "weather", "temp.madrid", "3", NULL);
    mlm_client_sendx (writer, "weather", "rain.madrid", "4", NULL);
    mlm_client_sendx (writer, "weather", "temp.london", "5", NULL);
    mlm_client_sendx (writer, "weather", "rain.london", "6", NULL);

    //  The simplest way to receive a message is via the recvx method,
    //  which stores multipart string data:
    char *address, *subject, *content;
    mlm_client_recvx (reader, &address, &subject, &content, NULL);
    assert (streq (address, "weather"));
    assert (streq (subject, "temp.moscow"));
    assert (streq (content, "1"));
    zstr_free (&address);
    zstr_free (&subject);
    zstr_free (&content);

    //  The last-received message has other properties:
    assert (streq (mlm_client_subject (reader), "temp.moscow"));
    assert (streq (mlm_client_command (reader), "STREAM DELIVER"));
    assert (streq (mlm_client_sender (reader), "writer"));
    assert (streq (mlm_client_address (reader), "weather"));

    //  Let's get the other two messages:
    mlm_client_recvx (reader, &address, &subject, &content, NULL);
    assert (streq (address, "weather"));
    assert (streq (subject, "temp.madrid"));
    assert (streq (content, "3"));
    zstr_free (&address);
    zstr_free (&subject);
    zstr_free (&content);

    mlm_client_recvx (reader, &address, &subject, &content, NULL);
    assert (streq (address, "weather"));
    assert (streq (subject, "temp.london"));
    assert (streq (content, "5"));
    zstr_free (&address);
    zstr_free (&subject);
    zstr_free (&content);

    //  Great, it all works. Now to shutdown, we use the destroy method,
    //  which does a proper deconnect handshake internally:
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);

    //  Finally, shut down the broker by destroying the actor; this does
    //  a proper shutdown so that all memory is freed as you'd expect.
    zactor_destroy (&broker);
    zactor_destroy (&auth);
    return 0;
}
