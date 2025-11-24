# Working with Relations - C++ SDK Documentation

## Overview

Relations allow you to link records between collections. BosBase supports both single and multiple relations, and provides powerful features for expanding related records and working with back-relations.

**Key Features:**
- Single and multiple relations
- Expand related records without additional requests
- Nested relation expansion (up to 6 levels)
- Back-relations for reverse lookups
- Field modifiers for append/prepend/remove operations

**Relation Field Types:**
- **Single Relation**: Links to one record (MaxSelect <= 1)
- **Multiple Relation**: Links to multiple records (MaxSelect > 1)

**Backend Behavior:**
- Relations are stored as record IDs or arrays of IDs
- Expand only includes relations the client can view (satisfies View API Rule)
- Back-relations use format: `collectionName_via_fieldName`
- Back-relation expand limited to 1000 records per field

## Setting Up Relations

### Creating a Relation Field

```cpp
auto collection = pb.collections->getOne("posts");

auto fields = collection["fields"];
fields.push_back(nlohmann::json{
    {"name", "user"},
    {"type", "relation"},
    {"options", nlohmann::json{{"collectionId", "users"}}},
    {"maxSelect", 1},           // Single relation
    {"required", true}
});

// Multiple relation field
fields.push_back(nlohmann::json{
    {"name", "tags"},
    {"type", "relation"},
    {"options", nlohmann::json{{"collectionId", "tags"}}},
    {"maxSelect", 10},          // Multiple relation (max 10)
    {"minSelect", 1},           // Minimum 1 required
    {"cascadeDelete", false}    // Don't delete post when tags deleted
});

pb.collections->update("posts", nlohmann::json{{"fields", fields}});
```

## Creating Records with Relations

### Single Relation

```cpp
// Create a post with a single user relation
auto post = pb.collection("posts").create(nlohmann::json{
    {"title", "My Post"},
    {"user", "USER_ID"}  // Single relation ID
});
```

### Multiple Relations

```cpp
// Create a post with multiple tags
auto post = pb.collection("posts").create(nlohmann::json{
    {"title", "My Post"},
    {"tags", nlohmann::json::array({"TAG_ID1", "TAG_ID2", "TAG_ID3"})}  // Array of IDs
});
```

### Mixed Relations

```cpp
// Create a comment with both single and multiple relations
auto comment = pb.collection("comments").create(nlohmann::json{
    {"message", "Great post!"},
    {"post", "POST_ID"},        // Single relation
    {"user", "USER_ID"},        // Single relation
    {"tags", nlohmann::json::array({"TAG1", "TAG2"})}  // Multiple relation
});
```

## Updating Relations

### Replace All Relations

```cpp
// Replace all tags
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"tags", nlohmann::json::array({"NEW_TAG1", "NEW_TAG2"})}
});
```

### Append Relations (Using + Modifier)

```cpp
// Append tags to existing ones
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"tags+", "NEW_TAG_ID"}  // Append single tag
});

// Append multiple tags
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"tags+", nlohmann::json::array({"TAG_ID1", "TAG_ID2"})}  // Append multiple tags
});
```

### Prepend Relations (Using + Prefix)

```cpp
// Prepend tags (tags will appear first)
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"+tags", "PRIORITY_TAG"}  // Prepend single tag
});

// Prepend multiple tags
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"+tags", nlohmann::json::array({"TAG1", "TAG2"})}  // Prepend multiple tags
});
```

### Remove Relations (Using - Modifier)

```cpp
// Remove single tag
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"tags-", "TAG_ID_TO_REMOVE"}
});

// Remove multiple tags
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"tags-", nlohmann::json::array({"TAG1", "TAG2"})}
});
```

### Complete Example

```cpp
// Get existing post
auto post = pb.collection("posts").getOne("POST_ID");
std::cout << "Tags: " << post["tags"].dump() << std::endl;  // ["tag1", "tag2"]

// Remove one tag, add two new ones
pb.collection("posts").update("POST_ID", nlohmann::json{
    {"tags-", "tag1"},           // Remove
    {"tags+", nlohmann::json::array({"tag3", "tag4"})}  // Append
});

auto updated = pb.collection("posts").getOne("POST_ID");
std::cout << "Updated tags: " << updated["tags"].dump() << std::endl;  // ["tag2", "tag3", "tag4"]
```

## Expanding Relations

The `expand` parameter allows you to fetch related records in a single request, eliminating the need for multiple API calls.

### Basic Expand

