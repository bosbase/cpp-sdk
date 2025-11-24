# Files Upload and Handling - C++ SDK Documentation

## Overview

BosBase allows you to upload and manage files through file fields in your collections. Files are stored with sanitized names and a random suffix for security (e.g., `test_52iwbgds7l.png`).

**Key Features:**
- Upload multiple files per field
- Maximum file size: ~8GB (2^53-1 bytes)
- Automatic filename sanitization and random suffix
- Image thumbnails support
- Protected files with token-based access
- File modifiers for append/prepend/delete operations

**Backend Endpoints:**
- `POST /api/files/token` - Get file access token for protected files
- `GET /api/files/{collection}/{recordId}/{filename}` - Download file

## File Field Configuration

Before uploading files, you must add a file field to your collection:

```cpp
auto collection = pb.collections->getOne("example");

auto fields = collection["fields"];
fields.push_back(nlohmann::json{
    {"name", "documents"},
    {"type", "file"},
    {"maxSelect", 5},        // Maximum number of files (1 for single file)
    {"maxSize", 5242880},    // 5MB in bytes (optional, default: 5MB)
    {"mimeTypes", nlohmann::json::array({"image/jpeg", "image/png", "application/pdf"})},
    {"thumbs", nlohmann::json::array({"100x100", "300x300"})},  // Thumbnail sizes for images
    {"protected", false}     // Require token for access
});

pb.collections->update("example", nlohmann::json{{"fields", fields}});
```

## Uploading Files

### Basic Upload with Create

When creating a new record, you can upload files directly:

```cpp
#include "bosbase/bosbase.h"
using namespace bosbase;

BosBase pb("http://localhost:8090");

// Method 1: Using FileAttachment
std::vector<FileAttachment> files;
files.push_back(FileAttachment{
    "documents",
    "file1.txt",
    "text/plain",
    std::vector<uint8_t>{'c', 'o', 'n', 't', 'e', 'n', 't', ' ', '1', '.'}
});
files.push_back(FileAttachment{
    "documents",
    "file2.txt",
    "text/plain",
    std::vector<uint8_t>{'c', 'o', 'n', 't', 'e', 'n', 't', ' ', '2', '.'}
});

auto createdRecord = pb.collection("example").create(nlohmann::json{
    {"title", "Hello world!"}
}, std::map<std::string, nlohmann::json>{}, files);
```

### Upload with Update

```cpp
// Update record and upload new files
std::vector<FileAttachment> files;
files.push_back(FileAttachment{
    "documents",
    "file3.txt",
    "text/plain",
    fileData
});

auto updatedRecord = pb.collection("example").update("RECORD_ID",
    nlohmann::json{{"title", "Updated title"}},
    std::map<std::string, nlohmann::json>{}, files);
```

### Append Files (Using + Modifier)

For multiple file fields, use the `+` modifier to append files:

```cpp
// Append files to existing ones
std::vector<FileAttachment> files;
files.push_back(FileAttachment{
    "documents+",  // Note: + in field name
    "file4.txt",
    "text/plain",
    fileData
});

pb.collection("example").update("RECORD_ID",
    nlohmann::json{},
    std::map<std::string, nlohmann::json>{}, files);
```

## Deleting Files

### Delete All Files

```cpp
// Delete all files in a field (set to empty array)
pb.collection("example").update("RECORD_ID", nlohmann::json{
    {"documents", nlohmann::json::array()}
});
```

### Delete Specific Files (Using - Modifier)

```cpp
// Delete individual files by filename
pb.collection("example").update("RECORD_ID", nlohmann::json{
    {"documents-", nlohmann::json::array({"file1.pdf", "file2.txt"})}
});
```

## File URLs

### Get File URL

Each uploaded file can be accessed via its URL:

```
http://localhost:8090/api/files/COLLECTION_ID_OR_NAME/RECORD_ID/FILENAME
```

**Using SDK:**

```cpp
auto record = pb.collection("example").getOne("RECORD_ID");

// Single file field (returns string)
std::string filename = record["documents"];
std::string url = pb.files->getURL(record, filename);

// Multiple file field (returns array)
if (record["documents"].is_array() && !record["documents"].empty()) {
    std::string firstFile = record["documents"][0];
    std::string url = pb.files->getURL(record, firstFile);
}
```

