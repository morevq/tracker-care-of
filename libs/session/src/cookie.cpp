#include "tracker_session/cookie.h"

#include <cctype>
#include <sstream>

namespace tracker_session::cookie {

    static inline void ltrim(std::string_view& s) {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.remove_prefix(1);
    }

    std::optional<std::string> getCookie(std::string_view cookieHeader, std::string_view name) {
        std::string_view s = cookieHeader;
        while (!s.empty()) {
            auto semi = s.find(';');
            std::string_view part = (semi == std::string_view::npos) ? s : s.substr(0, semi);
            s = (semi == std::string_view::npos) ? std::string_view{} : s.substr(semi + 1);

            ltrim(part);
            if (part.size() < name.size() + 1) continue;

            auto eq = part.find('=');
            if (eq == std::string_view::npos) continue;

            std::string_view k = part.substr(0, eq);
            std::string_view v = part.substr(eq + 1);

            while (!k.empty() && std::isspace(static_cast<unsigned char>(k.back()))) k.remove_suffix(1);

            if (k == name) {
                return std::string(v);
            }
        }
        return std::nullopt;
    }

    std::string buildSetCookie(
        const std::string& name,
        const std::string& value,
        int maxAgeSeconds,
        bool httpOnly,
        bool secure,
        const std::string& sameSite,
        const std::string& path
    ) {
        std::ostringstream oss;
        oss << name << "=" << value;
        oss << "; Path=" << path;

        if (maxAgeSeconds >= 0) {
            oss << "; Max-Age=" << maxAgeSeconds;
        }

        if (httpOnly) oss << "; HttpOnly";
        if (secure)   oss << "; Secure";

        if (!sameSite.empty()) {
            oss << "; SameSite=" << sameSite;
        }

        return oss.str();
    }

    std::string buildDeleteCookie(
        const std::string& name,
        bool httpOnly,
        bool secure,
        const std::string& sameSite,
        const std::string& path
    ) {
        return buildSetCookie(name, "", 0, httpOnly, secure, sameSite, path);
    }

}
