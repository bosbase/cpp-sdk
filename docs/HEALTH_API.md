# Health API - C++ SDK Documentation

## Overview

The Health API provides a simple endpoint to check the health status of the server. It returns basic health information and, when authenticated as a superuser, provides additional diagnostic information about the server state.

**Key Features:**
- No authentication required for basic health check
- Superuser authentication provides additional diagnostic data
- Lightweight endpoint for monitoring and health checks
- Supports both GET and HEAD methods

**Backend Endpoints:**
- `GET /api/health` - Check health status
- `HEAD /api/health` - Check health status (HEAD method)

**Note**: The health endpoint is publicly accessible, but superuser authentication provides additional information.

## Authentication

Basic health checks do not require authentication:

```cpp
#include "bosbase/bosbase.h"

bosbase::BosBase pb("http://127.0.0.1:8090");

// Basic health check (no auth required)
auto health = pb.health->check();
```

For additional diagnostic information, authenticate as a superuser:

```cpp
// Authenticate as superuser for extended health data
pb.collection("_superusers").authWithPassword("admin@example.com", "password");
auto health = pb.health->check();
```

## Health Check Response Structure

### Basic Response (Guest/Regular User)

```cpp
{
  "code": 200,
  "message": "API is healthy.",
  "data": {}
}
```

### Superuser Response

```cpp
{
  "code": 200,
  "message": "API is healthy.",
  "data": {
    "canBackup": boolean,           // Whether backup operations are allowed
    "realIP": string,               // Real IP address of the client
    "requireS3": boolean,           // Whether S3 storage is required
    "possibleProxyHeader": string   // Detected proxy header (if behind reverse proxy)
  }
}
```

## Check Health Status

Returns the health status of the API server.

### Basic Usage

```cpp
#include "bosbase/bosbase.h"
#include <iostream>

int main() {
    bosbase::BosBase pb("http://127.0.0.1:8090");
    
    // Simple health check
    auto health = pb.health->check();
    
    std::cout << "Message: " << health["message"] << std::endl;
    std::cout << "Code: " << health["code"] << std::endl;
    
    return 0;
}
```

### With Superuser Authentication

```cpp
// Authenticate as superuser first
pb.collection("_superusers").authWithPassword("admin@example.com", "password");

// Get extended health information
auto health = pb.health->check();

if (health["data"].contains("canBackup")) {
    std::cout << "Can Backup: " << health["data"]["canBackup"] << std::endl;
}
if (health["data"].contains("realIP")) {
    std::cout << "Real IP: " << health["data"]["realIP"] << std::endl;
}
if (health["data"].contains("requireS3")) {
    std::cout << "Require S3: " << health["data"]["requireS3"] << std::endl;
}
if (health["data"].contains("possibleProxyHeader")) {
    std::cout << "Proxy Header: " << health["data"]["possibleProxyHeader"] << std::endl;
}
```

## Response Fields

### Common Fields (All Users)

| Field | Type | Description |
|-------|------|-------------|
| `code` | number | HTTP status code (always 200 for healthy server) |
| `message` | string | Health status message ("API is healthy.") |
| `data` | object | Health data (empty for non-superusers, populated for superusers) |

### Superuser-Only Fields (in `data`)

| Field | Type | Description |
|-------|------|-------------|
| `canBackup` | boolean | `true` if backup/restore operations can be performed, `false` if a backup/restore is currently in progress |
| `realIP` | string | The real IP address of the client (useful when behind proxies) |
| `requireS3` | boolean | `true` if S3 storage is required (local fallback disabled), `false` otherwise |
| `possibleProxyHeader` | string | Detected proxy header name (e.g., "X-Forwarded-For", "CF-Connecting-IP") if the server appears to be behind a reverse proxy, empty string otherwise |

## Use Cases

### 1. Basic Health Monitoring

