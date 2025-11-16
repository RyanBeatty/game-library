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

Arena NewSubArena(Arena* perm, isize cap) {
    Arena arena = {0};
    arena.beg = New(perm, cap, byte);
    arena.end = arena.beg + cap;
    return arena;
}

/////////////////////////////////////////////////////////////////////////
// section:Str8

static Str8 Str8FromByteBuffer(ByteBuffer buffer) {
    Str8 str = {.data = (char*)buffer.data, .len = buffer.len};
    return str;
}

static bool Str8Eq(Str8 a, Str8 b) {
    if (a.len != b.len) {
        return false;
    }

    isize i = 0;
    isize j = 0;
    while (i < a.len && a.data[i++] == b.data[j++]);
    return i == a.len;
}

static bool Str8Split1(Str8 s, char sep, Str8* before, Str8* after) {
    return false;
    // isize i = 0;
    // for (; i < s.len && s.data[i] != sep; ++i);

    // if (i == s.len) {
    //     *before = s;
    //     *after = (Str8){.data = NULL, .len = 0};
    //     return false;
    // } else {
    //     *befoer
    // }
}
