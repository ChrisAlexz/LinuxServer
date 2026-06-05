# Linux HTTP Server

A high-performance HTTP/1.1 server built from scratch in C++ using Linux-native APIs.

## Architecture

- **epoll** — Linux I/O multiplexing to monitor thousands of connections efficiently
- **Thread pool** — 8 worker threads with `std::mutex` and `std::condition_variable` task queue
- **sendfile()** — zero-copy file transfer via Linux kernel syscall
- **SO_REUSEPORT** — multiple threads bind the same port to eliminate accept bottlenecks
- **signalfd** — graceful shutdown on `SIGINT`/`SIGTERM`
- **/stats endpoint** — live server metrics read directly from `/proc` filesystem

## Benchmark

Tested with `wrk` on WSL2 (Ubuntu):

## Build & Run

```bash
make
./server
```

Then visit `http://localhost:8080` or `http://localhost:8080/stats`.

## Files

| File | Description |
|------|-------------|
| `main.cpp` | Entry point, epoll loop, signal handling |
| `ThreadPool.h` | Fixed thread pool with condition variable task queue |
| `EpollServer.h` | Socket creation, epoll setup, SO_REUSEPORT |
| `http_handler.h` | HTTP parsing, file serving, /stats endpoint |
| `Makefile` | Build config |
| `www/` | Static files served by the server |