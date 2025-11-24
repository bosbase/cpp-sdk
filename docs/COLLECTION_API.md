# Collection API - C++ SDK Documentation

## Overview

The Collection API provides endpoints for managing collections (Base, Auth, and View types). All operations require superuser authentication and allow you to create, read, update, and delete collections along with their schemas and configurations.

**Key Features:**
- List and search collections
- View collection details
- Create collections (base, auth, view)
- Update collection schemas and rules
- Delete collections
- Truncate collections (delete all records)
- Import collections in bulk
- Get collection scaffolds (templates)
- Manage indexes
- Manage fields

**Backend Endpoints:**
- `GET /api/collections` - List collections
- `GET /api/collections/{collection}` - View collection
- `POST /api/collections` - Create collection
- `PATCH /api/collections/{collection}` - Update collection
- `DELETE /api/collections/{collection}` - Delete collection
- `DELETE /api/collections/{collection}/truncate` - Truncate collection
- `PUT /api/collections/import` - Import collections
- `GET /api/collections/meta/scaffolds` - Get scaffolds

**Note**: All Collection API operations require superuser authentication.

## Authentication

All Collection API operations require superuser authentication:

```cpp
#include "bosbase/bosbase.h"

bosbase::BosBase pb("http://127.0.0.1:8090");

// Authenticate as superuser
pb.collection("_superusers").authWithPassword("admin@example.com", "password");
```

## List Collections

Returns a paginated list of collections with support for filtering and sorting.

```cpp
// Basic list
auto result = pb.collections->getList(1, 30);

std::cout << "Page: " << result["page"] << std::endl;
std::cout << "Per Page: " << result["perPage"] << std::endl;
std::cout << "Total Items: " << result["totalItems"] << std::endl;
std::cout << "Items: " << result["items"].dump(2) << std::endl;
```

### Advanced Filtering and Sorting

```cpp
// Filter by type
std::map<std::string, nlohmann::json> query = {
    {"filter", "type = \"auth\""}
};
auto authCollections = pb.collections->getList(1, 100, query);

// Filter by name pattern
query = {{"filter", "name ~ \"user\""}};
auto matchingCollections = pb.collections->getList(1, 100, query);

// Sort by creation date
query = {{"sort", "-created"}};
auto sortedCollections = pb.collections->getList(1, 100, query);

// Complex filter
query = {
    {"filter", "type = \"base\" && system = false && created >= \"2023-01-01\""},
    {"sort", "name"}
};
auto filtered = pb.collections->getList(1, 100, query);
```

### Get Full List

```cpp
// Get all collections at once
std::map<std::string, nlohmann::json> query = {
    {"sort", "name"},
    {"filter", "system = false"}
};
auto allCollections = pb.collections->getFullList(query);
```

### Get First Matching Collection

```cpp
// Get first auth collection
auto authCollection = pb.collections->getFirstListItem("type = \"auth\"");
```

## View Collection

Retrieve a single collection by ID or name:

```cpp
// By name
auto collection = pb.collections->getOne("posts");

// By ID
auto collection = pb.collections->getOne("_pbc_2287844090");

// With field selection
std::map<std::string, nlohmann::json> query = {
    {"fields", "id,name,type,fields.name,fields.type"}
};
auto collection = pb.collections->getOne("posts", query);
```

## Create Collection

Create a new collection with schema fields and configuration.

**Note**: If the `created` and `updated` fields are not specified during collection initialization, BosBase will automatically create them. These system fields are added to all collections by default and track when records are created and last modified. You don't need to include them in your field definitions.

### Create Base Collection

