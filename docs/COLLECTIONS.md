# Collections - C++ SDK Documentation

## Overview

**Collections** represent your application data. Under the hood they are backed by plain SQLite tables that are generated automatically with the collection **name** and **fields** (columns).

A single entry of a collection is called a **record** (a single row in the SQL table).

## Collection Types

### Base Collection

Default collection type for storing any application data.

```cpp
#include "bosbase/bosbase.h"
using namespace bosbase;

BosBase pb("http://localhost:8090");
pb.collection("_superusers").authWithPassword("admin@example.com", "password");

auto collection = pb.collections->createBase("articles", nlohmann::json{
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}},
        nlohmann::json{{"name", "description"}, {"type", "text"}}
    })}
});
```

### View Collection

Read-only collection populated from a SQL SELECT statement.

```cpp
auto view = pb.collections->createView("post_stats", 
    "SELECT posts.id, posts.name, count(comments.id) as totalComments "
    "FROM posts LEFT JOIN comments on comments.postId = posts.id "
    "GROUP BY posts.id"
);
```

### Auth Collection

Base collection with authentication fields (email, password, etc.).

```cpp
auto users = pb.collections->createAuth("users", nlohmann::json{
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "name"}, {"type", "text"}, {"required", true}}
    })}
});
```

## Collections API

### List Collections

```cpp
auto result = pb.collections->getList(1, 50);
auto all = pb.collections->getFullList();
```

### Get Collection

```cpp
auto collection = pb.collections->getOne("articles");
```

### Create Collection

```cpp
// Using scaffolds
auto base = pb.collections->createBase("articles");
auto auth = pb.collections->createAuth("users");
auto view = pb.collections->createView("stats", "SELECT * FROM posts");

// Manual
auto collection = pb.collections->create(nlohmann::json{
    {"type", "base"},
    {"name", "articles"},
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}},
        nlohmann::json{
            {"name", "created"},
            {"type", "autodate"},
            {"required", false},
            {"onCreate", true},
            {"onUpdate", false}
        },
        nlohmann::json{
            {"name", "updated"},
            {"type", "autodate"},
            {"required", false},
            {"onCreate", true},
            {"onUpdate", true}
        }
    })}
});
```

### Update Collection

```cpp
// Update collection rules
pb.collections->update("articles", nlohmann::json{
    {"listRule", "published = true"}
});

// Update collection name
pb.collections->update("articles", nlohmann::json{
    {"name", "posts"}
});
```

### Add Fields to Collection

To add a new field to an existing collection, fetch the collection, add the field to the fields array, and update:

```cpp
// Get existing collection
auto collection = pb.collections->getOne("articles");

// Add new field to existing fields
auto fields = collection["fields"];
fields.push_back(nlohmann::json{
    {"name", "views"},
    {"type", "number"},
    {"min", 0},
    {"onlyInt", true}
});

// Update collection with new field
pb.collections->update("articles", nlohmann::json{
    {"fields", fields}
});

// Or add multiple fields at once
fields.push_back(nlohmann::json{
    {"name", "excerpt"},
    {"type", "text"},
    {"max", 500}
});
fields.push_back(nlohmann::json{
    {"name", "cover"},
    {"type", "file"},
    {"maxSelect", 1},
    {"mimeTypes", nlohmann::json::array({"image/jpeg", "image/png"})}
});

pb.collections->update("articles", nlohmann::json{
    {"fields", fields}
});
```

### Delete Fields from Collection

To delete a field, fetch the collection, remove the field from the fields array, and update:

```cpp
// Get existing collection
auto collection = pb.collections->getOne("articles");

// Remove field by filtering it out
auto fields = collection["fields"];
auto newFields = nlohmann::json::array();
for (const auto& field : fields) {
    if (field["name"] != "oldFieldName") {
        newFields.push_back(field);
    }
}

// Update collection without the deleted field
pb.collections->update("articles", nlohmann::json{
    {"fields", newFields}
});
```

