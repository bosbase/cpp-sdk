# File API - C++ SDK Documentation

## Overview

The File API provides endpoints for downloading and accessing files stored in collection records. It supports thumbnail generation for images, protected file access with tokens, and force download options.

**Key Features:**
- Download files from collection records
- Generate thumbnails for images (crop, fit, resize)
- Protected file access with short-lived tokens
- Force download option for any file type
- Automatic content-type detection
- Support for Range requests and caching

**Backend Endpoints:**
- `GET /api/files/{collection}/{recordId}/{filename}` - Download/fetch file
- `POST /api/files/token` - Generate protected file token

## Download / Fetch File

Downloads a single file resource from a record.

### Basic Usage

```cpp
#include "bosbase/bosbase.h"
#include <iostream>

int main() {
    bosbase::BosBase pb("http://127.0.0.1:8090");

    // Get a record with a file field
    auto record = pb.collection("posts").getOne("RECORD_ID");

    // Get the file URL
    std::string fileUrl = pb.files->getURL(record, record["image"].get<std::string>());

    // Use the URL (e.g., in HTTP client or display)
    std::cout << "File URL: " << fileUrl << std::endl;

    return 0;
}
```

### File URL Structure

The file URL follows this pattern:
```
/api/files/{collectionIdOrName}/{recordId}/{filename}
```

Example:
```
http://127.0.0.1:8090/api/files/posts/abc123/photo_xyz789.jpg
```

### Using in Applications

```cpp
// For image display (e.g., in Qt, web view, etc.)
std::string imageUrl = pb.files->getURL(record, record["image"].get<std::string>());

// For download link
std::string downloadUrl = pb.files->getURL(record, record["document"].get<std::string>(), 
    std::nullopt, std::nullopt, true);

// Use with HTTP client to fetch file
// (implementation depends on your HTTP client library)
```

## Thumbnails

Generate thumbnails for image files on-the-fly.

### Thumbnail Formats

The following thumbnail formats are supported:

| Format | Example | Description |
|--------|---------|-------------|
| `WxH` | `100x300` | Crop to WxH viewbox (from center) |
| `WxHt` | `100x300t` | Crop to WxH viewbox (from top) |
| `WxHb` | `100x300b` | Crop to WxH viewbox (from bottom) |
| `WxHf` | `100x300f` | Fit inside WxH viewbox (without cropping) |
| `0xH` | `0x300` | Resize to H height preserving aspect ratio |
| `Wx0` | `100x0` | Resize to W width preserving aspect ratio |

### Using Thumbnails

```cpp
auto record = pb.collection("posts").getOne("RECORD_ID");
std::string filename = record["image"].get<std::string>();

// Get thumbnail URL
std::string thumbUrl = pb.files->getURL(record, filename, "100x100");

// Different thumbnail sizes
std::string smallThumb = pb.files->getURL(record, filename, "50x50");
std::string mediumThumb = pb.files->getURL(record, filename, "200x200");
std::string largeThumb = pb.files->getURL(record, filename, "500x500");

// Fit thumbnail (no cropping)
std::string fitThumb = pb.files->getURL(record, filename, "200x200f");

// Resize to specific width
std::string widthThumb = pb.files->getURL(record, filename, "300x0");

// Resize to specific height
std::string heightThumb = pb.files->getURL(record, filename, "0x200");
```

### Thumbnail Behavior

- **Image Files Only**: Thumbnails are only generated for image files (PNG, JPG, JPEG, GIF, WEBP)
- **Non-Image Files**: For non-image files, the thumb parameter is ignored and the original file is returned
- **Caching**: Thumbnails are cached and reused if already generated
- **Fallback**: If thumbnail generation fails, the original file is returned
- **Field Configuration**: Thumb sizes must be defined in the file field's `thumbs` option or use default `100x100`

## Protected Files

Protected files require a special token for access, even if you're authenticated.

### Getting a File Token

```cpp
// Must be authenticated first
pb.collection("users").authWithPassword("user@example.com", "password");

// Get file token
auto tokenResponse = pb.files->getToken();
std::string token = tokenResponse["token"].get<std::string>();

std::cout << "File token: " << token << std::endl;
```

### Using Protected File Token