```cpp
nlohmann::json fields = nlohmann::json::array({
    nlohmann::json{
        {"name", "title"},
        {"type", "text"},
        {"required", true},
        {"min", 10},
        {"max", 255}
    },
    nlohmann::json{
        {"name", "content"},
        {"type", "editor"},
        {"required", false}
    },
    nlohmann::json{
        {"name", "published"},
        {"type", "bool"},
        {"required", false}
    },
    nlohmann::json{
        {"name", "author"},
        {"type", "relation"},
        {"required", true},
        {"options", nlohmann::json{{"collectionId", "_pbc_users_auth_"}}},
        {"maxSelect", 1}
    }
});

nlohmann::json overrides = {
    {"fields", fields},
    {"listRule", "@request.auth.id != \"\""},
    {"viewRule", "@request.auth.id != \"\" || published = true"},
    {"createRule", "@request.auth.id != \"\""},
    {"updateRule", "author = @request.auth.id"},
    {"deleteRule", "author = @request.auth.id"}
};

auto baseCollection = pb.collections->createBase("posts", overrides);
```

### Create Auth Collection

```cpp
nlohmann::json fields = nlohmann::json::array({
    nlohmann::json{
        {"name", "name"},
        {"type", "text"},
        {"required", false}
    },
    nlohmann::json{
        {"name", "avatar"},
        {"type", "file"},
        {"required", false},
        {"maxSelect", 1},
        {"maxSize", 2097152},  // 2MB
        {"mimeTypes", nlohmann::json::array({"image/jpeg", "image/png"})}
    }
});

nlohmann::json overrides = {
    {"fields", fields},
    {"listRule", nullptr},
    {"viewRule", "@request.auth.id = id"},
    {"createRule", nullptr},
    {"updateRule", "@request.auth.id = id"},
    {"deleteRule", "@request.auth.id = id"},
    {"manageRule", nullptr},
    {"authRule", "verified = true"},
    {"passwordAuth", nlohmann::json{
        {"enabled", true},
        {"identityFields", nlohmann::json::array({"email", "username"})}
    }},
    {"authToken", nlohmann::json{
        {"duration", 604800}  // 7 days
    }},
    {"oauth2", nlohmann::json{
        {"enabled", true},
        {"providers", nlohmann::json::array({
            nlohmann::json{
                {"name", "google"},
                {"clientId", "YOUR_CLIENT_ID"},
                {"clientSecret", "YOUR_CLIENT_SECRET"},
                {"authURL", "https://accounts.google.com/o/oauth2/auth"},
                {"tokenURL", "https://oauth2.googleapis.com/token"},
                {"userInfoURL", "https://www.googleapis.com/oauth2/v2/userinfo"},
                {"displayName", "Google"}
            }
        })}
    }}
};

auto authCollection = pb.collections->createAuth("users", overrides);
```

### Create View Collection

```cpp
std::string viewQuery = R"(
    SELECT 
      p.id,
      p.title,
      p.content,
      p.created,
      u.name as author_name,
      u.email as author_email
    FROM posts p
    LEFT JOIN users u ON p.author = u.id
    WHERE p.published = true
)";

nlohmann::json overrides = {
    {"listRule", "@request.auth.id != \"\""},
    {"viewRule", "@request.auth.id != \"\""}
};

auto viewCollection = pb.collections->createView("published_posts", viewQuery, overrides);
```

### Create from Scaffold

Use predefined scaffolds as a starting point:

```cpp
// Get available scaffolds
auto scaffolds = pb.collections->getScaffolds();

// Create base collection from scaffold
nlohmann::json baseOverrides = {
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}}
    })}
};
auto baseCollection = pb.collections->createBase("my_posts", baseOverrides);

// Create auth collection from scaffold
nlohmann::json authOverrides = {
    {"passwordAuth", nlohmann::json{
        {"enabled", true},
        {"identityFields", nlohmann::json::array({"email"})}
    }}
};
auto authCollection = pb.collections->createAuth("my_users", authOverrides);

// Create view collection from scaffold
auto viewCollection = pb.collections->createView("my_view", 
    "SELECT id, title FROM posts", 
    nlohmann::json{{"listRule", "@request.auth.id != \"\""}});
```

### Accessing Collection ID After Creation