```cpp
#include "bosbase/bosbase.h"
#include "bosbase/error.h"
#include <iostream>

bool checkServerHealth(bosbase::BosBase& pb) {
    try {
        auto health = pb.health->check();
        
        if (health["code"] == 200 && health["message"] == "API is healthy.") {
            std::cout << "✓ Server is healthy" << std::endl;
            return true;
        } else {
            std::cout << "✗ Server health check failed" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "✗ Health check error: " << e.what() << std::endl;
        return false;
    }
}

// Use in monitoring loop
void monitorHealth(bosbase::BosBase& pb) {
    while (true) {
        bool isHealthy = checkServerHealth(pb);
        if (!isHealthy) {
            std::cerr << "Server health check failed!" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
}
```

### 2. Backup Readiness Check

```cpp
bool canPerformBackup(bosbase::BosBase& pb) {
    try {
        // Authenticate as superuser
        pb.collection("_superusers").authWithPassword("admin@example.com", "password");
        
        auto health = pb.health->check();
        
        if (health["data"].contains("canBackup") && 
            health["data"]["canBackup"] == false) {
            std::cout << "⚠️ Backup operation is currently in progress" << std::endl;
            return false;
        }
        
        std::cout << "✓ Backup operations are allowed" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to check backup readiness: " << e.what() << std::endl;
        return false;
    }
}

// Use before creating backups
if (canPerformBackup(pb)) {
    pb.backups->create("backup.zip");
}
```

### 3. Monitoring Dashboard

```cpp
#include <map>
#include <string>

struct HealthStatus {
    bool healthy;
    std::string message;
    std::string timestamp;
    std::map<std::string, std::string> diagnostics;
};

HealthStatus getHealthStatus(bosbase::BosBase& pb, bool isSuperuser) {
    HealthStatus status;
    status.healthy = false;
    
    try {
        auto health = pb.health->check();
        
        status.healthy = health["code"] == 200;
        status.message = health["message"].get<std::string>();
        
        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        status.timestamp = std::ctime(&time);
        
        if (isSuperuser && health["data"].is_object() && !health["data"].empty()) {
            if (health["data"].contains("canBackup")) {
                status.diagnostics["canBackup"] = 
                    health["data"]["canBackup"].dump();
            }
            if (health["data"].contains("realIP")) {
                status.diagnostics["realIP"] = 
                    health["data"]["realIP"].get<std::string>();
            }
            if (health["data"].contains("requireS3")) {
                status.diagnostics["requireS3"] = 
                    health["data"]["requireS3"].dump();
            }
            if (health["data"].contains("possibleProxyHeader")) {
                std::string header = health["data"]["possibleProxyHeader"].get<std::string>();
                status.diagnostics["behindProxy"] = header.empty() ? "false" : "true";
                status.diagnostics["proxyHeader"] = header;
            }
        }
    } catch (const std::exception& e) {
        status.message = std::string("Error: ") + e.what();
    }
    
    return status;
}
```

### 4. Load Balancer Health Check

```cpp
bool simpleHealthCheck(bosbase::BosBase& pb) {
    try {
        auto health = pb.health->check();
        return health["code"] == 200;
    } catch (const std::exception& e) {
        return false;
    }
}
```

### 5. Proxy Detection

```cpp
struct ProxyInfo {
    bool behindProxy;
    std::string proxyHeader;
    std::string realIP;
};

ProxyInfo checkProxySetup(bosbase::BosBase& pb) {
    ProxyInfo info;
    info.behindProxy = false;
    
    try {
        pb.collection("_superusers").authWithPassword("admin@example.com", "password");
        
        auto health = pb.health->check();
        auto data = health["data"];
        
        if (data.contains("possibleProxyHeader")) {
            info.proxyHeader = data["possibleProxyHeader"].get<std::string>();
            info.behindProxy = !info.proxyHeader.empty();
            
            if (info.behindProxy) {
                std::cout << "⚠️ Server appears to be behind a reverse proxy" << std::endl;
                std::cout << "   Detected proxy header: " << info.proxyHeader << std::endl;
            }
        }
        
        if (data.contains("realIP")) {
            info.realIP = data["realIP"].get<std::string>();
            std::cout << "   Real IP: " << info.realIP << std::endl;
        }
        
        if (!info.behindProxy) {
            std::cout << "✓ No reverse proxy detected (or properly configured)" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to check proxy setup: " << e.what() << std::endl;
    }
    
    return info;
}
```