```cpp
// Get protected file URL with token
auto record = pb.collection("documents").getOne("RECORD_ID");
auto tokenResponse = pb.files->getToken();
std::string token = tokenResponse["token"].get<std::string>();

std::string protectedFileUrl = pb.files->getURL(record, 
    record["document"].get<std::string>(), 
    std::nullopt, 
    token);

// Use the URL to fetch the file
std::cout << "Protected file URL: " << protectedFileUrl << std::endl;
```

### Protected File Example

```cpp
#include "bosbase/bosbase.h"
#include "bosbase/error.h"
#include <iostream>

std::string loadProtectedImage(bosbase::BosBase& pb, const std::string& recordId) {
    try {
        // Authenticate
        pb.collection("users").authWithPassword("user@example.com", "password");
        
        // Get record
        auto record = pb.collection("documents").getOne(recordId);
        
        // Get file token
        auto tokenResponse = pb.files->getToken();
        std::string token = tokenResponse["token"].get<std::string>();
        
        // Get protected file URL
        std::string imageUrl = pb.files->getURL(record, 
            record["thumbnail"].get<std::string>(), 
            "300x300", 
            token);
        
        return imageUrl;
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

### Token Lifetime

- File tokens are short-lived (typically expires after a few minutes)
- Tokens are associated with the authenticated user/superuser
- Generate a new token if the previous one expires

## Force Download

Force files to download instead of being displayed in the browser.

```cpp
// Force download
auto record = pb.collection("documents").getOne("RECORD_ID");
std::string downloadUrl = pb.files->getURL(record, 
    record["document"].get<std::string>(), 
    std::nullopt, 
    std::nullopt, 
    true);  // download = true

// Use the URL for download
std::cout << "Download URL: " << downloadUrl << std::endl;
```

### Download Parameter Values

```cpp
// Force download
pb.files->getURL(record, filename, std::nullopt, std::nullopt, true);

// Allow inline display (default)
pb.files->getURL(record, filename);  // No download parameter
pb.files->getURL(record, filename, std::nullopt, std::nullopt, false);
```

## Complete Examples

### Example 1: Image Gallery

```cpp
void displayImageGallery(bosbase::BosBase& pb, const std::string& recordId) {
    auto record = pb.collection("posts").getOne(recordId);
    
    nlohmann::json images;
    if (record["images"].is_array()) {
        images = record["images"];
    } else if (record.contains("image")) {
        images = nlohmann::json::array({record["image"]});
    }
    
    for (const auto& filename : images) {
        // Thumbnail for gallery
        std::string thumbUrl = pb.files->getURL(record, 
            filename.get<std::string>(), 
            "200x200");
        
        // Full image URL
        std::string fullUrl = pb.files->getURL(record, 
            filename.get<std::string>());
        
        // Display thumbnails (implementation depends on your UI framework)
        displayThumbnail(thumbUrl, fullUrl);
    }
}
```

### Example 2: File Download Handler

```cpp
std::string getDownloadUrl(bosbase::BosBase& pb, 
                          const std::string& recordId, 
                          const std::string& filename) {
    auto record = pb.collection("documents").getOne(recordId);
    
    // Get download URL
    return pb.files->getURL(record, filename, std::nullopt, std::nullopt, true);
}
```

### Example 3: Protected File Viewer

```cpp
#include "bosbase/bosbase.h"
#include "bosbase/error.h"
#include <iostream>

