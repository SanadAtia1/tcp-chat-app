# TCP Chat Server & Client (C++)

A multithreaded chat application built in C++ using POSIX sockets. The server accepts multiple simultaneous client connections and broadcasts messages between them in real time.

## Features

- **Multi-client support** — the server handles several connected clients at once, not just one at a time.
- **Real-time message broadcasting** — a message sent by one client is delivered to all other connected clients.
- **Threading** — both the server and client use threads to keep the program responsive (e.g. sending and receiving happen concurrently rather than blocking each other).
- **Thread-safe shared state** — uses mutexes (`std::lock_guard`) to safely manage shared data (e.g. the list of connected clients) across multiple threads, preventing race conditions.
- **Graceful connect/disconnect handling** — clients can join or leave at any time without crashing the server or disrupting other active connections.
- **Clean exit command** — typing `quit` disconnects a client and removes them from the active client list without broadcasting anything to other users.
- **Named clients** — on connecting, each client is prompted to enter a username, which is displayed alongside their messages so other clients can see who sent what.

## Tech Stack

- **Language:** C++
- **Networking:** POSIX sockets (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`)
- **Concurrency:** POSIX threads, with `std::mutex` / `std::lock_guard` for thread-safe access to shared data
- **Environment:** Developed and tested on Linux (via WSL) and macOS

## Getting Started

### Prerequisites

- A C++ compiler (`g++`)
- A Linux or macOS environment (or WSL on Windows) — this project uses POSIX sockets, which are not natively available on Windows

### Compiling

From the project directory:

```bash
g++ TCPserver.cpp -o server
g++ TCPclient.cpp -o client
```

### Running

1. Start the server in one terminal:
   ```bash
   ./server
   ```
2. Start one or more clients in separate terminals:
   ```bash
   ./client
   ```
3. Each client will be prompted to enter a username first — this name will be shown alongside their messages to all other connected clients.
4. Type a message in any client terminal and press Enter — it will be broadcast to all other connected clients.
5. Type `quit` to disconnect that client cleanly — it will be removed from the active client list without broadcasting anything to the other clients.
6. Clients can also disconnect at any time (e.g. `Ctrl+C`) without affecting the server or other connected clients.

> **Note:** This project currently runs on `localhost` (all client and server instances on the same machine). It has not yet been tested or configured for communication across separate machines on a network.

## What This Project Demonstrates

- Core socket programming: creating, binding, listening, accepting, sending, and receiving over TCP
- Managing multiple concurrent connections without one client blocking another
- Safe handling of clients joining and leaving mid-session
- Safe concurrent access to shared data using mutexes, avoiding race conditions between threads
- Applying multithreading to a real, interactive networked application
