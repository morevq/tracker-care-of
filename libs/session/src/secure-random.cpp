#include "tracker_session/secure-random.h"

#include <cstdint>
#include <stdexcept>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <stdlib.h> // arc4random_buf for BSD and macOS
#else
#include <sys/random.h>
#include <errno.h>
#include <unistd.h>
#endif

namespace tracker_session {

    static void fillSecureRandom(uint8_t* data, std::size_t len) {
#if defined(_WIN32)
        if (BCryptGenRandom(nullptr, data, static_cast<ULONG>(len), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
            throw std::runtime_error("BCryptGenRandom failed");
        }
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
        arc4random_buf(data, len);
#else
        std::size_t off = 0;
        while (off < len) {
            ssize_t r = ::getrandom(data + off, len - off, 0);
            if (r < 0) {
                if (errno == EINTR) continue;
                throw std::runtime_error("getrandom failed");
            }
            off += static_cast<std::size_t>(r);
        }
#endif
    }

    static std::string toHexLower(const uint8_t* data, std::size_t len) {
        static const char* kHex = "0123456789abcdef";
        std::string out;
        out.resize(len * 2);
        for (std::size_t i = 0; i < len; ++i) {
            out[2 * i] = kHex[(data[i] >> 4) & 0xF];
            out[2 * i + 1] = kHex[data[i] & 0xF];
        }
        return out;
    }

    std::string secureRandomHex(std::size_t byteLen) {
        if (byteLen == 0) throw std::invalid_argument("byteLen must be > 0");
        std::vector<uint8_t> buf(byteLen);
        fillSecureRandom(buf.data(), buf.size());
        return toHexLower(buf.data(), buf.size());
    }

}
