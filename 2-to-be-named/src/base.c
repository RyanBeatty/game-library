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
    a.beg_orig = a.beg;
    return a;
}

void ArenaPrintInfo(Arena* arena) {
    char buffer[1024] = {0};
    snprintf(buffer, ArraySizeOf(buffer) - 1, "Arena:\n* Free: %lld", arena->end - arena->beg);
    PlatformLog(LLOG_INFO, "%s", buffer);
}

Arena NewSubArena(Arena* perm, isize cap) {
    Arena arena = {0};
    arena.beg = New(perm, cap, byte);
    arena.end = arena.beg + cap;
    arena.beg_orig = arena.beg;
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
    for (; i < a.len && a.data[i] == b.data[i]; ++i);
    return i == a.len;
}

static f32 F32FromStr8(Str8 s, bool* ok) {
    char buffer[256] = {0};
    if (s.len >= ArraySizeOf(buffer)) {
        *ok = false;
        return 0.0f;
    }

    memcpy(buffer, s.data, sizeof(char) * s.len);
    char* end = NULL;
    f32 f = strtof(buffer, &end);

    // 1. No conversion
    // 2. Range errors
    // 3. Optional: must consume entire string
    if (end == buffer || errno == ERANGE || *end != '\0') {
        *ok = false;
        return 0.0f;
    }

    *ok = true;
    return f;
}

static i32 I32FromStr8(Str8 s, bool* ok) {
    char buffer[256] = {0};
    if (s.len >= ArraySizeOf(buffer)) {
        *ok = false;
        return 0;
    }

    memcpy(buffer, s.data, sizeof(char) * s.len);
    char* end = NULL;
    i64 i = strtol(buffer, &end, 10);

    // 1. No conversion
    // 2. Range errors
    // 3. Optional: must consume entire string
    if (end == buffer || errno == ERANGE || *end != '\0' || i > I32_MAX || i < I32_MIN) {
        *ok = false;
        return 0;
    }

    *ok = true;
    return i;
}

static bool Str8Split1(Str8 s, char sep, Str8* before, Str8* after) {
    if (s.len == 0) {
        *before = s;
        after->data = NULL;
        after->len = 0;
        return false;
    }

    isize offset = 0;
    for (; offset < s.len && s.data[offset] != sep; ++offset);
    // sep not found
    if (offset >= s.len) {
        *before = s;
        after->data = NULL;
        after->len = 0;
        return false;
    }

    // sep found but is last element so after is empty
    if (offset + 1 == s.len) {
        before->data = s.data;
        before->len = offset;
        after->data = NULL;
        after->len = 0;
        return true;
    }

    // sep found and not last element, so after is not empty
    before->data = s.data;
    before->len = offset;
    after->data = &s.data[offset + 1];
    after->len = s.len - (offset + 1);
    return true;
}