When a collection is successfully created, the returned object includes the `id` property, which contains the unique identifier assigned by the backend. You can access it immediately after creation:

```cpp
// Create a collection and access its ID
auto collection = pb.collections->createBase("posts", nlohmann::json{
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}}
    })}
});

// Access the collection ID
std::string collectionId = collection["id"].get<std::string>();
std::cout << "Collection ID: " << collectionId << std::endl;

// Use the ID for subsequent operations
pb.collections->update(collectionId, nlohmann::json{
    {"listRule", "@request.auth.id != \"\""}
});
```

**Example: Creating multiple collections and storing their IDs**

```cpp
// Create posts collection
auto posts = pb.collections->createBase("posts", nlohmann::json{
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}},
        nlohmann::json{{"name", "content"}, {"type", "editor"}}
    })}
});

// Create categories collection
auto categories = pb.collections->createBase("categories", nlohmann::json{
    {"fields", nlohmann::json::array({
        nlohmann::json{{"name", "name"}, {"type", "text"}, {"required", true}}
    })}
});

// Access IDs immediately after creation
std::string postsId = posts["id"].get<std::string>();
std::string categoriesId = categories["id"].get<std::string>();

std::cout << "Posts collection ID: " << postsId << std::endl;
std::cout << "Categories collection ID: " << categoriesId << std::endl;

// Use IDs to create relations
auto postsUpdated = pb.collections->getOne(postsId);
auto fields = postsUpdated["fields"];
fields.push_back(nlohmann::json{
    {"name", "category"},
    {"type", "relation"},
    {"options", nlohmann::json{{"collectionId", categoriesId}}},
    {"maxSelect", 1}
});
pb.collections->update(postsId, nlohmann::json{{"fields", fields}});
```

## Update Collection

Update an existing collection's schema, fields, or rules:

```cpp
// Update collection name and rules
pb.collections->update("posts", nlohmann::json{
    {"name", "articles"},
    {"listRule", "@request.auth.id != \"\" || status = \"public\""},
    {"viewRule", "@request.auth.id != \"\" || status = \"public\""}
});

// Add new field
auto collection = pb.collections->getOne("posts");
auto fields = collection["fields"];
fields.push_back(nlohmann::json{
    {"name", "tags"},
    {"type", "select"},
    {"options", nlohmann::json{
        {"values", nlohmann::json::array({"tech", "science", "art"})}
    }}
});
pb.collections->update("posts", nlohmann::json{{"fields", fields}});

// Update field configuration
collection = pb.collections->getOne("posts");
fields = collection["fields"];
for (auto& field : fields) {
    if (field["name"] == "title") {
        field["max"] = 200;
        break;
    }
}
pb.collections->update("posts", nlohmann::json{{"fields", fields}});
```

### Updating Auth Collection Options

```cpp
// Update OAuth2 configuration
auto collection = pb.collections->getOne("users");
auto oauth2 = collection["oauth2"];
oauth2["enabled"] = true;
oauth2["providers"] = nlohmann::json::array({
    nlohmann::json{
        {"name", "github"},
        {"clientId", "NEW_CLIENT_ID"},
        {"clientSecret", "NEW_CLIENT_SECRET"},
        {"displayName", "GitHub"}
    }
});

pb.collections->update("users", nlohmann::json{{"oauth2", oauth2}});

// Update token duration
auto authToken = collection["authToken"];
authToken["duration"] = 2592000;  // 30 days
pb.collections->update("users", nlohmann::json{{"authToken", authToken}});
```

## Manage Indexes

BosBase stores collection indexes as SQL expressions. The C++ SDK provides dedicated helpers so you don't have to manually craft the SQL or resend the full collection payload every time you want to adjust an index.

### Add or Update Indexes

```cpp
// Create a unique slug index (index names are optional)
pb.collections->addIndex("posts", {"slug"}, true, "idx_posts_slug_unique");

// Composite (non-unique) index; defaults to idx_{collection}_{columns}
pb.collections->addIndex("posts", {"status", "published"});
```

