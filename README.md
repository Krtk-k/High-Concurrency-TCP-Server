# High-Concurrency TCP Server (C++)

A high-concurrency, event-driven TCP server built in modern C++ using **`epoll` (Linux I/O multiplexing)** and a lightweight **thread pool** for asynchronous task execution.

This project demonstrates how to handle a large number of simultaneous client connections efficiently by combining:

- Non-blocking sockets
- Edge-triggered event notification (`epoll`)
- Worker-thread offloading for request processing
- Minimal per-connection overhead

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Why This Design Is Efficient](#why-this-design-is-efficient)
- [Features](#features)
- [Repository Structure](#repository-structure)
- [Build Instructions](#build-instructions)
- [How to Run](#how-to-run)
- [Protocol](#protocol)
- [Load Test Client](#load-test-client)
- [Performance Notes](#performance-notes)
- [Known Limitations & Future Improvements](#known-limitations--future-improvements)
- [Tech Stack](#tech-stack)
- [Learning Outcomes](#learning-outcomes)

---

## Overview

This repository contains:

1. **`src/server.cpp`**  
   A TCP server that accepts and manages many concurrent client connections using `epoll`.

2. **`include/ThreadPool.hpp`**  
   A simple thread pool used to process arithmetic commands asynchronously (`ADD`, `SUB`) and send responses.

3. **`src/clients.cpp`**  
   A high-volume client simulator that opens up to **1000 concurrent non-blocking connections** and sends requests to benchmark server responsiveness.

---

## Architecture

### Event-Driven Network Core (`epoll`)

The server uses Linux `epoll` to monitor:
- The listening socket for new incoming connections
- Active client sockets for incoming data

This avoids one-thread-per-connection scalability bottlenecks and keeps context switching low.

### Non-Blocking I/O

Both server and client sockets are configured with `O_NONBLOCK`, enabling:
- Immediate return from socket operations
- Better responsiveness under high connection counts
- No blocking on slow clients

### Thread Pool Offload

Once a request is parsed, computation + response writing is pushed to a fixed-size thread pool:
- Main thread remains focused on I/O/event handling
- Worker threads execute request tasks concurrently
- Work queue smooths traffic spikes

---

## Why This Design Is Efficient

This system is efficient for high concurrency because of the following design choices:

### 1) `epoll` Scales Better Than Naive Polling
- `epoll` tracks only active file descriptors.
- Event-driven wakeups reduce unnecessary scanning.
- Suitable for large numbers of idle/active connections.

### 2) Non-Blocking Sockets Prevent Thread Stalls
- Server never waits on a single socket.
- A slow client does not block progress for others.
- Throughput remains stable under mixed client behavior.

### 3) Separation of Concerns: I/O vs Work
- Main loop handles accept/read readiness quickly.
- CPU work is delegated to worker threads.
- Prevents long-running work from delaying event loop latency.

### 4) Fixed Thread Pool Reduces Overhead
- Reuses worker threads instead of spawning per request.
- Lower thread-creation/destruction cost.
- Predictable resource usage under load.

### 5) Queue-Based Backpressure Buffer
- Incoming tasks are enqueued safely.
- Temporary bursts are absorbed without immediate collapse.
- Enables smoother request handling at peak load.

### 6) Lightweight Text Command Protocol
- Simple parsing (`ADD a b`, `SUB a b`) keeps per-request CPU usage low.
- Minimal payload size improves network and parse efficiency.

---

## Features

- High-concurrency connection handling with `epoll`
- Non-blocking network I/O
- Thread pool with task queue + condition variable signaling
- Concurrent arithmetic command processing:
  - `ADD <a> <b>`
  - `SUB <a> <b>`
- Stress/load simulation client for parallel request generation

---

## Repository Structure

```text
High-Concurrency-TCP-Server/
├── include/
│   └── ThreadPool.hpp      # Worker pool and task queue
├── src/
│   ├── server.cpp          # epoll-based concurrent TCP server
│   └── clients.cpp         # Non-blocking load test client (1000 requests)
└── CMakeLists.txt          # Build configuration (CMake)
```

---

## Build Instructions

### Prerequisites

- Linux (required for `epoll`)
- C++ compiler with C++11+ support (g++/clang++)
- CMake (if using CMake build)

### Build (CMake)

```bash
mkdir -p build
cd build
cmake ..
make -j
```

If your targets are named conventionally, binaries may be generated like:
- `server`
- `clients`

---

## How to Run

Open two terminals.

### Terminal 1 — Start server

```bash
./server
```

Expected log:
- `Server is listning on port 8080`

### Terminal 2 — Run load client

```bash
./clients
```

Expected behavior:
- Sends 1000 connection requests
- Sends command `ADD 5 11` for each connection
- Reports completed responses

---

## Protocol

The server accepts whitespace-delimited commands:

- `ADD a b` → returns `a + b`
- `SUB a b` → returns `a - b`

### Example

Request:
```text
ADD 5 11
```

Response:
```text
16
```

---

## Load Test Client

`src/clients.cpp`:
- Opens up to **1000 non-blocking TCP connections** to `127.0.0.1:8080`
- Uses `epoll` to track connect/write/read readiness
- Sends requests on writable sockets
- Collects responses from readable sockets
- Closes sockets after response receipt

This is useful for quickly validating concurrency behavior and responsiveness.

---

## Performance Notes

The implementation demonstrates key high-performance networking practices:

- **O(ready events)** event handling style through `epoll`
- **Low idle overhead** because inactive sockets consume little CPU
- **Improved CPU utilization** by delegating work to a reusable worker pool
- **Reduced lock contention scope** (queue lock held only during push/pop)

For deeper benchmarking, consider adding:
- requests/sec
- p50/p95/p99 latency
- CPU and memory profiles under varying thread counts and client limits

---

## Known Limitations & Future Improvements

To move toward production-grade robustness, consider:

1. **Thread-safety fix in pool shutdown**
   - `end` flag should be synchronized (protected by mutex or atomic).

2. **Error handling hardening**
   - Check return values for `socket`, `bind`, `listen`, `accept`, `read`, `write`, `epoll_ctl`, `epoll_wait`, etc.

3. **Edge-triggered correctness completeness**
   - In ET mode, reads/writes should generally loop until `EAGAIN` to avoid missed readiness.

4. **Connection lifecycle safety**
   - Ensure worker thread writes are safe if client closes before task executes.

5. **Socket options**
   - Add `SO_REUSEADDR` (and optionally `SO_REUSEPORT`) for restart friendliness.

6. **Protocol robustness**
   - Handle partial reads/writes, malformed payloads, and request framing.

7. **Graceful shutdown**
   - Add signal handling and controlled server termination path.

8. **Configurable parameters**
   - Make port, thread count, and max clients runtime-configurable.

9. **Observability**
   - Add structured logging and metrics export.

10. **Testing**
   - Unit tests for parser/thread pool; integration tests for server-client workflows.

---

## Tech Stack

- **Language:** C++
- **Concurrency:** `std::thread`, `std::mutex`, `std::condition_variable`
- **Networking:** POSIX sockets
- **Multiplexing:** Linux `epoll`
- **Build:** CMake

---

## Learning Outcomes

This project is a solid demonstration of:

- Event-driven server design on Linux
- Combining asynchronous I/O and worker concurrency
- Building practical load simulation clients
- Understanding performance-oriented systems programming in C++

---

## Author

Developed by **[@Krtk-k](https://github.com/Krtk-k)**

If you found this helpful, consider starring the repository.
