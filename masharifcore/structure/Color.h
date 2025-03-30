#ifndef COLOR_H
#define COLOR_H
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <masharifcore/macros.h>

namespace
_NAMESPACE {
    class Color {
    public:
        uint8_t r, g, b, a;

        // Default constructor (black, fully opaque)
        Color() : r(0), g(0), b(0), a(255) {
        }

        // RGB/A constructor
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {
        }

        // Constructor from hexadecimal string
        explicit Color(std::string_view hex) {
            setFromHex(hex);
        }

        // Constructor from 32-bit RGBA value
        explicit Color(uint32_t rgba) {
            r = (rgba >> 24) & 0xFF;
            g = (rgba >> 16) & 0xFF;
            b = (rgba >> 8) & 0xFF;
            a = rgba & 0xFF;
        }

        // Convert to hexadecimal string
        [[nodiscard]] std::string toHex(bool uppercase = false, bool force_alpha = false) const {
            std::ostringstream ss;
            ss << "#";
            if (uppercase) ss << std::uppercase;
            ss << std::hex << std::setfill('0');

            ss << std::setw(2) << static_cast<int>(r)
                    << std::setw(2) << static_cast<int>(g)
                    << std::setw(2) << static_cast<int>(b);

            if (force_alpha || a != 255) {
                ss << std::setw(2) << static_cast<int>(a);
            }

            return ss.str();
        }

        // Set color from hexadecimal string
        void setFromHex(std::string_view hex) {
            if (hex.empty() || hex[0] != '#') {
                throw std::invalid_argument("Hex string must start with '#'");
            }

            const size_t length = hex.length();
            const size_t num_digits = length - 1;

            // Validate length and characters
            const bool is_shorthand = num_digits == 3 || num_digits == 4;
            const bool is_full = num_digits == 6 || num_digits == 8;
            if (!is_shorthand && !is_full) {
                throw std::invalid_argument("Invalid length. Use #RGB, #RGBA, #RRGGBB, or #RRGGBBAA.");
            }

            for (size_t i = 1; i < length; ++i) {
                if (!std::isxdigit(hex[i])) {
                    throw std::invalid_argument("Contains non-hex characters");
                }
            }

            // Expand shorthand notation
            std::string expanded;
            if (is_shorthand) {
                expanded.reserve(num_digits * 2);
                for (size_t i = 1; i < length; ++i) {
                    expanded += hex[i];
                    expanded += hex[i];
                }
                hex = expanded;
            }

            // Parse components
            size_t pos = 1;
            auto parse = [&] {
                uint8_t val = static_cast<uint8_t>(std::stoi(std::string(hex.substr(pos, 2)), nullptr, 16));
                pos += 2;
                return val;
            };

            r = parse();
            g = parse();
            b = parse();
            a = (hex.length() - pos >= 2) ? parse() : 255;
        }

        // Equality operators
        bool operator==(const Color &other) const {
            return other.r == r && other.g == g && other.b == b && other.a == a;
        };

        bool operator!=(const Color &other) const {
            return !operator==(other);
        };

        // Utility methods
        [[nodiscard]] uint32_t toRGBA() const {
            return (r << 24) | (g << 16) | (b << 8) | a;
        }

        [[nodiscard]] Color withAlpha(uint8_t alpha) const {
            return {r, g, b, alpha};
        }
    };
}
#endif //COLOR_H
