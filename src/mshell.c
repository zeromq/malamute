/*  =========================================================================
    mshell - Malamute command-line shell

    Copyright (c) the Contributors as noted in the AUTHORS file.
    This file is part of the Malamute Project.

    Accepts either two or three arguments:
    mshell stream pattern         -- show all matching messages
    mshell stream subject body    -- send one message to this stream / subject

    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, You can obtain one at http://mozilla.org/MPL/2.0/.
    =========================================================================
*/

#include "malamute.h"

int main (int argc, char *argv [])
{
    int argn = 1;
    bool verbose = false;
    bool null_auth = false;
    if (argc > argn && streq (argv [argn], "-v")) {
        verbose = true;
        argn++;
    }
    if (argc > argn && streq (argv [argn], "-n")) {
        null_auth = true;
        argn++;
    }
    //  Get stream, subject/pattern, and optional content to send
    char *stream  = argn < argc? argv [argn++]: NULL;
    char *subject = argn < argc? argv [argn++]: NULL;
    char *content = argn < argc? argv [argn++]: NULL;

    if (!stream || !subject || streq (stream, "-h")) {
        printf ("syntax: mshell [-v] [-n] stream subject [ body ]\n");
        return 0;
    }
    mlm_client_verbose = verbose;
    mlm_client_t *client = mlm_client_new ();
    assert (client);
    if (mlm_client_connect (client, "ipc://@/malamute", 1000, null_auth? "mshell": "mshell/mshell")) {
        zsys_error ("mshell: server not reachable at ipc://@/malamute");
        return 0;
    }
    if (content) {
        mlm_client_set_producer (client, stream);
        mlm_client_sendx (client, subject, content, NULL);
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
            printf ("Content=%s sender=%s subject=%s\n",
                content, mlm_client_sender (client), mlm_client_subject (client));
            zstr_free (&content);
            zmsg_destroy (&msg);
        }
    }
    mlm_client_destroy (&client);
    return 0;
}
