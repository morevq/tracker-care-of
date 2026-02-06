#pragma once
#include <optional>
#include <string>
#include <string_view>

namespace tracker_session::cookie {

    std::optional<std::string> getCookie(std::string_view cookieHeader, std::string_view name);

    std::string buildSetCookie(
        const std::string& name,
        const std::string& value,
        int maxAgeSeconds,
        bool httpOnly,
        bool secure,
        const std::string& sameSite,   // "Strict" / "Lax" / "None"
        const std::string& path = "/"
    );

    std::string buildDeleteCookie(
        const std::string& name,
        bool httpOnly,
        bool secure,
        const std::string& sameSite,
        const std::string& path = "/"
    );

} 
