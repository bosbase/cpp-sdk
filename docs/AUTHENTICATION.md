# Authentication - C++ SDK Documentation

## Overview

Authentication in BosBase is stateless and token-based. A client is considered authenticated as long as it sends a valid `Authorization: YOUR_AUTH_TOKEN` header with requests.

**Key Points:**
- **No sessions**: BosBase APIs are fully stateless (tokens are not stored in the database)
- **No logout endpoint**: To "logout", simply clear the token from your local state (`pb.authStore()->clear()`)
- **Token generation**: Auth tokens are generated through auth collection Web APIs or programmatically
- **Admin users**: `_superusers` collection works like regular auth collections but with full access (API rules are ignored)
- **OAuth2 limitation**: OAuth2 is not supported for `_superusers` collection

## Authentication Methods

BosBase supports multiple authentication methods that can be configured individually for each auth collection:

1. **Password Authentication** - Email/username + password
2. **OTP Authentication** - One-time password via email
3. **OAuth2 Authentication** - Google, GitHub, Microsoft, etc.
4. **Multi-factor Authentication (MFA)** - Requires 2 different auth methods

## Authentication Store

The SDK maintains an `authStore` that automatically manages the authentication state:

```cpp
#include "bosbase/bosbase.h"
using namespace bosbase;

BosBase pb("http://localhost:8090");

// Check authentication status
std::cout << "Is valid: " << pb.authStore()->isValid() << std::endl;
std::cout << "Token: " << pb.authStore()->token() << std::endl;
std::cout << "Record: " << pb.authStore()->record().dump() << std::endl;

// Clear authentication (logout)
pb.authStore()->clear();
```

## Password Authentication

Authenticate using email/username and password. The identity field can be configured in the collection options (default is email).

**Backend Endpoint:** `POST /api/collections/{collection}/auth-with-password`

### Basic Usage

```cpp
#include "bosbase/bosbase.h"
using namespace bosbase;

BosBase pb("http://localhost:8090");

// Authenticate with email and password
auto authData = pb.collection("users").authWithPassword(
    "test@example.com",
    "password123"
);

// Auth data is automatically stored in pb.authStore()
std::cout << "Is valid: " << pb.authStore()->isValid() << std::endl;
std::cout << "Token: " << pb.authStore()->token() << std::endl;
std::cout << "User ID: " << pb.authStore()->record()["id"] << std::endl;
```

### Response Format

```cpp
// authData contains:
// {
//   "token": "eyJhbGciOiJIUzI1NiJ9...",
//   "record": {
//     "id": "record_id",
//     "email": "test@example.com",
//     // ... other user fields
//   }
// }
```

### Error Handling with MFA

```cpp
#include "bosbase/error.h"
#include <stdexcept>

try {
    pb.collection("users").authWithPassword("test@example.com", "pass123");
} catch (const ClientResponseError& err) {
    // Check for MFA requirement
    if (err.data().contains("mfaId")) {
        auto mfaId = err.data()["mfaId"].get<std::string>();
        // Handle MFA flow (see Multi-factor Authentication section)
    } else {
        std::cerr << "Authentication failed: " << err.what() << std::endl;
    }
}
```

## OTP Authentication

One-time password authentication via email.

**Backend Endpoints:**
- `POST /api/collections/{collection}/request-otp` - Request OTP
- `POST /api/collections/{collection}/auth-with-otp` - Authenticate with OTP

### Request OTP

```cpp
// Send OTP to user's email
auto result = pb.collection("users").requestOTP("test@example.com");
std::cout << "OTP ID: " << result["otpId"] << std::endl;  // OTP ID to use in authWithOTP
```

### Authenticate with OTP

```cpp
// Step 1: Request OTP
auto result = pb.collection("users").requestOTP("test@example.com");

// Step 2: User enters OTP from email
auto authData = pb.collection("users").authWithOTP(
    result["otpId"].get<std::string>(),
    "123456"  // OTP code from email
);
```

## OAuth2 Authentication

**Backend Endpoint:** `POST /api/collections/{collection}/auth-with-oauth2`

### Manual Code Exchange

