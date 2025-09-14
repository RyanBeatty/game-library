#include "base.h"

void __attribute__((noinline)) breakpoint() {}

byte* PlatformAllocate(isize cap) {
    byte* buffer = (byte*)malloc(cap * sizeof(byte));
    Assert(buffer != NULL);
    return buffer;
}

void* ArenaAlloc(Arena* a, isize count, isize size, isize alignment) {
    isize padding = -(uptr)a->beg & (alignment - 1);
    isize nbytes = count * size;
    // I think <= is appropriate here. if remaining = 1 and we want to allocate 1, then we should be able to do so
    Assert(count <= (a->end - a->beg - padding) / size);
    void* ptr = a->beg + padding;
    a->beg += padding + nbytes;
    return memset(ptr, 0, nbytes);
};

Arena NewArena(byte* buffer, isize cap) {
    Arena a = {0};
    a.beg = buffer;
    Assert(a.beg != NULL);
    a.end = a.beg + cap;
    return a;
}
