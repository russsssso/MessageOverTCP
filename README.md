# TCP-UDP Client-Server
This repository contains the code for a TCP-UDP client-server application. The server listens for incoming TCP connections and UDP datagrams, while the client connects to the server using TCP. The server handles multiple client connections and provides functionality for clients to subscribe, unsubscribe, and receive messages from various topics.

## Code Organization
The code is organized into several files:

- hash.h and hash.c: These files contain the implementation of a hash table data structure used for topic storage.
- helpers.h and helpers.c: These files contain helper functions for socket creation, address binding, and socket management.
- message.h and message.c: These files define the message data structure used for storing and sending messages.
- handle.h and handle.c: These files define functions for handling client requests and managing client connections.
- main.c: This file contains the main entry point of the server application.

## Usage
To compile the code, you can use the provided Makefile. Here are the available targets:

- server: Compiles the server application.
- subscriber: Compiles the subscriber client application.
- all (default): Compiles both the server and subscriber applications.
- clean: Removes the compiled object files and executable files.
  
To compile the server, use the following command:
```make server```

To compile the subscriber client, use the following command:
```make subscriber```

Once the server is compiled, you can run it by providing the desired port number as a command-line argument:
```./server <port>```

The subscriber client can be run similarly:
```./subscriber```

Once the server is running and clients are connected, clients can send various commands, such as subscribing to topics, unsubscribing from topics, and receiving messages.