### Modify Fields in Collection

To modify an existing field, fetch the collection, update the field object, and save:

```cpp
// Get existing collection
auto collection = pb.collections->getOne("articles");

// Find and modify a field
auto fields = collection["fields"];
for (auto& field : fields) {
    if (field["name"] == "title") {
        field["max"] = 200;  // Change max length
        field["required"] = true;  // Make required
    }
    
    if (field["name"] == "status") {
        // Note: Changing field types may require data migration
        field["type"] = "select";
        field["options"] = nlohmann::json{
            {"values", nlohmann::json::array({"draft", "published", "archived"})}
        };
        field["maxSelect"] = 1;
    }
}

// Save changes
pb.collections->update("articles", nlohmann::json{
    {"fields", fields}
});
```

### Delete Collection

```cpp
pb.collections->deleteCollection("articles");
```

## Records API

### List Records

```cpp
auto result = pb.collection("articles").getList(1, 20, false, 
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "published = true",
    "-created",
    "author"
);
```

### Get Record

```cpp
auto record = pb.collection("articles").getOne("RECORD_ID", 
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "author,category"
);
```

### Create Record

```cpp
auto record = pb.collection("articles").create(nlohmann::json{
    {"title", "My Article"},
    {"views+", 1}  // Field modifier
});
```

### Update Record

```cpp
pb.collection("articles").update("RECORD_ID", nlohmann::json{
    {"title", "Updated"},
    {"views+", 1},
    {"tags+", "new-tag"}
});
```

### Delete Record

```cpp
pb.collection("articles").remove("RECORD_ID");
```

## Field Types

### BoolField

```cpp
// Field definition
nlohmann::json{{"name", "published"}, {"type", "bool"}, {"required", true}}

// Usage
pb.collection("articles").create(nlohmann::json{
    {"published", true}
});
```

### NumberField

```cpp
// Field definition
nlohmann::json{{"name", "views"}, {"type", "number"}, {"min", 0}}

// Usage with modifier
pb.collection("articles").update("ID", nlohmann::json{
    {"views+", 1}
});
```

### TextField

```cpp
// Field definition
nlohmann::json{
    {"name", "title"},
    {"type", "text"},
    {"required", true},
    {"min", 6},
    {"max", 100}
}

// Usage with autogenerate
pb.collection("articles").create(nlohmann::json{
    {"slug:autogenerate", "article-"}
});
```

### EmailField

```cpp
// Field definition
nlohmann::json{{"name", "email"}, {"type", "email"}, {"required", true}}
```

### URLField

```cpp
// Field definition
nlohmann::json{{"name", "website"}, {"type", "url"}}
```

### EditorField

```cpp
// Field definition
nlohmann::json{{"name", "content"}, {"type", "editor"}, {"required", true}}

// Usage
pb.collection("articles").create(nlohmann::json{
    {"content", "<p>HTML content</p>"}
});
```

### DateField

```cpp
// Field definition
nlohmann::json{{"name", "published_at"}, {"type", "date"}}

// Usage
pb.collection("articles").create(nlohmann::json{
    {"published_at", "2024-11-10 18:45:27.123Z"}
});
```

### AutodateField

**Important Note:** Bosbase does not initialize `created` and `updated` fields by default. To use these fields, you must explicitly add them when initializing the collection. For autodate fields, `onCreate` and `onUpdate` must be direct properties, not nested in options:

```cpp
// Create field with proper structure
nlohmann::json{
    {"name", "created"},
    {"type", "autodate"},
    {"required", false},
    {"onCreate", true},  // Set on record creation (direct property)
    {"onUpdate", false}  // Don't update on record update (direct property)
}

// For updated field
nlohmann::json{
    {"name", "updated"},
    {"type", "autodate"},
    {"required", false},
    {"onCreate", true},  // Set on record creation (direct property)
    {"onUpdate", true}   // Update on record update (direct property)
}

// The value is automatically set by the backend based on onCreate and onUpdate properties
```