```cpp
// Get comment with expanded user
auto comment = pb.collection("comments").getOne("COMMENT_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "user"
);

std::cout << "User name: " << comment["expand"]["user"]["name"] << std::endl;
std::cout << "User ID: " << comment["user"] << std::endl;  // Still the ID: "USER_ID"
```

### Expand Multiple Relations

```cpp
// Expand multiple relations (comma-separated)
auto comment = pb.collection("comments").getOne("COMMENT_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "user,post"
);

std::cout << "User: " << comment["expand"]["user"]["name"] << std::endl;
std::cout << "Post: " << comment["expand"]["post"]["title"] << std::endl;
```

### Nested Expand (Dot Notation)

You can expand nested relations up to 6 levels deep using dot notation:

```cpp
// Expand post and its tags, and user
auto comment = pb.collection("comments").getOne("COMMENT_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "user,post.tags"
);

// Access nested expands
auto tags = comment["expand"]["post"]["expand"]["tags"];
// Array of tag records

// Expand even deeper
auto post = pb.collection("posts").getOne("POST_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "user,comments.user"
);

// Access: post["expand"]["comments"][0]["expand"]["user"]
```

### Expand with List Requests

```cpp
// List comments with expanded users
auto comments = pb.collection("comments").getList(1, 20, false,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    std::nullopt,
    std::nullopt,
    "user"
);

for (const auto& comment : comments["items"]) {
    std::cout << "Message: " << comment["message"] << std::endl;
    std::cout << "User: " << comment["expand"]["user"]["name"] << std::endl;
}
```

### Expand Single vs Multiple Relations

```cpp
// Single relation - expand.user is an object
auto post = pb.collection("posts").getOne("POST_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "user"
);
// post["expand"]["user"] is an object

// Multiple relation - expand.tags is an array
auto postWithTags = pb.collection("posts").getOne("POST_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "tags"
);
// postWithTags["expand"]["tags"] is an array
```

### Expand Permissions

**Important**: Only relations that satisfy the related collection's `viewRule` will be expanded. If you don't have permission to view a related record, it won't appear in the expand.

```cpp
// If you don't have view permission for user, expand.user will be undefined
auto comment = pb.collection("comments").getOne("COMMENT_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "user"
);

if (comment["expand"].contains("user")) {
    std::cout << "User name: " << comment["expand"]["user"]["name"] << std::endl;
} else {
    std::cout << "User not accessible or not found" << std::endl;
}
```

## Back-Relations

Back-relations allow you to query and expand records that reference the current record through a relation field.

### Back-Relation Syntax

The format is: `collectionName_via_fieldName`

- `collectionName`: The collection that contains the relation field
- `fieldName`: The name of the relation field that points to your record

### Example: Posts with Comments

```cpp
// Get a post and expand all comments that reference it
auto post = pb.collection("posts").getOne("POST_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "comments_via_post"
);

// comments_via_post is always an array (even if original field is single)
auto comments = post["expand"]["comments_via_post"];
// Array of comment records
```

### Back-Relation with Nested Expand

```cpp
// Get post with comments, and expand each comment's user
auto post = pb.collection("posts").getOne("POST_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "comments_via_post.user"
);

// Access nested expands
for (const auto& comment : post["expand"]["comments_via_post"]) {
    std::cout << "Message: " << comment["message"] << std::endl;
    std::cout << "User: " << comment["expand"]["user"]["name"] << std::endl;
}
```

### Filtering with Back-Relations

```cpp
// List posts that have at least one comment containing "hello"
auto posts = pb.collection("posts").getList(1, 20, false,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "comments_via_post.message ?~ 'hello'",
    std::nullopt,
    "comments_via_post.user"
);

for (const auto& post : posts["items"]) {
    std::cout << "Post: " << post["title"] << std::endl;
    for (const auto& comment : post["expand"]["comments_via_post"]) {
        std::cout << "  - " << comment["message"] 
                  << " by " << comment["expand"]["user"]["name"] << std::endl;
    }
}
```

### Back-Relation Caveats

1. **Always Multiple**: Back-relations are always treated as arrays, even if the original relation field is single. This is because one record can be referenced by multiple records.

   ```cpp
   // Even if comments.post is single, comments_via_post is always an array
   auto post = pb.collection("posts").getOne("POST_ID",
       std::map<std::string, nlohmann::json>{},
       std::map<std::string, std::string>{},
       "comments_via_post"
   );
   
   // Always an array
   auto comments = post["expand"]["comments_via_post"];
   // comments.is_array() == true
   ```

