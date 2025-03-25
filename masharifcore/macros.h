

#include <array>
#include <string_view>

#define NAMESPACE masharif
// Platform-specific visibility settings
#ifdef _WIN32
#define EXPORT_ENUM __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
  #define EXPORT_ENUM __attribute__((visibility("default")))
#else
  #define EXPORT_ENUM
#endif

// Enum declaration macros (uses scoped `enum class`)
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


#define STYLE_AWARE_STRUCT(StructName, ...)                 \
struct StructName {                                     \
__VA_ARGS__                                         \
std::function<void()> markDirtyCallback;            \
StructName(std::function<void()> cb = nullptr)      \
: markDirtyCallback(std::move(cb)) {}          \
void markDirty() { if (markDirtyCallback) markDirtyCallback(); } \
};