- `collectionIdOrName` can be either the collection name or internal id.
- `columns` must reference existing columns (system fields such as `id`, `created`, and `updated` are allowed).
- `unique` (default `false`) controls whether `CREATE UNIQUE INDEX` or `CREATE INDEX` is generated.
- `indexName` is optional; omit it to let the SDK generate `idx_{collection}_{column1}_{column2}` automatically.

Calling `addIndex` twice with the same name replaces the definition on the backend, making it easy to iterate on your schema.

### Remove Indexes

```cpp
// Remove the index that targets the slug column
pb.collections->removeIndex("posts", {"slug"});
```

`removeIndex` looks for indexes that contain all of the provided columns (in any order) and drops them from the collection. This deletes the actual database index when the collection is saved.

### List Indexes

```cpp
auto indexes = pb.collections->getIndexes("posts");

for (const auto& idx : indexes) {
    std::cout << idx.get<std::string>() << std::endl;
}
// => CREATE UNIQUE INDEX `idx_posts_slug_unique` ON `posts` (`slug`)
```

`getIndexes` returns the raw SQL strings stored on the collection so you can audit existing indexes or decide whether you need to create new ones.

## Manage Fields

### Add Field

```cpp
nlohmann::json newField = {
    {"name", "description"},
    {"type", "text"},
    {"required", false}
};

auto updated = pb.collections->addField("posts", newField);
```

### Update Field

```cpp
nlohmann::json updates = {
    {"max", 500}
};

pb.collections->updateField("posts", "title", updates);
```

### Remove Field

```cpp
pb.collections->removeField("posts", "old_field");
```

### Get Field

```cpp
auto field = pb.collections->getField("posts", "title");
std::cout << field.dump(2) << std::endl;
```

## Manage Rules

### Set All Rules

```cpp
nlohmann::json rules = {
    {"listRule", "@request.auth.id != \"\""},
    {"viewRule", "@request.auth.id != \"\" || status = \"published\""},
    {"createRule", "@request.auth.id != \"\""},
    {"updateRule", "author = @request.auth.id"},
    {"deleteRule", "author = @request.auth.id"}
};

pb.collections->setRules("posts", rules);
```

### Set Single Rule

```cpp
// Set listRule
pb.collections->setRule("posts", "listRule", "@request.auth.id != \"\"");

// Remove rule (set to empty string for public access)
pb.collections->setRule("posts", "listRule", "");

// Lock rule (set to null for superuser only)
pb.collections->setRule("posts", "deleteRule", std::nullopt);
```

### Get Rules

```cpp
auto rules = pb.collections->getRules("posts");
std::cout << "List Rule: " << rules["listRule"].dump() << std::endl;
std::cout << "View Rule: " << rules["viewRule"].dump() << std::endl;
```

## Delete Collection

Delete a collection (including all records and files):

```cpp
// Delete by name
pb.collections->deleteCollection("old_collection");

// Delete by ID
pb.collections->deleteCollection("_pbc_2287844090");
```

**Warning**: This operation is destructive and will:
- Delete the collection schema
- Delete all records in the collection
- Delete all associated files
- Remove all indexes

**Note**: Collections referenced by other collections cannot be deleted.

## Truncate Collection

Delete all records in a collection while keeping the collection schema:

```cpp
// Truncate collection (delete all records)
pb.collections->truncate("posts");

// This will:
// - Delete all records in the collection
// - Delete all associated files
// - Delete cascade-enabled relations
// - Keep the collection schema intact
```

**Warning**: This operation is destructive and cannot be undone. It's useful for:
- Clearing test data
- Resetting collections
- Bulk data removal

**Note**: View collections cannot be truncated.

## Import Collections

Bulk import multiple collections at once:

```cpp
nlohmann::json collectionsToImport = nlohmann::json::array({
    nlohmann::json{
        {"name", "posts"},
        {"type", "base"},
        {"fields", nlohmann::json::array({
            nlohmann::json{
                {"name", "title"},
                {"type", "text"},
                {"required", true}
            },
            nlohmann::json{
                {"name", "content"},
                {"type", "editor"}
            }
        })},
        {"listRule", "@request.auth.id != \"\""}
    },
    nlohmann::json{
        {"name", "categories"},
        {"type", "base"},
        {"fields", nlohmann::json::array({
            nlohmann::json{
                {"name", "name"},
                {"type", "text"},
                {"required", true}
            }
        })}
    }
});

// Import without deleting existing collections
pb.collections->importCollections(collectionsToImport, false);

// Import and delete collections not in the import list
pb.collections->importCollections(collectionsToImport, true);
```

### Import Options

- **`deleteMissing: false`** (default): Only create/update collections in the import list
- **`deleteMissing: true`**: Delete all collections not present in the import list

**Warning**: Using `deleteMissing: true` will permanently delete collections and all their data.

## Get Scaffolds

Get collection templates for creating new collections:

```cpp
auto scaffolds = pb.collections->getScaffolds();

// Available scaffold types
std::cout << "Base scaffold: " << scaffolds["base"].dump(2) << std::endl;
std::cout << "Auth scaffold: " << scaffolds["auth"].dump(2) << std::endl;
std::cout << "View scaffold: " << scaffolds["view"].dump(2) << std::endl;

// Use scaffold as starting point
auto baseTemplate = scaffolds["base"];
nlohmann::json newCollection = baseTemplate;
newCollection["name"] = "my_collection";
auto fields = baseTemplate["fields"];
fields.push_back(nlohmann::json{
    {"name", "custom_field"},
    {"type", "text"}
});
newCollection["fields"] = fields;

pb.collections->create(newCollection);
```

## Filter Syntax

Collections support filtering with the same syntax as records:

### Supported Fields

- `id` - Collection ID
- `created` - Creation date
- `updated` - Last update date
- `name` - Collection name
- `type` - Collection type (`base`, `auth`, `view`)
- `system` - System collection flag (boolean)

### Filter Examples

```cpp
// Filter by type
std::map<std::string, nlohmann::json> query = {
    {"filter", "type = \"auth\""}
};

// Filter by name pattern
query = {{"filter", "name ~ \"user\""}};

// Filter non-system collections
query = {{"filter", "system = false"}};

// Multiple conditions
query = {
    {"filter", "type = \"base\" && system = false && created >= \"2023-01-01\""}
};

// Complex filter
query = {
    {"filter", "(type = \"auth\" || type = \"base\") && name !~ \"test\""}
};
```

## Sort Options

Supported sort fields:

- `@random` - Random order
- `id` - Collection ID
- `created` - Creation date
- `updated` - Last update date
- `name` - Collection name
- `type` - Collection type
- `system` - System flag

```cpp
// Sort examples
std::map<std::string, nlohmann::json> query = {
    {"sort", "name"}           // ASC by name
};
query = {{"sort", "-created"}};       // DESC by creation date
query = {{"sort", "type,-created"}};  // ASC by type, then DESC by created
```

## Complete Examples

### Example 1: Setup Blog Collections

