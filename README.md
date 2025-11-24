BosBase C++ SDK
================

Modern C++17 SDK for interacting with the BosBase HTTP API. The surface mirrors the JavaScript SDK: CRUD helpers, auth flows, realtime SSE, pub/sub, vector, LangChaingo, LLM documents, cache, backups, crons, and settings management.

## Build

Dependencies:
- CMake 3.16+
- C++17 compiler
- libcurl with SSL
- Boost (system, thread, Asio headers)
- OpenSSL

```bash
cd cpp-sdk
cmake -S . -B build
cmake --build build
```

The build produces a static library `libbosbase.a` plus headers under `include/`.

## Quick start

```cpp
#include "bosbase/bosbase.h"
using namespace bosbase;

int main() {
    BosBase pb("http://127.0.0.1:8090");

    // Authenticate
    pb.collection("_superusers").authWithPassword("admin@example.com", "password");

    // Create record
    auto post = pb.collection("articles").create({{"title", "Hello BosBase"}});

    // List
    auto list = pb.collection("articles").getList(1, 20);

    // Realtime
    auto unsubscribe = pb.collection("articles").subscribe("*", [](const nlohmann::json& evt) {
        std::cout << "Realtime change: " << evt.dump() << std::endl;
    });
    // ...
    unsubscribe();
}
```

## Services

- `collection(id)->RecordService`: CRUD, auth (password, OTP, OAuth2), realtime subscribe/unsubscribe, impersonation
- `collections`: Collection management, scaffolds, fields, indexes, rules, truncate/import
- `files`: File URLs and auth token helpers
- `logs`, `settings`, `health`, `backups`, `crons`
- `caches`: Cache definitions and entries
- `vectors`: Vector collections, insert/update/delete, batch insert, search
- `langchaingo`, `llmDocuments`, `graphql`
- `pubsub`: WebSocket publish/subscribe
- `realtime`: SSE-based realtime topics (used by `RecordService::subscribe`)
- `createBatch()`: queue multi-collection CRUD requests transactionally

Hooks:
- `pb.beforeSend(url, options)` to mutate outgoing requests
- `pb.afterSend(status, headers, data)` to normalize responses

## Notes

- Multipart uploads use `FileAttachment` (field, filename, contentType, data bytes).
- SSE and WebSocket support require network access; the client auto-reconnects when possible.
- Auth tokens are kept in `pb.authStore()` and validated before each request.
