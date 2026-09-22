// #include <stdint.h>
#include <stdbool.h>
// #include <stdio.h>
// #include <sys/mman.h>
// #include <string.h>

#define size_t unsigned int
#define uint64_t size_t
#define uint32_t size_t

__attribute__((always_inline)) void __spatial_runtime_check(void *addr, uint64_t size, void *lower_bound, void *upper_bound) {
    // printf("SAN: __spatial_runtime_check(addr=%p (size=%lu), bound=%p..%p)\n", addr, size, lower_bound, upper_bound);
    if (addr < lower_bound || addr + size > upper_bound) {
        // fprintf(stderr, "WARNING: __spatial_runtime_check(addr=%p (size=%lu), bound=%p..%p)\r", addr, size, lower_bound, upper_bound);
    }
}

#define METADATA_SIZE_P2 5
#define ADDRESS_WIDTH 48
#define POINTER_ALIGNMENT_P2 3
#define STAGE1_BITS 24
#define STAGE2_BITS (ADDRESS_WIDTH - POINTER_ALIGNMENT_P2 - STAGE1_BITS)
#define STAGE1_ENTRIES (1ull << STAGE1_BITS)
#define STAGE2_ENTRIES (1ull << STAGE2_BITS)
#define INDEX_STAGE1(addr) (((uint64_t)(addr) >> (POINTER_ALIGNMENT_P2 + STAGE2_BITS)) & ((1ull << STAGE1_BITS) - 1))
#define INDEX_STAGE2(addr) (((uint64_t)(addr) >> POINTER_ALIGNMENT_P2) & ((1ull << STAGE2_BITS) - 1))

#define LIKELY(x)      __builtin_expect(!!(x), 1)
#define UNLIKELY(x)    __builtin_expect(!!(x), 0)

struct metadata {
    void* data[4];
};

// repr(Rust)
struct location {
    char *file_str;
    size_t str_size;
    uint32_t line;
    uint32_t col;
};

bool __instrument_enable = false;
int __instrument_err_count = 0;
static bool __instrument_printing = false;

static __attribute__((always_inline)) size_t min_usize(size_t x, size_t y) {
    return x < y ? x : y;
}

void __aster_print(struct location *location, void *p, size_t req_size, void *base, size_t base_size);
__attribute__((weak)) void __aster_print(struct location *location, void *p, size_t req_size, void *base, size_t base_size) {}
void __aster_print_memmove(void *dst, void *src, size_t size);
__attribute__((weak)) void __aster_print_memmove(void *dst, void *src, size_t size) {}

void __instrument_check_fail(struct location *location, void *p, size_t req_size, void *base, size_t base_size) {
    if (__instrument_printing)
        return;
    // __instrument_printing = true;
    // __aster_print(location, p, req_size, base, base_size);
    // __instrument_printing = false;

// void __instrument_check_fail() {
    if (__instrument_enable) {
        // fprintf(stderr, "Check Failed: %p+0x%lx not in %p+0x%lx, src: ", p, req_size, base, base_size);
        // if (location) {
        //     fwrite(location->file_str, sizeof(char), location->str_size, stderr);
        //     fprintf(stderr, ":%d:%d\n", location->line, location->col);
        // } else {
        //     fprintf(stderr, "unknown\n");
        // }
    }
    __instrument_err_count ++;

    // __builtin_trap();
}

__attribute__((noinline)) void __instrument_on_access_hook(const char *location) {
    asm volatile("" : : "m"(location) : "memory");
}

static int rng() {
    static unsigned seed = 10086;
    seed = (seed * 1103515245 + 12345) & ((1 << 31) - 1);
    return seed;
}

size_t __instrument_histogram[64] = {0};
void __instrument_on_access(const char *location, void *p, size_t old_elapsed, size_t now_elapsed) {
    if (old_elapsed < 0x7FFFFFFFFFFFFFFF) {
        size_t log = __builtin_clzll(now_elapsed - old_elapsed + 1);
        __instrument_histogram[63 - log]++;
        if (log >= 10 && (rng() & 0xFFF) == 0)
            __instrument_on_access_hook(location);
    }
}

static __attribute__((always_inline)) struct metadata* get_shadow_addr_inner(void *addr) {
    return (void*)(( (unsigned long)addr - 0x0 ) * 4 + 0x8000000);

    // static struct metadata dummy;

    // const unsigned long OFFSET_NULL = 0xffffffffffffffff;
    // unsigned long offset = OFFSET_NULL;
    // if (0xffff800000000000 <= (unsigned long)addr && (unsigned long)addr < 0xffff800100000000)       // linear mapping area
    //     offset = ((unsigned long)addr - 0xffff800000000000) * 4;
    // else if (0xffffffff88000000 <= (unsigned long)addr && (unsigned long)addr < 0xffffffff8a000000)  // kernel binary
    //     offset = ((unsigned long)addr - 0xffffffff80000000) * 4;
    // else if (0xffffe00000000000 <= (unsigned long)addr && (unsigned long)addr < 0xfffff00000000000)  // meta pages [Move]
    //     offset = ((unsigned long)addr - 0xffffe00000000000) * 4 + 0x51028000;
    // else if (0xffffdc0000000000 <= (unsigned long)addr && (unsigned long)addr < 0xffffe00000000000)  // kernel stack [Move]
    //     offset = ((unsigned long)addr - 0xffffdc0000000000) * 4 + 0x100000000000;

    // if (offset != OFFSET_NULL)
    //     return (void*)(0xffff900000000000 + offset);

    // return &dummy;
}

__attribute__((always_inline)) void* __instrument_get_shadow_addr(void *addr, void *inner) {
    struct metadata *data = get_shadow_addr_inner(addr);

    if (data->data[2] != inner) {
        data->data[0] = (void*)0;
        data->data[1] = (void*)0xFFFFFFFFFFFFFFFFull;
        data->data[2] = inner;
        // data->data[3] = (void*)0xFFFFFFFFFFFFFFFFull;
    }

    return (void*)data;
}

extern unsigned long SHADOW_MEMORY_LOADED;
void __instrument_memmove(void **dst, void **src, size_t len) {
    // if (!__instrument_printing && ((unsigned long)dst < 0xFFFFFFFF00000000ull || (unsigned long)src < 0xFFFFFFFF00000000ull)) {
    //     __instrument_printing = true;
    //     __aster_print_memmove(dst, src, len);
    //     __instrument_printing = false;
    // }

    void *dst_shadow = get_shadow_addr_inner(dst);
    void *src_shadow = get_shadow_addr_inner(src);

    __builtin_memmove(dst_shadow, src_shadow, len << 2);

    return;
}