std::string viewProtectedFile(bosbase::BosBase& pb, const std::string& recordId) {
    // Authenticate
    if (!pb.authStore()->isValid()) {
        pb.collection("users").authWithPassword("user@example.com", "password");
    }
    
    // Get record
    auto record = pb.collection("private_docs").getOne(recordId);
    
    // Get token
    std::string token;
    try {
        auto tokenResponse = pb.files->getToken();
        token = tokenResponse["token"].get<std::string>();
    } catch (const ClientResponseError& err) {
        std::cerr << "Failed to get file token: " << err.what() << std::endl;
        return "";
    }
    
    // Get file URL
    std::string fileUrl = pb.files->getURL(record, 
        record["file"].get<std::string>(), 
        std::nullopt, 
        token);
    
    return fileUrl;
}
```

### Example 4: Multiple Files with Thumbnails

```cpp
void displayFileList(bosbase::BosBase& pb, const std::string& recordId) {
    auto record = pb.collection("attachments").getOne(recordId);
    
    nlohmann::json files = record.value("files", nlohmann::json::array());
    
    for (const auto& filename : files) {
        std::string fn = filename.get<std::string>();
        
        // Check if it's an image
        std::string ext = fn.substr(fn.find_last_of(".") + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        bool isImage = (ext == "jpg" || ext == "jpeg" || ext == "png" || 
                       ext == "gif" || ext == "webp");
        
        if (isImage) {
            // Show thumbnail
            std::string thumbUrl = pb.files->getURL(record, fn, "100x100");
            displayThumbnail(thumbUrl);
        } else {
            // Show file icon
            displayFileIcon(fn);
        }
        
        // File name and download link
        std::string downloadUrl = pb.files->getURL(record, fn, 
            std::nullopt, std::nullopt, true);
        displayDownloadLink(fn, downloadUrl);
    }
}
```

### Example 5: Image Upload Preview with Thumbnail

```cpp
std::string previewUploadedImage(bosbase::BosBase& pb, 
                                 const nlohmann::json& record, 
                                 const std::string& filename) {
    // Get thumbnail for preview
    std::string previewUrl = pb.files->getURL(record, filename, "200x200f");
    
    return previewUrl;
}
```

## Error Handling

```cpp
#include "bosbase/error.h"

try {
    auto record = pb.collection("posts").getOne("RECORD_ID");
    std::string fileUrl = pb.files->getURL(record, record["image"].get<std::string>());
    
    // Verify URL is valid
    if (fileUrl.empty()) {
        throw std::runtime_error("Invalid file URL");
    }
    
    // Use the URL (fetch file, display, etc.)
    std::cout << "File URL: " << fileUrl << std::endl;
    
} catch (const ClientResponseError& err) {
    std::cerr << "File access error: " << err.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}
```

### Protected File Token Error Handling

```cpp
std::string getProtectedFileUrl(bosbase::BosBase& pb, 
                                const nlohmann::json& record, 
                                const std::string& filename) {
    try {
        // Get token
        auto tokenResponse = pb.files->getToken();
        std::string token = tokenResponse["token"].get<std::string>();
        
        // Get file URL
        return pb.files->getURL(record, filename, std::nullopt, token);
        
    } catch (const ClientResponseError& err) {
        if (err.status() == 401) {
            std::cerr << "Not authenticated" << std::endl;
            // Redirect to login or re-authenticate
        } else if (err.status() == 403) {
            std::cerr << "No permission to access file" << std::endl;
        } else {
            std::cerr << "Failed to get file token: " << err.what() << std::endl;
        }
        return "";
    }
}
```

## Best Practices

1. **Use Thumbnails for Lists**: Use thumbnails when displaying images in lists/grids to reduce bandwidth
2. **Lazy Loading**: Load images on-demand when possible
3. **Cache Tokens**: Store file tokens and reuse them until they expire
4. **Error Handling**: Always handle file loading errors gracefully
5. **Content-Type**: Let the server handle content-type detection automatically
6. **Range Requests**: The API supports Range requests for efficient video/audio streaming
7. **Caching**: Files are cached with a 30-day cache-control header
8. **Security**: Always use tokens for protected files, never expose them in client-side code

## Thumbnail Size Guidelines

| Use Case | Recommended Size |
|----------|-----------------|
| Profile picture | `100x100` or `150x150` |
| List thumbnails | `200x200` or `300x300` |
| Card images | `400x400` or `500x500` |
| Gallery previews | `300x300f` (fit) or `400x400f` |
| Hero images | Use original or `800x800f` |
| Avatar | `50x50` or `75x75` |

## Limitations

- **Thumbnails**: Only work for image files (PNG, JPG, JPEG, GIF, WEBP)
- **Protected Files**: Require authentication to get tokens
- **Token Expiry**: File tokens expire after a short period (typically minutes)
- **File Size**: Large files may take time to generate thumbnails on first request
- **Thumb Sizes**: Must match sizes defined in field configuration or use default `100x100`

## Related Documentation

- [Files Upload and Handling](./FILES.md) - Uploading and managing files
- [API Records](./API_RECORDS.md) - Working with records
- [Collections](./COLLECTIONS.md) - Collection configuration

