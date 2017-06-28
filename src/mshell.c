/*  =========================================================================
    mshell - Malamute command-line shell

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    Accepts either two or three arguments:
    mshell stream pattern         -- show all matching messages
    mshell stream subject message -- send message to stream / subject

    By default connects to a broker at tcp://127.0.0.1:9999; to connect
    to another endpoint, use the option -e endpoint.

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "mlm_classes.h"

int main (int argc, char *argv [])
{
    int argn = 1;
    bool verbose = false;
    bool null_auth = false;
    char *username = "mshell";
    char *password = "mshell";
    char *endpoint = "tcp://127.0.0.1:9999";
    if (argc > argn && streq (argv [argn], "-v")) {
        verbose = true;
        argn++;
    }
    if (argc > argn && streq (argv [argn], "-n")) {
        null_auth = true;
        argn++;
    }
    if (argc > argn && streq (argv [argn], "-e")) {
        endpoint = argv [++argn];
        argn++;
    }
    if (argc > argn && streq (argv [argn], "-p")) {
        username = argv [++argn];
        password = argv [++argn];
        argn++;
    }
    //  Get stream, subject/pattern, and optional content to send
    char *stream  = argn < argc? argv [argn++]: NULL;
    char *subject = argn < argc? argv [argn++]: NULL;
    char *content = argn < argc? argv [argn++]: NULL;

    if (!stream || !subject || streq (stream, "-h")) {
        printf ("syntax: mshell [-v] [-n] [-e endpoint] [-p username password] stream subject [body]\n");
        return 0;
    }
    mlm_client_t *client = mlm_client_new ();
    assert (client);
    mlm_client_set_verbose (client, verbose);

    if (!null_auth)
        mlm_client_set_plain_auth (client, username, password);

    if (mlm_client_connect (client, endpoint, 1000, "")) {
        zsys_error ("mshell: server not reachable at %s", endpoint);
        mlm_client_destroy (&client);
        return 0;
    }
    if (content) {
        mlm_client_sendx (client, stream, subject, content, NULL);
    }
    else {
        //  Consume the event subjects specified by the pattern
        mlm_client_set_consumer (client, stream, subject);
        while (true) {
            //  Now receive and print any messages we get
            zmsg_t *msg = mlm_client_recv (client);
            if (!msg)
                break;          //  Interrupted
            char *content = zmsg_popstr (msg);
            printf ("Content=%s subject=%s\n", content, mlm_client_subject (client));
            zstr_free (&content);
            zmsg_destroy (&msg);
        }
    }
    mlm_client_destroy (&client);
    return 0;
}
