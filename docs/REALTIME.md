# Realtime API - C++ SDK Documentation

## Overview

The Realtime API enables real-time updates for collection records using **Server-Sent Events (SSE)**. It allows you to subscribe to changes in collections or specific records and receive instant notifications when records are created, updated, or deleted.

**Key Features:**
- Real-time notifications for record changes
- Collection-level and record-level subscriptions
- Automatic connection management and reconnection
- Authorization support
- Subscription options (expand, custom headers, query params)
- Event-driven architecture

**Backend Endpoints:**
- `GET /api/realtime` - Establish SSE connection
- `POST /api/realtime` - Set subscriptions

## How It Works

1. **Connection**: The SDK establishes an SSE connection to `/api/realtime`
2. **Client ID**: Server sends `PB_CONNECT` event with a unique `clientId`
3. **Subscriptions**: Client submits subscription topics via POST request
4. **Events**: Server sends events when matching records change
5. **Reconnection**: SDK automatically reconnects on connection loss

## Basic Usage

### Subscribe to Collection Changes

Subscribe to all changes in a collection:

```cpp
#include "bosbase/bosbase.h"
#include <iostream>

int main() {
    bosbase::BosBase pb("http://127.0.0.1:8090");

    // Subscribe to all changes in the 'posts' collection
    auto unsubscribe = pb.collection("posts").subscribe("*", [](const nlohmann::json& e) {
        std::cout << "Action: " << e["action"] << std::endl;  // 'create', 'update', or 'delete'
        std::cout << "Record: " << e["record"].dump(2) << std::endl;  // The record data
    });

    // Later, unsubscribe
    unsubscribe();

    return 0;
}
```

### Subscribe to Specific Record

Subscribe to changes for a single record:

```cpp
// Subscribe to changes for a specific post
pb.collection("posts").subscribe("RECORD_ID", [](const nlohmann::json& e) {
    std::cout << "Record changed: " << e["record"].dump(2) << std::endl;
    std::cout << "Action: " << e["action"] << std::endl;
});
```

### Multiple Subscriptions

You can subscribe multiple times to the same or different topics:

```cpp
auto handleChange = [](const nlohmann::json& e) {
    std::cout << "Change event: " << e.dump(2) << std::endl;
};

auto handleAllChanges = [](const nlohmann::json& e) {
    std::cout << "Collection-wide change: " << e.dump(2) << std::endl;
};

// Subscribe to multiple records
auto unsubscribe1 = pb.collection("posts").subscribe("RECORD_ID_1", handleChange);
auto unsubscribe2 = pb.collection("posts").subscribe("RECORD_ID_2", handleChange);
auto unsubscribe3 = pb.collection("posts").subscribe("*", handleAllChanges);

// Unsubscribe individually
unsubscribe1();
unsubscribe2();
unsubscribe3();
```

## Event Structure

Each event received contains:

```cpp
{
  "action": "create" | "update" | "delete",  // Action type
  "record": {                                 // Record data
    "id": "RECORD_ID",
    "collectionId": "COLLECTION_ID",
    "collectionName": "collection_name",
    "created": "2023-01-01 00:00:00.000Z",
    "updated": "2023-01-01 00:00:00.000Z",
    // ... other fields
  }
}
```

### PB_CONNECT Event

When the connection is established, you receive a `PB_CONNECT` event. You can subscribe to this using the realtime service directly:

```cpp
// Note: PB_CONNECT subscription may need to be handled via realtime service
// Check SDK implementation for exact method
```

## Subscription Topics

### Collection-Level Subscription

Subscribe to all changes in a collection:

```cpp
// Wildcard subscription - all records in collection
pb.collection("posts").subscribe("*", handler);
```

**Access Control**: Uses the collection's `ListRule` to determine if the subscriber has access to receive events.

### Record-Level Subscription

Subscribe to changes for a specific record:

```cpp
// Specific record subscription
pb.collection("posts").subscribe("RECORD_ID", handler);
```

**Access Control**: Uses the collection's `ViewRule` to determine if the subscriber has access to receive events.

## Subscription Options

You can pass additional options when subscribing:

```cpp
std::map<std::string, nlohmann::json> query = {
    {"filter", "status = \"published\""},
    {"expand", "author"}
};

std::map<std::string, std::string> headers = {
    {"X-Custom-Header", "value"}
};

pb.collection("posts").subscribe("*", handler, query, headers);
```

### Expand Relations

Expand relations in the event data:

```cpp
std::map<std::string, nlohmann::json> query = {
    {"expand", "author,categories"}
};

pb.collection("posts").subscribe("RECORD_ID", [](const nlohmann::json& e) {
    if (e["record"].contains("expand") && e["record"]["expand"].contains("author")) {
        std::cout << "Author: " << e["record"]["expand"]["author"]["name"] << std::endl;
    }
}, query);
```

### Filter with Query Parameters

Use query parameters for API rule filtering:

```cpp
std::map<std::string, nlohmann::json> query = {
    {"filter", "status = \"published\""}
};

pb.collection("posts").subscribe("*", handler, query);
```

## Unsubscribing

### Unsubscribe from Specific Topic

```cpp
// Remove all subscriptions for a specific record
pb.collection("posts").unsubscribe("RECORD_ID");

// Remove all wildcard subscriptions for the collection
pb.collection("posts").unsubscribe("*");
```

### Unsubscribe from All

```cpp
// Unsubscribe from all subscriptions in the collection
pb.collection("posts").unsubscribe();

// Note: Check SDK for global unsubscribe method if available
```

### Unsubscribe Using Returned Function

```cpp
auto unsubscribe = pb.collection("posts").subscribe("*", handler);

// Later...
unsubscribe();  // Removes this specific subscription
```

## Connection Management

### Connection Status

Check if the realtime connection is established (implementation may vary):

```cpp
// Note: Check SDK implementation for connection status methods
// This may be available through a realtime service or connection manager
```

### Automatic Reconnection

The SDK automatically:
- Reconnects when the connection is lost
- Resubmits all active subscriptions
- Handles network interruptions gracefully
- Closes connection after 5 minutes of inactivity (server-side timeout)

## Authorization

### Authenticated Subscriptions

Subscriptions respect authentication. If you're authenticated, events are filtered based on your permissions:

```cpp
// Authenticate first
pb.collection("users").authWithPassword("user@example.com", "password");

// Now subscribe - events will respect your permissions
pb.collection("posts").subscribe("*", handler);
```

### Authorization Rules

- **Collection-level (`*`)**: Uses `ListRule` to determine access
- **Record-level**: Uses `ViewRule` to determine access
- **Superusers**: Can receive all events (if rules allow)
- **Guests**: Only receive events they have permission to see

### Auth State Changes

When authentication state changes, you may need to resubscribe:

```cpp
// After login/logout, resubscribe to update permissions
pb.collection("users").authWithPassword("user@example.com", "password");

// Re-subscribe to update auth state in realtime connection
pb.collection("posts").subscribe("*", handler);
```

## Advanced Examples

### Example 1: Real-time Chat

```cpp
#include "bosbase/bosbase.h"
#include <iostream>
#include <functional>

std::function<void()> setupChatRoom(bosbase::BosBase& pb, const std::string& roomId) {
    std::map<std::string, nlohmann::json> query = {
        {"filter", "roomId = \"" + roomId + "\""}
    };

    auto unsubscribe = pb.collection("messages").subscribe("*", 
        [&pb, roomId](const nlohmann::json& e) {
            // Filter for this room only
            if (e["record"]["roomId"] == roomId) {
                std::string action = e["action"];
                if (action == "create") {
                    displayMessage(e["record"]);
                } else if (action == "delete") {
                    removeMessage(e["record"]["id"].get<std::string>());
                }
            }
        }, query);

    return unsubscribe;
}

// Usage
auto unsubscribeChat = setupChatRoom(pb, "ROOM_ID");

// Cleanup
unsubscribeChat();
```

### Example 2: Real-time Dashboard