```cpp
void setupBlog(bosbase::BosBase& pb) {
    // Create posts collection
    nlohmann::json postsFields = nlohmann::json::array({
        nlohmann::json{{"name", "title"}, {"type", "text"}, {"required", true}, {"min", 10}, {"max", 255}},
        nlohmann::json{{"name", "slug"}, {"type", "text"}, {"required", true}, {"options", nlohmann::json{{"pattern", "^[a-z0-9-]+$"}}}},
        nlohmann::json{{"name", "content"}, {"type", "editor"}, {"required", true}},
        nlohmann::json{{"name", "featured_image"}, {"type", "file"}, {"maxSelect", 1}, {"maxSize", 5242880}, {"mimeTypes", nlohmann::json::array({"image/jpeg", "image/png"})}},
        nlohmann::json{{"name", "published"}, {"type", "bool"}, {"required", false}},
        nlohmann::json{{"name", "author"}, {"type", "relation"}, {"options", nlohmann::json{{"collectionId", "_pbc_users_auth_"}}}, {"maxSelect", 1}},
        nlohmann::json{{"name", "categories"}, {"type", "relation"}, {"options", nlohmann::json{{"collectionId", "categories"}}}, {"maxSelect", 5}}
    });

    auto posts = pb.collections->createBase("posts", nlohmann::json{
        {"fields", postsFields},
        {"listRule", "@request.auth.id != \"\" || published = true"},
        {"viewRule", "@request.auth.id != \"\" || published = true"},
        {"createRule", "@request.auth.id != \"\""},
        {"updateRule", "author = @request.auth.id"},
        {"deleteRule", "author = @request.auth.id"}
    });

    // Create categories collection
    auto categories = pb.collections->createBase("categories", nlohmann::json{
        {"fields", nlohmann::json::array({
            nlohmann::json{{"name", "name"}, {"type", "text"}, {"required", true}, {"unique", true}},
            nlohmann::json{{"name", "slug"}, {"type", "text"}, {"required", true}},
            nlohmann::json{{"name", "description"}, {"type", "text"}, {"required", false}}
        })},
        {"listRule", "@request.auth.id != \"\""},
        {"viewRule", "@request.auth.id != \"\""}
    });

    // Access collection IDs immediately after creation
    std::string postsId = posts["id"].get<std::string>();
    std::string categoriesId = categories["id"].get<std::string>();

    std::cout << "Posts collection ID: " << postsId << std::endl;
    std::cout << "Categories collection ID: " << categoriesId << std::endl;

    // Update posts collection to use the categories collection ID
    auto postsUpdated = pb.collections->getOne(postsId);
    auto fields = postsUpdated["fields"];
    for (auto& field : fields) {
        if (field["name"] == "categories") {
            field["options"]["collectionId"] = categoriesId;
            break;
        }
    }
    pb.collections->update(postsId, nlohmann::json{{"fields", fields}});

    std::cout << "Blog setup complete!" << std::endl;
}
```

## Error Handling

```cpp
#include "bosbase/error.h"

try {
    pb.collections->createBase("test", nlohmann::json{
        {"fields", nlohmann::json::array()}
    });
} catch (const ClientResponseError& err) {
    if (err.status() == 401) {
        std::cerr << "Not authenticated" << std::endl;
    } else if (err.status() == 403) {
        std::cerr << "Not a superuser" << std::endl;
    } else if (err.status() == 400) {
        std::cerr << "Validation error: " << err.data().dump(2) << std::endl;
    } else {
        std::cerr << "Unexpected error: " << err.what() << std::endl;
    }
}
```

## Best Practices

1. **Always Authenticate**: Ensure you're authenticated as a superuser before making requests
2. **Backup Before Import**: Always backup existing collections before using `importCollections` with `deleteMissing: true`
3. **Validate Schema**: Validate collection schemas before creating/updating
4. **Use Scaffolds**: Use scaffolds as starting points for consistency
5. **Test Rules**: Test API rules thoroughly before deploying to production
6. **Index Important Fields**: Add indexes for frequently queried fields
7. **Document Schemas**: Keep documentation of your collection schemas
8. **Version Control**: Store collection schemas in version control for migration tracking

## Limitations

- **Superuser Only**: All operations require superuser authentication
- **System Collections**: System collections cannot be deleted or renamed
- **View Collections**: Cannot be truncated (they don't store records)
- **Relations**: Collections referenced by others cannot be deleted
- **Field Modifications**: Some field type changes may require data migration

## Related Documentation

- [Collections Guide](./COLLECTIONS.md) - Working with collections and records
- [API Records](./API_RECORDS.md) - Record CRUD operations
- [API Rules and Filters](./API_RULES_AND_FILTERS.md) - Understanding API rules