2. **1000 Record Limit**: Back-relation expand is limited to 1000 records per field. For larger datasets, use separate paginated requests:

   ```cpp
   // Instead of expanding all comments (if > 1000)
   auto post = pb.collection("posts").getOne("POST_ID");
   
   // Fetch comments separately with pagination
   auto comments = pb.collection("comments").getList(1, 100, false,
       std::map<std::string, nlohmann::json>{},
       std::map<std::string, std::string>{},
       "post = \"" + post["id"].get<std::string>() + "\"",
       "-created",
       "user"
   );
   ```

## Complete Examples

### Example 1: Blog Post with Author and Tags

```cpp
// Create a blog post with relations
auto post = pb.collection("posts").create(nlohmann::json{
    {"title", "Getting Started with BosBase"},
    {"content", "Lorem ipsum..."},
    {"author", "AUTHOR_ID"},           // Single relation
    {"tags", nlohmann::json::array({"tag1", "tag2", "tag3"})} // Multiple relation
});

// Retrieve with all relations expanded
auto fullPost = pb.collection("posts").getOne(post["id"],
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "author,tags"
);

std::cout << "Title: " << fullPost["title"] << std::endl;
std::cout << "Author: " << fullPost["expand"]["author"]["name"] << std::endl;
std::cout << "Tags:" << std::endl;
for (const auto& tag : fullPost["expand"]["tags"]) {
    std::cout << "  - " << tag["name"] << std::endl;
}
```

### Example 2: Comment System with Nested Relations

```cpp
// Create a comment on a post
auto comment = pb.collection("comments").create(nlohmann::json{
    {"message", "Great article!"},
    {"post", "POST_ID"},
    {"user", "USER_ID"}
});

// Get post with all comments and their authors
auto post = pb.collection("posts").getOne("POST_ID",
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "author,comments_via_post.user"
);

std::cout << "Post: " << post["title"] << std::endl;
std::cout << "Author: " << post["expand"]["author"]["name"] << std::endl;
std::cout << "Comments (" << post["expand"]["comments_via_post"].size() << "):" << std::endl;
for (const auto& comment : post["expand"]["comments_via_post"]) {
    std::cout << "  " << comment["expand"]["user"]["name"] 
              << ": " << comment["message"] << std::endl;
}
```

### Example 3: Dynamic Tag Management

```cpp
class PostManager {
    BosBase& pb_;
    
public:
    PostManager(BosBase& pb) : pb_(pb) {}
    
    void addTag(const std::string& postId, const std::string& tagId) {
        pb_.collection("posts").update(postId, nlohmann::json{
            {"tags+", tagId}
        });
    }
    
    void removeTag(const std::string& postId, const std::string& tagId) {
        pb_.collection("posts").update(postId, nlohmann::json{
            {"tags-", tagId}
        });
    }
    
    nlohmann::json getPostWithTags(const std::string& postId) {
        return pb_.collection("posts").getOne(postId,
            std::map<std::string, nlohmann::json>{},
            std::map<std::string, std::string>{},
            "tags"
        );
    }
};

// Usage
PostManager manager(pb);
manager.addTag("POST_ID", "NEW_TAG_ID");
auto post = manager.getPostWithTags("POST_ID");
```

### Example 4: Filtering Posts by Tag

```cpp
// Get all posts with a specific tag
auto posts = pb.collection("posts").getList(1, 50, false,
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "tags.id ?= \"TAG_ID\"",
    "-created",
    "author,tags"
);

for (const auto& post : posts["items"]) {
    std::cout << post["title"] << " by " 
              << post["expand"]["author"]["name"] << std::endl;
}
```

## Best Practices

1. **Use Expand Wisely**: Only expand relations you actually need to reduce response size and improve performance.

2. **Handle Missing Expands**: Always check if expand data exists before accessing:

   ```cpp
   if (record["expand"].contains("user")) {
       std::cout << record["expand"]["user"]["name"] << std::endl;
   }
   ```

3. **Pagination for Large Back-Relations**: If you expect more than 1000 related records, fetch them separately with pagination.

4. **Cache Expansion**: Consider caching expanded data on the client side to reduce API calls.

5. **Error Handling**: Handle cases where related records might not be accessible due to API rules.

6. **Nested Limit**: Remember that nested expands are limited to 6 levels deep.

## Performance Considerations

- **Expand Cost**: Expanding relations doesn't require additional round trips, but increases response payload size
- **Back-Relation Limit**: The 1000 record limit for back-relations prevents extremely large responses
- **Permission Checks**: Each expanded relation is checked against the collection's `viewRule`
- **Nested Depth**: Limit nested expands to avoid performance issues (max 6 levels supported)

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection and field configuration
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Filtering and querying related records