### SelectField

```cpp
// Single select
nlohmann::json{
    {"name", "status"},
    {"type", "select"},
    {"options", nlohmann::json{{"values", nlohmann::json::array({"draft", "published"})}}},
    {"maxSelect", 1}
}
pb.collection("articles").create(nlohmann::json{
    {"status", "published"}
});

// Multiple select
nlohmann::json{
    {"name", "tags"},
    {"type", "select"},
    {"options", nlohmann::json{{"values", nlohmann::json::array({"tech", "design"})}}},
    {"maxSelect", 5}
}
pb.collection("articles").update("ID", nlohmann::json{
    {"tags+", "marketing"}
});
```

### FileField

```cpp
// Single file
nlohmann::json{
    {"name", "cover"},
    {"type", "file"},
    {"maxSelect", 1},
    {"mimeTypes", nlohmann::json::array({"image/jpeg"})}
}

// Upload file
std::vector<FileAttachment> files;
files.push_back(FileAttachment{
    "cover",
    "image.jpg",
    "image/jpeg",
    imageData  // std::vector<uint8_t>
});
auto record = pb.collection("articles").create(nlohmann::json{
    {"title", "My Article"}
}, std::map<std::string, nlohmann::json>{}, files);
```

### RelationField

```cpp
// Field definition
nlohmann::json{
    {"name", "author"},
    {"type", "relation"},
    {"options", nlohmann::json{{"collectionId", "users"}}},
    {"maxSelect", 1}
}

// Usage
pb.collection("articles").create(nlohmann::json{
    {"author", "USER_ID"}
});

// Get with expand
auto record = pb.collection("articles").getOne("ID", 
    std::map<std::string, nlohmann::json>{},
    std::map<std::string, std::string>{},
    "author"
);
```

### JSONField

```cpp
// Field definition
nlohmann::json{{"name", "metadata"}, {"type", "json"}}

// Usage
pb.collection("articles").create(nlohmann::json{
    {"metadata", nlohmann::json{
        {"seo", nlohmann::json{
            {"title", "SEO Title"}
        }}
    }}
});
```

### GeoPointField

```cpp
// Field definition
nlohmann::json{{"name", "location"}, {"type", "geoPoint"}}

// Usage
pb.collection("places").create(nlohmann::json{
    {"location", nlohmann::json{
        {"lon", 139.6917},
        {"lat", 35.6586}
    }}
});
```

## Complete Example

```cpp
#include "bosbase/bosbase.h"
#include <iostream>
using namespace bosbase;

int main() {
    BosBase pb("http://localhost:8090");
    pb.collection("_superusers").authWithPassword("admin@example.com", "password");

    // Create collections
    auto users = pb.collections->createAuth("users");
    auto articles = pb.collections->createBase("articles", nlohmann::json{
        {"fields", nlohmann::json::array({
            nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}},
            nlohmann::json{
                {"name", "author"},
                {"type", "relation"},
                {"options", nlohmann::json{{"collectionId", users["id"]}}},
                {"maxSelect", 1}
            }
        })}
    });

    // Create and authenticate user
    auto user = pb.collection("users").create(nlohmann::json{
        {"email", "user@example.com"},
        {"password", "password123"},
        {"passwordConfirm", "password123"}
    });
    pb.collection("users").authWithPassword("user@example.com", "password123");

    // Create article
    auto article = pb.collection("articles").create(nlohmann::json{
        {"title", "My Article"},
        {"author", user["id"]}
    });

    // Subscribe to changes
    auto unsubscribe = pb.collection("articles").subscribe("*", [](const nlohmann::json& e) {
        std::cout << "Action: " << e["action"] << std::endl;
        std::cout << "Record: " << e["record"].dump() << std::endl;
    });

    // Later, unsubscribe
    unsubscribe();

    return 0;
}
```

