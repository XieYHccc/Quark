#pragma once

#include <stddef.h>
#include <stdexcept>
#include <new>

namespace quark::util
{
void* memalign_alloc(size_t boundary, size_t size);
void* memalign_calloc(size_t boundary, size_t size);
void memalign_free(void *ptr);

}