```cpp
// Get auth methods
auto authMethods = pb.collection("users").listAuthMethods();
// Find provider in authMethods["oauth2"]["providers"]

// Exchange code for token (after OAuth2 redirect)
auto authData = pb.collection("users").authWithOAuth2Code(
    provider["name"].get<std::string>(),
    code,
    provider["codeVerifier"].get<std::string>(),
    redirectUrl
);
```

## Multi-Factor Authentication (MFA)

Requires 2 different auth methods.

```cpp
std::string mfaId;

try {
    // First auth method (password)
    pb.collection("users").authWithPassword("test@example.com", "pass123");
} catch (const ClientResponseError& err) {
    if (err.data().contains("mfaId")) {
        mfaId = err.data()["mfaId"].get<std::string>();
        
        // Second auth method (OTP)
        auto otpResult = pb.collection("users").requestOTP("test@example.com");
        pb.collection("users").authWithOTP(
            otpResult["otpId"].get<std::string>(),
            "123456",
            nlohmann::json{{"mfaId", mfaId}}  // Pass MFA ID
        );
    }
}
```

## User Impersonation

Superusers can impersonate other users.

**Backend Endpoint:** `POST /api/collections/{collection}/impersonate/{id}`

```cpp
// Authenticate as superuser
pb.collection("_superusers").authWithPassword("admin@example.com", "adminpass");

// Impersonate a user
auto impersonateClient = pb.collection("users").impersonate(
    "USER_RECORD_ID",
    3600  // Optional: token duration in seconds
);

// Use impersonate client
auto data = impersonateClient.collection("posts").getFullList();
```

## Auth Token Verification

Verify token by calling `authRefresh()`.

**Backend Endpoint:** `POST /api/collections/{collection}/auth-refresh`

```cpp
try {
    auto authData = pb.collection("users").authRefresh();
    std::cout << "Token is valid" << std::endl;
} catch (const ClientResponseError& err) {
    std::cerr << "Token verification failed: " << err.what() << std::endl;
    pb.authStore()->clear();
}
```

## List Available Auth Methods

**Backend Endpoint:** `GET /api/collections/{collection}/auth-methods`

```cpp
auto authMethods = pb.collection("users").listAuthMethods();
std::cout << "Password enabled: " << authMethods["password"]["enabled"] << std::endl;
std::cout << "OAuth2 providers: " << authMethods["oauth2"]["providers"].dump() << std::endl;
std::cout << "MFA enabled: " << authMethods["mfa"]["enabled"] << std::endl;
```

## Complete Examples

### Example 1: Complete Authentication Flow with Error Handling

```cpp
#include "bosbase/bosbase.h"
#include "bosbase/error.h"
#include <iostream>
using namespace bosbase;

nlohmann::json authenticateUser(BosBase& pb, const std::string& email, const std::string& password) {
    try {
        // Try password authentication
        auto authData = pb.collection("users").authWithPassword(email, password);
        
        std::cout << "Successfully authenticated: " << authData["record"]["email"] << std::endl;
        return authData;
        
    } catch (const ClientResponseError& err) {
        // Check if MFA is required
        if (err.status() == 401 && err.data().contains("mfaId")) {
            std::cout << "MFA required, proceeding with second factor..." << std::endl;
            return handleMFA(pb, email, err.data()["mfaId"].get<std::string>());
        }
        
        // Handle other errors
        if (err.status() == 400) {
            throw std::runtime_error("Invalid credentials");
        } else if (err.status() == 403) {
            throw std::runtime_error("Password authentication is not enabled for this collection");
        } else {
            throw;
        }
    }
}

nlohmann::json handleMFA(BosBase& pb, const std::string& email, const std::string& mfaId) {
    // Request OTP for second factor
    auto otpResult = pb.collection("users").requestOTP(email);
    
    // In a real app, show a modal/form for the user to enter OTP
    // For this example, we'll simulate getting the OTP
    std::string userEnteredOTP = getUserOTPInput(); // Your UI function
    
    try {
        // Authenticate with OTP and MFA ID
        auto authData = pb.collection("users").authWithOTP(
            otpResult["otpId"].get<std::string>(),
            userEnteredOTP,
            nlohmann::json{{"mfaId", mfaId}}
        );
        
        std::cout << "MFA authentication successful" << std::endl;
        return authData;
    } catch (const ClientResponseError& err) {
        if (err.status() == 429) {
            throw std::runtime_error("Too many OTP attempts, please request a new OTP");
        }
        throw std::runtime_error("Invalid OTP code");
    }
}

// Usage
int main() {
    BosBase pb("http://localhost:8090");
    try {
        authenticateUser(pb, "user@example.com", "password123");
        std::cout << "User is authenticated: " << pb.authStore()->record().dump() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Authentication failed: " << e.what() << std::endl;
    }
    return 0;
}
```

