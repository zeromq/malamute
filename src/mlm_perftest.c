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
#include "mlm_classes.h"

int main (int argc, char *argv [])
{
    //  We will remove all buffer limits on internal plumbing; instead,
    //  we use credit-based flow control to rate-limit traffic per client.
    //  If we allow limits, then the engines will block under stress.
    zsys_set_pipehwm (0);
    zsys_set_sndhwm (0);
    zsys_set_rcvhwm (0);

    zactor_t *broker = zactor_new (mlm_server, NULL);
    zsock_send (broker, "ss", "BIND", "tcp://127.0.0.1:9999");
//     zsock_send (broker, "s", "VERBOSE");

    //  1. Throughput test with minimal density
    mlm_client_t *reader = mlm_client_new ();
    assert (reader);
    int rc = mlm_client_connect (reader, "tcp://127.0.0.1:9999", 0, "reader");
    assert (rc == 0);

    mlm_client_t *writer = mlm_client_new ();
    assert (writer);
    rc = mlm_client_connect (writer, "tcp://127.0.0.1:9999", 0, "writer");
    assert (rc == 0);

    mlm_client_set_consumer (reader, "weather", "temp.");

    int64_t start = zclock_time ();
    int count = 100;
    if (argc > 1)
        count = atoi (argv [1]);
    printf ("COUNT=%d\n", count);

    while (count) {
        mlm_client_sendx (writer, "weather", "temp.moscow", "10", NULL);
        mlm_client_sendx (writer, "weather", "rain.moscow", "0", NULL);
        count--;
    }
    mlm_client_sendx (writer, "weather", "temp.signal", "END", NULL);

    while (true) {
        zmsg_t *msg = mlm_client_recv (reader);
        assert (msg);
        if (streq (mlm_client_subject (reader), "temp.signal"))
            break;
        zmsg_destroy (&msg);
        count++;
    }
    printf (" -- sending %d messages: %d msec\n", count, (int) (zclock_time () - start));
    mlm_client_destroy (&reader);
    mlm_client_destroy (&writer);

    zactor_destroy (&broker);
    return 0;
}