```cpp
void setupDashboard(bosbase::BosBase& pb) {
    // Posts updates
    std::map<std::string, nlohmann::json> postsQuery = {
        {"filter", "status = \"published\""},
        {"expand", "author"}
    };
    
    pb.collection("posts").subscribe("*", [](const nlohmann::json& e) {
        std::string action = e["action"];
        if (action == "create") {
            addPostToFeed(e["record"]);
        } else if (action == "update") {
            updatePostInFeed(e["record"]);
        }
    }, postsQuery);

    // Comments updates
    std::map<std::string, nlohmann::json> commentsQuery = {
        {"expand", "user"}
    };
    
    pb.collection("comments").subscribe("*", [](const nlohmann::json& e) {
        updateCommentsCount(e["record"]["postId"].get<std::string>());
    }, commentsQuery);
}
```

### Example 3: User Activity Tracking

```cpp
void trackUserActivity(bosbase::BosBase& pb, const std::string& userId) {
    std::map<std::string, nlohmann::json> query = {
        {"filter", "author = \"" + userId + "\""}
    };

    pb.collection("posts").subscribe("*", [userId](const nlohmann::json& e) {
        // Only track changes to user's own posts
        if (e["record"]["author"] == userId) {
            std::string action = e["action"];
            std::cout << "Your post " << action << ": " 
                      << e["record"]["title"] << std::endl;
            
            if (action == "update") {
                showNotification("Post updated");
            }
        }
    }, query);
}

// Usage
pb.collection("users").authWithPassword("user@example.com", "password");
auto user = pb.authStore()->record();
trackUserActivity(pb, user["id"].get<std::string>());
```

### Example 4: Real-time Collaboration

```cpp
void trackDocumentEdits(bosbase::BosBase& pb, const std::string& documentId) {
    std::map<std::string, nlohmann::json> query = {
        {"expand", "lastEditor"}
    };

    pb.collection("documents").subscribe(documentId, [](const nlohmann::json& e) {
        if (e["action"] == "update") {
            auto lastEditor = e["record"]["lastEditor"];
            auto updatedAt = e["record"]["updated"];
            
            // Show who last edited the document
            showEditorIndicator(lastEditor, updatedAt);
        }
    }, query);
}
```

## Error Handling

```cpp
#include "bosbase/error.h"

try {
    pb.collection("posts").subscribe("*", handler);
} catch (const ClientResponseError& err) {
    if (err.status() == 403) {
        std::cerr << "Permission denied" << std::endl;
    } else if (err.status() == 404) {
        std::cerr << "Collection not found" << std::endl;
    } else {
        std::cerr << "Subscription error: " << err.what() << std::endl;
    }
}
```

## Best Practices

1. **Unsubscribe When Done**: Always unsubscribe when components are destroyed or subscriptions are no longer needed
2. **Handle Disconnections**: Implement error handling for connection issues
3. **Filter Server-Side**: Use query parameters to filter events server-side when possible
4. **Limit Subscriptions**: Don't subscribe to more collections than necessary
5. **Use Record-Level When Possible**: Prefer record-level subscriptions over collection-level when you only need specific records
6. **Monitor Connection**: Track connection state for debugging and user feedback
7. **Handle Errors**: Wrap subscriptions in try-catch blocks
8. **Respect Permissions**: Understand that events respect API rules and permissions

## Limitations

- **Maximum Subscriptions**: Up to 1000 subscriptions per client
- **Topic Length**: Maximum 2500 characters per topic
- **Idle Timeout**: Connection closes after 5 minutes of inactivity
- **Network Dependency**: Requires stable network connection
- **Platform Support**: SSE requires modern platforms with HTTP client support

## Troubleshooting

### Connection Not Establishing

```cpp
// Check if subscription is working
try {
    auto unsubscribe = pb.collection("posts").subscribe("*", handler);
    std::cout << "Subscription successful" << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Subscription failed: " << e.what() << std::endl;
}
```

### Events Not Received

1. Check API rules - you may not have permission
2. Verify subscription is active
3. Check network connectivity
4. Review server logs for errors

### Memory Leaks

Always unsubscribe:

```cpp
// Good
auto unsubscribe = pb.collection("posts").subscribe("*", handler);
// ... later
unsubscribe();

// Bad - no cleanup
pb.collection("posts").subscribe("*", handler);
// Never unsubscribed - potential memory leak!
```

## Related Documentation

- [API Records](./API_RECORDS.md) - CRUD operations
- [Collections](./COLLECTIONS.md) - Collection configuration
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Understanding API rules