### Image Thumbnails

If your file field has thumbnail sizes configured, you can request thumbnails:

```cpp
auto record = pb.collection("example").getOne("RECORD_ID");
std::string filename = record["avatar"];  // Image file

// Get thumbnail with specific size
std::string thumbUrl = pb.files->getURL(record, filename, "100x300");

// Different thumbnail sizes
std::string smallThumb = pb.files->getURL(record, filename, "50x50");
std::string mediumThumb = pb.files->getURL(record, filename, "200x200");
std::string largeThumb = pb.files->getURL(record, filename, "500x500");

// Fit thumbnail (no cropping)
std::string fitThumb = pb.files->getURL(record, filename, "200x200f");
```

**Thumbnail Formats:**

- `WxH` (e.g., `100x300`) - Crop to WxH viewbox from center
- `WxHt` (e.g., `100x300t`) - Crop to WxH viewbox from top
- `WxHb` (e.g., `100x300b`) - Crop to WxH viewbox from bottom
- `WxHf` (e.g., `100x300f`) - Fit inside WxH viewbox (no cropping)
- `0xH` (e.g., `0x300`) - Resize to H height, preserve aspect ratio
- `Wx0` (e.g., `100x0`) - Resize to W width, preserve aspect ratio

**Supported Image Formats:**
- JPEG (`.jpg`, `.jpeg`)
- PNG (`.png`)
- GIF (`.gif` - first frame only)
- WebP (`.webp` - stored as PNG)

### Force Download

To force browser download instead of preview:

```cpp
std::string url = pb.files->getURL(record, filename, std::nullopt, std::nullopt, true);
```

## Protected Files

By default, all files are publicly accessible if you know the full URL. For sensitive files, you can mark the field as "Protected" in the collection settings.

### Setting Up Protected Files

```cpp
auto collection = pb.collections->getOne("example");

auto fields = collection["fields"];
for (auto& field : fields) {
    if (field["name"] == "documents") {
        field["protected"] = true;
        break;
    }
}

pb.collections->update("example", nlohmann::json{{"fields", fields}});
```

### Accessing Protected Files

Protected files require authentication and a file token:

```cpp
// Step 1: Authenticate
pb.collection("users").authWithPassword("user@example.com", "password123");

// Step 2: Get file token (valid for ~2 minutes)
auto tokenResponse = pb.files->getToken();
std::string fileToken = tokenResponse["token"];

// Step 3: Get protected file URL with token
auto record = pb.collection("example").getOne("RECORD_ID");
std::string url = pb.files->getURL(record, record["privateDocument"], 
    std::nullopt, fileToken);

// Use the URL
std::cout << "Protected file URL: " << url << std::endl;
```

**Important:**
- File tokens are short-lived (~2 minutes)
- Only authenticated users satisfying the collection's `viewRule` can access protected files
- Tokens must be regenerated when they expire

### Complete Protected File Example

```cpp
#include "bosbase/bosbase.h"
#include "bosbase/error.h"
#include <iostream>
using namespace bosbase;

std::string loadProtectedImage(BosBase& pb, const std::string& recordId, const std::string& filename) {
    try {
        // Check if authenticated
        if (!pb.authStore()->isValid()) {
            throw std::runtime_error("Not authenticated");
        }

        // Get fresh token
        auto tokenResponse = pb.files->getToken();
        std::string token = tokenResponse["token"];

        // Get file URL
        auto record = pb.collection("example").getOne(recordId);
        std::string url = pb.files->getURL(record, filename, std::nullopt, token);

        return url;
    } catch (const ClientResponseError& err) {
        if (err.status() == 404) {
            std::cerr << "File not found or access denied" << std::endl;
        } else if (err.status() == 401) {
            std::cerr << "Authentication required" << std::endl;
            pb.authStore()->clear();
        }
        throw;
    }
}
```

## Complete Examples

### Example 1: Image Upload with Thumbnails

