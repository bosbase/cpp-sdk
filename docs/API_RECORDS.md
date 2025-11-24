# API Records - C++ SDK Documentation

## Overview

The Records API provides comprehensive CRUD (Create, Read, Update, Delete) operations for collection records, along with powerful search, filtering, and authentication capabilities.

**Key Features:**
- Paginated list and search with filtering and sorting
- Single record retrieval with expand support
- Create, update, and delete operations
- Batch operations for multiple records
- Authentication methods (password, OAuth2, OTP)
- Email verification and password reset
- Relation expansion up to 6 levels deep
- Field selection and excerpt modifiers

**Backend Endpoints:**
- `GET /api/collections/{collection}/records` - List records
- `GET /api/collections/{collection}/records/{id}` - View record
- `POST /api/collections/{collection}/records` - Create record
- `PATCH /api/collections/{collection}/records/{id}` - Update record
- `DELETE /api/collections/{collection}/records/{id}` - Delete record
- `POST /api/batch` - Batch operations

## CRUD Operations

### List/Search Records

Returns a paginated records list with support for sorting, filtering, and expansion.

```cpp
#include "bosbase/bosbase.h"
using namespace bosbase;

BosBase pb("http://127.0.0.1:8090");

// Basic list with pagination
auto result = pb.collection("posts").getList(1, 50);

std::cout << "Page: " << result["page"] << std::endl;
std::cout << "Per Page: " << result["perPage"] << std::endl;
std::cout << "Total Items: " << result["totalItems"] << std::endl;
std::cout << "Total Pages: " << result["totalPages"] << std::endl;
auto items = result["items"];  // Array of records
```

#### Advanced List with Filtering and Sorting

```cpp
// Filter and sort
auto result = pb.collection("posts").getList(1, 50, false,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "created >= \"2022-01-01 00:00:00\" && status = \"published\"",
    "-created,title",  // DESC by created, ASC by title
    "author,categories"
);

// Filter with operators
auto result2 = pb.collection("posts").getList(1, 50, false,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "title ~ \"javascript\" && views > 100",
    "-views"
);
```

#### Get Full List

Fetch all records at once (useful for small collections):

```cpp
// Get all records
auto allPosts = pb.collection("posts").getFullList(500,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "status = \"published\"",
    "-created"
);
```

#### Get First Matching Record

Get only the first record that matches a filter:

```cpp
auto post = pb.collection("posts").getFirstListItem(
    "slug = \"my-post-slug\"",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "author,categories.tags"
);
```

### View Record

Retrieve a single record by ID:

```cpp
// Basic retrieval
auto record = pb.collection("posts").getOne("RECORD_ID");

// With expanded relations
auto record = pb.collection("posts").getOne("RECORD_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "author,categories,tags"
);

// Nested expand
auto record = pb.collection("comments").getOne("COMMENT_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "post.author,user"
);

// Field selection
auto record = pb.collection("posts").getOne("RECORD_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    std::nullopt,
    "id,title,content,author.name"
);
```

### Create Record

Create a new record:

```cpp
// Simple create
auto record = pb.collection("posts").create(nlohmann::json{
    {"title", "My First Post"},
    {"content", "Lorem ipsum..."},
    {"status", "draft"}
});

// Create with relations
auto record = pb.collection("posts").create(nlohmann::json{
    {"title", "My Post"},
    {"author", "AUTHOR_ID"},           // Single relation
    {"categories", nlohmann::json::array({"cat1", "cat2"})}  // Multiple relation
});

// Create with file upload
std::vector<FileAttachment> files;
files.push_back(FileAttachment{
    "image",
    "photo.jpg",
    "image/jpeg",
    imageData
});
auto record = pb.collection("posts").create(nlohmann::json{
    {"title", "My Post"}
}, std::map<std::string, nlohmann::json>{}, files);

// Create with expand to get related data immediately
auto record = pb.collection("posts").create(nlohmann::json{
    {"title", "My Post"},
    {"author", "AUTHOR_ID"}
}, std::map<std::string, nlohmann::json>{}, std::vector<FileAttachment>{},
    std::map<std::string, std::string>{}, "author");
```

### Update Record

Update an existing record:

```cpp
// Simple update
auto record = pb.collection("posts").update("RECORD_ID", nlohmann::json{
    {"title", "Updated Title"},
    {"status", "published"}
});

// Update with relations
pb.collection("posts").update("RECORD_ID", nlohmann::json{
    {"categories+", "NEW_CATEGORY_ID"},  // Append
    {"tags-", "OLD_TAG_ID"}              // Remove
});

// Update with file upload
std::vector<FileAttachment> files;
files.push_back(FileAttachment{
    "image",
    "newImage.jpg",
    "image/jpeg",
    newImageData
});
auto record = pb.collection("posts").update("RECORD_ID",
    nlohmann::json{{"title", "Updated Title"}},
    std::map<std::string, nlohmann::json>{}, files);

// Update with expand
auto record = pb.collection("posts").update("RECORD_ID",
    nlohmann::json{{"title", "Updated"}},
    std::map<std::string, nlohmann::json>{},
    std::vector<FileAttachment>{},
    std::map<std::string, std::string>{},
    "author,categories");
```

### Delete Record

Delete a record:

```cpp
// Simple delete
pb.collection("posts").remove("RECORD_ID");

// Note: Returns 204 No Content on success
// Throws error if record doesn't exist or permission denied
```

## Filter Syntax

The filter parameter supports a powerful query syntax:

### Comparison Operators

```cpp
// Equal
filter: "status = \"published\""

// Not equal
filter: "status != \"draft\""

// Greater than / Less than
filter: "views > 100"
filter: "created < \"2023-01-01\""

// Greater/Less than or equal
filter: "age >= 18"
filter: "price <= 99.99"
```

### String Operators

```cpp
// Contains (like)
filter: "title ~ \"javascript\""
// Equivalent to: title LIKE "%javascript%"

// Not contains
filter: "title !~ \"deprecated\""

// Exact match (case-sensitive)
filter: "email = \"user@example.com\""
```

### Array Operators (for multiple relations/files)

```cpp
// Any of / At least one
filter: "tags.id ?= \"TAG_ID\""         // Any tag matches
filter: "tags.name ?~ \"important\""    // Any tag name contains "important"

// All must match
filter: "tags.id = \"TAG_ID\" && tags.id = \"TAG_ID2\""
```

### Logical Operators

```cpp
// AND
filter: "status = \"published\" && views > 100"

// OR
filter: "status = \"published\" || status = \"featured\""

// Parentheses for grouping
filter: "(status = \"published\" || featured = true) && views > 50"
```

## Sorting

Sort records using the `sort` parameter:

```cpp
// Single field (ASC)
sort: "created"

// Single field (DESC)
sort: "-created"

// Multiple fields
sort: "-created,title"  // DESC by created, then ASC by title

// Supported fields
sort: "@random"         // Random order
sort: "@rowid"          // Internal row ID
sort: "id"              // Record ID
sort: "fieldName"       // Any collection field

// Relation field sorting
sort: "author.name"     // Sort by related author's name
```

## Field Selection

Control which fields are returned:

```cpp
// Specific fields
fields: "id,title,content"

// All fields at level
fields: "*"

// Nested field selection
fields: "*,author.name,author.email"

// Excerpt modifier for text fields
fields: "*,content:excerpt(200,true)"
// Returns first 200 characters with ellipsis if truncated
```

## Expanding Relations

Expand related records without additional API calls:

```cpp
// Single relation
expand: "author"

// Multiple relations
expand: "author,categories,tags"

// Nested relations (up to 6 levels)
expand: "author.profile,categories.tags"

// Back-relations
expand: "comments_via_post.user"
```

See [Relations Documentation](./RELATIONS.md) for detailed information.

## Pagination Options

```cpp
// Skip total count (faster queries)
auto result = pb.collection("posts").getList(1, 50, true,  // skip_total = true
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "status = \"published\""
);
// totalItems and totalPages will be -1

// Get Full List with batch processing
auto allPosts = pb.collection("posts").getFullList(200,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    std::nullopt,
    "-created"
);
// Processes in batches of 200 to avoid memory issues
```

## Batch Operations

Execute multiple operations in a single transaction:

```cpp
// Create a batch
auto batch = pb.createBatch();

// Add operations
batch->collection("posts").create(nlohmann::json{
    {"title", "Post 1"},
    {"author", "AUTHOR_ID"}
});

batch->collection("posts").create(nlohmann::json{
    {"title", "Post 2"},
    {"author", "AUTHOR_ID"}
});

batch->collection("tags").update("TAG_ID", nlohmann::json{
    {"name", "Updated Tag"}
});

batch->collection("categories").remove("CAT_ID");

// Upsert (create or update based on id)
batch->collection("posts").update("EXISTING_ID", nlohmann::json{
    {"id", "EXISTING_ID"},
    {"title", "Updated Post"}
});

// Send batch request
auto results = batch->send();

// Results is an array matching the order of operations
for (size_t i = 0; i < results.size(); ++i) {
    auto result = results[i];
    if (result["status"].get<int>() >= 400) {
        std::cerr << "Operation " << i << " failed: " << result["body"].dump() << std::endl;
    } else {
        std::cout << "Operation " << i << " succeeded: " << result["body"].dump() << std::endl;
    }
}
```