### Example 2: Token Management and Refresh

> **BosBase note:** Calls to `pb.collection("users").authWithPassword()` now return static, non-expiring tokens. Environment variables can no longer shorten their lifetime, so the refresh logic below is only required for custom auth collections, impersonation flows, or any token you mint manually.

```cpp
#include "bosbase/bosbase.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace bosbase;

bool checkAuth(BosBase& pb) {
    if (pb.authStore()->isValid()) {
        std::cout << "User is authenticated: " << pb.authStore()->record()["email"] << std::endl;
        
        // Verify token is still valid and refresh if needed
        try {
            pb.collection("users").authRefresh();
            std::cout << "Token refreshed successfully" << std::endl;
            return true;
        } catch (const ClientResponseError& err) {
            std::cout << "Token expired or invalid, clearing auth" << std::endl;
            pb.authStore()->clear();
            return false;
        }
    }
    return false;
}

// Usage
int main() {
    BosBase pb("http://localhost:8090");
    if (checkAuth(pb)) {
        // User is authenticated
    } else {
        // Redirect to login
    }
    return 0;
}
```

### Example 3: Admin Impersonation for Support

```cpp
#include "bosbase/bosbase.h"
#include <iostream>
using namespace bosbase;

nlohmann::json impersonateUserForSupport(BosBase& pb, const std::string& userId) {
    // Authenticate as admin
    pb.collection("_superusers").authWithPassword("admin@example.com", "adminpassword");
    
    // Impersonate the user (1 hour token)
    auto userClient = pb.collection("users").impersonate(userId, 3600);
    
    std::cout << "Impersonating user: " << userClient.authStore()->record()["email"] << std::endl;
    
    // Use the impersonated client to test user experience
    auto userRecords = userClient.collection("posts").getFullList();
    std::cout << "User can see " << userRecords.size() << " posts" << std::endl;
    
    // Check what the user sees
    auto userView = userClient.collection("posts").getList(1, 10, false,
        std::map<std::string, nlohmann::json>{},
        std::map<std::string, std::string>{},
        "published = true"
    );
    
    return nlohmann::json{
        {"canAccess", userView["items"].size()},
        {"totalPosts", userRecords.size()}
    };
}

// Usage in support dashboard
int main() {
    BosBase pb("http://localhost:8090");
    try {
        auto result = impersonateUserForSupport(pb, "user_record_id");
        std::cout << "User access check: " << result.dump() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Impersonation failed: " << e.what() << std::endl;
    }
    return 0;
}
```

## Best Practices

1. **Secure Token Storage**: Never expose tokens in client-side code or logs
2. **Token Refresh**: Implement automatic token refresh before expiration
3. **Error Handling**: Always handle MFA requirements and token expiration
4. **OAuth2 Security**: Always validate the `state` parameter in OAuth2 callbacks
5. **API Keys**: Use impersonation tokens for server-to-server communication only
6. **Superuser Tokens**: Never expose superuser impersonation tokens in client code
7. **OTP Security**: Use OTP with MFA for security-critical applications
8. **Rate Limiting**: Be aware of rate limits on authentication endpoints

## Troubleshooting

### Token Expired
If you get 401 errors, check if the token has expired:
```cpp
try {
    pb.collection("users").authRefresh();
} catch (const ClientResponseError& err) {
    // Token expired, require re-authentication
    pb.authStore()->clear();
    // Redirect to login
}
```

### MFA Required
If authentication returns 401 with mfaId:
```cpp
if (err.status() == 401 && err.data().contains("mfaId")) {
    // Proceed with second authentication factor
}
```

## Related Documentation

- [Collections](./COLLECTIONS.md)
- [API Rules](./API_RULES_AND_FILTERS.md)

