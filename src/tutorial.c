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
#include "../include/malamute.h"

int main (int argc, char *argv [])
{
    //  Let's start a new Malamute broker
    zactor_t *broker = zactor_new (mlm_server, NULL);

    //  We control the broker by sending it commands. It's a CZMQ actor, and
    //  we can talk to it using the zsock API (or zstr, or zframe, or zmsg).
    //  To get things started, let's tell the broker to bind to an endpoint:
    zsock_send (broker, "ss", "BIND", "ipc://@/mlm");

    //  And now, switch on verbose tracing... this gets a little overwhelming
    //  so you can comment or delete this when you're bored with it:
    zsock_send (broker, "s", "VERBOSE");

    //  This is how we configure a server from an external config file, which
    //  is in http://rfc.zeromq.org/spec:4/ZPL format:
    zstr_sendx (broker, "CONFIGURE", "malamute.cfg", NULL);

    //  We can also, or alternatively, set server properties by sending it
    //  SET commands like this (see malamute.cfg for details):
    zsock_send (broker, "sss", "SET", "server/timeout", "5000");

    //  The broker is now running. Let's start two clients, one to publish
    //  messages and one to receive them. We're going to test the stream
    //  pattern with some natty wildcard patterns.

    //  This is how we create the two clients (using a timeout of zero as
    //  we just started the broker so it's unlikely to not be there...):
    mlm_client_t *reader = mlm_client_new ("ipc://@/mlm", 0);
    mlm_client_t *writer = mlm_client_new ("ipc://@/mlm", 0);

    mlm_client_produce (writer, "weather");
    mlm_client_consume (reader, "weather", "temp.*");

    //  The writer sends a series of messages with various subjects:
    mlm_client_send (writer, "temp.moscow", "1");
    mlm_client_send (writer, "rain.moscow", "2");
    mlm_client_send (writer, "temp.chicago", "3");
    mlm_client_send (writer, "rain.chicago", "4");
    mlm_client_send (writer, "temp.london", "5");
    mlm_client_send (writer, "rain.london", "6");

    //  The reader can now receive its messages. We expect three messages.
    //  This is how we receive a message:
    char *message = mlm_client_recv (reader);
    assert (streq (message, "1"));

    //  We don't own the returned value. It is the string content (the API
    //  currently does not implement full messages, just strings). We can
    //  also get other properties: subject, content, and sender (the API
    //  doesn't implement senders yet, so this is empty):
    assert (streq (mlm_client_subject (reader), "temp.moscow"));
    assert (streq (mlm_client_sender (reader), ""));

    //  Let's get the other two messages:
    message = mlm_client_recv (reader);
    assert (streq (message, "3"));
    assert (streq (mlm_client_subject (reader), "temp.chicago"));
    message = mlm_client_recv (reader);
    assert (streq (message, "5"));
    assert (streq (mlm_client_subject (reader), "temp.london"));

    //  Great, it all works. Now to shutdown, we use the destroy method,
    //  which does a proper deconnect handshake internally:
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
    
    //  Finally, shut down the broker by destroying the actor; this does a
    //  proper shutdown so that all memory is freed as you'd expect.
    zactor_destroy (&broker);
    
    return 0;
}