**Note**: Batch operations must be enabled in Dashboard > Settings > Application.

## Authentication Actions

### List Auth Methods

Get available authentication methods for a collection:

```cpp
auto methods = pb.collection("users").listAuthMethods();

std::cout << "Password enabled: " << methods["password"]["enabled"] << std::endl;
std::cout << "OAuth2 enabled: " << methods["oauth2"]["enabled"] << std::endl;
auto providers = methods["oauth2"]["providers"];  // Array of OAuth2 providers
std::cout << "OTP enabled: " << methods["otp"]["enabled"] << std::endl;
std::cout << "MFA enabled: " << methods["mfa"]["enabled"] << std::endl;
```

### Auth with Password

```cpp
auto authData = pb.collection("users").authWithPassword(
    "user@example.com",  // username or email
    "password123"
);

// Auth data is automatically stored in pb.authStore()
std::cout << "Is valid: " << pb.authStore()->isValid() << std::endl;
std::cout << "Token: " << pb.authStore()->token() << std::endl;
std::cout << "User ID: " << pb.authStore()->record()["id"] << std::endl;

// Access the returned data
std::cout << "Token: " << authData["token"] << std::endl;
std::cout << "Record: " << authData["record"].dump() << std::endl;

// With expand
auto authData2 = pb.collection("users").authWithPassword(
    "user@example.com",
    "password123",
    "profile"  // expand
);
```

### Auth with OAuth2

```cpp
// Step 1: Get OAuth2 URL (usually done in UI)
auto methods = pb.collection("users").listAuthMethods();
// Find provider in methods["oauth2"]["providers"]

// Step 2: After redirect, exchange code for token
auto authData = pb.collection("users").authWithOAuth2Code(
    "google",                    // Provider name
    "AUTHORIZATION_CODE",        // From redirect URL
    provider["codeVerifier"],    // From step 1
    "https://yourapp.com/callback", // Redirect URL
    nlohmann::json{{"name", "John Doe"}}  // Optional data for new accounts
);
```

### Auth with OTP (One-Time Password)

```cpp
// Step 1: Request OTP
auto otpRequest = pb.collection("users").requestOTP("user@example.com");
// Returns: {"otpId": "..."}

// Step 2: User enters OTP from email
// Step 3: Authenticate with OTP
auto authData = pb.collection("users").authWithOTP(
    otpRequest["otpId"].get<std::string>(),
    "123456"  // OTP from email
);
```

### Auth Refresh

Refresh the current auth token and get updated user data:

```cpp
// Refresh auth (useful on page reload)
auto authData = pb.collection("users").authRefresh();

// Check if still valid
if (pb.authStore()->isValid()) {
    std::cout << "User is authenticated" << std::endl;
} else {
    std::cout << "Token expired or invalid" << std::endl;
}
```

### Email Verification

```cpp
// Request verification email
pb.collection("users").requestVerification("user@example.com");

// Confirm verification (on verification page)
pb.collection("users").confirmVerification("VERIFICATION_TOKEN");
```

### Password Reset

```cpp
// Request password reset email
pb.collection("users").requestPasswordReset("user@example.com");

// Confirm password reset (on reset page)
// Note: This invalidates all previous auth tokens
pb.collection("users").confirmPasswordReset(
    "RESET_TOKEN",
    "newpassword123",
    "newpassword123"  // Confirm
);
```

### Email Change

```cpp
// Must be authenticated first
pb.collection("users").authWithPassword("user@example.com", "password");

// Request email change
pb.collection("users").requestEmailChange("newemail@example.com");

// Confirm email change (on confirmation page)
// Note: This invalidates all previous auth tokens
pb.collection("users").confirmEmailChange(
    "EMAIL_CHANGE_TOKEN",
    "currentpassword"
);
```

### Impersonate (Superuser Only)

Generate a token to authenticate as another user:

```cpp
// Must be authenticated as superuser
pb.collection("_superusers").authWithPassword("admin@example.com", "password");

// Impersonate a user
auto impersonateClient = pb.collection("users").impersonate("USER_ID", 3600);
// Returns a new client instance with impersonated user's token

// Use the impersonated client
auto posts = impersonateClient.collection("posts").getFullList();

// Access the token
std::cout << "Token: " << impersonateClient.authStore()->token() << std::endl;
std::cout << "Record: " << impersonateClient.authStore()->record().dump() << std::endl;
```

