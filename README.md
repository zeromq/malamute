# Malamute

All the enterprise messaging patterns in one box.

![Malamute](http://i.imgur.com/WJsvV.jpg)

Malamute is a messaging broker and a protocol for enterprise messaging. Malamute helps to connect the pieces in a distributed software system. Before we explain what Malamute does, and why, let's discuss the different kinds of data we most often send around a distributed system.

*Note: this is a discussion whitepaper, it does not describe an actual system, and does not constitute a promise to build such a system. You can contact the author Pieter Hintjens at ph@imatix.com.*

## Live Streamed Data

Live data tell us about things happening across a distributed system. Live data typically flows in "streams" from a small set of producers to a large set of consumers, thus "live streamed data". Typically:

* One producer broadcasts a single stream.
* One consumer receives one, or a few streams.
* Streams are composed of small discrete messages.
* Many consumers can receive the same stream, with no extra effort for the producer.
* Live streamed data is asynchronous, and usually not acknowledged.
* Live streamed data is ephemeral, that is, there is no storage between producer and consumer.
* Live streamed data expires rapidly, that is, it loses its value within a short time.

Live streamed data is widely used in the real world, in a broadcast model. It defines TV and radio, and matches closely the physical world of sound and light radiation. Technically we can implement live streamed data over various transports, including UDP multicast, TCP unicast, hardware-enabled multicast (PGM), and so on.

Live streamed data can scale to thousands, or millions of consumers, and millions of messages per second, if designed correctly. The main pitfall is when producers attempt to buffer outgoing messages, when the network capacity is less than peak publishing rates. Broadcast models are susceptible to "slow consumers", which cause such buffers to rapidly fill up. Without strict limits, and a strategy for dealing with full buffers, a producer will exhaust its memory, and crash.

There are two plausible strategies for dealing with full buffers. One is to slow down the producer (so-called "back pressure"). This works in some cases. It does not however scale to real world use, first because we cannot slow down real-world information sources, and secondly because it allows one misbehaved consumer to punish all other consumers.

The second, and better strategy, is to discard messages for that misbehaving consumer. This is indeed how the Internet Protocol deals with congestion: it discards packets.

Live streamed data has these interesting properties:

* It does not need any mediation. Producers can talk to consumers directly, if they have a shared broadcast network. Indeed, mediation is undesirable in very high volume scenarios, as it adds latency and increases the risk of failure.

## On-Demand Streamed Data

A classic broadcast model, while scalable and robust, presents consumers with two main problems:

* First, they may randomly lose messages if the network is congested, or if they aren't keeping up with producers. The broadcast model is unreliable by design.

* Second, they cannot catch-up with old data, if they join the broadcast late. This is like arriving late for a movie and missing the opening scene.

We can solve this by recording live streamed data, and then offering consumers on-demand streamed data. This is how we recover from broadcast failures in real life.

On-demand streamed data has some interesting properties:

* It does not need coordination with producers. That is, to implement on-demand streamed data, we place a proxy between producers and consumers. The proxy must be fast enough, and close enough to the producers, to capture produced data without dropping packets.

* On-demand streamed data usually has a limited history, be it a few minutes, hours, or days. This history grows as the cost of storage falls, so today we can replay TV programs from years ago.

It is possible to create hybrid live/on-demand streamed architectures where late consumers can first request data on-demand streams, and then as they catch up, switch to live streams. This assumes that consumers can process messages faster than producers produce them.

## Publish-and-Subscribe (Pub-Sub)

As well as losing messages, the broadcast model does not provide producers with any mechanism for selective transmission. On a true multicast transport this is not an issue, as consumers can filter the data they receive. However when we implement broadcasts over a point-to-point transport, it is important to send a particular message only to consumers that have explicitly asked for it. Otherwise, we get network congestion, and lost message failures.

The solution is for consumers to subscribe to specific parts of a stream, by sending "subscription" to the producers. We then have to implement message-to-subscriber matching in the producers. There are many ways to do this, ranging from simple prefix matching to full content-based matching. The pitfall with pub-sub matching systems is that naive algorithms are O(n) where n is the number of subscriptions. A constant-cost matching system O(c) is difficult to design, and is impossible for content-based matching. 

We can thus divide pub-sub systems into three classes:

* Subscriber-side matching, where all messages in a stream are sent to all subscribers, which filter them. This is probably the only way to do content-based matching on a large scale.

* Publisher-side matching, where a simple and very fast matching algorithm can be hard-coded into each publisher, driven by subscriptions sent upstream by subscribers.

* Mediated matching, where matching is done by a specialized engine that can be upgraded, extended, and configured centrally.

## Direct Messaging

A direct message is addressed to a specific node. Direct messaging depends on identifying nodes, and tracking them as they appear, and disappear from the network.

Direct messaging usually has these properties:

* The recipient has a unique, long-lasting address, or identity.
* Direct messages are asynchronous, and may be acknowledged in some cases.
* Direct messages do not expire rapidly, and keep their value for long periods, even forever.
* Direct messages are low-volume, and usually significantly larger than streamed data messages.

Direct messaging is used widely in the real world: our postal service and email are classic examples.

The main pitfall for direct messaging is that senders and recipients are often separated in time. That is, the sender may appear, send messages, and disappear, while the recipient is absent. Distributed systems are typified by such separation in time and space, so a scalable direct messaging system depends on mediation, that is, proxies able to store and forward messages.

Direct messaging has these interesting properties:

* It needs a universal addressing scheme for recipients.

* It needs proxies or brokers able to accept, store, and then forward messages. Thus directed data is generally expected to be "reliable", with various forms of hand-over and acknowledgement.

* When the buffer for a particular recipient is full, the proxy will not discard data, rather it will tell the sender to wait, and try again later.

* We do have different expectations depending on different classes of direct message. Some messages are simply more important than others, so will be sent with more handshaking (so slower, and more reliably). It is typically the sender who determines this.

In real-life networks, we generally assume the mediation network is "almost" reliable (e.g. the post office), and we add confirmation by other channels (e.g. we call someone to ask if their package arrived). Very occasional lost messages are accepted as a cost of doing business. There are several well-known workarounds, including re-sending a message for which no confirmation has been received.

## Service Requests

A service request is a request to a group of nodes for some service. We assume all nodes are equally capable of providing the service. In a real world case, when we hail a taxi, we make a service request. Service requests assume some "contract" between the requester, and the worker. By announcing their intention to provide a particular service, workers implicitly accept the contract.

Service requests have two main varieties. The most simple model is unacknowledged service request distribution. In this case, requests are partitioned between a set of workers, without waiting for the workers to be "ready". This model does not require mediation, and requesters can talk directly to workers. However it has two main pitfalls:

* If one worker arrives before the others (which typically happens in a distributed system), it will receive a disproportionate number of service requests.

* If some requests are significantly more expensive than others, they will punish faster requests who by chance are queued behind them. Requests are then processed out of order.

We see this in the queue for the post office. If one queue forms behind each counter, a single very slow client will punish all those waiting behind him or her. If there is a single queue that feeds all counters, as they become ready, clients are processed fairly, and in order.

The simplest solution is to add a mediator that assigns new service requests to workers only when they are ready. This ensures that requests are processed on a fair first-come, first-served basis.

## The Malamute Model

Malamute defines these concepts:

* A *client* is an application sending or receiving messages.

* The Malamute *broker* is a mediator that connects a set of clients.

* The broker manages a set of *streams* that carry streamed data.

* The broker manages a set of *mailboxes* for direct messages.

* The broker manages a set of *queues* for service requests.

Clients can:

* Connect and authenticate.

* Send a message to a stream, to a mailbox, or to a queue.

* Subscribe to a stream, providing one or more match criteria and possibly a position for on-demand data.

* Signal readiness to receive stream data.

* Signal readiness to receive a service request, and reply to a service request once processed.

* Signal readiness to direct messages from a specific mailbox, and acknowledge a direct message once received.

The Malamute broker can:

* Check clients' credentials when they connect.

* Record streams to persistent storage, with a certain history, that can be defined per stream.

* Accept subscriptions from clients, and store these in the form of matching tables.

* Perform matching on stream messages, to determine the set of clients that receive any given message.

* Deliver matching live streamed data from a stream to a client.

* Deliver direct messages from a mailbox to a client.

* Deliver service requests from a mailbox to a client.

* Switch from live streamed data delivery to on-demand streamed data delivery, for slow or late clients, and switch back for clients that can catch up.

## Credit Based Flow Control (CBFC)

Malamute uses credit based flow control (CBFC) to manage the buffering of data from broker to client. In this model, the client sends credit to the broker, and the broker sends data in return. These two flows are asynchronous and overlapping, so that as long as the client can consume data at full speed, and send more credit, the broker will not pause it. However if the client slows down and does not send sufficient credit, the broker will also slow down and stop sending data.

Malamute does not use CBFC when sending data from client to broker.

## Reliability

We consider several types of failure to be frequent enough to need a recovery strategy built-in to Malamute.

* Clients that crash while processing service requests (common, because clients run application code, which is often buggy).

* Clients that disappear from the network without warning (common with certain classes of client, e.g. mobile devices).

* Clients that process streamed data too slowly to keep up with live data streams.

* Clients that stop responding, either because they are blocked, or because they are behind a dead network connection.

* Networks that are congested and cannot carry traffic from or to clients.

The Malamute broker itself is designed to never crash, nor block. The hardware it runs on may crash, or provide insufficient capacity for the broker. These are considered deployment failures, not software failures.

The CBFC model keeps data in the network pipeline between broker and client (that is, in send and receive buffers, and on the network itself in the form of IP packets). This reduces latency, however it will result in data loss when clients disconnect prematurely. We handle this in different ways depending on the data type:

* For streamed data, we discard any in-flight data. When the client recovers and reconnects, it can request on-demand streamed data starting from the last message it received.

* For direct messages, the client acknowledges the receipt of each message. The broker only removes a message from the mailbox when it has received such an acknowledgement.

* For service requests, the worker acknowledges receipt of a request and/or completion of the request. If a worker disconnects before sending such a request, the request will be re-queued and sent to another worker when possible. This can result in out-of-order processing of service requests.

## Detailed Technical Designs

### Streamed Data

A stream can have many writers, and many readers. It consists of a series of messages, numbered uniquely. A stream exists in a single Malamute broker. The full address of a stream is thus the broker address, followed by the stream name. This can be expressed as a URI: malamute://broker-address/stream/name.

Each message in a stream has two parts:

* A *header*, consisting of one or more keys.
* A *body*, consisting of an opaque blob (or list of blobs in practice).

While the Malamute broker uses the header, it never examines the body.

The Malamute matching engine accepts a key and returns a list of clients that have requested to receive this key. This list can be large, as Malamute is designed to scale to tens of thousands of clients. Various techniques may be used to store and process that list efficiently, such as compressed bitmaps.

The message is then replicated to each client, and simultaneously a dedicated background thread writes that to persistent storage. The storage may be implemented in various ways. It is typically a pre-allocated circular buffer on a magnetic or silicon disk.

If a particular client has no credit, the message is dropped and the client is switched to on-demand streaming, if it was still on live streaming. 

For on-demand streaming, the broker queries the journal and replays it to the client, according to the credit that the client sends. If the client exhausts the journal, and has credit, it is switched to live streaming.

The matching engine follows the design explained in Appendix A, to deliver O(c) performance. It supports an extensible set of matching algorithms including prefix matching, regular expression matching, topic matchings, and multi-field matching.

### Direct Messaging

When a client connects to a Malamute broker, it can optionally specify an identity (which the broker can authenticate if that is needed). Every client with an identity then automatically has a mailbox on the broker. Mailboxes may be persistent in the same way as streams (that is, the actual persistence mechanism is configurable).

A direct message is an opaque blob (or list of blobs). Additionally, the message carries the sender identity, if known, to allow replies. 

The broker acknowledges each direct message when it has stored it in the mailbox. If there is no mailbox for the recipient identity, this is created on-demand and automatically.

The recipient client acknowledges a direct message when it has received it from the mailbox. Malamute provides batching to recipients using the CBFC mechanism.

### Service Request Distribution

When a client sends a service request, the broker creates a service queue if needed. Service queues are persisted in the same way as mailboxes. When a service request has been stored, the broker acknowledges that.

A service request message consists of:

* The requested service name.
* The sender identity, if known.
* The request, which is an opaque blob (or list of blobs).

As and when workers become ready for the specified service, the broker delivers them requests. Workers acknowledge the request when they have completed it. Such a service reply is also sent back to the originating client. Workers can continue to send replies, before being available for a new request.

Replies back to an originating client are sent to the client mailbox. Thus clients may send service requests, disconnect, and then retrieve their replies at a later time.

## Malamute Packs

Malamute is designed to cluster easily and in various configurations. There are several common models:

* A pair of Malamute brokers that work together to back each other up. In this design, the two brokers share the workload, and if one broker crashes, the other will take over its duty.

* A directed graph of Malamute brokers that process stream data in various ways. This can also be done by applications that read from one stream, and publish into a different stream, on the same broker. Such applications are called "ticker plants".

* A peer-to-peer cluster of Malamute brokers serving a cloud, where each system on the cloud runs one Malamute broker. Applications connect to their local broker over IPC, and the brokers talk to each other over TCP. Malamute uses the Zyre clustering technology to rapidly discover and connect to new Malamute brokers.

## Multicast Capabilities

Malamute makes use of ZeroMQ's multicast publisher capability to stream data over PGM or NORM multicast. A Malamute broker can act as a proxy, reading a TCP stream and re-broadcasting it over multicast.

## Appendix A -- A High-Speed Matching Engine

This section describes a matching algorithm that provides the functionality needed for matching on one or more fields. This text is based on documentation of the OpenAMQ project from 2005.

### Argumentation

Message-to-subscription matching is a classic bottleneck in a pub-sub broker. Naive approaches have O(n) performance where n is the number of subscribers. This does not scale to large numbers of subscribers. One strategy is to drastically simplify the matching algorithms, e.g. to allow only prefix matching (cf. ZeroMQ). This does scale yet it limits usability.

We define a "request" as consisting of one or more "criteria", and a message as providing one or more "fields". The request specifies the desired criteria in terms of fields: for example, a field having a certain value, or matching a certain pattern.

The most obvious matching technique is to compare each message with each request. If the cost of such a comparison is C, then the cost of matching one message is:

    C * R

Where R is the number of requests active. The cost of C is proportional to the complexity of the request. In a low-volume scenario, R might be 1-10. In a high volume scenario we might have 10,000 active requests (R = 10,000), and a matching cost of 100 microseconds (C = 100), giving us a cost of 10,000 * 100 microseconds per message, or 1 second per message.

We can exploit the fact that many requests are identical. to normalize requests, and reduce R significantly. However we will still be limited. E.g. if R is 100, and C is 100, we can match 100 messages per second. This is too slow for practical purposes.

Our goal is to get a matching cost of 2 microseconds per message, for a potential throughput of 500,000 messages per second.

### The Inverted Bitmap Technique

In 1980-81, working for Lockheed, Leif Svalgaard and Bo Tveden built the Directory Assistance System for the New York Telephone company. The system consisted of 20 networked computers serving 2000 terminals, handling more than 2 million lookups per day. In 1982 Svalgaard and Tveden adapted the system for use in the Pentagon (Defense Telephone Service).

Svalgaard and Tveden invented the concept of "inverted bitmaps" to enable rapid matching of requests with names in the directory. The inverted bitmap technique is based on these principles:

1. We change data rarely, but we search frequently.
2. The number of possible searches is finite and can be represented by a large, sparse array of items against criteria, with a bit set in each position where an item matches a criteria and cleared elsewhere.

The indexing technique works as follows:

1. We number each searchable item from 0 upwards.
2. We analyse each item to give a set of "criteria" name and value tuples.
3. We store the criteria tuple in a table indexed by the name and value.
4. For each criteria tuple we store a long bitmap representing each item that matches it.

The searching technique works as follows:

1. We analyse the search request to give a set of criteria name and value tuples.
2. We look up each criteria tuple in the table, giving a set of bitmaps.
3. We AND or OR each bitmap to give a final result bitmap.
4. Each 1 bit in the result bitmap represents a matching item.

Note that the bitmaps can be huge, representing millions of items, and are usually highly compressible. Much of the art in using inverted bitmaps comes from:

1. Deriving accurate criteria tuples from items and search requests.
2. Careful compression techniques on the large sparse bitmaps.
3. Post-filtering search results to discard false positives.

We (Hintjens et al) have built several search engines using these techniques.

### Application to Message Matching

The inverted bitmap technique thus works by pre-indexing a set of searchable items so that a search request can be resolved with a minimal number of operations.

It is efficient if and only if the set of searchable items is relatively stable with respect to the number of search requests. Otherwise the cost of re-indexing is excessive.

When we apply the inverted bitmap technique to message matching, we may be confused into thinking that the message is the "searchable item". This seems logical except that message matching requests are relatively stable with respect to messages.

So, we must invert the roles so that:

1. The "searchable item" is the subscription.
2. The "search request" is the message.

The indexing process now works as follows:

1. We number each subscription from 0 upwards.
2. We analyse each subscription to give a set of criteria tuples.
3. We store the criteria tuples in a table indexed by name and value.
4. For each criteria tuple we store a long bitmap representing each subscription that asks for it.

The message matching process works as follows:

1. We analyse the message to give a set of criteria tuples.
2. We look up each tuple in the table, giving a set of bitmaps.
3. We accumulate the bitmaps to give a final result bitmap.
4. Each 1 bit in the result bitmap represents a matching subscription.

### Worked Examples

#### Topic Matching Example

Let's say we have these topics (a common style used in pub-sub matching):

    forex
    forex.gbp
    forex.eur
    forex.usd
    trade
    trade.usd
    trade.jpy

And these subscriptions, where * matches one topic name part and # matches one or more part:

    0 = "forex.*"
        matches: forex, forex.gbp, forex.eur, forex.usd
    1 = "*.usd"
        matches: forex.usd, trade.usd
    2 = "*.eur"
        matches: forex.eur
    3 = "#"
        matches: forex, forex.gbp, forex.usd, trade, trade.usd, trade.jpy

When we index the matches for each request we get these bitmaps:

    Criteria        0 1 2 3
    -----------------------
    forex           1 0 0 1
    forex.gbp       1 0 0 1
    forex.eur       1 0 1 1
    forex.usd       1 1 0 1
    trade           0 0 0 1
    trade.usd       0 1 0 1
    trade.jpy       0 0 0 1

Now let us examine in detail what happens for a series of messages:

    Message A -> "forex.eur"
        forex.eur   1 0 1 1 => requests 0, 2, 3

    Message B -> "forex"
        forex       1 0 0 1 => requests 0, 3

    Message C -> "trade.jpy"
        trade.jpy   0 0 0 1 => requests 3

The list of topics must either be known in advance, so that a request can be correctly mapped to the full set of topic names it represents, or the requests must be "recompiled" when a new topic name is detected for the first time.

#### Field Matching Example

For our example we will allow matching on field value and/or presence. That is, a request can specify a precise value for a field or ask that the field be present.

Imagine these subscriptions:

    Nbr  Criteria                     Number of criteria
    ---- ---------------------------  ------------------
    0    currency=USD, urgent         2
    1    currency=EUR                 1
    2    market=forex, currency=EUR   2
    3    urgent                       1

When we index the field name/value tuples for each subscription we get these bitmaps:

    Criteria        0 1 2 3
    -----------------------
    currency        0 0 0 0
    currency=USD    1 0 0 0
    currency=EUR    0 1 1 0
    market          0 0 0 0
    market=forex    0 0 1 0
    urgent          1 0 0 1

Now let us examine in detail what happens for a series of messages:

    Message A -> "currency=JPY, market=forex, slow"

        currency=JPY    0 0 0 0
        market=forex    0 0 1 0
        slow            0 0 0 0
        -----------------------
        Hits            0 0 1 0

    Message B -> "currency=JPY, urgent"

        currency=JPY    0 0 0 0
        urgent          1 0 0 1
        -----------------------
        Hits            1 0 0 1

    Message C -> "market=forex, currency=EUR"

        market=forex    0 0 1 0
        currency=EUR    0 1 1 0
        -----------------------
        Hits            0 1 2 0

Note that the hit count is:

- zero for subscriptions that do not match.
- 1 or greater for subscriptions that have at least one matching criteria (a logical OR match).
- Equal to the criteria count when ALL criteria match (a logical AND match).

### Fast Bitmap Implementation

The bitmap implementation is a key to good performance. We use these assumptions when making our bitmap design:

1. The maximum number of distinct subscriptions is several thousand. The bitmap for one criteria can therefore be held in memory with compression.

2. The maximum number of criteria is around a hundred thousand. Thus the bitmaps can be held in memory (on a suitable sized machine).

The bitmap engine provides methods for:

- Allocating a new empty bitmap.
- Setting or clearing a specific bit.
- Testing whether a specific bit is set to 1.
- Resetting a bitmap to all zeroes.
- Counting the number of bits set to 1.
- Performing a bitwise AND, OR, or XOR on two bitmaps.
- Performing a bitwise NOT on a bitmaps.
- Locating the first, last, next, or previous bit set to 1.
- Locating the first, last, next, or previous bit set to zero.
- Serialising a bitmap to and from a file stream.

### Performance

Real-time performance of the OpenAMQ matching engine on a 2Ghz workstation is as follows:

- Building a list of 2000 subscribers and compiling for 2000 topics (4M comparisons) - 12 seconds.
- Generating 1M messages and matching against all subscriptions (170M matches) - 26 seconds.

Thus we can do roughly 3M matches per second per Ghz.


