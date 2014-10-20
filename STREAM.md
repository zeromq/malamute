+ The Stream API

In Malamute, a stream engine is responsible for handling subscriptions and persistence. Malamute offers an extensible framework into which we can plug arbitrary engines. This framework uses a standard message-based API, which this document explains. This API runs over ZeroMQ inproc sockets.

Stream engines are always CZMQ zactors, written in C, and the stream API extends the CZMQ actor interface. The basic example stream engine is mlm_stream_simple. You can use this as the basis for your own engines.

The goal of the stream API is to create a fully asynchronous flow of subscriptions, messages, and credit through the processing chain: clients, server, engines, back to server, and clients again.

One Malamute stream exists as one stream engine (so one actor, and one OS thread).

++ Set-up

To create a new stream, e.g. using mlm_stream_simple:

    zactor_t *stream = zactor_new (mlm_stream_simple, "my stream");

To destroy a stream:

    zactor_destroy (&stream);

To enable verbose logging of a stream:

    zstr_send (stream, "VERBOSE");

++ Publishing to a Stream

To compile a subscription (this command is idempotent in the stream engine):
    
    zsock_send (stream, "sps", "COMPILE", client, pattern);

Where the client is a void * reference to a client, and the pattern is a string understood by the engine.

To cancel all subscriptions for a given client (this command is idempotent in the stream engine):

    zsock_send (stream, "sp", "CANCEL", client);

To publish a message to a stream:

    zsock_send (stream, "spssm", "ACCEPT", client, address, subject, content);

Where the client is the reference of the sender, so that it does not get the published message. The 'address' and 'subject' are fields sent with the message. The content is a zmsg_t * instance.

++ Message Delivery

Messages are sent asynchronously back to the server via a push-pull socket pattern. All streams have a PUSH socket, and the server has one PULL socket. The server takes messages off the PULL socket and dispatches them to the appropriate client according to the client reference provided in each message.

The server tells a new stream what endpoint to bind its PULL socket to (this command is idempotent in the stream engine):

    zsock_send (stream, "ss", "TRAFFIC", endpoint);

The engine creates a PUSH socket and uses that to send messages to the server:

    zsock_send (push_socket, "pssm", client, sender, subject, content);

Where 'client' is a client reference.

