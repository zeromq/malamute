# Malamute vs AMQP - Interview

AMQP 0.8/9 was mainly designed by Pieter Hintjens, also the designer of Malamute and numerous other protocols. We asked Pieter to explain why Malamute was better than AMQP.

Q: Why make Malamute, when AMQP is already so popular?

A: Let me be clear: AMQP was great ten years ago when we invented it. I did that core design so I know it very well. Today AMQP is a mess. The popular version, AMQP 0.9, which RabbitMQ implements, is frozen. There's no process for improving it. It is a dead protocol. The official version is 1.0, a totally different protocol, which does not even do message queuing at all! It is just a very complex point-to-point transport protocol.

Q: Surely even if it's frozen, it is usable?

A: We have learned a lot about how to make messaging protocols, in the last decade. We derived AMQP from the Java Messaging System (JMS). It is very complex. Today we make simpler protocols that do the same work, faster and better.

Q: Can you give me one example?

A: Both AMQP and Malamute are described as XML models (we use similar code generation tools today as then). AMQP is 2,000 lines. Malamute is 250 lines. Malamute is based around credit-based flow control. AMQP uses clumsy handshaking. To make AMQP five times faster, for DowJones in 2007, we had to write our own extensions to it. We could not improve the official protocol: our extensions were rejected. Similarly we could not add multicast to AMQP. This was a large part of our decision to abandon AMQP and focus on ZeroMQ.

Q: So you are sure that AMQP is dead?

A: Sadly, yes. Otherwise we would not have shut down OpenAMQ, which was a superb broker, fast and stable. When the protocols cannot be improved, they die. Malamute's protocols are "free and open standards" using the Digital Standards Organization's model, so they cannot be destroyed. We can use and improve them for 50 years if needed.

Q: What other advantages does ZeroMQ bring?

A: It brings a large and growing community with answers to every problem. Since Malamute is based on ZeroMQ we get excellent security like CURVE and GSSAPI, support for all languages and platforms (including native Java, .NET and Erlang stacks), clustering technology like Zyre, and high-performance multicast (PGM and NORM).

Q: How does the Malamute broker compare with iMatix's OpenAMQ broker?

A: Because ZeroMQ takes care of so many things for us, Malamute is smaller and simpler to do the same kind of work. That means less code to go wrong. Our modern libraries and tools like CZMQ and zproto are very powerful. We had similar things in 2004, yet they weren't as good. OpenAMQ did multithreading the old way, using locks. This was difficult to make stable. Malamute uses "actors", i.e. sends messages between threads, which is far easier and very stable.

Q: When can we start using Malamute?

A: Today. It is ready now, for simple pub-sub tasks. We will drive Malamute using the same process we have used successfully for ZeroMQ for several years: by customer issues and pull requests, with guarantees of backwards compatibility.

Q: Can you summarize this again, why is Malamute better?

A: It does the same kinds of work as AMQP, yet is significantly faster and simpler. The Malamute broker is smaller. Malamute is part of the ZeroMQ family, so benefits from ZeroMQ's security, multicast, platform support, language support, and community. Whereas the AMQP protocol could be broken by a committee, Malamute's protocols are guaranteed free and open forever. 