## Complete Examples

### Example 1: Blog Post Search with Filters

```cpp
nlohmann::json searchPosts(const std::string& query, 
                           const std::string& categoryId, 
                           int minViews) {
    std::string filter = "title ~ \"" + query + "\" || content ~ \"" + query + "\"";
    
    if (!categoryId.empty()) {
        filter += " && categories.id ?= \"" + categoryId + "\"";
    }
    
    if (minViews > 0) {
        filter += " && views >= " + std::to_string(minViews);
    }
    
    auto result = pb.collection("posts").getList(1, 20, false,
        std::map<std::string, nlohmann::json>{},
        std::map<std::string, std::string>{},
        filter,
        "-created",
        "author,categories"
    );
    
    return result["items"];
}
```

### Example 2: User Dashboard with Related Content

```cpp
nlohmann::json getUserDashboard(const std::string& userId) {
    // Get user's posts
    auto posts = pb.collection("posts").getList(1, 10, false,
        std::map<std::string, nlohmann::json>{},
        std::map<std::string, std::string>{},
        "author = \"" + userId + "\"",
        "-created",
        "categories"
    );
    
    // Get user's comments
    auto comments = pb.collection("comments").getList(1, 10, false,
        std::map<std::string, nlohmann::json>{},
        std::map<std::string, std::string>{},
        "user = \"" + userId + "\"",
        "-created",
        "post"
    );
    
    return nlohmann::json{
        {"posts", posts["items"]},
        {"comments", comments["items"]}
    };
}
```

### Example 3: Advanced Filtering

```cpp
// Complex filter example
auto result = pb.collection("posts").getList(1, 50, false,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "(status = \"published\" || featured = true) && "
    "created >= \"2023-01-01\" && "
    "(tags.id ?= \"important\" || categories.id = \"news\") && "
    "views > 100 && "
    "author.email != \"\"",
    "-views,created",
    "author.profile,tags,categories",
    "*,content:excerpt(300),author.name,author.email"
);
```

### Example 4: Batch Create Posts

```cpp
std::vector<nlohmann::json> createMultiplePosts(
    const std::vector<nlohmann::json>& postsData) {
    auto batch = pb.createBatch();
    
    for (const auto& postData : postsData) {
        batch->collection("posts").create(postData);
    }
    
    auto results = batch->send();
    
    // Check for failures
    std::vector<nlohmann::json> failures;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i]["status"].get<int>() >= 400) {
            failures.push_back(nlohmann::json{
                {"index", i},
                {"result", results[i]}
            });
        }
    }
    
    if (!failures.empty()) {
        std::cerr << "Some posts failed to create" << std::endl;
    }
    
    std::vector<nlohmann::json> created;
    for (const auto& r : results) {
        created.push_back(r["body"]);
    }
    return created;
}
```

## Error Handling

```cpp
#include "bosbase/error.h"
#include <stdexcept>

try {
    auto record = pb.collection("posts").create(nlohmann::json{
        {"title", "My Post"}
    });
} catch (const ClientResponseError& error) {
    if (error.status() == 400) {
        // Validation error
        std::cerr << "Validation errors: " << error.data().dump() << std::endl;
    } else if (error.status() == 403) {
        // Permission denied
        std::cerr << "Access denied" << std::endl;
    } else if (error.status() == 404) {
        // Not found
        std::cerr << "Collection or record not found" << std::endl;
    } else {
        std::cerr << "Unexpected error: " << error.what() << std::endl;
    }
}
```

## Best Practices

1. **Use Pagination**: Always use pagination for large datasets
2. **Skip Total When Possible**: Use `skip_total = true` for better performance when you don't need counts
3. **Batch Operations**: Use batch for multiple operations to reduce round trips
4. **Field Selection**: Only request fields you need to reduce payload size
5. **Expand Wisely**: Only expand relations you actually use
6. **Filter Before Sort**: Apply filters before sorting for better performance
7. **Cache Auth Tokens**: Auth tokens are automatically stored in `authStore`, no need to manually cache
8. **Handle Errors**: Always handle authentication and permission errors gracefully

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection configuration
- [Relations](./RELATIONS.md) - Working with relations
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Filter syntax details
- [Authentication](./AUTHENTICATION.md) - Detailed authentication guide
- [Files](./FILES.md) - File uploads and handling

