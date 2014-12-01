/*  =========================================================================
    perftest.c - Malamute performance tester

    Runs various tests on Malamute to test performance of the core engines.

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
    //  We will remove all buffer limits on internal plumbing; instead,
    //  we use credit-based flow control to rate-limit traffic per client.
    //  If we allow limits, then the engines will block under stress.
    zsys_set_pipehwm (0);
    zsys_set_sndhwm (0);
    zsys_set_rcvhwm (0);
    
    zactor_t *broker = zactor_new (mlm_server, NULL);
    zsock_send (broker, "ss", "BIND", "ipc://@/malamute");
//     zsock_send (broker, "s", "VERBOSE");

    //  1. Throughput test with minimal density
    mlm_client_t *reader = mlm_client_new ("ipc://@/malamute", 0, "reader");
    assert (reader);
    mlm_client_t *writer = mlm_client_new ("ipc://@/malamute", 0, "writer");
    assert (writer);
    mlm_client_produce (writer, "weather");
    mlm_client_consume (reader, "weather", "temp.");

    int64_t start = zclock_time ();
    int count = 100;
    if (argc > 1)
        count = atoi (argv [1]);
    printf ("COUNT=%d\n", count);
    
    zmsg_t *msg;
    while (count) {
        msg = zmsg_new ();
        zmsg_addstr (msg, "10");
        mlm_client_stream_send (writer, "temp.moscow", &msg);
        msg = zmsg_new ();
        zmsg_addstr (msg, "0");
        mlm_client_stream_send (writer, "rain.moscow", &msg);
        count--;
    }
    msg = zmsg_new ();
    zmsg_addstr (msg, "END");
    mlm_client_stream_send (writer, "temp.signal", &msg);
    
    while (true) {
        zmsg_t *msg = mlm_client_recv (reader);
        assert (msg);
        if (streq (mlm_client_subject (reader), "temp.signal"))
            break;
        //  TODO: define a clean strategy for message ownership?
//         zmsg_destroy (&msg);
        count++;
    }
    printf (" -- sending %d messages: %d msec\n", count, (int) (zclock_time () - start));
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);
    
    zactor_destroy (&broker);
    printf (" -- total number of allocs: %" PRId64 ", %d/msg\n", zsys_allocs,
            (int) (zsys_allocs / count));
    return 0;
}
