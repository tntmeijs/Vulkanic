#define VMA_IMPLEMENTATION

#ifdef WIN32
#pragma warning(push, 4)
#pragma warning(disable: 4127) // conditional expression is constant
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4189) // local variable is initialized but not referenced
#include <vk_mem_alloc.h>
#pragma warning(pop)
#endif // WIN32

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare" // comparison of unsigned expression < 0 is always false
#include <vk_mem_alloc.h>
#pragma clang diagnostic pop
#endif // __clang__
