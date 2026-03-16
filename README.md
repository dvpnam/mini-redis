**A lightweight Redis-like key-value server written in C**

---

## 🏗️ Architecture

```
  Client (nc / telnet)
         │
         │  raw text commands
         ▼
┌──────────────────┐
│   TCP Socket     │  ← port 6379, persistent connections
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│    server.c      │  ← accepts connections, read/write loop
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│    parser.c      │  ← tokenises raw input → cmd / key / value
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│    storage.c     │  ← executes commands against in-memory KV store
└──────────────────┘
```

Each file has a single responsibility, making the codebase easy to extend.

---

## ⚡ Supported Commands

| Command | Description | Response |
|---|---|---|
| `PING` | Health check | `PONG` |
| `SET key value` | Store a key-value pair | `OK` |
| `SET key value EX n` | Store with TTL of *n* seconds | `OK` |
| `GET key` | Retrieve a value | value or `NULL` |
| `DEL key` | Delete a key | `OK` or `NULL` |
| `KEYS` | List all non-expired keys | newline-separated |
| `TTL key` | Seconds until expiry (`-1` = none, `-2` = absent) | integer |

---

## 🚀 Build & Run

**Requirements:** GCC, Make, POSIX-compatible OS (Linux / macOS)

```bash
# Build
make

# Run server
make run
# or
./mini-redis
```

```bash
# Connect with netcat (new terminal)
nc localhost 6379
```

---

## 💬 Example Session

```
$ nc localhost 6379

PING
PONG

SET name Nam
OK

SET city Hanoi EX 30
OK

GET name
Nam

TTL city
28

KEYS
name
city

DEL name
OK

GET name
NULL
```

---

## 🔧 Implementation Notes

- **Lazy expiration** — expired keys are evicted on access, not by a background timer. This is the same strategy used by Redis itself.

- **Buffer safety** — all string copies use `strncpy`/`strncat` with explicit null-termination to prevent buffer overflows.

- **Persistent connections** — a client can send multiple commands in one session; the connection stays open until the client disconnects.

- **`SO_REUSEADDR`** — the server socket is configured to allow immediate port reuse after restart, avoiding `Address already in use` errors during development.

---

## 📄 License

MIT © mini-redis contributors