```cpp
#include "bosbase/bosbase.h"
#include <iostream>
using namespace bosbase;

int main() {
    BosBase pb("http://localhost:8090");
    pb.collection("_superusers").authWithPassword("admin@example.com", "password");

    // Create collection with image field and thumbnails
    auto collection = pb.collections->createBase("products", nlohmann::json{
        {"fields", nlohmann::json::array({
            nlohmann::json{{"name", "name"}, {"type", "text"}, {"required", true}},
            nlohmann::json{
                {"name", "image"},
                {"type", "file"},
                {"maxSelect", 1},
                {"mimeTypes", nlohmann::json::array({"image/jpeg", "image/png"})},
                {"thumbs", nlohmann::json::array({"100x100", "300x300", "800x600f"})}
            }
        })}
    });

    // Upload product with image
    std::vector<FileAttachment> files;
    files.push_back(FileAttachment{
        "image",
        "product.jpg",
        "image/jpeg",
        imageBlob  // std::vector<uint8_t>
    });
    
    auto product = pb.collection("products").create(nlohmann::json{
        {"name", "My Product"}
    }, std::map<std::string, nlohmann::json>{}, files);

    // Display thumbnail in UI
    std::string thumbnailUrl = pb.files->getURL(product, product["image"], "300x300");
    std::cout << "Thumbnail URL: " << thumbnailUrl << std::endl;

    return 0;
}
```

### Example 2: Multiple File Upload

```cpp
void uploadMultipleFiles(BosBase& pb, const std::vector<std::vector<uint8_t>>& fileDataList) {
    std::vector<FileAttachment> files;
    
    for (size_t i = 0; i < fileDataList.size(); ++i) {
        files.push_back(FileAttachment{
            "documents",
            "file" + std::to_string(i) + ".pdf",
            "application/pdf",
            fileDataList[i]
        });
    }

    try {
        auto record = pb.collection("example").create(nlohmann::json{
            {"title", "Document Set"}
        }, std::map<std::string, nlohmann::json>{}, files);
        
        std::cout << "Uploaded files: " << record["documents"].dump() << std::endl;
    } catch (const ClientResponseError& err) {
        std::cerr << "Upload failed: " << err.what() << std::endl;
    }
}
```

## File Field Modifiers

### Summary

- **No modifier** - Replace all files: `documents: [file1, file2]`
- **`+` suffix** - Append files: `documents+: file3`
- **`+` prefix** - Prepend files: `+documents: file0`
- **`-` suffix** - Delete files: `documents-: ["file1.pdf"]`

## Best Practices

1. **File Size Limits**: Always validate file sizes on the client before upload
2. **MIME Types**: Configure allowed MIME types in collection field settings
3. **Thumbnails**: Pre-generate common thumbnail sizes for better performance
4. **Protected Files**: Use protected files for sensitive documents (ID cards, contracts)
5. **Token Refresh**: Refresh file tokens before they expire for protected files
6. **Error Handling**: Handle 404 errors for missing files and 401 for protected file access
7. **Filename Sanitization**: Files are automatically sanitized, but validate on client side too

## Error Handling

```cpp
#include "bosbase/error.h"

try {
    std::vector<FileAttachment> files;
    files.push_back(FileAttachment{
        "documents",
        "test.txt",
        "text/plain",
        std::vector<uint8_t>{'c', 'o', 'n', 't', 'e', 'n', 't'}
    });
    
    auto record = pb.collection("example").create(nlohmann::json{
        {"title", "Test"}
    }, std::map<std::string, nlohmann::json>{}, files);
} catch (const ClientResponseError& err) {
    if (err.status() == 413) {
        std::cerr << "File too large" << std::endl;
    } else if (err.status() == 400) {
        std::cerr << "Invalid file type or field validation failed" << std::endl;
    } else if (err.status() == 403) {
        std::cerr << "Insufficient permissions" << std::endl;
    } else {
        std::cerr << "Upload failed: " << err.what() << std::endl;
    }
}
```

## Storage Options

By default, BosBase stores files in `pb_data/storage` on the local filesystem. For production, you can configure S3-compatible storage (AWS S3, MinIO, Wasabi, DigitalOcean Spaces, etc.) from:
**Dashboard > Settings > Files storage**

This is configured server-side and doesn't require SDK changes.

## Related Documentation

- [Collections](./COLLECTIONS.md) - Collection and field configuration
- [Authentication](./AUTHENTICATION.md) - Required for protected files