### 6. Pre-Flight Checks

```cpp
struct PreFlightChecks {
    bool serverHealthy;
    bool canBackup;
    bool storageConfigured;
    std::vector<std::string> issues;
};

PreFlightChecks preFlightCheck(bosbase::BosBase& pb) {
    PreFlightChecks checks;
    checks.serverHealthy = false;
    checks.canBackup = false;
    checks.storageConfigured = false;
    
    try {
        // Basic health check
        auto health = pb.health->check();
        checks.serverHealthy = health["code"] == 200;
        
        if (!checks.serverHealthy) {
            checks.issues.push_back("Server health check failed");
            return checks;
        }
        
        // Authenticate as superuser for extended checks
        try {
            pb.collection("_superusers").authWithPassword("admin@example.com", "password");
            
            auto detailedHealth = pb.health->check();
            auto data = detailedHealth["data"];
            
            if (data.contains("canBackup")) {
                checks.canBackup = data["canBackup"] == true;
            }
            
            if (data.contains("requireS3")) {
                checks.storageConfigured = data["requireS3"] == false;
            } else {
                checks.storageConfigured = true; // Assume configured if not required
            }
            
            if (!checks.canBackup) {
                checks.issues.push_back("Backup operations are currently unavailable");
            }
            
            if (data.contains("requireS3") && data["requireS3"] == true) {
                checks.issues.push_back("S3 storage is required but may not be configured");
            }
        } catch (const std::exception& authError) {
            checks.issues.push_back("Superuser authentication failed - limited diagnostics available");
        }
    } catch (const std::exception& e) {
        checks.issues.push_back(std::string("Health check error: ") + e.what());
    }
    
    return checks;
}
```

## Error Handling

```cpp
#include "bosbase/error.h"

struct HealthCheckResult {
    bool success;
    nlohmann::json data;
    std::string error;
    int code;
};

HealthCheckResult safeHealthCheck(bosbase::BosBase& pb) {
    HealthCheckResult result;
    result.success = false;
    result.code = 0;
    
    try {
        auto health = pb.health->check();
        result.success = true;
        result.data = health;
    } catch (const ClientResponseError& err) {
        result.error = err.what();
        result.code = err.status();
    } catch (const std::exception& e) {
        result.error = e.what();
    }
    
    return result;
}

// Handle different error scenarios
auto result = safeHealthCheck(pb);
if (!result.success) {
    if (result.code == 0) {
        std::cerr << "Network error or server unreachable" << std::endl;
    } else {
        std::cerr << "Server returned error: " << result.code << std::endl;
    }
}
```

## Best Practices

1. **Monitoring**: Use health checks for regular monitoring (e.g., every 30-60 seconds)
2. **Load Balancers**: Configure load balancers to use the health endpoint for health checks
3. **Pre-flight Checks**: Check `canBackup` before initiating backup operations
4. **Error Handling**: Always handle errors gracefully as the server may be down
5. **Rate Limiting**: Don't poll the health endpoint too frequently (avoid spamming)
6. **Caching**: Consider caching health check results for a few seconds to reduce load
7. **Logging**: Log health check results for troubleshooting and monitoring
8. **Alerting**: Set up alerts for consecutive health check failures
9. **Superuser Auth**: Only authenticate as superuser when you need diagnostic information
10. **Proxy Configuration**: Use `possibleProxyHeader` to detect and configure reverse proxy settings

## Response Codes

| Code | Meaning |
|------|---------|
| 200 | Server is healthy |
| Network Error | Server is unreachable or down |

## Limitations

- **No Detailed Metrics**: The health endpoint does not provide detailed performance metrics
- **Basic Status Only**: Returns basic status, not detailed system information
- **Superuser Required**: Extended diagnostics require superuser authentication
- **No Historical Data**: Only returns current status, no historical health data

## Related Documentation

- [Backups API](./BACKUPS_API.md) - Using `canBackup` to check backup readiness
- [Authentication](./AUTHENTICATION.md) - Superuser authentication

