#pragma once

#include <array>
#include <string_view>


#define ENUM_BEGIN(NAME) enum class NAME
#define ENUM_END(NAME)

// Helper to extract enum name at compile-time
#define ENUM_NAME(NAME) #NAME

// Compile-time enum-to-string conversion
#define ENUM_TO_STRING(NAME, ...)                                      \
constexpr std::string_view NAME##ToStr(NAME value) {                 \
switch (value) {                                                   \
__VA_ARGS__                                                      \
default: return "Unknown";                                       \
}                                                                  \
}

// Helper to create string cases
#define ENUM_CASE(VALUE) case VALUE: return #VALUE;
