#ifndef UNITY_BUILD
#include "game.h"

#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <platform.h>
#include <shared.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "base.h"
#include "glad.h"
#include "stb_truetype.h"
#endif

////////////////////////////////////////////////////////////
// S:Color

typedef Vec4 RGBA;

#define RGBALit(r, g, b, a) (Vec4Lit((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a)))
#define RGBLit(r, g, b) (RGBALit((r), (g), (b), 1.0f))

static const Vec4 LCLEAR = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f);
static const Vec4 LRED = Vec4Lit(1.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LGREEN = Vec4Lit(0.0f, 1.0f, 0.0f, 1.0f);
static const Vec4 LBLUE = Vec4Lit(0.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LBLACK = Vec4Lit(0.0f, 0.0f, 0.0f, 1.0f);
static const Vec4 LPURPLE = Vec4Lit(1.0f, 0.0f, 1.0f, 1.0f);
static const Vec4 LORANGE = Vec4Lit(1.0f, 0.5f, 0.0f, 1.0f);
static const Vec4 LGRAY = Vec4Lit(199.0f / 255.0f, 197.0f / 255.0f, 198.0f / 255.0f, 1.0f);
static const Vec4 LYELLOW = Vec4Lit(1.0f, 1.0f, 0.0f, 1.0f);
static const Vec4 LWHITE = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f);

// Lerp towards white.
static Vec4 Lighten(Vec4 color, f32 t) {
    return (Vec4){
        .r = (1.0f - t) * color.r + t,
        .g = (1.0f - t) * color.g + t,
        .b = (1.0f - t) * color.b + t,
        .a = (1.0f - t) * color.a + t,
    };
}

static Vec4 Darken(Vec4 color, f32 t) {
    return (Vec4){
        .r = (1.0f - t) * color.r,
        .g = (1.0f - t) * color.g,
        .b = (1.0f - t) * color.b,
        .a = (1.0f - t) * color.a,
    };
}

////////////////////////////////////////////////////////////
// S:Render Data

typedef struct {
    Vec3 pos;
    Vec3 normal;
    Vec2 uv;
} Vertex;

typedef struct {
    Vertex* data;
    isize len;
    isize cap;
} VertexArr;

typedef struct {
    union {
        u32 vs[3];
        struct {
            u32 va;
            u32 vb;
            u32 vc;
        };
    };
} Triangle;

#define TriLit(n0, n1, n2) ((Triangle){.va = (n0), .vb = (n1), .vc = (n2)})

static u32 VAO = 0;
static u32 VBO = 0;
static u32 VIO = 0;  // Index buffer.

#define MAX_VERTICES (1 << 14)
static VertexArr VBUFFER = {0};

typedef struct {
    u32* data;
    isize len;
    isize cap;
} U32Arr;

#define MAX_TRIANGLES (1 << 14)
static U32Arr IBUFFER = {0};

typedef struct {
    u32 ibufferidx;  // Offset into index buffer
    u32 ntriangles;
} LMesh;

typedef struct {
    LMesh* data;
    isize len;
    isize cap;
} LMeshArr;

static LMeshArr MESHES = {0};

typedef enum MID {
    M_CUBE = 0,
    M_QUAD,
    M_ISO_TRIANGLE,
    M_PYRAMID,
    M_2S_ISO_TRIANGLE,
    M_ICOSAHEDRON,
    M_ICOSPHERE,
    M_HELLO_CUBE,
    M_HELLO_MONKEY,
    M_TRACK1,
    M_COUNT
} MID;

typedef u32 TextureId;

typedef struct {
    TextureId id;
    i32 width;
    i32 height;

    byte* data;
} LTexture;

typedef enum {
    FILTER_NONE = 0,
    FILTER_NEAREST = GL_NEAREST,
    FILTER_LINEAR = GL_LINEAR,
} LTextureFilter;

typedef struct {
    union {
        u32 rgba;

        struct {
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        };
    };
} RGBA256;

LCORE RGBA256 LTextureGetPx(LTexture tx, i32 x, i32 y) {
    RGBA256 rgba = {0};
    // Remember the size in bytes for each cell is sizeof(u32)
    i32 offset = (tx.width * y * sizeof(u32)) + (x * sizeof(u32));
    rgba.r = tx.data[offset + 0];
    rgba.g = tx.data[offset + 1];
    rgba.b = tx.data[offset + 2];
    rgba.a = tx.data[offset + 3];
    return rgba;
}

// :data - flattened 2d array of rgba data (e.g. 4 bytes, 1 byte per channel).
// :width - length of width of buffer in rgba units.
// :height - length of height of buffer in rgba units.
// So size of buffer should be (width * 4 bytes) * (height * 4 bytes)
// (0, 0) in uv space maps to the bottom right corner of the texture visually which maps to
// the start of the buffer.
// NOTE: We take ownership of data here.
LCORE LTexture TextureFromBuffer(byte* data, i32 width, i32 height, LTextureFilter filter) {
    LTexture tx = {0};
    tx.width = width;
    tx.height = height;
    tx.data = data;
    glGenTextures(1, &tx.id);
    glBindTexture(GL_TEXTURE_2D, tx.id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tx.width, tx.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tx.data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    // Clamp to edge can be useful to avoid texture bleeding at edges when sampling
    // from a texture atlas.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tx;
}

static LTexture TextureFromFile(const char* filename, LTextureFilter filter) {
    LTexture tx = {0};
    i32 n = 0;
    // TODO: Change the allocator on stb.
    stbi_set_flip_vertically_on_load(true);
    u8* data = stbi_load(filename, &tx.width, &tx.height, &n, 0);
    Assert(data != NULL);

    // The texture takes ownership of the data.
    tx = TextureFromBuffer(data, tx.width, tx.height, filter);
    // stbi_image_free(data);
    return tx;
}

#define MAX_TEXTURES 32
typedef struct {
    u32 txlen;
    u32 txcap;
    LTexture textures[MAX_TEXTURES];
} TextureCache;

static TextureCache TCACHE;

typedef enum TextureLabel { TX_DEFAULT = 0, TX_COUNT } TextureLabel;

static LTexture GetTexture(TextureLabel label) {
    TextureCache* tcache = &TCACHE;
    return tcache->textures[label];
}

#define TextureBottomLeft(tx) (Vec3Lit(-0.5f * (tx).width, -0.5f * (tx).height, 0.0f))

// Address center, and all 4 corners of an aseprite pixel. We consider the aseprite coord
// given in the UI to be the center of the pixel.
#define AspritePxCtr(px) (px)
#define AsepritePxBL(px) (Vec2Lit((px).x - 0.5f, (px).y + 0.5f))
#define AsepritePxBR(px) (Vec2Lit((px).x + 0.5f, (px).y + 0.5f))
#define AsepritePxTL(px) (Vec2Lit((px).x - 0.5f, (px).y - 0.5f))
#define AsepritePxTR(px) (Vec2Lit((px).x + 0.5f, (px).y - 0.5f))

// This essentially moves the origin from aseprite top left corner to bottom right corner. Why do we do this?
// Its because when we import the texture/png we flip the Y axis there. subtract 1 from height so that the
// max height aseprite pixel coords get mapped to 0
#define AsepritePosToFlipped(tx, apos) (Vec2Lit((apos).x, (tx).height - 1.0f - (apos).y))

// Conver the position in aseprite space to world space. aseprite space is the coordinate given by
// aseprite UI.
static Vec2 AsepritePosToWorldPos(LTexture tx, Vec2 pos_aseprite) {
    pos_aseprite = AsepritePosToFlipped(tx, pos_aseprite);

    // The origin in world space of the texture is at its center. We want to find the offset from the origin in
    // aseprite space (which is the center of the bottom left pixel since we flip the Y coords) to the center of
    // the texture itself. Subtract 1 from width/height because we want to find the midpoint between the bottem left
    // pixel's center and the top right pixel's center.
    Vec2 origin_offset = Vec2Lit(-0.5f * (tx.width - 1.0f), -0.5f * (tx.height - 1.0f));
    Vec2 pos_world = Vec2Add(pos_aseprite, origin_offset);
    return pos_world;
}

typedef struct {
    LTexture tx;

    // The min and max coordinate in image coordinates [0, with], [0, height]
    Vec2 min;
    Vec2 max;
    Vec2 dims;

    // UV space coordinates for drawing (computed once when loaded)
    Vec2 minuv;
    Vec2 dimsuv;
} Sprite;

#define MAX_SPRITES (32)

typedef struct {
    // TODO: Figure out memory later if we really need to make this dynamic. Remember that
    // audio memory takes up so much that it might not make sense to focus on optimizing this.
    Sprite data[MAX_SPRITES];
    isize len;
    isize cap;
} SpriteArr;

typedef enum {
    SBB_FULL = 0,  // bb = width, height of texture
    SBB_CLOSEST_FIT,
} SpriteBoundingBox;

LCORE Sprite SpriteFromMinMax(LTexture tx, Vec2 min, Vec2 max) {
    Sprite sprite = {.tx = tx, .min = min, .max = max};
    // think more about if adding 1,1 here is correct. I think it is.
    sprite.dims = Vec2Sub(Vec2Add(sprite.max, Vec2Lit(1.0f, 1.0f)), sprite.min);
    sprite.minuv = Vec2Lit(sprite.min.x / sprite.tx.width, sprite.min.y / sprite.tx.height);
    sprite.dimsuv = Vec2Lit(sprite.dims.x / sprite.tx.width, sprite.dims.y / sprite.tx.height);
    return sprite;
}

LCORE Sprite SpriteFromLTexture(LTexture tx, SpriteBoundingBox bbox) {
    Vec2 min = {0};
    Vec2 max = {0};
    switch (bbox) {
        case SBB_FULL: {
            min = Vec2Zero();
            max = Vec2Lit(tx.width - 1, tx.height - 1);
            break;
        }
        case SBB_CLOSEST_FIT: {
            // Init the min to be the max px index of the texture. If we don't do this, min will always be (0, 0).
            min = Vec2Lit(tx.width - 1, tx.height - 1);
            for (i32 i = 0; i < tx.width; ++i) {
                for (i32 j = 0; j < tx.height; ++j) {
                    Vec2 pos = Vec2Lit(i, j);
                    RGBA256 rgba = LTextureGetPx(tx, i, j);
                    if (rgba.a != 0.0f) {
                        min.x = f32min(min.x, pos.x);
                        min.y = f32min(min.y, pos.y);
                        max.x = f32max(max.x, pos.x);
                        max.y = f32max(max.y, pos.y);
                    }
                }
            }
            break;
        }
        default: {
            PlatformLog(LLOG_ERROR, "Unknown sprite bounding box %d", bbox);
            Assert(0);
        }
    }

    Sprite sprite = SpriteFromMinMax(tx, min, max);
    return sprite;
}

LCORE Sprite DefaultSprite() { return SpriteFromLTexture(GetTexture(TX_DEFAULT), SBB_FULL); }

// This extra layer of indirection might be redundant. But possible we will want to store other data here, so
// leave for now.
typedef struct {
    SpriteArr sprites;
    // For time based animations, this will be elapsed time in seconds for event based animations (like
    // walk cycles), this will be in range 0-1
    f32 duration;
    bool loops;
} Animation;

LCORE Animation AnimationFromSprite(Sprite sprite) {
    Animation animation = {.duration = 0.0f};
    animation.sprites.cap = ArraySizeOf(animation.sprites.data);
    ArrPush(&animation.sprites, sprite);
    animation.loops = true;
    return animation;
}

LCORE Animation DefaultAnimation() { return AnimationFromSprite(DefaultSprite()); }

// TODO: This should be some sort of platform layer thing.
i32 CountFilesInDir(const char* path) {
    DIR* dir;
    struct dirent* entry;
    i32 count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        PlatformLog(LLOG_ERROR, "error opening dir: %s", path);
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (entry->d_name[0] == '.' &&
            (entry->d_name[1] == '\0' || (entry->d_name[1] == '.' && entry->d_name[2] == '\0'))) {
            continue;
        }

        count++;
    }

    closedir(dir);
    return count;
}

// Assumes that all files under :dir: follow the format of :filename_prefixNNNNN.png where NNNNN
// is a 0 left padded number denoting the frame.
LCORE Animation AnimationFromDisk(const char* dir, const char* filename_prefix, f32 duration) {
    Animation animation = {.duration = duration};
    animation.sprites.cap = ArraySizeOf(animation.sprites.data);

    SpriteArr* sprites = &animation.sprites;
    i32 nfiles = CountFilesInDir(dir);

    LTextureFilter filter = FILTER_LINEAR;
    for (isize i = 0; i < nfiles; ++i) {
        char filename[256] = {0};
        snprintf(filename, ArraySizeOf(filename) - 1, "%s/%s%05lld.png", dir, filename_prefix, i);
        LTexture tx = TextureFromFile(filename, filter);
        ArrPush(sprites, SpriteFromLTexture(tx, SBB_FULL));
    }

    animation.loops = true;
    return animation;
}

static void TexturesInit() {
    TextureCache* tcache = &TCACHE;
    tcache->txlen = 0;
    tcache->txcap = MAX_TEXTURES;

    // Default black texture.
    {
        Assert(tcache->txlen < tcache->txcap);

        LTexture tx = {0};
        tx.width = 1;
        tx.height = 1;
        glGenTextures(1, &tx.id);
        glBindTexture(GL_TEXTURE_2D, tx.id);

        // The texture will be all white. Important to do this so that we can properly blend
        // texture colors and uniform frag color.
        GLubyte transparentPixel[] = {255, 255, 255, 255};
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, transparentPixel);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        tcache->textures[TX_DEFAULT] = tx;
        ++tcache->txlen;
    }

    ++tcache->txlen;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// S:noise

// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
u32 PCGHash(u32 x) {
    u32 state = x * 747796405u + 2891336453u;
    u32 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

u32 PCGHash2D(u32 x, u32 y) {
    u32 seed = (x * 1610612741u) + (y * 1791552031u);
    return PCGHash(seed);
}

// Generates a whitenoise texture with pixel dimension width x height
LCORE LTexture GenerateWhitenoiseTexture(Arena* temp, i32 width, i32 height) {
    ByteBuffer buffer = {.cap = width * height * 4};  // * 4 is because we store rgba data
    buffer.data = New(temp, buffer.cap, byte);

    for (i32 i = 0; i < width; ++i) {
        for (i32 j = 0; j < height; ++j) {
            u32 rand = PCGHash((i * width) + j);
            // shift noise from [0-1] with some precision loss potentially due to large float division.
            f32 noise = ((f32)rand) / ((f32)U32_MAX);
            // shift noise from [0-1] to [0-255]
            i32 rgb = (i32)(noise * 255.0f);
            PushArrS(&buffer, rgb);
            PushArrS(&buffer, rgb);
            PushArrS(&buffer, rgb);
            PushArrS(&buffer, 255);
        }
    }

    return TextureFromBuffer(buffer.data, width, height, FILTER_NEAREST);
}

LCORE LTexture GenValueNoiseTexture(Arena* temp, i32 width, i32 height, i32 cell_size) {
    ByteBuffer buffer = {.cap = width * height * 4};
    buffer.data = New(temp, buffer.cap, byte);

    for (i32 i = 0; i < width; ++i) {
        for (i32 j = 0; j < height; ++j) {
            // Determine bottom left corner coordinates based on lattice size.
            u32 x = i / (u32)cell_size;
            u32 y = j / (u32)cell_size;

            f32 bl = (f32)PCGHash2D(x, y) / ((f32)U32_MAX);
            f32 br = (f32)PCGHash2D(x + cell_size, y) / ((f32)U32_MAX);
            f32 tl = (f32)PCGHash2D(x, y + cell_size) / ((f32)U32_MAX);
            f32 tr = (f32)PCGHash2D(x + cell_size, y + cell_size) / ((f32)U32_MAX);

            // Determine the percentage within the cell we are in x and y direction, that
            // is our lerp factor.
            f32 tx = ((f32)(i - x) / cell_size);
            f32 ty = ((f32)(j - y) / cell_size);

            // bilinear lerp between the bottom and top lerped values.
            // Shift noise range from [0, 1], to [0, 255]
            f32 l1 = f32lerp(bl, br, tx);
            f32 l2 = f32lerp(tl, tr, tx);
            u32 noise = (u32)(f32lerp(l1, l2, ty) * 255.0f);

            // // Find bottom left corner uv.
            // Vec2 bl = Vec2Lit((f32)i / (f32)width, (f32)j / (f32)height);
            // Vec2 br = Vec2Add(bl, Vec2Lit(1.0f, 0.0f));
            // Vec2 tl = Vec2Add(bl, Vec2Lit(0.0f, 1.0f));
            // Vec2 tr = Vec2Add(bl, Vec2Lit(1.0f, 1.0f));

            PushArrS(&buffer, noise);
            PushArrS(&buffer, noise);
            PushArrS(&buffer, noise);
            PushArrS(&buffer, 255);
        }
    }

    return TextureFromBuffer(buffer.data, width, height, FILTER_NEAREST);
}

///////////////////////////////////////////////////////////////////////////////
// S:render

typedef struct {
    u32 fbid;        // framebuffer id
    TextureId txid;  // texture id
    u32 width;
    u32 height;
} RenderTarget;

static RenderTarget CreateRenderTarget(u32 width, u32 height) {
    u32 fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    u32 txid;
    glGenTextures(1, &txid);
    glBindTexture(GL_TEXTURE_2D, txid);

    // Init blank texture and add as the color attachment to the framebuffer
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    // TODO: Unclear to me if we want linear or nearest here. pixel art seems to look better with nearest
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, txid, 0);

    // Create a render buffer for our framebuffer for stencil/depth testing.
    u32 rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    // Bind default framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Note: This texture is not in the texture cache.
    return (RenderTarget){.fbid = fbo, .txid = txid, .width = width, .height = height};
}

static LMesh GetMesh(u32 mid) { return ArrGet(&MESHES, mid); }

static LMesh AddMesh(Vec3* vs, u32 vsize, Triangle* ts, u32 tsize) {
    u32 voffset = ArrLen(&VBUFFER);
    u32 ioffset = ArrLen(&IBUFFER);

    Assert(tsize * 3 < 1024);
    Vec3 ns[1024] = {0};
    for (u32 i = 0; i < tsize; ++i) {
        Triangle t = ts[i];
        Vec3 a = vs[t.va];
        Vec3 b = vs[t.vb];
        Vec3 c = vs[t.vc];
        Vec3 n = Vec3Normalize(Vec3Cross(Vec3Sub(b, a), Vec3Sub(c, a)));

        ns[t.va] = n;
        ns[t.vb] = n;
        ns[t.vc] = n;

        ArrPush(&IBUFFER, voffset + t.va);
        ArrPush(&IBUFFER, voffset + t.vb);
        ArrPush(&IBUFFER, voffset + t.vc);
    }

    for (u32 i = 0; i < vsize; ++i) {
        Vertex* vertex = ArrReserve(&VBUFFER);
        vertex->pos = vs[i];
        vertex->normal = ns[i];
        vertex->uv = Vec2Zero();
    }

    LMesh mesh = {0};
    mesh.ibufferidx = ioffset;
    mesh.ntriangles = tsize;
    return mesh;
}

static LMesh AddMesh2(Vec3* vs, u32 vsize, Vec2* uvs, u32 uvsize, Triangle* ts, u32 tsize) {
    Assert(vsize == uvsize);

    u32 voffset = ArrLen(&VBUFFER);
    u32 ioffset = ArrLen(&IBUFFER);

    Assert(tsize * 3 < 1024);
    Vec3 ns[1024] = {0};
    for (u32 i = 0; i < tsize; ++i) {
        Triangle t = ts[i];
        Vec3 a = vs[t.va];
        Vec3 b = vs[t.vb];
        Vec3 c = vs[t.vc];
        Vec3 n = Vec3Normalize(Vec3Cross(Vec3Sub(b, a), Vec3Sub(c, a)));

        ns[t.va] = n;
        ns[t.vb] = n;
        ns[t.vc] = n;

        ArrPush(&IBUFFER, voffset + t.va);
        ArrPush(&IBUFFER, voffset + t.vb);
        ArrPush(&IBUFFER, voffset + t.vc);
    }

    for (u32 i = 0; i < vsize; ++i) {
        Vertex* vertex = ArrReserve(&VBUFFER);
        vertex->pos = vs[i];
        vertex->normal = ns[i];
        vertex->uv = uvs[i];
    }

    LMesh mesh = {0};
    mesh.ibufferidx = ioffset;
    mesh.ntriangles = tsize;
    return mesh;
}

// Is this really a lexer? I dunno.
typedef struct {
    Str8 s;
    isize offset;
} Str8Parser;

#define Str8ParserIsEmpty(p) ((p)->offset >= (p)->s.len)

static void Str8ParserConsumeWhitespace(Str8Parser* parser) {
    for (; parser->offset < parser->s.len && parser->s.data[parser->offset] == ' '; ++parser->offset);
}

// If we have reached eof, will return a str with 0 len.
static Str8 Str8ParserNextToken(Str8Parser* parser) {
    Str8ParserConsumeWhitespace(parser);

    Str8 token = {.data = &parser->s.data[parser->offset], .len = 0};
    isize begin = parser->offset;
    for (; parser->offset < parser->s.len && parser->s.data[parser->offset] != ' ' &&
           parser->s.data[parser->offset] != '\r' && parser->s.data[parser->offset] != '\n';
         ++parser->offset);
    token.len = parser->offset - begin;
    return token;
}

static void Str8ParserConsumeLine(Str8Parser* parser) {
    // Consume rest of remaining non line ending characters.
    for (; parser->offset < parser->s.len && parser->s.data[parser->offset] != '\r' &&
           parser->s.data[parser->offset] != '\n';
         ++parser->offset);
    // At this point we have either reached end of line or end of file. Try to consume line endings.
    for (; parser->offset < parser->s.len &&
           (parser->s.data[parser->offset] == '\r' || parser->s.data[parser->offset] == '\n');
         ++parser->offset);
    return;
}

typedef struct {
    i32 face[3][3];
} OBJFace;

typedef struct {
    OBJFace* data;
    isize len;
    isize cap;
} OBJFaceArr;

static LMesh LoadMeshFromOBJ(Arena* perm, const char* filename) {
    Vec3Arr verts = {.cap = 2048};
    verts.data = New(perm, verts.cap, Vec3);

    // TODO: This is probably over allocating slightly since likely each vertex won't have a unique norm.
    Vec3Arr norms = {.cap = 1024};
    norms.data = New(perm, norms.cap, Vec3);

    Vec2Arr uvs = {.cap = 1024};
    uvs.data = New(perm, uvs.cap, Vec2);

    OBJFaceArr faces = {.cap = 4096};
    faces.data = New(perm, faces.cap, OBJFace);

    Arena temp = *perm;
    ByteBuffer buffer = PlatformReadFileBytes(&temp, filename);

    Str8 str = Str8FromByteBuffer(buffer);
    buffer = (ByteBuffer){0};

    u32 object_group_count = 0;

    Str8Parser parser = {.s = str};
    while (!Str8ParserIsEmpty(&parser)) {
        Str8 peeks = Str8ParserNextToken(&parser);

        if (Str8Eq(peeks, Str8Lit("#"))) {
            Str8ParserConsumeLine(&parser);
        } else if (Str8Eq(peeks, Str8Lit("mtllib"))) {
            PlatformLog(LLOG_DEBUG, "Ignoring material line directive %.*s", (int)peeks.len, (char*)peeks.data);
            Str8ParserConsumeLine(&parser);
        } else if (Str8Eq(peeks, Str8Lit("o"))) {
            if (object_group_count > 1) {
                PlatformLog(LLOG_FATAL,
                            "Cannot support loading OBJ files with multiple object groupings. New object group: %.*s",
                            (int)peeks.len, (char*)peeks.data);
            }
            ++object_group_count;

            Str8ParserConsumeLine(&parser);
        } else if (Str8Eq(peeks, Str8Lit("v"))) {
            Vec3 v = {0};
            // Parse vert coords
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.x = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse vertex coordinate x from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.y = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse vertex coordinate y from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.z = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse vertex coordinate z from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            Str8ParserConsumeLine(&parser);

            ArrPush(&verts, v);
        } else if (Str8Eq(peeks, Str8Lit("vn"))) {
            Vec3 v = {0};
            // Parse vert coords
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.x = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse vertex normal x from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.y = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse vertex normal y from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.z = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse vertex normal z from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            Str8ParserConsumeLine(&parser);

            ArrPush(&norms, v);
        } else if (Str8Eq(peeks, Str8Lit("vt"))) {
            Vec2 v = {0};
            // Parse vert coords
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.x = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse texture coordinate x from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            {
                Str8 ftoken = Str8ParserNextToken(&parser);
                bool ok = false;
                v.y = F32FromStr8(ftoken, &ok);
                if (!ok) {
                    PlatformLog(LLOG_FATAL, "Failed to parse texture coordinate y from string: %.*s", (int)ftoken.len,
                                (char*)ftoken.data);
                }
            }
            Str8ParserConsumeLine(&parser);

            ArrPush(&uvs, v);
        } else if (Str8Eq(peeks, Str8Lit("s"))) {
            Str8 itoken = Str8ParserNextToken(&parser);
            bool ok = false;
            i32 smoothing_option = I32FromStr8(itoken, &ok);
            if (!ok) {
                PlatformLog(LLOG_FATAL, "Failed to parse smoothing option value from string: %.*s", (int)itoken.len,
                            (char*)itoken.data);
            }

            if (smoothing_option != 0) {
                PlatformLog(LLOG_FATAL, "Smoothing option with a non value 0 not supported: %d", smoothing_option);
            }

            Str8ParserConsumeLine(&parser);
        } else if (Str8Eq(peeks, Str8Lit("usemtl"))) {
            PlatformLog(LLOG_DEBUG, "Ignoring usemtl line directive");
            Str8ParserConsumeLine(&parser);
        } else if (Str8Eq(peeks, Str8Lit("f"))) {
            OBJFace face = {0};

            for (isize j = 0; j < ArraySizeOf(face.face); ++j) {
                Str8 vtoken = Str8ParserNextToken(&parser);
                for (isize i = 0; i < ArraySizeOf(face.face[j]); ++i) {
                    Str8 before = {0};
                    Str8 after = {0};

                    if (i < 2) {
                        bool found = Str8Split1(vtoken, '/', &before, &after);
                        if (!found) {
                            PlatformLog(LLOG_FATAL, "Did not find '/' while parsing face vertex %d attribute %d: %.*s",
                                        j, i, vtoken.len, vtoken.data);
                        }
                    } else {
                        before = vtoken;
                    }

                    bool ok = false;
                    i32 idx = I32FromStr8(before, &ok);
                    if (!ok) {
                        PlatformLog(LLOG_FATAL, "Failed to parse vertex %d attribute %d: %.*s", j, i, before.len,
                                    before.data);
                    }
                    if (idx <= 0) {
                        PlatformLog(LLOG_FATAL, "OBJ negative face vertex index values not supported: %.*s", before.len,
                                    before.data);
                    }
                    // obj file indexes are 0 based.
                    --idx;
                    switch (i) {
                        case 0: {
                            if (idx >= ArrLen(&verts)) {
                                PlatformLog(LLOG_FATAL,
                                            "Parsed face vertex index number %d is greater than vertex array length",
                                            idx);
                            }
                            break;
                        }
                        case 1: {
                            if (idx >= ArrLen(&uvs)) {
                                PlatformLog(LLOG_FATAL,
                                            "Parsed face uv index number %d is greater than uv array length", idx);
                            }
                            break;
                        }
                        case 2: {
                            if (idx >= ArrLen(&norms)) {
                                PlatformLog(LLOG_FATAL,
                                            "Parsed face normals index number %d is greater than normals array length",
                                            idx);
                            }
                            break;
                        }
                        default: {
                            PlatformLog(LLOG_FATAL, "Unknown iteration number %d while parsing face index", i);
                        }
                    }
                    face.face[j][i] = idx;

                    vtoken = after;
                }
            }
            Str8ParserConsumeLine(&parser);

            ArrPush(&faces, face);
        } else {
            PlatformLog(LLOG_FATAL, "Unexpected peek string while parsing OBJ: %.*s", (int)peeks.len,
                        (char*)peeks.data);
        }
    }

    // Put loaded mesh data into vertex and index buffer + create new lmesh entry.
    {
        u32 ibuffer_base = ArrLen(&IBUFFER);

        for (isize i = 0; i < ArrLen(&faces); ++i) {
            OBJFace* face = ArrGetPtr(&faces, i);

            u32 vbuffer_base = ArrLen(&VBUFFER);

            // Add the triangle verts for the face.
            // TODO: Technically we are a bit wasteful here since we create a new vert for each face vertex which might
            // be sharing info.
            for (isize j = 0; j < 3; ++j) {
                Vertex* vertex = ArrReserve(&VBUFFER);
                vertex->pos = ArrGet(&verts, face->face[j][0]);
                vertex->normal = ArrGet(&norms, face->face[j][1]);
                vertex->uv = ArrGet(&uvs, face->face[j][2]);
            }

            ArrPush(&IBUFFER, vbuffer_base + 0);
            ArrPush(&IBUFFER, vbuffer_base + 1);
            ArrPush(&IBUFFER, vbuffer_base + 2);
        }

        {
            LMesh mesh = {0};
            mesh.ibufferidx = ibuffer_base;
            mesh.ntriangles = ArrLen(&faces);
            return mesh;
        }
    }
}

static void RenderDataInit(Arena* perm) {
    VAO = 0;
    VBO = 0;
    VIO = 0;

    VBUFFER = (VertexArr){.cap = MAX_VERTICES};
    VBUFFER.data = New(perm, VBUFFER.cap, Vertex);

    // Note that Max_triangles is a bit misleading since 3 elements of this buffer make a tri.
    IBUFFER = (U32Arr){.cap = MAX_TRIANGLES};
    IBUFFER.data = New(perm, IBUFFER.cap, u32);

    MESHES = (LMeshArr){.cap = M_COUNT};
    MESHES.data = New(perm, MESHES.cap, LMesh);
    // Reserve all meshes so we can set them by index.
    for (isize i = 0; i < M_COUNT; ++i) {
        ArrReserve(&MESHES);
    }

    // Important!!!! Init the buffer before sending to GPU or else no vertex data!!!!!

    // Cube.
    {
        Vec3 vs[] = {
            // Back.
            Vec3Lit(0.5f, -0.5f, -0.5f),
            Vec3Lit(-0.5f, -0.5f, -0.5f),
            Vec3Lit(-0.5f, 0.5f, -0.5f),
            Vec3Lit(0.5f, 0.5f, -0.5f),

            // Front.
            Vec3Lit(-0.5f, -0.5f, 0.5f),
            Vec3Lit(0.5f, -0.5f, 0.5f),
            Vec3Lit(0.5f, 0.5f, 0.5f),
            Vec3Lit(-0.5f, 0.5f, 0.5f),

            // Left.
            Vec3Lit(-0.5f, 0.5f, 0.5f),
            Vec3Lit(-0.5f, 0.5f, -0.5f),
            Vec3Lit(-0.5f, -0.5f, -0.5f),
            Vec3Lit(-0.5f, -0.5f, 0.5f),

            // Right.
            Vec3Lit(0.5f, 0.5f, -0.5f),
            Vec3Lit(0.5f, 0.5f, 0.5f),
            Vec3Lit(0.5f, -0.5f, 0.5f),
            Vec3Lit(0.5f, -0.5f, -0.5f),

            // Bottom.
            Vec3Lit(-0.5f, -0.5f, -0.5f),
            Vec3Lit(0.5f, -0.5f, -0.5f),
            Vec3Lit(0.5f, -0.5f, 0.5f),
            Vec3Lit(-0.5f, -0.5f, 0.5f),

            // Top.
            Vec3Lit(0.5f, 0.5f, -0.5f),
            Vec3Lit(-0.5f, 0.5f, -0.5f),
            Vec3Lit(-0.5f, 0.5f, 0.5f),
            Vec3Lit(0.5f, 0.5f, 0.5f),
        };
        Triangle ts[] = {
            // Back.
            TriLit(0, 1, 2),  //
            TriLit(2, 3, 0),  //

            // Front.
            TriLit(4, 5, 6),  //
            TriLit(6, 7, 4),  //

            // Left.
            TriLit(8, 9, 10),   //
            TriLit(10, 11, 8),  //

            // Right.
            TriLit(12, 13, 14),  //
            TriLit(14, 15, 12),  //

            // Bottom.
            TriLit(16, 17, 18),  //
            TriLit(18, 19, 16),  //

            // Top.
            TriLit(20, 21, 22),  //
            TriLit(22, 23, 20),  //
        };
        Vec2 uvs[] = {
            // Back.
            Vec2Lit(1.0f, 0.0f),  // 0
            Vec2Lit(0.0f, 0.0f),  // 1
            Vec2Lit(0.0f, 1.0f),  // 2
            Vec2Lit(1.0f, 1.0f),  // 3

            // Front.
            Vec2Lit(0.0f, 0.0f),  // 4
            Vec2Lit(1.0f, 0.0f),  // 5
            Vec2Lit(1.0f, 1.0f),  // 6
            Vec2Lit(0.0f, 1.0f),  // 7

            // Left.
            Vec2Lit(1.0f, 1.0f),  // 8
            Vec2Lit(0.0f, 1.0f),  // 9
            Vec2Lit(0.0f, 0.0f),  // 10
            Vec2Lit(1.0f, 0.0f),  // 11

            // Right.
            Vec2Lit(0.0f, 1.0f),  // 12
            Vec2Lit(1.0f, 1.0f),  // 13
            Vec2Lit(1.0f, 0.0f),  // 14
            Vec2Lit(0.0f, 0.0f),  // 15

            // Bottom.
            Vec2Lit(0.0f, 1.0f),  // 16
            Vec2Lit(1.0f, 1.0f),  // 17
            Vec2Lit(1.0f, 0.0f),  // 18
            Vec2Lit(0.0f, 0.0f),  // 19

            // Top.
            Vec2Lit(1.0f, 1.0f),  // 20
            Vec2Lit(0.0f, 1.0f),  // 21
            Vec2Lit(0.0f, 0.0f),  // 22
            Vec2Lit(1.0f, 0.0f),  // 23
        };
        *ArrGetPtr(&MESHES, M_CUBE) = AddMesh2(vs, ArraySizeOf(vs), uvs, ArraySizeOf(uvs), ts, ArraySizeOf(ts));
    }

    // Quad.
    {
        // quad is aligned with the xy plane. the full width of the quad is 1.0f so scaling by 2 will
        // make the width 2
        Vec3 vs[] = {
            Vec3Lit(-0.5f, 0.5f, 0.0f),
            Vec3Lit(-0.5f, -0.5f, 0.0f),
            Vec3Lit(0.5f, -0.5f, 0.0f),
            Vec3Lit(0.5f, 0.5f, 0.0f),
        };
        Vec2 uvs[] = {
            Vec2Lit(0.0f, 1.0f),
            Vec2Lit(0.0f, 0.0f),
            Vec2Lit(1.0f, 0.0f),
            Vec2Lit(1.0f, 1.0f),
        };
        Triangle ts[] = {
            TriLit(0, 1, 2),  //
            TriLit(2, 3, 0),  //
        };
        *ArrGetPtr(&MESHES, M_QUAD) = AddMesh2(vs, ArraySizeOf(vs), uvs, ArraySizeOf(uvs), ts, ArraySizeOf(ts));
    }

    // ISO TRIANGLE Mesh. triangle with all sides length 1 centered at 0, 0
    {
        Vec3 vs[] = {
            Vec3Lit(0.0f, 0.57735027f, 0.0f),
            Vec3Lit(-0.5f, -0.28867513f, 0.0f),
            Vec3Lit(0.5f, -0.28867513f, 0.0f),
        };
        Vec2 uvs[] = {
            Vec2Lit(0.5f, 1.0f),
            Vec2Lit(0.0f, 0.0f),
            Vec2Lit(1.0f, 0.0f),
        };
        Triangle ts[] = {TriLit(0, 1, 2)};
        *ArrGetPtr(&MESHES, M_ISO_TRIANGLE) = AddMesh2(vs, ArraySizeOf(vs), uvs, ArraySizeOf(uvs), ts, ArraySizeOf(ts));
    }

    // PYRAMID Mesh. centered at 0, 0, 0. all sides 1, forward along -z
    {
        // Vec3 vs[] = {
        //     Vec3Lit(0.0, 0.0, -0.61237243),          //
        //     Vec3Lit(-0.5, -0.28867513, 0.20412415),  //
        //     Vec3Lit(0.5, -0.28867513, 0.20412415),   //
        //                                              //
        //     Vec3Lit(0.0, 0.0, -0.61237243),          //
        //     Vec3Lit(0.5, -0.28867513, 0.20412415),   //
        //     Vec3Lit(0.0, 0.57735027, 0.20412415),    //
        //                                              //
        //     Vec3Lit(0.0, 0.0, -0.61237243),          //
        //     Vec3Lit(0.0, 0.57735027, 0.20412415),    //
        //     Vec3Lit(-0.5, -0.28867513, 0.20412415),  //
        //                                              //
        //     Vec3Lit(-0.5, -0.28867513, 0.20412415),  //
        //     Vec3Lit(0.5, -0.28867513, 0.20412415),   //
        //     Vec3Lit(0.0, 0.57735027, 0.20412415),    //
        // };
        // Triangle ts[] = {
        //     TriLit(0, 1, 2),    //
        //     TriLit(3, 4, 5),    //
        //     TriLit(6, 7, 8),    //
        //     TriLit(9, 10, 11),  //
        // };
        // Vec2 uvs[] = {
        //     Vec2Lit(0.5, 0.8660254),
        //     Vec2Lit(0.0f, 0.0f),
        //     Vec2Lit(1.0f, 0.0f),
        //     //
        //     Vec2Lit(0.5, 0.8660254),
        //     Vec2Lit(0.0f, 0.0f),
        //     Vec2Lit(1.0f, 0.0f),
        //     //
        //     Vec2Lit(0.5, 0.8660254),
        //     Vec2Lit(0.0f, 0.0f),
        //     Vec2Lit(1.0f, 0.0f),
        //     //
        //     Vec2Lit(0.0f, 0.0f),
        //     Vec2Lit(1.0f, 0.0f),
        //     Vec2Lit(0.5, 0.8660254),
        // };
        Vec3 vs[] = {
            // Face 1: A-B-C
            Vec3Normalize(Vec3Lit(0.0, 0.0, -0.35355339)),
            Vec3Normalize(Vec3Lit(-0.35355339, -0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, -0.35355339, 0.35355339)),

            // Face 2: A-C-D
            Vec3Normalize(Vec3Lit(0.0, 0.0, -0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, -0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, 0.35355339, 0.35355339)),

            // Face 3: A-D-E
            Vec3Normalize(Vec3Lit(0.0, 0.0, -0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, 0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(-0.35355339, 0.35355339, 0.35355339)),

            // Face 4: A-E-B
            Vec3Normalize(Vec3Lit(0.0, 0.0, -0.35355339)),
            Vec3Normalize(Vec3Lit(-0.35355339, 0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(-0.35355339, -0.35355339, 0.35355339)),

            // Base: B-C-D-E (split into two triangles)
            Vec3Normalize(Vec3Lit(-0.35355339, -0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, -0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, 0.35355339, 0.35355339)),

            Vec3Normalize(Vec3Lit(-0.35355339, -0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(0.35355339, 0.35355339, 0.35355339)),
            Vec3Normalize(Vec3Lit(-0.35355339, 0.35355339, 0.35355339)),
        };

        Triangle ts[] = {
            TriLit(0, 1, 2),     // A-B-C
            TriLit(3, 4, 5),     // A-C-D
            TriLit(6, 7, 8),     // A-D-E
            TriLit(9, 10, 11),   // A-E-B
            TriLit(12, 13, 14),  // base tri 1
            TriLit(15, 16, 17),  // base tri 2
        };

        Vec2 uvs[] = {
            // Face 1: A-B-C
            Vec2Lit(0.5, 0.8660254),
            Vec2Lit(0.0, 0.0),
            Vec2Lit(1.0, 0.0),

            // Face 2: A-C-D
            Vec2Lit(0.5, 0.8660254),
            Vec2Lit(0.0, 0.0),
            Vec2Lit(1.0, 0.0),

            // Face 3: A-D-E
            Vec2Lit(0.5, 0.8660254),
            Vec2Lit(0.0, 0.0),
            Vec2Lit(1.0, 0.0),

            // Face 4: A-E-B
            Vec2Lit(0.5, 0.8660254),
            Vec2Lit(0.0, 0.0),
            Vec2Lit(1.0, 0.0),

            // Base tri 1: B-C-D
            Vec2Lit(0.0, 0.0),
            Vec2Lit(1.0, 0.0),
            Vec2Lit(1.0, 1.0),

            // Base tri 2: B-D-E
            Vec2Lit(0.0, 0.0),
            Vec2Lit(1.0, 1.0),
            Vec2Lit(0.0, 1.0),
        };
        *ArrGetPtr(&MESHES, M_PYRAMID) = AddMesh2(vs, ArraySizeOf(vs), uvs, ArraySizeOf(uvs), ts, ArraySizeOf(ts));
    }

    // 2S ISO TRIANGLE Mesh.
    {
        Vec3 vs[] = {
            Vec3Lit(1.0f, 0.0f, 0.0f),      //
            Vec3Lit(-0.5f, 0.866f, 0.0f),   //
            Vec3Lit(-0.5f, -0.866f, 0.0f),  //
            Vec3Lit(1.0f, 0.0f, 0.0f),      //
            Vec3Lit(-0.5f, -0.866f, 0.0f),  //
            Vec3Lit(-0.5f, 0.866f, 0.0f),   //
        };
        Triangle ts[] = {
            TriLit(0, 1, 2),  //
            TriLit(3, 4, 5),  //
        };
        *ArrGetPtr(&MESHES, M_2S_ISO_TRIANGLE) = AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // Icosahedron Mesh.
    // https://en.wikipedia.org/wiki/Icosahedron
    {
        // golden ratio.
        f32 t = LPHI;

        Vec3 vs[] = {
            Vec3Normalize(Vec3Lit(-1.0f, t, 0.0f)),  Vec3Normalize(Vec3Lit(1.0f, t, 0.0f)),
            Vec3Normalize(Vec3Lit(-1.0f, -t, 0.0f)), Vec3Normalize(Vec3Lit(1.0f, -t, 0.0f)),
            Vec3Normalize(Vec3Lit(0.0f, -1.0f, t)),  Vec3Normalize(Vec3Lit(0.0f, 1.0f, t)),
            Vec3Normalize(Vec3Lit(0.0f, -1.0f, -t)), Vec3Normalize(Vec3Lit(0.0f, 1.0f, -t)),
            Vec3Normalize(Vec3Lit(t, 0.0f, -1.0f)),  Vec3Normalize(Vec3Lit(t, 0.0f, 1.0f)),
            Vec3Normalize(Vec3Lit(-t, 0.0f, -1.0f)), Vec3Normalize(Vec3Lit(-t, 0.0f, 1.0f)),
        };
        Triangle ts[] = {
            // 5 faces around point 0
            TriLit(0, 11, 5),
            TriLit(0, 5, 1),
            TriLit(0, 1, 7),
            TriLit(0, 7, 10),
            TriLit(0, 10, 11),

            // 5 adjacent faces
            TriLit(1, 5, 9),
            TriLit(5, 11, 4),
            TriLit(11, 10, 2),
            TriLit(10, 7, 6),
            TriLit(7, 1, 8),

            // 5 faces around point 3
            TriLit(3, 9, 4),
            TriLit(3, 4, 2),
            TriLit(3, 2, 6),
            TriLit(3, 6, 8),
            TriLit(3, 8, 9),

            // 5 adjacent faces
            TriLit(4, 9, 5),
            TriLit(2, 4, 11),
            TriLit(6, 2, 10),
            TriLit(8, 6, 7),
            TriLit(9, 8, 1),
        };
        *ArrGetPtr(&MESHES, M_ICOSAHEDRON) = AddMesh(vs, ArraySizeOf(vs), ts, ArraySizeOf(ts));
    }

    // ICOSPHERE Mesh.
    // https://gist.github.com/skeeto/b4aecead36ddec149d99b9f09830a373
    {
        // golden ratio.
        f32 t = LPHI;

        Vec3 vs[512] = {0};
        u32 vsize = 0;

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-1.0f, t, 0.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(1.0f, t, 0.0f));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-1.0f, -t, 0.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(1.0f, -t, 0.0f));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, -1.0f, t));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, 1.0f, t));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, -1.0f, -t));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(0.0f, 1.0f, -t));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(t, 0.0f, -1.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(t, 0.0f, 1.0f));

        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-t, 0.0f, -1.0f));
        Assert(vsize < ArraySizeOf(vs));
        vs[vsize++] = Vec3Normalize(Vec3Lit(-t, 0.0f, 1.0f));

        Triangle ts[2048] = {0};
        u32 tsize = 0;

        // 5 faces around point 0
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 11, 5);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 5, 1);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 1, 7);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 7, 10);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(0, 10, 11);

        // 5 adjacent faces
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(1, 5, 9);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(5, 11, 4);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(11, 10, 2);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(10, 7, 6);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(7, 1, 8);

        // 5 faces around point 3
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 9, 4);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 4, 2);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 2, 6);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 6, 8);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(3, 8, 9);

        // 5 adjacent faces
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(4, 9, 5);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(2, 4, 11);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(6, 2, 10);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(8, 6, 7);
        Assert(tsize < ArraySizeOf(ts));
        ts[tsize++] = TriLit(9, 8, 1);

        // TODO: Issue. seems like a seg fault when I bump this too high. probably something
        // to do with max ibuffer or vbuffer size.
        u32 nsubdivides = 1;

        bool found[512][512] = {0};
        u32 midpoints[512][512] = {0};
        for (u32 count = 0; count < nsubdivides; ++count) {
            Triangle ts2[2048] = {0};
            u32 ts2size = 0;
            for (u32 ti = 0; ti < tsize; ++ti) {
                Triangle tri = ts[ti];
                u32 edges[][2] = {{tri.va, tri.vb}, {tri.vb, tri.vc}, {tri.vc, tri.va}};
                // compute midpoint of each edge.
                for (u32 ei = 0; ei < ArraySizeOf(edges); ++ei) {
                    u32 va = edges[ei][0];
                    u32 vb = edges[ei][1];
                    if (!found[va][vb]) {
                        Vec3 midpoint = Vec3Scale(Vec3Add(vs[va], vs[vb]), 0.5f);
                        u32 idx = vsize;
                        Assert(vsize < ArraySizeOf(vs));
                        vs[vsize++] = Vec3Normalize(midpoint);

                        found[va][vb] = true;
                        found[vb][va] = true;
                        midpoints[va][vb] = idx;
                        midpoints[vb][va] = idx;
                    }
                }

                u32 a = midpoints[tri.va][tri.vb];
                u32 b = midpoints[tri.vb][tri.vc];
                u32 c = midpoints[tri.vc][tri.va];

                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(tri.va, a, c);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(tri.vb, b, a);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(tri.vc, c, b);
                Assert(ts2size < ArraySizeOf(ts2));
                ts2[ts2size++] = TriLit(a, b, c);
            }

            for (u32 j = 0; j < ts2size; ++j) {
                ts[j] = ts2[j];
            }
            tsize = ts2size;
        }

        *ArrGetPtr(&MESHES, M_ICOSPHERE) = AddMesh(vs, vsize, ts, tsize);
    }

    *ArrGetPtr(&MESHES, M_HELLO_CUBE) = LoadMeshFromOBJ(perm, "assets/models/hello-cube.obj");
    *ArrGetPtr(&MESHES, M_HELLO_MONKEY) = LoadMeshFromOBJ(perm, "assets/models/hello-monkey.obj");

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * VBUFFER.len, VBUFFER.data, GL_STATIC_DRAW);

    glGenBuffers(1, &VIO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VIO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * IBUFFER.len, IBUFFER.data, GL_STATIC_DRAW);

    // NOTE: This is very tricky to get right. Don't dynamically set these based on shader attribute
    // locations. just hardcode and make sure that the shader code specifies the corAserect layout.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);

    TexturesInit();
}

#define GLSL(src) "#version 330 core\n" #src

typedef u32 ShaderId;

typedef struct {
    // Slot 0 data
    Mat4 view;
    Mat4 proj;
    ///////
    // Slot 1 data.

    Vec2 resolution;
    f32 dt;
    f32 timer;
    i32 nframe;
} EnableShaderParams;

typedef struct {
    Mat4 modelm;  // converts from model space to world space.
    Vec4 color;
    Vec2 uvoffset;  // offset of the bottom left corner/origin in texture space (0, 0)
    Vec2 uvscale;   // width/height in texture space of the Aserect to sample inside the texture [0, 1]
    LTexture tx;
} UniformShaderParams;

// Forward decls to be able to define enable shader func. This is somewhat akward, maybe consider
// trying to do what the File Pilot guy does and auto gen header files. That might be work tho.
struct LShader;
typedef struct LShader LShader;

typedef void (*EnableShaderFunc)(LShader*, EnableShaderParams*);
typedef void (*SetUniformsFunc)(LShader*, UniformShaderParams*);

struct LShader {
    ShaderId pid;  // program id.
    u32 vid;
    u32 fid;

    EnableShaderFunc enable_func;
    SetUniformsFunc set_uniforms_func;
};

typedef enum {
    CMD_NONE = 0,
    CMD_DRAW_MESH,
    CMD_CLEAR,
    CMD_BEGIN_PASS,
    CMD_COUNT,
} DrawCmdType;

typedef struct {
    DrawCmdType type;

    union {
        // Draws a mesh.
        struct {
            LMesh meshid;
            // TODO: Remove all of the below when I figure out how to split into a push uniforms cmd.
            Mat4 modelm;  // converts from model space to world space.
            Vec4 color;
            Vec2 uvoffset;  // offset of the bottom left corner/origin in texture space (0, 0)
            Vec2 uvscale;   // width/height in texture space of the Aserect to sample inside the texture [0, 1]
            LTexture tx;
        } meshcmd;

        struct {
            // TODO: We are duplicating the RenderPass fields here due to circular dep issues. Change this to just store
            // a RenderPass value Once we've moved DrawCmd to be defined after both RenderPass (easy) and
            // ShaderPass(hard).
            /////////////////////////////////
            // Pipeline/load actions.
            bool depth_test;

            Vec4 clear_color;
            bool clear_color_flag;
            /////////////////////////////////
            // Attachments
            RenderTarget target;

            /////////////////////////////////
            // Bindings

            i32 vao;
            /////////////////////////////////
            // Pipeline
            LShader shader;
            EnableShaderParams enable_params;

            /////////////////////////////////
        } beginpass;
    };

    // Pass arbitrary shader data (ex. like new uniforms to set) via this member.
    // Mostly exists as convenience when iterating on new shader ideas.
    void* extra;
} DrawCmd;

typedef struct {
    DrawCmd* data;
    isize len;
    isize cap;
} DrawCmdArr;

// Assumes the struct has been memset already.
LCORE void DrawMeshCmdInit(DrawCmd* cmd) {
    cmd->type = CMD_DRAW_MESH;
    cmd->meshcmd.color = LCLEAR;
    cmd->meshcmd.modelm = Mat4Identity();
    cmd->meshcmd.uvoffset = Vec2Lit(0.0f, 0.0f);
    cmd->meshcmd.uvscale = Vec2Lit(1.0f, 1.0f);
    cmd->meshcmd.tx = GetTexture(TX_DEFAULT);
}

LCORE void DrawCmdExecute(DrawCmd* cmd) {
    // TODO: SO HACKY. WTF. I SHOULD'VE RESEARCHED PROPER DRAW CMD SETUP IN SOKOL BEFORE WRITING THIS.
    static LShader SHADER = {0};

    switch (cmd->type) {
        case CMD_DRAW_MESH: {
            // TODO: SO HACKY. Uniforms should be pushed in a separate command. A DrawCmd should just draw geometry.
            UniformShaderParams uniform_params = {
                .modelm = cmd->meshcmd.modelm,
                .color = cmd->meshcmd.color,
                .uvoffset = cmd->meshcmd.uvoffset,
                .uvscale = cmd->meshcmd.uvscale,
                .tx = cmd->meshcmd.tx,
            };
            SHADER.set_uniforms_func(&SHADER, &uniform_params);

            glDrawElements(GL_TRIANGLES, cmd->meshcmd.meshid.ntriangles * 3, GL_UNSIGNED_INT,
                           (void*)(cmd->meshcmd.meshid.ibufferidx * sizeof(GLuint)));
            break;
        };

        case CMD_BEGIN_PASS: {
            // Render draw commands to render target.
            glBindFramebuffer(GL_FRAMEBUFFER, cmd->beginpass.target.fbid);
            // IMPORTANT.
            glViewport(0, 0, cmd->beginpass.target.width, cmd->beginpass.target.height);
            // I forget exactly why we want depth testing. I think maybe has to do with not drawing portions of
            // sprites that are blank? maybe thats just alpha blending.
            if (cmd->beginpass.depth_test) {
                glEnable(GL_DEPTH_TEST);
            } else {
                glDisable(GL_DEPTH_TEST);
            }

            // Pass depth test if incomming_depth <= stored_depth. This is necessary to ensure we still draw
            // sprites which have the same z value which so far seems like a desirable thing.
            glDepthFunc(GL_LEQUAL);

            i32 clear_mask = 0;
            if (cmd->beginpass.clear_color_flag) {
                Vec4 clear_color = cmd->beginpass.clear_color;
                glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
                clear_mask |= GL_COLOR_BUFFER_BIT;
            }
            clear_mask |= GL_DEPTH_BUFFER_BIT;
            glClear(clear_mask);

            glBindVertexArray(cmd->beginpass.vao);

            LShader* shader = &cmd->beginpass.shader;
            shader->enable_func(shader, &cmd->beginpass.enable_params);

            SHADER = *shader;
            break;
        }

        default: {
            Assert(false);
            break;
        };
    }
}

static void LShaderInit(LShader* shader, const char* vsrc, const char* fsrc) {
    char error[1024] = {0};
    u32 size = ArraySizeOf(error);

    // vertex shader.
    {
        i32 success = 0;
        shader->vid = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(shader->vid, 1, &vsrc, NULL);
        glCompileShader(shader->vid);
        glGetShaderiv(shader->vid, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader->vid, size, NULL, error);
            PlatformLog(LLOG_ERROR, "Error compiling vertex shader: %s", error);
            Assert(false);
        }
    }

    // fragment shader.
    {
        i32 success = 0;
        shader->fid = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader->fid, 1, &fsrc, NULL);
        glCompileShader(shader->fid);
        glGetShaderiv(shader->fid, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader->fid, size, NULL, error);
            PlatformLog(LLOG_ERROR, "Error compiling fragment shader: %s", error);
            Assert(false);
        }
    }

    // linking.
    {
        i32 success = 0;
        shader->pid = glCreateProgram();
        glAttachShader(shader->pid, shader->vid);
        glAttachShader(shader->pid, shader->fid);
        glLinkProgram(shader->pid);
        glGetProgramiv(shader->pid, GL_LINK_STATUS, &success);
        Assert(success);
    }
}

#define MAX_CMDS 4092

typedef struct {
    bool depth_test;

    Vec4 clear_color;
    bool clear_color_flag;
} RenderPassLoadAction;

// TODO: Think about transitioning to a single command buffer. begin/end passes are then just commands which have
// references to a RenderPass struct.
// context: https://chatgpt.com/c/69b71662-5bdc-832e-8986-444cf2da1b05
/*
Printer Analogy	  Sokol Equivalent	Explanation
Printer settings
    * Pipeline
    * The “rules” for rendering: shaders, blending, culling, primitive type. You configure it once for how you want to
render geometry. Document to print
    * Bindings
    * The actual data: vertex buffers, index buffers, textures. Pipelines define how to render; bindings define what to
render.
Paper / tray
    * Pass attachments
    * Where you are rendering to: screen framebuffer, offscreen texture, depth buffer.
“Print this sheet now” command
    * Draw call (sg_draw)
    * Tells the GPU to actually render a set of vertices with the current pipeline + bindings.
Instructions like “clear paper first”
    * Pass action (sg_pass_action)
    * Defines load/clear behavior for the render target: clear, load existing content, or don’t care.
Start printing a batch
    * Begin pass (sg_begin_pass)
    * Marks the start of rendering to a specific render target with specific actions.
Finish printing a batch
    * End pass (sg_end_pass)
    * Marks the end of the pass. Nothing is executed yet; just recorded.
Flush printer queue and actually print
    * Commit (sg_commit)
    * Sends all the recorded commands to the GPU and executes them.
*/
typedef struct {
} ShaderPass;

LCORE void EnableDefaultShader(LShader* shader, EnableShaderParams* params) {
    static bool INIT = false;
    static i32 V_VIEW = 0;
    static i32 V_PROJ = 0;
    static i32 F_RESOLUTION = 0;
    static i32 F_TIMER = 0;
    static i32 F_NFRAME = 0;

    glUseProgram(shader->pid);

    if (!INIT) {
        INIT = true;
        V_VIEW = glGetUniformLocation(shader->pid, "vView");
        V_PROJ = glGetUniformLocation(shader->pid, "vProj");
        F_RESOLUTION = glGetUniformLocation(shader->pid, "resolution");
        F_TIMER = glGetUniformLocation(shader->pid, "timer");
        F_NFRAME = glGetUniformLocation(shader->pid, "NFrame");

        // IMPORTANT! Set texture uniform loc.
        glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
    }

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&params->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&params->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&params->resolution.f);
    glUniform1f(F_TIMER, params->timer);

    glUniform1i(F_NFRAME, params->nframe);
}

LCORE void DefaultShaderSetUniforms(LShader* shader, UniformShaderParams* params) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&params->modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&params->color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&params->uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&params->uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, params->tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, params->tx.width, params->tx.height);
}

static LShader GetDefaultShader() {
    // clang-format off
    static const char* VSRC = GLSL(
        layout (location = 0) in vec3 vPos;
        layout (location = 2) in vec2 vTextCoords;
        
        uniform mat4 vModel;
        uniform mat4 vView;
        uniform mat4 vProj;
        uniform vec4 vUColor;
        uniform vec2 vUVOffset;
        uniform vec2 vUVScale;
        
        out vec4 fColor;
        out vec2 fTextCoords;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor; in vec2 fTextCoords;

        uniform sampler2D texture0;
        uniform vec2 Texture0Resolution;
        uniform vec2 resolution;
        uniform float timer;
        uniform int NFrame;

        out vec4 FragColor;

        void main() {
            vec4 txColor = texture(texture0, fTextCoords);
            // Crucial for having depth buffer + blending enabled. when I write quads with textures with some tansparent
            // parts. We want to discard those transparent parts else those values will get written to the depth buffer.
            if (txColor.a < 0.01) {
                discard;
            }
            // For blending to work here properly, its important that either texture is specified or texture
            // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
            FragColor = txColor * fColor;
            // FragColor = txColor;
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableDefaultShader;
        SHADER.set_uniforms_func = DefaultShaderSetUniforms;
    }

    return SHADER;
}

// LCORE void EnableLineShader(LShader* shader, EnableShaderParams* params) {
//     static bool INIT = false;
//     static i32 V_VIEW = 0;
//     static i32 V_PROJ = 0;
//     static i32 F_RESOLUTION = 0;
//     static i32 F_TIMER = 0;
//     static i32 F_NFRAME = 0;

//     glUseProgram(shader->pid);

//     if (!INIT) {
//         INIT = true;
//         V_VIEW = glGetUniformLocation(shader->pid, "vView");
//         V_PROJ = glGetUniformLocation(shader->pid, "vProj");
//         F_RESOLUTION = glGetUniformLocation(shader->pid, "resolution");
//         F_TIMER = glGetUniformLocation(shader->pid, "timer");
//         F_NFRAME = glGetUniformLocation(shader->pid, "NFrame");

//         // IMPORTANT! Set texture uniform loc.
//         glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
//     }

//     glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&params->view.m);
//     glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&params->proj.m);

//     glUniform2fv(F_RESOLUTION, 1, (const f32*)&params->resolution.f);
//     glUniform1f(F_TIMER, params->timer);

//     glUniform1i(F_NFRAME, params->nframe);
// }

// LCORE void LineShaderSetUniforms(LShader* shader, UniformShaderParams* params) {
//     static bool INIT = false;
//     static i32 V_MODEL = 0;
//     static i32 V_UVCOLOR = 0;
//     static i32 V_UVSCALE = 0;
//     static i32 V_UVOFFSET = 0;
//     static i32 F_TEXTURE0_RESOLUTION = 0;

//     if (!INIT) {
//         INIT = true;

//         V_MODEL = glGetUniformLocation(shader->pid, "vModel");
//         V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
//         V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
//         V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
//         F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
//     }

//     glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&params->modelm.m);
//     glUniform4fv(V_UVCOLOR, 1, (const f32*)&params->color.f);
//     glUniform2fv(V_UVOFFSET, 1, (const f32*)&params->uvoffset.f);
//     glUniform2fv(V_UVSCALE, 1, (const f32*)&params->uvscale.f);

//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, params->tx.id);
//     glUniform2f(F_TEXTURE0_RESOLUTION, params->tx.width, params->tx.height);
// }

// LCORE LShader GetLineShader() {
//     // clang-format off
//     static const char* VSRC = GLSL(
//         layout (location = 0) in vec3 vPos;
//         layout (location = 2) in vec2 vTextCoords;

//         uniform mat4 vModel;
//         uniform mat4 vView;
//         uniform mat4 vProj;
//         uniform vec4 vUColor;
//         uniform vec2 vUVOffset;
//         uniform vec2 vUVScale;

//         out vec4 fColor;
//         out vec2 fTextCoords;
//         out vec2 fPosLocal;

//         void main() {
//            fTextCoords = vUVOffset + (vUVScale * vTextCoords);
//            gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
//            fColor = vUColor;
//            fPosLocal = vPos.xy;
//         }
//     );
//     // clang-format on

//     // clang-format off
//     static const char* FSRC = GLSL(
//         in vec4 fColor;
//         in vec2 fTextCoords;
//         in vec2 fPosLocal;

//         uniform sampler2D texture0;
//         uniform vec2 Texture0Resolution;
//         uniform vec2 resolution;
//         uniform float timer;
//         uniform int NFrame;

//         out vec4 FragColor;

//         void main() {
//             vec4 txColor = texture(texture0, fTextCoords);
//             // Crucial for having depth buffer + blending enabled. when I write quads with textures with some
//             tansparent
//             // parts. We want to discard those transparent parts else those values will get written to the depth
//             buffer. if (txColor.a < 0.01) {
//                 discard;
//             }
//             // For blending to work here properly, its important that either texture is specified or texture
//             // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
//             FragColor = txColor * fColor;

//             float alpha_mask = 1.0;

//             // {
//             //     float u = fTextCoords.x;
//             //     // aa radius. This represents the radius of a circle that determines how much alpha falloff
//             //     // the line pixels experience as our u coordinate goes from inside the line to outside the line.
//             //     // we are fully within the line in the range [r, 1-r] and outside the line outside of that range.
//             //     // Note that this assumes that the drawn line quad is oriented such that the u coordinate of each
//             //     // fragment lies along the width axis of the quad.
//             //     float r = 0.3;
//             //     // Construct a piecewise smoothing function that transitions smoothly from [0, 1] between the
//             //     // domain of [0, r], stays at 1 from [r, 1-r] and transitions smoothly decreasing from [1, 0]
//             //     // in the domain [1-r, r].
//             //     alpha_mask = smoothstep(0.0, 1.0, u / r) -
//             //                     smoothstep(0.0, 1.0, (max(u - 1 + r, 0.0)) / r);
//             // }

//             {
//                 // Calculates AA in local space of the quad model. Assumes the quad has width of 0.5 in local space
//                 // and that we are using an orthographic projection (I think due to perspective correct interpolation
//                 of
//                 // vertex attribute concerns.)
//                 float x = abs(fPosLocal.x);  // pass in our vertex position in local space.
//                 float edge = 0.5;            // Assume the quad model has edge vertex of 0.5
//                 float w = 2.0 * fwidth(x);         // Get size of 1 pixel in screen space
//                 alpha_mask *= 1.0 - smoothstep(edge - w, edge, x);  // Apply alpha blending of 1 pixel width at edge.

//                 // Do the same for AA the end edges of the line. Note that we use y for fwidth
//                 // which I'm not 100% sure is correct.
//                 float y = abs(fPosLocal.y);
//                 float h = fwidth(y);  // Get ~size of 1 pixel in screen space in the y direction.
//                 alpha_mask *= 1.0 - smoothstep(edge - h, edge, y);
//             }

//             FragColor.a *= alpha_mask;
//         }
//     );
//     // clang-format on

//     static bool INIT = false;
//     static LShader SHADER = {0};

//     if (!INIT) {
//         INIT = true;
//         LShaderInit(&SHADER, VSRC, FSRC);
//         SHADER.enable_func = EnableLineShader;
//         SHADER.set_uniforms_func = LineShaderSetUniforms;
//     }

//     return SHADER;
// }

// LCORE void EnableNoiseShader(LShader* shader, EnableShaderParams* params) {
//     static bool INIT = false;
//     static i32 V_VIEW = 0;
//     static i32 V_PROJ = 0;
//     static i32 F_RESOLUTION = 0;
//     static i32 F_TIMER = 0;
//     static i32 F_NFRAME = 0;

//     glUseProgram(shader->pid);

//     if (!INIT) {
//         INIT = true;
//         V_VIEW = glGetUniformLocation(shader->pid, "vView");
//         V_PROJ = glGetUniformLocation(shader->pid, "vProj");
//         F_RESOLUTION = glGetUniformLocation(shader->pid, "resolution");
//         F_TIMER = glGetUniformLocation(shader->pid, "timer");
//         F_NFRAME = glGetUniformLocation(shader->pid, "NFrame");

//         // IMPORTANT! Set texture uniform loc.
//         glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
//     }

//     glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&params->view.m);
//     glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&params->proj.m);

//     glUniform2fv(F_RESOLUTION, 1, (const f32*)&params->resolution.f);
//     glUniform1f(F_TIMER, params->timer);

//     glUniform1i(F_NFRAME, params->nframe);
// }

// LCORE void NoiseShaderSetUniforms(LShader* shader, UniformShaderParams* params) {
//     static bool INIT = false;
//     static i32 V_MODEL = 0;
//     static i32 V_UVCOLOR = 0;
//     static i32 V_UVSCALE = 0;
//     static i32 V_UVOFFSET = 0;
//     static i32 F_TEXTURE0_RESOLUTION = 0;

//     if (!INIT) {
//         INIT = true;

//         V_MODEL = glGetUniformLocation(shader->pid, "vModel");
//         V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
//         V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
//         V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
//         F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
//     }

//     glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&params->modelm.m);
//     glUniform4fv(V_UVCOLOR, 1, (const f32*)&params->color.f);
//     glUniform2fv(V_UVOFFSET, 1, (const f32*)&params->uvoffset.f);
//     glUniform2fv(V_UVSCALE, 1, (const f32*)&params->uvscale.f);

//     glActiveTexture(GL_TEXTURE0);
//     glBindTexture(GL_TEXTURE_2D, params->tx.id);
//     glUniform2f(F_TEXTURE0_RESOLUTION, params->tx.width, params->tx.height);
// }

// LCORE LShader GetNoiseShader() {
//     // clang-format off
//     static const char* VSRC = GLSL(
//         layout (location = 0) in vec3 vPos;
//         layout (location = 2) in vec2 vTextCoords;

//         uniform mat4 vModel;
//         uniform mat4 vView;
//         uniform mat4 vProj;
//         uniform vec4 vUColor;
//         uniform vec2 vUVOffset;
//         uniform vec2 vUVScale;

//         out vec4 fColor;
//         out vec2 fTextCoords;
//         out vec3 fWorldPos;

//         void main() {
//            fTextCoords = vUVOffset + (vUVScale * vTextCoords);
//            gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
//            fColor = vUColor;
//            fWorldPos = (vModel * vec4(fWorldPos, 1.0)).xyz;
//         }
//     );
//     // clang-format on

//     // clang-format off
//     static const char* FSRC = GLSL(
//         in vec4 fColor;
//         in vec2 fTextCoords;
//         in vec3 fWorldPos;

//         uniform sampler2D texture0;
//         uniform vec2 Texture0Resolution;
//         uniform vec2 resolution;
//         uniform float timer;
//         uniform int NFrame;

//         out vec4 FragColor;

//         // https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
//         uint pcg_hash(uint x) {
//             uint state = x * 747796405u + 2891336453u;
//             uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
//             return (word >> 22u) ^ word;
//         }

//         float uv2whitenoise(vec2 uv, ivec2 dims) {
//             ivec2 texel = ivec2(floor(uv * dims));
//             uint seed = uint((texel.x * dims.y) + texel.y);
//             return float(pcg_hash(seed)) * (1.0 / 4294967295.0);
//         }

//         float whitenoise(vec3 value) {
//             // Produce a random value by dotting it with a random direction vector.
//             // The resulting random value will be how close our input vector is to the direciton vector
//             // multiplied by magnitudes.
//             float random = dot(value, vec3(2.971, 49.149, 0));
//             // Random could be a large number since our vectors are assumed to be unit vectors
//             // clamp to -1 to 1 range with sin function. This also gives our random value a wave like
//             // property. then to avoid tiling, multiply with a very large random number to essentially
//             // convert our small number to range [0, large number] which should eliminate tiling.
//             return fract(sin(random) * 143758.5453);
//         }

//         void main() {
//             vec4 txColor = texture(texture0, fTextCoords);
//             // Crucial for having depth buffer + blending enabled. when I write quads with textures with some
//             tansparent
//             // parts. We want to discard those transparent parts else those values will get written to the depth
//             buffer. if (txColor.a < 0.01) {
//                 discard;
//             }
//             // For blending to work here properly, its important that either texture is specified or texture
//             // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
//             FragColor = txColor * fColor;
//             float noise = whitenoise(vec3(fTextCoords, 1.0));
//             // Remember cant use texture dims as some textures are 1 pixel.
//             float noisehash = uv2whitenoise(fTextCoords, ivec2(10000, 10000));
//             FragColor = vec4(vec3(noisehash), 1.0);
//             if (noisehash == 0.0) {
//                 FragColor = vec4(1, 0, 0, 1);
//             }
//         }
//     );
//     // clang-format on

//     static bool INIT = false;
//     static LShader SHADER = {0};

//     if (!INIT) {
//         INIT = true;
//         LShaderInit(&SHADER, VSRC, FSRC);
//         SHADER.enable_func = EnableNoiseShader;
//         SHADER.set_uniforms_func = NoiseShaderSetUniforms;
//     }

//     return SHADER;
// }

LCORE void EnableEffectsShader(LShader* shader, EnableShaderParams* params) {
    static bool INIT = false;
    static i32 V_VIEW = 0;
    static i32 V_PROJ = 0;

    glUseProgram(shader->pid);

    if (!INIT) {
        INIT = true;
        V_VIEW = glGetUniformLocation(shader->pid, "vView");
        V_PROJ = glGetUniformLocation(shader->pid, "vProj");

        // IMPORTANT! Set texture uniform loc.
        glUniform1i(glGetUniformLocation(shader->pid, "texture0"), 0);
    }

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&params->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&params->proj.m);
}

LCORE void EffectsShaderSetUniforms(LShader* shader, UniformShaderParams* params) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;

    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&params->modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&params->color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&params->uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&params->uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, params->tx.id);
}

static LShader GetEffectsShader() {
    // clang-format off
    static const char* VSRC = GLSL(
        layout (location = 0) in vec3 vPos;
        layout (location = 2) in vec2 vTextCoords;

        uniform mat4 vModel;
        uniform mat4 vView;
        uniform mat4 vProj;

        uniform vec4 vUColor;
        uniform vec2 vUVOffset;
        uniform vec2 vUVScale;

        out vec4 fColor;
        out vec2 fTextCoords;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor;
        in vec2 fTextCoords;

        uniform sampler2D texture0;

        out vec4 FragColor;

        vec2 uvremap(vec2 uv) {
            // as we near the edge of our screen apply greater distortion using a cubic function
            vec2 curvature = vec2(7.0, 7.0);
            uv = uv * 2.0 - 1.0;
            vec2 offset = abs(uv.yx) / vec2(curvature.x, curvature.y);
            uv = uv + uv * offset * offset;
            uv = uv * 0.5 + 0.5;
            return uv;
        }

        vec4 scanLineIntensity(float uv, float resolution, float opacity) {
            float PI = 3.1415926535;
            float intensity = sin(uv * resolution * PI * 2.0);
            intensity = ((0.5 * intensity) + 0.5) * 0.9 + 0.1;
            return vec4(vec3(pow(intensity, opacity)), 1.0);
        }

        void main() {
           vec2 uv = fTextCoords;
           vec4 txcolor = texture(texture0, uv);
           FragColor = txcolor;
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableEffectsShader;
        SHADER.set_uniforms_func = EffectsShaderSetUniforms;
    }

    return SHADER;
}

#define MAX_SHADER_PASSES 10
static const u32 PASSNULL = MAX_SHADER_PASSES;

typedef struct {
    u32 passidx;
    u32 passlen;
    u32 passcap;
    ShaderPass passes[MAX_SHADER_PASSES];

    f32 dt;
    f32 timer;
    i32 nframe;

    // What the game world renders to.
    RenderTarget game_target;
    // What we render to screen.
    RenderTarget screen_target;

    DrawCmdArr cmds;
} RenderState;

static RenderState RSTATE;
#define GetRenderState() (&RSTATE)

static DrawCmd* AddCmd() {
    static DrawCmd ZERO = {0};

    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen != 0);

    DrawCmd* cmd = ArrReserve(&rstate->cmds);
    *cmd = ZERO;
    return cmd;
}

static DrawCmd* AddDrawMeshCmd() {
    DrawCmd* cmd = AddCmd();
    DrawMeshCmdInit(cmd);
    return cmd;
}

static RenderTarget GetDefaultRenderTarget() { return GetRenderState()->game_target; }
static RenderTarget GetScreenRenderTarget() { return GetRenderState()->screen_target; }

static void RenderStateInit(u32 game_width, u32 game_height, Arena* perm) {
    RenderState* rstate = &RSTATE;

    memset(rstate, 0, sizeof(RenderTarget));
    rstate->game_target = CreateRenderTarget(game_width, game_height);

    // A little hacky since we don't specify the texture, but good enough for now.
    rstate->screen_target = (RenderTarget){
        .fbid = 0,
        .width = PlatformScreenWidth(),
        .height = PlatformScreenHeight(),
    };

    InitArr(&rstate->cmds, perm, DrawCmd, 1000);
}

// Reset render state and allow accumulation of shader passes.
static void BeginRender(f32 dt) {
    RenderState* rstate = &RSTATE;
    rstate->passidx = PASSNULL;
    rstate->passlen = 0;
    rstate->passcap = MAX_SHADER_PASSES - 1;  // last entry reserved for null idx.
    rstate->dt = dt;
    rstate->timer += dt;
    ++rstate->nframe;

    rstate->screen_target.width = PlatformScreenWidth();
    rstate->screen_target.height = PlatformScreenHeight();
}

static RenderPassLoadAction DefaultLoadAction() {
    RenderPassLoadAction load_action = {
        .clear_color = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f),
        .clear_color_flag = true,
        .depth_test = true,
    };
    return load_action;
}

static void BeginShaderPass(LShader shader, RenderPassLoadAction load_action, Mat4 view, Mat4 proj,
                            RenderTarget target) {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen < rstate->passcap);
    Assert(rstate->passidx == PASSNULL);

    rstate->passidx = rstate->passlen++;

    {
        DrawCmd* cmd = AddCmd();

        *cmd = (DrawCmd){.type = CMD_BEGIN_PASS,
                         .beginpass = {
                             .depth_test = load_action.depth_test,

                             .clear_color = load_action.clear_color,
                             .clear_color_flag = load_action.clear_color_flag,
                             .target = target,
                             .vao = VAO,
                             .shader = shader,
                             .enable_params =
                                 {
                                     .view = view,
                                     .proj = proj,
                                     .dt = rstate->dt,
                                     .nframe = rstate->nframe,
                                     .resolution = Vec2Lit(target.width, target.height),
                                     .timer = rstate->timer,
                                 },
                         }};
    }
}

static void EndShaderPass() {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passidx != PASSNULL);
    rstate->passidx = PASSNULL;
}

// Execute the draw commands and send to gpu.
static void EndRender() {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passidx == PASSNULL);

    // // Enable/Disable culling.
    // // TODO: Make this nicer at some point. Useful for debugging.
    // {
    //     if (gstate->debug) {
    //         glDisable(GL_CULL_FACE);
    //     } else {
    //         glEnable(GL_CULL_FACE);
    //         glCullFace(GL_BACK);
    //         glFrontFace(GL_CCW);
    //     }
    // }

    for (isize i = 0; i < ArrLen(&rstate->cmds); ++i) {
        DrawCmd* cmd = ArrGetPtr(&rstate->cmds, i);
        DrawCmdExecute(cmd);
    }

    ArrClear(&rstate->cmds);
}

#define Render(dt) Scoped(BeginRender(dt), EndRender())

LCORE void DrawLSphere(Vec3 center, f32 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_ICOSPHERE);
    cmd->meshcmd.modelm = Mat4SRT(radius, Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

LCORE void DrawLCircle(Vec3 center, f32 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    // We cheat and just use an icosphere. assumes ortho projection which should chop it into a circle.
    cmd->meshcmd.meshid = GetMesh(M_ICOSPHERE);
    cmd->meshcmd.modelm = Mat4SRT(radius, Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

LCORE DrawCmd* DrawLCube(Vec3 center, Vec3 scale, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_CUBE);
    cmd->meshcmd.modelm = Mat4SVRT(scale, Mat4Identity(), center);
    cmd->meshcmd.color = color;
    return cmd;
}

LCORE DrawCmd* DrawQuad(Vec3 center, f32 width, f32 height, Mat4 orient, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Lit(width, height, 1.0f), orient, center);
    cmd->meshcmd.color = color;
    return cmd;
}

#define DrawQuad2D(center2d, width, height, orient, color) \
    (DrawQuad(Vec2ToVec3(center2d), (width), (height), (orient), (color)))

LCORE DrawCmd* DrawMesh(LMesh mesh, Vec3 pos, Mat4 orient, Vec3 scale, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = mesh;
    cmd->meshcmd.modelm = Mat4SVRT(scale, orient, pos);
    cmd->meshcmd.color = color;
    return cmd;
}

// Note assumes drawing in 2d. the z is just for depth ordering.
// We assume that the quad's up direction is oriented parallel to the line segment.
LCORE DrawCmd* DrawLLine(Vec3 start, Vec3 end, f32 width, Vec4 color) {
    // Assumption here is that we are applying anti aliasing in frag shader. That AA algo requires some
    // area of the line quad to apply smoothing, so lets just bump the size of the requested line by
    // that area amount to give us some buffer. This does put a cap on the smallest line we can draw.
    // It maybe makes sense to make this slightly bigger than the aa radius.
    static const f32 AA_BUFFER_X = 3.0;

    Assert(start.z == end.z);
    Vec3 pos = Vec3Lerp(start, end, 0.5f);
    Vec3 span = Vec3Sub(end, start);
    Vec3 dir = Vec3Normalize(span);

    // Remember we aren't computing a rotation (which would be some displacement difference
    // between identity orient and new orient), we are just computing an orientation.
    // The direction of line is our up
    Vec3 up = dir;
    // Compute right basis vector by taking 2d normal (since z is assumed for depth only).
    Vec3 right = Vec3Lit(dir.y, -dir.x, dir.z);
    Vec3 forward = Vec3Lit(0.0f, 0.0f, 1.0f);

    Mat4 rot = Mat4Identity();
    rot.v.a = right;
    rot.v.b = up;
    rot.v.c = forward;

    f32 height = Vec3Len(span);
    return DrawQuad(pos, width + AA_BUFFER_X, height, rot, color);
}

// Dims is full width and height.
LCORE void DrawAABB(Vec3 center, Vec3 dims, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_CUBE);
    cmd->meshcmd.modelm = Mat4SVRT(dims, Mat4Identity(), center);
    cmd->meshcmd.color = color;
}

// Remember
// src == where to draw in texture space
// dest == where to draw in world space.
// (0, 0)
// * maps to bottom left corner in src texture space (this is not uv space, but width/height space)
// * maps to origin in world space.
// radius
// * need to be full width/height of quad in texture space
// * need to be half width/height in world space. Is this right? it looks like it might need to be full width/height
LCORE void DrawLTexturePro(LTexture tx, Vec2 c_src, Vec2 r_src, Vec2 c_dst, Vec2 r_dst, f32 rotation, Vec4 color) {
    // TODO: is scale right?
    Vec2 uvscale = Vec2Lit(r_src.x / tx.width, r_src.y / tx.height);
    Vec2 uvoffset = Vec2Lit(c_src.x / tx.width, c_src.y / tx.height);

    Mat4 scale = Mat4ScaleV(Vec3Lit(r_dst.x, r_dst.y, 1.0f));
    Mat4 orient = Mat4Rotate(rotation, Vec3Lit(0.0f, 0.0f, 1.0f));
    Mat4 translate = Mat4TranslateV(Vec3Lit(c_dst.x, c_dst.y, 0.0f));
    Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.modelm = model;
    cmd->meshcmd.color = color;
    cmd->meshcmd.uvoffset = uvoffset;
    cmd->meshcmd.uvscale = uvscale;
    cmd->meshcmd.tx = tx;
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
}

// Remember
// src == where to draw in texture space
// dest == where to draw in world space.
// (0, 0)
// * maps to bottom left corner in src texture space (uv space)
// * maps to origin in world space.
// :c_src - source origin (lower left corner) of where to read from texture in uv space
// :r_src - full width/height of sub rect to read from texture in uv space
// :c_dst - destination origin (center of quad) of where to draw in world space
// :r_dst - full width + height of quad
LCORE DrawCmd* DrawLTextureProUV(LTexture tx, Vec2 c_src, Vec2 r_src, Vec2 c_dst, Vec2 r_dst, f32 rotation,
                                 Vec4 color) {
    Mat4 scale = Mat4ScaleV(Vec3Lit(r_dst.x, r_dst.y, 1.0f));
    Mat4 orient = Mat4Rotate(rotation, Vec3Lit(0.0f, 0.0f, 1.0f));
    Mat4 translate = Mat4TranslateV(Vec3Lit(c_dst.x, c_dst.y, 0.0f));
    Mat4 model = Mat4Multiply(Mat4Multiply(translate, orient), scale);

    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.modelm = model;
    cmd->meshcmd.color = color;
    cmd->meshcmd.uvoffset = c_src;
    cmd->meshcmd.uvscale = r_src;
    cmd->meshcmd.tx = tx;
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
    return cmd;
}

LCORE void DrawLTextureEx(LTexture tx, Vec2 pos, f32 scale, f32 rotation, Vec4 color) {
    DrawLTexturePro(tx, Vec2Lit(0.0f, 0.0f), Vec2Lit(tx.width, tx.height), pos, Vec2Lit(scale, scale), rotation, color);
}

// Remember that pos will be the center of the quad.
// Scale will control the multiple of original size of the texture.
LCORE void DrawLTexture(LTexture tx, Vec2 pos, f32 scale) {
    DrawLTexturePro(tx, Vec2Lit(0.0f, 0.0f), Vec2Lit(tx.width, tx.height), pos,
                    Vec2Lit(tx.width * scale, tx.height * scale), 0.0f, LCLEAR);
}

////////////////////////////////////////////////////////////
// S:Geometry

static const f32 SLOP = 2.0f * F32_EPSILON;

static const Vec3 AXIS[] = {Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f)};

typedef Vec2 Point2D;

LCORE Vec3 ComputeCentroid(Vec3* vs, u32 vsize) {
    Vec3 center = {0};
    for (u32 i = 0; i < vsize; ++i) {
        center = Vec3Add(center, vs[i]);
    }
    center.x /= vsize;
    center.y /= vsize;
    center.z /= vsize;
    return center;
}

typedef struct {
    Vec2 c;
    f32 r;
} Circle;

#define CircleLit(cc, rr) ((Circle){.c = (cc), .r = (rr)})

// Returns normal pointing from b to a if there is a collision. Assumes both
// circles are in same coordinate space.
static bool TestCircleCircle(Circle a, Circle b, Vec2* normal, f32* depth) {
    Vec2 ba = Vec2Sub(a.c, b.c);
    f32 dsquared = (ba.x * ba.x) + (ba.y * ba.y);
    f32 r = a.r + b.r;
    // if (squared) distance between a and b is greater than the (squared) distance of the
    // combined radii, then no collision.
    if (r * r < dsquared) {
        return false;
    }

    f32 d = f32sqrt(dsquared);
    *normal = Vec2Scale(ba, 1.0f / d);
    *depth = r - d;
    return true;
}

typedef struct {
    Vec3 c;  // center
    f32 r;   // radius
} Sphere;

typedef struct {
    Vec2 c;  // center.
    Vec2 r;  // half width extents along each axis.
} AABB2D;

#define AABB2DWidth(aabb) (2.0f * (aabb).r.x)
#define AABB2DHeight(aabb) (2.0f * (aabb).r.y)
#define AABB2DDims(aabb) (Vec2Lit(AABB2DWidth(aabb), AABB2DHeight(aabb)))
#define AABB2DTopLeft(aabb) (Vec2Lit((aabb).c.x - (aabb).r.x, (aabb).c.y + (aabb).r.y))
#define AABB2DBottomLeft(aabb) (Vec2Lit((aabb).c.x - (aabb).r.x, (aabb).c.y - (aabb).r.y))

// Returns a aabb2d from a bottom left and top right
LCORE AABB2D AABB2DFromMinMax(Vec2 pmin, Vec2 pmax) {
    Vec2 r = Vec2Sub(pmax, pmin);
    r.x *= 0.5f;
    r.y *= 0.5f;

    AABB2D box = {0};
    box.r = r;
    box.c = Vec2Add(pmin, r);
    return box;
}

// Returns a aabb2d given a center + full dims.
LCORE AABB2D AABB2DFromWidthHeight(Vec2 center, Vec2 dims) {
    AABB2D box = {0};
    box.c = center;
    box.r = Vec2Lit(dims.x * 0.5f, dims.y * 0.5f);
    return box;
}

// Convert an aseprite bb on a marked up png to a world position bb.
// The aseprite bb min will be the bottom left corner of the box THAT I SEE ON THE SCREEN. Not the bottom
// left according to the pixel coords in the UI. The Top right will be similar.
LCORE AABB2D AABB2DFromAsepriteBB(LTexture tx, Vec2 min_aseprite, Vec2 max_aseprite) {
    Vec2 min_world = AsepritePosToWorldPos(tx, min_aseprite);
    Vec2 max_world = AsepritePosToWorldPos(tx, max_aseprite);
    AABB2D bb = AABB2DFromMinMax(min_world, max_world);
    return bb;
}

// Normal from b -> a if there is a collision.
LCORE bool TestAABB2DVsAABB2D(AABB2D a, AABB2D b, Vec2* normal, f32* depth) {
    Vec2 ba = Vec2Sub(a.c, b.c);

    *normal = Vec2Zero();
    *depth = F32_MAX;

    // Remember that r is half extents, so we don't need to divide by 2 to find minimum distance allowed
    // between centers along a given axis. Take abs because we just want magnitude.
    f32 depthx = (a.r.x + b.r.x) - f32abs(ba.x);
    if (depthx < 0.0f) {
        return false;
    }

    f32 depthy = (a.r.y + b.r.y) - f32abs(ba.y);
    if (depthy < 0.0f) {
        return false;
    }

    if (depthx <= depthy) {
        *normal = Vec2Lit(1.0f * (ba.x < 0 ? -1.0f : 1.0f), 0.0f);
        *depth = depthx;
    } else {
        *normal = Vec2Lit(0.0f, 1.0f * (ba.y < 0 ? -1.0f : 1.0f));
        *depth = depthy;
    }

    return true;
}

// Normal will point towards the axis closest that would resolve penetration.
LCORE bool TestPoint2DVsAABB2D(Point2D point, AABB2D box, Vec2* normal, f32* depth) {
    // Get vector between point and box center.
    Vec2 dist = Vec2Sub(point, box.c);

    *normal = Vec2Zero();
    *depth = F32_MAX;

    // If we are futher than the half horizontal width of the box, no collision.
    f32 depthx = box.r.x - f32abs(dist.x);
    if (depthx < 0.0f) {
        return false;
    }

    // If we are further than the half veritical height of the box, no collision.
    f32 depthy = box.r.y - f32abs(dist.y);
    if (depthy < 0.0f) {
        return false;
    }

    // Otherwise we have a collision and need to find axis of separation.
    if (depthx <= depthy) {
        *normal = Vec2Lit(1.0f * (dist.x < 0 ? -1.0f : 1.0f), 0.0f);
        *depth = depthx;
    } else {
        *normal = Vec2Lit(0.0f, 1.0f * (dist.y < 0 ? -1.0f : 1.0f));
        *depth = depthy;
    }

    return true;
}

// Normal will point from aabb to circle.
LCORE bool TestCircleVsAABB2D(Circle circle, AABB2D aabb, Vec2* normal, f32* depth) {
    Vec2 min = Vec2Sub(aabb.c, aabb.r);
    Vec2 max = Vec2Add(aabb.c, aabb.r);

    // Find closest point from circle center to aabb (which might be inside the aabb).
    Vec2 p = circle.c;
    for (u32 i = 0; i < 2; ++i) {
        p.f[i] = f32clamp(p.f[i], min.f[i], max.f[i]);
    }

    // If squared distance from closest point to circle on aabb to circle center is greater
    // than the squared radius of the circle, then we don't have a collision.
    Vec2 disp = Vec2Sub(circle.c, p);
    f32 d = (disp.x * disp.x) + (disp.y * disp.y);
    if (circle.r * circle.r < d) {
        return false;
    }

    // If the center is inside of the aabb, then the normal and depth will not be corAserect, since the
    // closest point will be the center itself. But at least we will report the collision happened.
    *depth = f32abs(f32sqrt(d) - circle.r);
    *normal = Vec2Normalize(disp);
    return true;
}

typedef struct {
    Vec2 c;       // center
    Vec2 hdims;   // half width/height extents along the orient.
    Mat2 orient;  // The local to world 2d rotation matrix. orthonormal basis vectors.
} OBB2D;

LCORE Vec2 ClosestPointToOBB2D(Vec2 start, OBB2D obb) {
    // The broad idea here is to find the closest point from the circle center to the obb. We do this
    // by transforming the circle center into the local coordinate space of th obb (i.e. we take away translation and
    // rotation). Then we clamp the transformed center to be within the obb if necessary.

    // Determine closest point from circle center to obb (which might be inside obb).
    // Get the vector from obb center to circle center as we are going to transform this
    // into the local coordinate space of the untransalated obb. We need to first remove the translation,
    // Then we can undo the rotation.
    Vec2 q = Vec2Sub(start, obb.c);

    // Reminder that a rotation matrix inverse is its transpose, so transforming a vector
    // by the inverse of a rotation matrix is the same as projecting the vector onto each
    // basis vector (column vector) of the rotation matrix. This shifts the coordinates from
    // world to local.
    f32 x = Vec2Dot(q, obb.orient.v.a);
    // Clamp the projected value (which is now in local space of obb) to be within the dimenions of the
    // axis we projected along.
    x = f32clamp(x, -obb.hdims.x, obb.hdims.x);
    // To get back to world, scale the basis vector of the local to world rotation matrix by the
    // local projected amounted. This gets us back from local to world.
    Vec2 qx = Vec2Scale(obb.orient.v.a, x);

    f32 y = Vec2Dot(q, obb.orient.v.b);
    y = f32clamp(y, -obb.hdims.y, obb.hdims.y);
    Vec2 qy = Vec2Scale(obb.orient.v.b, y);

    // We now have our closest point in world space from the circle center to the obb (which again might be inside the
    // obb). Remember that qx and qy are essentially our closest point in world space expressed in terms of the obb
    // local to world basis vectors. So x_local * x_basis + y_local * y_basis. Adding these together plus adding the
    // obb center world coordinate yields the point's location in world space.
    q = Vec2Add(obb.c, Vec2Add(qx, qy));
    return q;
}

LCORE bool TestCircleVsOBB2D(Circle circle, OBB2D obb, Vec2* normal, f32* depth) {
    Vec2 q = ClosestPointToOBB2D(circle.c, obb);

    Vec2 direction = Vec2Sub(circle.c, q);
    f32 squared_dist = (direction.x * direction.x) + (direction.y * direction.y);
    if (squared_dist > circle.r * circle.r) {
        return false;
    }

    // Compute the collision normal (which is just the direction from the point on obb to the circle center).
    // And the penetration depth which is just the difference between the circle radius and distance from the closest
    // point to the circle center
    f32 dist = f32sqrt(squared_dist);
    direction.x /= dist;
    direction.y /= dist;
    *depth = circle.r - dist;
    *normal = direction;
    return true;
}

LCORE bool TestAABB2DvsOBB2D2(AABB2D aabb2d, OBB2D obb2d, Vec2* normal, f32* depth) {
    // static bool DEBUG = false;
    // static i32 NUMBER = 0;
    // if (PlatformKeyDown(LKEY_ENTER)) {
    //     DEBUG = true;
    // }

    *depth = F32_MAX;

    Vec2 a1 = Vec2Add(aabb2d.c, aabb2d.r);
    Vec2 a2 = Vec2Add(aabb2d.c, Vec2Lit(-aabb2d.r.x, aabb2d.r.y));
    Vec2 b1 = Vec2Add(obb2d.c, Vec2Transform(obb2d.hdims, obb2d.orient));
    Vec2 b2 = Vec2Add(obb2d.c, Vec2Transform(Vec2Lit(-obb2d.hdims.x, obb2d.hdims.y), obb2d.orient));

    // Use the SAT test to determine collision.
    // Note, we could unroll this loop and eliminate a few operations when checking the aabb to make this faster,
    // but lets just focus on correctness for now.
    Vec2 axiss[] = {
        Vec2Lit(1.0f, 0.0f),
        Vec2Lit(0.0f, 1.0f),
        obb2d.orient.v.a,
        obb2d.orient.v.b,
    };
    for (isize i = 0; i < ArraySizeOf(axiss); ++i) {
        Vec2 axis = axiss[i];

        // Find the largest projection along the axis. Since the aabb is symmetric, we just need to check corners on the
        // same side to have checked all points.
        f32 ca = Vec2Dot(aabb2d.c, axis);
        f32 pa1 = Vec2Dot(a1, axis);
        f32 pa2 = Vec2Dot(a2, axis);
        // Save the max projected distance
        f32 ra = f32max(f32abs(pa1 - ca), f32abs(pa2 - ca));

        f32 cb = Vec2Dot(obb2d.c, axis);
        f32 pb1 = Vec2Dot(b1, axis);
        f32 pb2 = Vec2Dot(b2, axis);
        f32 rb = f32max(f32abs(pb1 - cb), f32abs(pb2 - cb));

        // If the combined projected interval is less than the projected interval between the centers, then
        // we have found a separating axis and have no collision.
        f32 d = (ra + rb) - f32abs(ca - cb);
        if (d < 0.0f) {
            // if (DEBUG) {
            //     breakpoint();
            // }
            // PlatformLog(LLOG_DEBUG, "aabb2d vs obb2d: ith: %d axis %f %f separates", i, axis.x, axis.y);
            return false;
        }

        if (d < *depth) {
            *depth = d;
            // If the projected center of a is not further along the axis than projected center of b, we need to negate
            // the separating axis so that we point towards body a.
            *normal = ca >= cb ? axis : Vec2Negate(axis);
            // if (DEBUG) {
            //     breakpoint();
            // }
            // NUMBER = i;
        }
    }

    // if (DEBUG) {
    //     breakpoint();
    // }
    // PlatformLog(LLOG_DEBUG, "aabb2d vs obb collision: %d. normal %f %f. depth: %f", NUMBER, normal->x, normal->y,
    //             *depth);
    return true;
}

// Uses SAT test. See AABB2D vs OBB2D for more comments.
LCORE bool TestOBB2DVsOBB2D(OBB2D a, OBB2D b, Vec2* normal, f32* depth) {
    // static u32 NUMBER = 0;
    // static bool DEBUG = false;

    *depth = F32_MAX;

    Vec2 a1 = Vec2Add(a.c, Vec2Transform(a.hdims, a.orient));
    Vec2 a2 = Vec2Add(a.c, Vec2Transform(Vec2Lit(-a.hdims.x, a.hdims.y), a.orient));

    Vec2 b1 = Vec2Add(b.c, Vec2Transform(b.hdims, b.orient));
    Vec2 b2 = Vec2Add(b.c, Vec2Transform(Vec2Lit(-b.hdims.x, b.hdims.y), b.orient));

    Vec2 axiss[] = {
        a.orient.v.a,
        a.orient.v.b,
        b.orient.v.a,
        b.orient.v.b,
    };
    isize size = ArraySizeOf(axiss);
    for (isize i = 0; i < size; ++i) {
        Vec2 axis = axiss[i];

        f32 pac = Vec2Dot(a.c, axis);
        f32 pa1 = Vec2Dot(a1, axis);
        f32 pa2 = Vec2Dot(a2, axis);
        f32 ra = f32max(f32abs(pa1 - pac), f32abs(pa2 - pac));

        f32 pbc = Vec2Dot(b.c, axis);
        f32 pb1 = Vec2Dot(b1, axis);
        f32 pb2 = Vec2Dot(b2, axis);
        f32 rb = f32max(f32abs(pb1 - pbc), f32abs(pb2 - pbc));

        f32 d = (ra + rb) - f32abs(pac - pbc);
        if (d < 0.0f) {
            return false;
        }

        // if (DEBUG) {
        //     breakpoint();
        // }

        if (d < *depth) {
            *depth = d;
            *normal = pac >= pbc ? axis : Vec2Negate(axis);
            // NUMBER = i;
        }
    }

    // breakpoint();
    // DEBUG = true;
    // PlatformLog(LLOG_DEBUG, "obb2d vs obb2d collision: %d. normal %f %f. depth: %f", NUMBER, normal->x, normal->y,
    //             *depth);

    return true;
}

typedef struct {
    Vec3 c;  // center.
    Vec3 r;  // half width extents along each axis.
} AABB;

typedef enum PolyhedronType {
    PH_NONE = 0,
    PH_CIRCLE,
    PH_SPHERE,
    PH_AABB2D,
    PH_OBB2D,
    PH_AABB,
    PH_COUNT
} PolyhedronType;

typedef struct {
    PolyhedronType type;

    union {
        Circle circle;
        AABB2D aabb2d;
        OBB2D obb2d;
        Sphere sphere;
        AABB aabb;
    };
} Polyhedron;

#define PolyhedronIsNull(p) ((p)->type == PH_NONE)

LCORE void PolyhedronInit(Polyhedron* p) {
    memset(p, 0, sizeof(Polyhedron));
    p->type = PH_NONE;
}

// lc: bottom left corner
// dims: full width/height of bbox
// grid_dims: width x height of sprite
// Useful for setting collision info based on visually drawn box in programs like aseprite.
// We assume that (0, 0) is the bottom left *corner* (not the center of the bottom left pixel).
// To translate from aseprite, count the number of pixels (where bottom left pixel counts as 1) until the first
// pixel of the collider, then subtract (-1, -1) to get the lc.
LCORE void PolyhedronSetAABB2DFromGridAseRect(Polyhedron* shape, Vec2 lc, Vec2 dims, Vec2 grid_dims) {
    shape->type = PH_AABB2D;

    // The input params assume that the min and max are given as if the origin is the bottom left
    // corner of the sprite, but our collision system assumes that the center of the bounding box is relative
    // to the center of the sprite/entity so we need to offset.
    Vec2 offset = Vec2Scale(grid_dims, -0.5f);

    // Compute half width/height.
    Vec2 r = Vec2Scale(dims, 0.5f);
    Vec2 c = Vec2Add(Vec2Add(lc, r), offset);

    shape->aabb2d.r = r;
    shape->aabb2d.c = c;
}

// Set AABB2D From center and full width and height.
LCORE void PolyHedronSetAABB2DFromWH(Polyhedron* shape, Vec2 c, Vec2 dims) {
    shape->type = PH_AABB2D;
    shape->aabb2d.c = c;
    shape->aabb2d.r = Vec2Lit(0.5f * dims.x, 0.5f * dims.y);
}

LCORE void PolyHedronSetAABB2D(Polyhedron* shape, AABB2D aabb) {
    shape->type = PH_AABB2D;
    shape->aabb2d = aabb;
}

LCORE void PolyhedronSetCircle(Polyhedron* shape, Vec2 c, f32 r) {
    shape->type = PH_CIRCLE;
    shape->circle.c = c;
    shape->circle.r = r;
}

// Set AABB2D From center and full width and height.
LCORE void PolyHedronSetOBB2D(Polyhedron* shape, Vec2 c, Vec2 dims, Mat2 orient) {
    shape->type = PH_OBB2D;
    shape->obb2d.c = c;
    shape->obb2d.hdims = Vec2Lit(0.5f * dims.x, 0.5f * dims.y);
    shape->obb2d.orient = orient;
}

// Important to pass decomposed transformation matrix (scale, rot, translate) because some polyhedron
// like world axis-aligned shapes do not rotate for instance.
// TODO: Consider making an interface where I get the corAserect polyhedron transformation matrix as a separate
// function (maybe take an entity as an arg) and then I can just take a single matrix here
LCORE Polyhedron PolyhedronTransform(Polyhedron shape, Vec3 s, Mat4 r, Vec3 t) {
    switch (shape.type) {
        case PH_NONE: {
            break;
        }
        // TODO: look at what we do for obb2d.
        case PH_CIRCLE: {
            // This might be a bit jank. but I transform the center with translation and then
            // drop the 3rd dimension (essentially projecting back to 2d).
            shape.circle.c = Vec3ToVec2(Vec3Add(Vec2ToVec3(shape.circle.c), t));
            // Essentially for scaling since a radius is directionless, we just scale by the maximal scale factor in 2d.
            shape.circle.r *= f32max(s.x, s.y);
            break;
        }
        case PH_AABB2D: {
            // This might be a bit jank. but I transform the center with translation and then
            // drop the 3rd dimension (essentially projecting back to 2d).
            // TODO: I think to be fully correct here I need to take into account scale.
            shape.aabb2d.c = Vec3ToVec2(Vec3Add(Vec2ToVec3(shape.aabb2d.c), t));
            // TODO: Do I need to take scale into account here? think about this more. This might be more correct.
            // Mat4 m = Mat4Multiply(Mat4TranslateV(t), Mat4ScaleV(s));
            // shape.aabb2d.c = Vec3ToVec2(Vec3WTransform(Vec2ToVec3(shape.aabb2d.c), m));
            shape.aabb2d.r.x *= s.x;
            shape.aabb2d.r.y *= s.y;
            break;
        }
        case PH_OBB2D: {
            // This should still result in an orthonormal matrix assuming r is already ortho normal.
            Mat2 mrotate = Mat4ToMat2(r);

            // This might be a bit jank. but I transform the center with translation and then
            // drop the 3rd dimension (essentially projecting back to 2d).
            shape.obb2d.c = Vec3ToVec2(Vec3Add(Vec2ToVec3(shape.obb2d.c), t));
            shape.obb2d.hdims.x *= s.x;
            shape.obb2d.hdims.y *= s.y;
            shape.obb2d.orient = Mat2Multiply(mrotate, shape.obb2d.orient);
            break;
        }
        case PH_AABB: {
            Mat4 m = Mat4Multiply(Mat4TranslateV(t), Mat4ScaleV(s));

            shape.aabb.c = Vec3WTransform(shape.aabb.c, m);
            shape.aabb.r = Vec3Transform(shape.aabb.r, m);
            break;
        }
        default: {
            Assert(false);
        }
    }
    return shape;
}

// static Polyhedron ComputePolyhedron(Vec3* vs, u32 vsize, PolyhedronType type) {
//     Polyhedron p = {0};
//     p.type = type;
//     switch (type) {
//         case PH_SPHERE: {
//             p.sphere = BoundingSphere(vs, vsize);
//             break;
//         }
//         case PH_AABB: {
//             p.aabb = ComputeAABB(vs, vsize);
//             break;
//         }
//         default: {
//             PlatformLog(LLOG_ERROR,
//                      "ComputePolyhedron: Unknown polyhedron type=%d. Most likely entity needs bounding volume type
//                      set", type);
//             Assert(false);
//         }
//     }
//     return p;
// }

// Returns true if both polyhedrons collide. normal will point from b to a.
// Assumes polyhedrons are both in the same coordinate space.
LCORE bool TestPolyhedronVsPolyhedron(Polyhedron* a, Polyhedron* b, Vec3* normal, f32* depth) {
    bool collides = false;
    Vec3 n = Vec3Zero();
    Vec2 n2d = Vec2Zero();
    f32 d = 0.0f;

    // Collision shape table.
    switch (a->type) {
        case PH_CIRCLE: {
            switch (b->type) {
                case PH_CIRCLE: {
                    collides = TestCircleCircle(a->circle, b->circle, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                case PH_AABB2D: {
                    collides = TestCircleVsAABB2D(a->circle, b->aabb2d, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                case PH_OBB2D: {
                    collides = TestCircleVsOBB2D(a->circle, b->obb2d, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                default: {
                    // Collision not supported.
                    PlatformLog(LLOG_WARN, "Collision type between %d and %d not supported", a->type, b->type);
                    break;
                }
            }
            break;
        }
        case PH_AABB2D: {
            switch (b->type) {
                case PH_CIRCLE: {
                    collides = TestCircleVsAABB2D(b->circle, a->aabb2d, &n2d, &d);
                    n = Vec2ToVec3(Vec2Negate(n2d));
                    break;
                }
                case PH_AABB2D: {
                    collides = TestAABB2DVsAABB2D(a->aabb2d, b->aabb2d, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                case PH_OBB2D: {
                    collides = TestAABB2DvsOBB2D2(a->aabb2d, b->obb2d, &n2d, &d);
                    n = Vec3Lit(n2d.x, n2d.y, 0.0f);
                    break;
                }
                default: {
                    // Collision not supported.
                    PlatformLog(LLOG_WARN, "Collision type between %d and %d not supported", a->type, b->type);
                    break;
                }
            }
            break;
        }
        case PH_OBB2D: {
            switch (b->type) {
                case PH_CIRCLE: {
                    collides = TestCircleVsOBB2D(b->circle, a->obb2d, &n2d, &d);
                    n = Vec2ToVec3(Vec2Negate(n2d));
                    break;
                }
                case PH_AABB2D: {
                    collides = TestAABB2DvsOBB2D2(b->aabb2d, a->obb2d, &n2d, &d);
                    n = Vec2ToVec3(Vec2Negate(n2d));
                    break;
                }
                case PH_OBB2D: {
                    collides = TestOBB2DVsOBB2D(a->obb2d, b->obb2d, &n2d, &d);
                    n = Vec2ToVec3(n2d);
                    break;
                }
                default: {
                    // Collision not supported.
                    PlatformLog(LLOG_WARN, "Collision type between %d and %d not supported", a->type, b->type);
                    break;
                }
            }
            break;
        }
        default: {
            // Collision not supported.
            PlatformLog(LLOG_WARN, "Collision type between %d and %d not supported", a->type, b->type);
            break;
        }
    }

    *normal = n;
    *depth = d;
    return collides;
}

LCORE void DrawPolyhedron(Polyhedron* shape, Vec4 color) {
    switch (shape->type) {
        case PH_NONE: {
            break;
        }
        case PH_CIRCLE: {
            DrawLCircle(Vec3Lit(shape->circle.c.x, shape->circle.c.y, 0.0f), shape->circle.r, color);
            break;
        }
        case PH_AABB2D: {
            // Because we store half extents, scale them up by 2x to get the proper visual scaling factor.
            DrawQuad(Vec3Lit(shape->aabb2d.c.x, shape->aabb2d.c.y, 0.0f), 2.0f * shape->aabb2d.r.x,
                     2.0f * shape->aabb2d.r.y, Mat4Identity(), color);
            break;
        }
        case PH_AABB: {
            // Because we store half extents, scale them up by 2x to get the proper visual scaling factor.
            DrawLCube(shape->aabb.c, Vec3Scale(shape->aabb.r, 2.0f), color);
            break;
        }
        default: {
            Assert(false);
        }
    }
}

////////////////////////////////////////////////////////////
// S:Font

// Loads a true type file at :filename and bakes it into a 512x512 texture containing
// characters at :fontsize size.
static LTexture BakeTrueTypeFontBitmap(char* filename, f32 fontsize, stbtt_bakedchar* cdata) {
    byte ttf_buffer[1 << 20];
    byte bitmap_alpha[512 * 512];
    fread(ttf_buffer, 1, 1 << 20, fopen(filename, "rb"));
    stbtt_BakeFontBitmap(ttf_buffer, 0, fontsize, bitmap_alpha, 512, 512, 32, 96, cdata);

    // Output alpha bitmap to pgm file (greyscal file). viewable in GIMP for debugging.
    if (false) {
        FILE* out = fopen("C:/Users/rvbea/scratch/font_dump.pgm", "wb");
        if (out) {
            fprintf(out, "P5\n%d %d\n255\n", 512, 512);
            fwrite(bitmap_alpha, 1, 512 * 512, out);
            fclose(out);
        } else {
            perror("Failed to open font_dump.pgm for writing");
        }
    }

    // The baked bitmap assumes origin in top left with +y pointing down. Flip the engine expects
    // origin in bottom left with +y going up.
    for (i32 i = 0; i < (512 * 512) / 2; ++i) {
        i32 col = i % 512;
        i32 top_row = i / 512;
        i32 bottom_row = 512 - top_row - 1;
        i32 top_idx = (top_row * 512) + col;
        i32 bottom_idx = (bottom_row * 512) + col;

        byte temp = bitmap_alpha[top_idx];
        bitmap_alpha[top_idx] = bitmap_alpha[bottom_idx];
        bitmap_alpha[bottom_idx] = temp;
    }

    if (false) {
        FILE* out = fopen("C:/Users/rvbea/scratch/font_dump_flipped.pgm", "wb");
        if (out) {
            fprintf(out, "P5\n%d %d\n255\n", 512, 512);
            fwrite(bitmap_alpha, 1, 512 * 512, out);
            fclose(out);
        } else {
            perror("Failed to open font_dump.pgm for writing");
        }
    }

    // The baked bitmap contains only alpha channel information. Convert to rgba so that we can draw
    // this with our normal shader. yes this is a bit wasteful.
    byte* bitmap = calloc(512 * 512 * 4, sizeof(byte));
    for (i32 i = 0; i < (512 * 512); ++i) {
        bitmap[(i * 4) + 0] = 255;
        bitmap[(i * 4) + 1] = 255;
        bitmap[(i * 4) + 2] = 255;
        bitmap[(i * 4) + 3] = bitmap_alpha[i];
    }

    LTexture tx = TextureFromBuffer(bitmap, 512, 512, FILTER_LINEAR);
    // free(bitmap);
    return tx;
}

typedef struct {
    LTexture tx;
    f32 fontsize;
    stbtt_bakedchar cdata[96];  // ASCII 32..126 is 95 glyphs
} LFont;

typedef enum {
    LFONT_DEFAULT = 0,
    LFONT_SMALL,
    LFONT_COUNT,
} FontLabel;

static LFont FONTS[LFONT_COUNT] = {0};

#define MAX_FONTS 128

typedef struct {
    LFont data[10];
    isize len;
    isize cap;
} FontStack;

typedef struct {
    isize labels[LFONT_COUNT];

    // Font sizes acts as a map between fontsize and an LFont.
    f32 fontsizes[MAX_FONTS];
    LFont fonts[MAX_FONTS];

    FontStack fontstack;
} FontCache;

static FontCache FONT_CACHE = {0};

LCORE void FontCacheInit(FontCache* fcache) {
    FontCache zero = {0};
    *fcache = zero;

    // Preload the cache with some amount of font sizes which we can use
    // for development purposes to draw at various fonts sizes without thinking much.
    // Remember this is probably less than audio memory footprint.
    {
        isize i = 0;
        for (f32 fontsize = 8.0f; fontsize < 60.0f; fontsize += 0.5f, ++i) {
            Assert(i < ArraySizeOf(fcache->fonts));
            LFont* font = &fcache->fonts[i];
            font->fontsize = fontsize;
            font->tx = BakeTrueTypeFontBitmap("c:/windows/fonts/CascadiaMono.ttf", font->fontsize, font->cdata);
            fcache->fontsizes[i] = fontsize;
        }
    }

    memset(FONTS, 0, sizeof(LFont) * LFONT_COUNT);

    {
        LFont font = {0};
        font.fontsize = 32.0f;
        font.tx = BakeTrueTypeFontBitmap("c:/windows/fonts/CascadiaMono.ttf", font.fontsize, font.cdata);
        FONTS[LFONT_DEFAULT] = font;
    }

    {
        LFont font = {0};
        font.fontsize = 16.0f;
        font.tx = BakeTrueTypeFontBitmap("c:/windows/fonts/CascadiaMono.ttf", font.fontsize, font.cdata);
        FONTS[LFONT_SMALL] = font;
    }

    // Push default font as current selected font.
    fcache->fontstack.cap = ArraySizeOf(fcache->fontstack.data);
    ArrPush(&fcache->fontstack, FONTS[LFONT_DEFAULT]);
}

LCORE LFont GetActiveFont() {
    FontCache* fcache = &FONT_CACHE;
    Assert(fcache->fontstack.len);
    return ArrGet(&fcache->fontstack, fcache->fontstack.len - 1);
}

LCORE f32 GetActiveFontsize() {
    FontCache* fcache = &FONT_CACHE;
    Assert(fcache->fontstack.len);
    return ArrGet(&fcache->fontstack, fcache->fontstack.len - 1).fontsize;
}

LCORE void PushLFont(LFont font) {
    FontCache* fcache = &FONT_CACHE;
    ArrPush(&fcache->fontstack, font);
}

LCORE void PopLFont() {
    FontCache* fcache = &FONT_CACHE;
    Assert(fcache->fontstack.len > 1);
    ArrPop(&fcache->fontstack);
}

LCORE void InitFonts() {
    FontCache* fcache = &FONT_CACHE;
    FontCacheInit(fcache);
}

// TODO: Maybe remove this.
LCORE LFont GetLFontByLabel(FontLabel label) {
    Assert(label < LFONT_COUNT);
    return FONTS[label];
}

// Will get the closest font size in cache that is <= :fontsize: unless there is no such fontsize
// in which case it will use the first/smallest fontsize
LCORE LFont GetLFontClosest(f32 desired_fontsize) {
    FontCache* fcache = &FONT_CACHE;
    isize choice = 0;
    for (isize i = 0; i < ArraySizeOf(fcache->fonts); ++i) {
        if (fcache->fontsizes[i] > desired_fontsize) {
            break;
        }

        choice = i;
    }

    return fcache->fonts[choice];
}

LCORE Vec2 DrawLTextEx(LFont font, char* text, i32 len, Vec2 baseline_pos, Vec4 color) {
    for (i32 i = 0; i < len; ++i) {
        stbtt_aligned_quad quad = {0};
        // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        // NOTE: Although we pass pointers to the baseline_pos, it doesn't look like GetBakedQuad actually modifies it.
        stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
                           &quad, true);

        /*
            Remember. the key to understanding all of these transformations is to visualize them from the perspective
            of the originating coordinate system (stb_truetype's coordinate system). If you try to visualize
            them from the perspective of the final coordinate system, it will be hard to do.
        */

        Vec2 qdims = Vec2Sub(Vec2Lit(quad.x1, quad.y1), Vec2Lit(quad.x0, quad.y0));
        // Note we don't need to flip anything about the quads world space positions even though stb_truetype assumes
        // origin top left and +Y is down. The rendering position data will work.
        Vec2 qorigin = {0};
        qorigin.x = quad.x0 + (0.5f * qdims.x);
        // We need to flip the y position coords since stb_truetype assumes that positioning is relative to a
        // baseline where the origin is top left and +y is down. Our pos.y is our baseline, so to reflect
        // we take the height relative to the baseline and subtract that from the baseline. Think of this as if
        // we were in the coordinate space of stb_truetype. we have a baseline pos.y and some position quad.y1
        // which is the highest y position of the glyph. We need to subtract the height delta from the baseline
        // to reflect in stb_truetype coordinate system which will result in upright positions in lcore engine.
        f32 height_relative_to_baseline = quad.y1 - baseline_pos.y;
        qorigin.y = (baseline_pos.y - height_relative_to_baseline) + (0.5f * qdims.y);

        // Don't need to modify anything to calculate the dims since dimensions are not in reference of a particular
        // coordinate system.
        Vec2 quvdims = Vec2Sub(Vec2Lit(quad.s1, quad.t1), Vec2Lit(quad.s0, quad.t0));
        Vec2 quvoffset = {0};
        quvoffset.x = quad.s0;
        // We do need to flip the y text coords because we have flipped the texture after baking.
        // 1 - t0 will map the top left of the sub rect to the new top left of the flipped texture.
        // By subtracting the height we then get the bottom left corner which is what the engine expects.
        quvoffset.y = 1.0f - quad.t0 - quvdims.y;

        // if (true) {
        //     DrawQuad(Vec2ToVec3(qorigin), qdims.x, qdims.y, Mat4Identity(), LRED);
        // }
        DrawLTextureProUV(font.tx, quvoffset, quvdims, qorigin, qdims, 0.0f, color);
    }
    return baseline_pos;
}

LCORE void DrawLTextExF(LFont font, Vec2 pos, Vec4 color, const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DrawLTextEx(font, buffer, strlen(buffer), pos, color);
}

// // This will return the dimensions of the quad that would be necessary to span the entire text (i.e. if somehow
// // the text was baked into a texture, what quad dims would be needed to draw the texture).
// LCORE Vec2 LFontMeasureText(LFont font, char* text, i32 len) {
//     Vec2 br = Vec2Lit(FLT_MIN, FLT_MIN);
//     Vec2 tl = Vec2Lit(FLT_MAX, FLT_MAX);

//     Vec2 baseline_pos = Vec2Zero();

//     for (i32 i = 0; i < len; ++i) {
//         stbtt_aligned_quad quad = {0};
//         // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
//         stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
//                            &quad, true);

//         tl.x = f32min(tl.x, quad.x0);
//         tl.y = f32min(tl.y, quad.y0);

//         br.x = f32max(br.x, quad.x1);
//         br.y = f32max(br.y, quad.y1);
//     }

//     Vec2 dims = Vec2Sub(br, tl);
//     return dims;
// }

typedef struct {
    AABB2D aabb;  // bl at origin. quad to use for drawing the text.
    // This gives the vector going from center to baseline pos. So if you add to center, you get baseline pos.
    Vec2 baseline_offset;  // offset from the center of where the baseline exists. i.e. where to draw text from such
                           // that it will be centered in the quad.
} TextBox;

// Returns a positioned quad that is centered and sized such that if the center is drawn at baseline_pos + quad_center,
// the quad should span the text. Importantly we will move the quad such that the bottom left corner is positioned at
// (0, 0) so that this quad can be used for UI layout calculations.
LCORE TextBox LFontMeasureStr8Quad(LFont font, Str8 str) {
    i32 len = str.len;
    char* text = str.data;

    Vec2 bl = Vec2Lit(F32_MAX, F32_MAX);
    Vec2 tr = Vec2Lit(F32_MIN, F32_MIN);

    Vec2 baseline_pos = Vec2Zero();

    for (i32 i = 0; i < len; ++i) {
        stbtt_aligned_quad quad = {0};
        // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
                           &quad, true);

        /*
        Remember. the key to understanding all of these transformations is to visualize them from the perspective
        of the originating coordinate system (stb_truetype's coordinate system). If you try to visualize
        them from the perspective of the final coordinate system, it will be hard to do.
        */

        Vec2 qdims = Vec2Sub(Vec2Lit(quad.x1, quad.y1), Vec2Lit(quad.x0, quad.y0));
        // Note we don't need to flip anything about the quads world space positions even though stb_truetype assumes
        // origin top left and +Y is down. The rendering position data will work.
        Vec2 qorigin = {0};
        qorigin.x = quad.x0 + (0.5f * qdims.x);
        // We need to flip the y position coords since stb_truetype assumes that positioning is relative to a
        // baseline where the origin is top left and +y is down. Our pos.y is our baseline, so to reflect
        // we take the height relative to the baseline and subtract that from the baseline. Think of this as if
        // we were in the coordinate space of stb_truetype. we have a baseline pos.y and some position quad.y1
        // which is the highest y position of the glyph. We need to subtract the height delta from the baseline
        // to reflect in stb_truetype coordinate system which will result in upright positions in lcore engine.
        f32 height_relative_to_baseline = quad.y1 - baseline_pos.y;
        qorigin.y = (baseline_pos.y - height_relative_to_baseline) + (0.5f * qdims.y);

        bl.x = f32min(bl.x, qorigin.x - (0.5f * qdims.x));
        bl.y = f32min(bl.y, qorigin.y - (0.5f * qdims.y));

        tr.x = f32max(tr.x, qorigin.x + (0.5f * qdims.x));
        tr.y = f32max(tr.y, qorigin.y + (0.5f * qdims.y));
    }

    // Position bottom left of quad at the origin (0, 0)
    Vec2 baseline = Vec2Sub(Vec2Zero(), bl);
    tr = Vec2Sub(tr, bl);
    bl = Vec2Zero();
    AABB2D aabb = AABB2DFromMinMax(bl, tr);
    TextBox tbox = {.aabb = aabb, .baseline_offset = Vec2Sub(baseline, aabb.c)};
    return tbox;
}

// Assumes the font texture contains the ascii printable characters on a single line.
// Characters (note the space beginning the sequence is included):
// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
LCORE void DrawLText(char* text, u32 len, Vec2 pos, Vec4 color) {
    DrawLTextEx(GetActiveFont(), text, len, Vec2Lit(pos.x, pos.y), color);
}

LCORE void DrawStr8Ex(LFont font, Vec2 pos, Vec4 color, Str8 str) { DrawLTextEx(font, str.data, str.len, pos, color); }

LCORE void DrawLTextF(Vec2 pos, Vec4 color, const char* format, ...) {
    char buffer[1024];

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DrawLText(buffer, strlen(buffer), pos, color);
}

////////////////////////////////////////////////////////////
// S:Sound

typedef enum {
    SOUND_NONE = 0,
    SOUND_BEEP,
    SOUND_COUNT,
} SoundTag;

static SoundId SOUNDS[SOUND_COUNT] = {0};

#define GetSound(sid) (SOUNDS[(sid)])

static void LoadSounds() {
    static bool INIT = false;
    if (INIT) {
        return;
    }
    INIT = true;
    SOUNDS[SOUND_NONE] = 0;
    SOUNDS[SOUND_BEEP] = PlatformLoadSound("assets/sounds/beep.wav");
}

////////////////////////////////////////////////////////////
typedef enum {
    PF_NONE = 1 << 0,
    PF_ACTIVE = 1 << 1,
    PF_GRAVITY = 1 << 2,
} ParticleFlag;

typedef struct {
    ParticleFlag flags;

    Vec3 p;
    Vec3 v;
    Vec3 scale;
    Vec4 color;
    Mat4 orient;

    f32 lifetime;
    f32 lifedt;
    // For the first x% of the lifetime of the particle. linearly fade from transparent to color.alpha.
    f32 fade_in_pct;
    // For the last x% of the lifetime of the particle. linearly fade from color.alpha to transparent.
    // By default set to 1.0 for the full range.
    f32 fade_out_pct;
    // For the first x% of lifetime, scale in from 0 to full size.
    f32 scale_in_pct;
    // For the last x% of lifetime, scale out from full size to 0.
    f32 scale_out_pct;

} Particle;

LCORE void ParticleSetFadeIn(Particle* particle, f32 fade_in_pct) {
    particle->fade_in_pct = fade_in_pct;
    f32 rem = 1.0f - (particle->fade_in_pct + particle->fade_out_pct);
    particle->fade_out_pct += rem < 0.0f ? rem : 0.0f;
}

LCORE void ParticleSetFadeOut(Particle* particle, f32 fade_out_pct) {
    particle->fade_out_pct = fade_out_pct;
    f32 rem = 1.0f - (particle->fade_out_pct + particle->fade_out_pct);
    particle->fade_in_pct += rem < 0.0f ? rem : 0.0f;
}

#define MAX_PARTICLES 512

typedef struct {
    Particle particles[MAX_PARTICLES];
    u32 pcap;
} ParticleSystem;

static ParticleSystem PARTICLES;

#define GetParticleSystem() (&PARTICLES)

static void ParticleSystemInit() {
    ParticleSystem* particles = GetParticleSystem();
    ParticleSystem zero = {0};
    *particles = zero;
    particles->pcap = MAX_PARTICLES;
}

LCORE Particle* AllocateParticle() {
    ParticleSystem* particles = GetParticleSystem();
    Particle* particle = NULL;
    for (i32 i = 0; i < particles->pcap; ++i) {
        if (!TestFlag(particles->particles[i].flags, PF_ACTIVE)) {
            particle = &particles->particles[i];
            break;
        }
    }
    Assert(particle != NULL);
    memset(particle, 0, sizeof(Particle));
    particle->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    particle->orient = Mat4Identity();
    particle->fade_out_pct = 1.0f;
    FlagOn(particle->flags, PF_ACTIVE);
    return particle;
}

LCORE Particle* SpawnParticle(Vec3 p, Vec2 dims, Mat4 orient, f32 lifetime, RGBA color) {
    Assert(lifetime > 0.0f);
    Particle* particle = AllocateParticle();
    particle->p = p;
    particle->scale = Vec3Lit(dims.x, dims.y, 1.0f);
    particle->orient = orient;
    particle->lifetime = lifetime;
    particle->color = color;
    return particle;
}

LCORE void ParticleSystemTick(f32 dt) {
    ParticleSystem* particles = GetParticleSystem();
    for (u32 i = 0; i < particles->pcap; ++i) {
        Particle* particle = &particles->particles[i];
        particle->lifedt += dt;
        if (particle->lifedt >= particle->lifetime) {
            FlagOff(particle->flags, PF_ACTIVE);
            continue;
        }

        if (TestFlag(particle->flags, PF_GRAVITY)) {
            Vec3 G = Vec3Lit(0.0f, -350.0f, 0.0f);
            particle->v = Vec3Add(particle->v, Vec3Scale(G, dt));
        }
        particle->p = Vec3Add(particle->p, Vec3Scale(particle->v, dt));
    }
}

LCORE void ParticleSystemDraw() {
    ParticleSystem* particles = GetParticleSystem();
    for (u32 i = 0; i < particles->pcap; ++i) {
        Particle* particle = &particles->particles[i];

        // Find % along lifetime we are.
        f32 lifedt_pct = f32lerp_pct(0.0f, particle->lifetime, particle->lifedt);

        f32 alpha_mask = 1.0f;
        if (particle->fade_in_pct != 0.0f) {
            // Linearly fade in if we are within the fade in window by the percent we are in the fade in window.
            // Remember that multiplying here is essentially our lerp because lerp(0, e, x) = e * x. X is
            // already a percentage in range 0..1 because of f32lerp_pct.
            alpha_mask *= f32lerp_pct(0.0f, particle->fade_in_pct, lifedt_pct);
        }
        // Determine the % of the way through we are through the fade out window. Since our fade out window
        // is given in percent of the ending (i.e. relative). We need to subtract 100% - fadeout pct to get
        // absolute percent. then we can determine percent we are through using abs percent. Need to do
        // 1.0f - lerp pct because we are fading out not in.
        // Because we are fading out f32lerp(color.a, 0.0f, x) this reduces to (1.0f - x) * color.a
        // Which reduces to alpha *= 1.0f - x where x is percentage way through fade out window.
        alpha_mask *= 1.0f - f32lerp_pct(1.0f - particle->fade_out_pct, 1.0f, lifedt_pct);
        RGBA color = particle->color;
        color.a *= alpha_mask;

        f32 scale_mask = 1.0f;
        if (particle->scale_in_pct != 0.0f) {
            scale_mask *= f32lerp_pct(0.0f, particle->scale_in_pct, lifedt_pct);
        }
        if (particle->scale_out_pct != 0.0f) {
            scale_mask *= 1.0f - f32lerp_pct(1.0f - particle->scale_out_pct, 1.0f, lifedt_pct);
        }
        Vec3 scale = Vec3Scale(particle->scale, scale_mask);

        DrawQuad(particle->p, scale.x, scale.y, particle->orient, color);
        // DrawCmd* cmd = AddDrawMeshCmd();
        // cmd->meshcmd.modelm = Mat4SVRT(particle->scale, particle->orient, particle->p);
        // cmd->meshcmd.meshid = GetMesh(M_QUAD);
        // cmd->meshcmd.color = particle->color;
    }
}

////////////////////////////////////////////////////////////

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawSprite(Sprite sprite, Vec3 pos, Mat4 orient, Vec3 scale) {
    // By default scale of 1 will mean draw the sprite with same width/height
    DrawCmd* cmd = DrawQuad(pos, sprite.dims.x * scale.x, sprite.dims.y * scale.y, orient, LCLEAR);
    cmd->meshcmd.tx = sprite.tx;
    cmd->meshcmd.uvoffset = sprite.minuv;
    cmd->meshcmd.uvscale = sprite.dimsuv;
    return cmd;
}

// Rect in aseprite pos space.
typedef struct {
    Vec2 lc;  // bottom left corner.
    Vec2 r;   // width + height.
} AseRect;

typedef struct {
    AseRect start;  // Start Aserect of the animation.
    Vec2 step;      // step to move each frame in aseprite coord space.
    u32 nframes;
} AnimationAseRect;

// A model is basically a "static" set of geometry and visual info data.
// We can load a model that has been created and durably stored a priori once and then
// just refer to it in many places (e.g. load the idle state model and swap out with jump state model).
typedef struct {
    // The underlying geometry info.
    // These transform fields are "static", i.e. they are just set once on model load.
    // Allows for easy persistent transform (i.e. load this model rotated by 90 degrees).
    Vec3 pos;
    Mat4 orient;
    Vec3 scale;
    Vec4 tint;  // Color.
    LMesh mesh;

    // This is like the material info basically. How the model "looks"
    // Unclear where custom shader info will need to go, maybe on the graphics state?
    Animation animation;
} LModel;

LCORE LModel LModelFromAnimation(Animation animation, Vec3 pos, Mat4 orient, Vec3 scale, Vec4 tint) {
    LModel model = {
        .pos = pos,
        .orient = orient,
        .scale = scale,
        .tint = tint,
        .mesh = GetMesh(M_QUAD),
        .animation = animation,
    };
    return model;
}

LCORE LModel LModelFromSprite(Sprite sprite, Vec3 pos, Mat4 orient, Vec3 scale, Vec4 tint) {
    return LModelFromAnimation(AnimationFromSprite(sprite), pos, orient,
                               Vec3Lit(sprite.dims.x * scale.x, sprite.dims.y * scale.y, scale.z), tint);
}

LCORE LModel LModelFromMesh(LMesh mesh, LTexture texture, Vec3 pos, Mat4 orient, Vec3 scale, Vec4 tint) {
    LModel model = {
        .pos = pos,
        .orient = orient,
        .scale = scale,
        .tint = tint,
        .mesh = mesh,
        .animation = AnimationFromSprite(SpriteFromLTexture(texture, SBB_FULL)),
    };
    return model;
}

// TODO: We should take in a blend tint here.
LCORE DrawCmd* DrawLModelEx(LModel model, isize frame_idx, Vec3 pos, Mat4 orient, Vec3 scale) {
    Assert(model.animation.sprites.len > frame_idx);
    // Draw the model at its base scale multiplied by the passed in scale.
    Sprite* sprite = ArrGetR(&model.animation.sprites, frame_idx);
    // We don't want to derive size from the texture itself. We want to be explicit about the scaling/dimensions of the
    // mesh we are drawing. if we want to match the sprite/texture, then that should be controlled by a high level
    // wrapper
    f32 scalex = scale.x * model.scale.x;  // * sprite->dims.x;
    f32 scaley = scale.y * model.scale.y;  // * sprite->dims.y;
    f32 scalez = scale.z * model.scale.z;  // * sprite->dims.z;

    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = model.mesh;
    cmd->meshcmd.modelm =
        Mat4SVRT(Vec3Lit(scalex, scaley, scalez), Mat4Multiply(orient, model.orient), Vec3Add(pos, model.pos));
    cmd->meshcmd.color = model.tint;

    // Draw first frame of animation (if its an animation, else same as Drawsprite).
    cmd->meshcmd.tx = sprite->tx;
    cmd->meshcmd.uvoffset = sprite->minuv;
    cmd->meshcmd.uvscale = sprite->dimsuv;
    return cmd;
}

LCORE DrawCmd* DrawLModelEx2(LModel model, isize frame_idx, Mat4 world_mat) {
    Assert(model.animation.sprites.len > frame_idx);
    // Draw the model at its base scale multiplied by the passed in scale.
    Sprite* sprite = ArrGetR(&model.animation.sprites, frame_idx);

    Mat4 local_mat = Mat4SVRT(model.scale, model.orient, model.pos);

    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = model.mesh;
    cmd->meshcmd.modelm = Mat4Multiply(world_mat, local_mat);
    cmd->meshcmd.color = model.tint;

    // Draw first frame of animation (if its an animation, else same as Drawsprite).
    cmd->meshcmd.tx = sprite->tx;
    cmd->meshcmd.uvoffset = sprite->minuv;
    cmd->meshcmd.uvscale = sprite->dimsuv;
    return cmd;
}

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawLModel(LModel model, Vec3 pos, Mat4 orient, Vec3 scale) {
    return DrawLModelEx(model, 0, pos, orient, scale);
}

typedef enum {
    ET_NONE = 0,     // No easing, error!
    ET_SMOOTH_LERP,  // Uses exponential damped lerp: https://www.youtube.com/watch?v=YJB1QnEmlTs
    ET_IN_EXPO,
    ET_OUT_BACK,
} EasingType;

typedef struct {
    EasingType type;

    f32 T;

    union {
        struct {
            f32 rate;  // This should be between 0 and 1. Essentially after 1s how much of the error should remain?
        } lerp;
    };
} Easing;

static void EasingZero(Easing* easing) {
    Easing e = {0};
    e.type = ET_NONE;
    *easing = e;
}

static Easing EaseSmoothLerp(f32 rate) { return (Easing){.type = ET_SMOOTH_LERP, .lerp = {.rate = rate}}; }

static Vec3 EaseVec3(Vec3 cur, Vec3 target, f32 dt, Easing easing) {
    f32 T = easing.T;

    switch (easing.type) {
        case ET_SMOOTH_LERP: {
            T = 1.0f - f32pow(easing.lerp.rate, dt);
            break;
        }
        case ET_IN_EXPO: {
            T = T == 0.0f ? 0.0f : f32pow(2.0f, 10.0f * T - 10.0f);
            break;
        }
        case ET_OUT_BACK: {
            static f32 c1 = 1.70158;
            f32 c3 = c1 + 1;

            T = 1.0f + c3 * f32pow(T - 1.0f, 3.0f) + c1 * f32pow(T - 1.0f, 2.0f);
            T = f32sin(T * LPI);
            break;
        }
        case ET_NONE:
        default: {
            Assert(0);
        }
    }

    cur = Vec3Lerp(cur, target, T);
    return cur;
}

typedef enum {
    VA_SNAP_TO = 0,
    VA_MOVE_TO = 1,
    VA_ANIMATE_TO = 2,
} VisualAnimationType;

typedef struct {
    // Visual attributes.
    Vec3 pos;
    Vec3 scale;
    Mat4 orient;

    Mat4 transform;

    Vec3 pos_start;
    Vec3 pos_end;
    Easing pos_easing;
    VisualAnimationType pos_type;

    Vec3 scale_start;
    Vec3 scale_end;
    Easing scale_easing;
    VisualAnimationType scale_type;

    LModel model;
    isize frame_idx;
    f32 elapsed;
    i32 nloops;

    // If true, we will render the animation flipped.
    bool facing_left;
} GraphicsInfo;

static void GraphicsInfoMemset(GraphicsInfo* graphics) {
    memset(graphics, 0, sizeof(GraphicsInfo));
    graphics->orient = Mat4Identity();
    graphics->pos = Vec3Zero();
    graphics->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    graphics->facing_left = false;

    EasingZero(&graphics->pos_easing);

    graphics->model = LModelFromAnimation(DefaultAnimation(), Vec3Zero(), Mat4Identity(), Vec3LitAll(1.0f), LCLEAR);
}

// The key thing here is that elapsed_delta should be in the same units as the graphics elapsed.
// Remember elapsed will not always be time in the case where it might be like distance traveled or something.
LCORE void GraphicsInfoTickAnimation(GraphicsInfo* graphics, f32 elapsed_delta) {
    graphics->elapsed += elapsed_delta;
    if (graphics->elapsed >= graphics->model.animation.duration) {
        if (graphics->model.animation.loops) {
            graphics->elapsed = f32mod(graphics->elapsed, graphics->model.animation.duration);
            ++graphics->nloops;
        } else {
            graphics->elapsed = graphics->model.animation.duration;
            graphics->nloops = 1;
        }
    }

    // minus 1 is important for case where elapsed == duration.
    graphics->frame_idx = (isize)((graphics->elapsed / graphics->model.animation.duration) *
                                  ((f32)(graphics->model.animation.sprites.len - 1)));
}

LCORE void GraphicsInfoTick(GraphicsInfo* graphics, f32 dt, Vec3 pos_game, Vec3 scale_game, Mat4 orient_game) {
    switch (graphics->pos_type) {
        case VA_SNAP_TO: {
            graphics->pos = pos_game;
            break;
        }
        case VA_ANIMATE_TO: {
            graphics->pos = EaseVec3(graphics->pos_start, graphics->pos_end, dt, graphics->pos_easing);
            break;
        }
        case VA_MOVE_TO: {
            graphics->pos = EaseVec3(graphics->pos, pos_game, dt, graphics->pos_easing);
            break;
        }
    }

    // e->graphics.pos = EaseVec3(e->graphics.pos, e->pos, dt, e->graphics.pos_easing);
    // if (TestFlag(e->flags, EFLAG_WORKER | EFLAG_RESEARCHER)) {
    //     // TODO: Rework this.
    //     static f32 ROT = 0.0f;

    //     ROT = f32lerp(Deg2Rad(-1.5f), Deg2Rad(1.5f), f32sin(1.3f * gstate->timer));

    //     e->graphics.orient = Mat4RotateZ(ROT);
    // } else {
    //     e->graphics.orient = e->orient;
    // }
    graphics->orient = orient_game;

    switch (graphics->scale_type) {
        case VA_SNAP_TO: {
            graphics->scale = scale_game;
            break;
        }
        case VA_ANIMATE_TO: {
            graphics->scale = EaseVec3(graphics->scale_start, graphics->scale_end, dt, graphics->scale_easing);
            break;
        }
        case VA_MOVE_TO: {
            graphics->scale = EaseVec3(graphics->scale, scale_game, dt, graphics->scale_easing);
            break;
        }
    }

    GraphicsInfoTickAnimation(graphics, dt);
}

#define GraphicsInfoPositionSnapTo(gg) ((gg)->pos_type = VA_SNAP_TO)

LCORE void GraphicsInfoPositionMoveTo(GraphicsInfo* graphics, f32 rate) {
    graphics->pos_type = VA_MOVE_TO;
    graphics->pos_easing = EaseSmoothLerp(rate);
}

LCORE void GraphicsInfoPositionAnimateTo(GraphicsInfo* graphics, Vec3 start, Vec3 end, Easing easing) {
    graphics->pos_type = VA_ANIMATE_TO;
    graphics->pos_start = start;
    graphics->pos_end = end;
    graphics->pos_easing = easing;
}

#define GraphicsInfoScaleSnapTo(gg) ((gg)->scale_type = VA_SNAP_TO)

LCORE void GraphicsInfoScaleMoveTo(GraphicsInfo* graphics, f32 rate) {
    graphics->scale_type = VA_MOVE_TO;
    graphics->scale_easing = EaseSmoothLerp(rate);
}

LCORE void GraphicsInfoScaleAnimateTo(GraphicsInfo* graphics, Vec3 start, Vec3 end, Easing easing) {
    graphics->scale_type = VA_ANIMATE_TO;
    graphics->scale_start = start;
    graphics->scale_end = end;
    graphics->scale_easing = easing;
}

LCORE void GraphicsInfoSetLModel(GraphicsInfo* graphics, LModel model) {
    graphics->model = model;
    graphics->elapsed = 0.0f;
    graphics->frame_idx = 0;
    graphics->nloops = 0;
}

LCORE DrawCmd* DrawGraphicsInfo(GraphicsInfo* graphics) {
    DrawCmd* cmd = DrawLModelEx(graphics->model, graphics->frame_idx, graphics->pos, graphics->orient, graphics->scale);

    // If sprite is facing left, then we shift the bottom left corner to the bottom right corner
    // and set a negative u dimension. This will cause us to interpolate from bottom right to top left.
    if (graphics->facing_left) {
        cmd->meshcmd.uvoffset.x += cmd->meshcmd.uvscale.x;
        cmd->meshcmd.uvscale.x *= -1.0f;
    }
    return cmd;
}

LCORE DrawCmd* DrawGraphicsInfo2(GraphicsInfo* graphics) {
    DrawCmd* cmd = DrawLModelEx2(graphics->model, graphics->frame_idx, graphics->transform);

    // If sprite is facing left, then we shift the bottom left corner to the bottom right corner
    // and set a negative u dimension. This will cause us to interpolate from bottom right to top left.
    if (graphics->facing_left) {
        cmd->meshcmd.uvoffset.x += cmd->meshcmd.uvscale.x;
        cmd->meshcmd.uvscale.x *= -1.0f;
    }
    return cmd;
}

// LCORE void GraphicsInfoSetShape(GraphicsInfo* graphics, LMesh mesh, Vec3 scale, Vec4 color) {
//     graphics->mid = mesh;
//     graphics->scale = scale;
//     graphics->txinfo.tint = color;
// }

////////////////////////////////////////////////////////////

static const Vec3 ORIGIN = Vec3Lit(0.0f, 0.0f, 0.0f);
static const Vec2 ORIGIN2D = Vec2Lit(0.0f, 0.0f);
static const Vec3 FORWARD = Vec3Lit(0.0f, 0.0f, -1.0f);

////////////////////////////////////////////////////////////

typedef enum EntityFlag {
    EFLAG_NONE = 0,
    EFLAG_ACTIVE = 1 << 0,
    EFLAG_DRAW = 1 << 1,
    EFLAG_PLAYER_CONTROLLED = 1 << 2,
    EFLAG_WALL = 1 << 3,
    EFLAG_WORKER = 1 << 4,
    EFLAG_RESEARCHER = 1 << 5,
    EFLAG_MAIL = 1 << 6,
} EntityFlag;

typedef struct {
    isize idx;
    u64 generation;
} EntityId;
static const EntityId NULLID = {0};

#define EntityIdEq(a, b) (((a).idx == (b).idx) && ((a).generation == (b).generation))
#define EntityIdNotNull(a) (!EntityIdEq((a), NULLID))
#define EntityIdNull(a) (EntityIdEq((a), NULLID))

typedef struct {
    EntityId* data;
    isize len;
    isize cap;
} EntityIdArr;

#define EntityIdArrIter(arr, it) for (EntityId* it = ArrGetR(arr, 0); it <= ArrTailR(arr); ++it)

////////////////////////////////////////////////////////////
// s:gameplay fields.

typedef enum {
    STATE_ALIVE = 0,
    STATE_DEAD,
} EntityState;

typedef struct {
    i32 range;
    i32 dmg;
    bool ranged;  // True if the attacker is a ranged unit.
} AttackInfo;

////////////////////////////////////////////////////////////

typedef struct {
    /////////////////////////////////////////////////////////////////
    // Core field.
    EntityId id;
    // TODO: Convert to u64
    EntityFlag flags;
    bool inactive_to_be;

    // Decomposition of model matrix.
    Vec3 pos;
    Vec3 ppos;  // prev pos.
    Vec3 scale;
    Mat4 orient;

    // Physics related properties
    // Velocity
    // Vec3 v;
    f32 speed;
    Vec3 vdir;
    f32 mass_inv;  // Inverse mass. 0 == infinite mass.

    Polyhedron shape;

    GraphicsInfo graphics;

    f32 active_dt;

    // TODO: I think unused at the moment.
    EntityId down_id;   // Child
    EntityId right_id;  // Neighbor/sibling
    EntityId up_id;     // Parent

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.
    i32 hp;

    EntityState state;
    i32 move_ap;
    i32 attack_ap;

    i32 base_speed;

    AttackInfo attack;
} Entity;

typedef struct {
    Entity* data;
    isize len;
    isize cap;
} EntityArr;

#define ENTITY_CAP 1024
#define ENTITY_INACTIVE_TO_BE_CAP 20

typedef struct {
    EntityArr entities;
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

#define GetNumEntities() (EntityM.cap)

// TODO: Move these
#define EntitySetMass(e, m) ((e)->mass_inv = (m) != 0.0f ? 1.0f / (m) : 0.0f)
#define EntityGetMass(e) ((e)->mass_inv != 0.0f ? 1.0f / (e)->mass_inv : 0.0f)
#define EntityGetMassInv(e) ((e)->mass_inv)

static void EntityManagerReset(EntityManager* em) {
    for (isize i = 0; i < ArrLen(&em->entities); ++i) {
        Entity* entity = ArrGetPtr(&em->entities, i);
        entity->id.idx = i;
        // Set generation to be 1 so that we don't accidentally reference some entity
        // with default value of 0.
        entity->id.generation = 1;
    }
}

static void EntitySystemInit(Arena* perm) {
    EntityManager* em = &EntityM;

    InitArr(&em->entities, perm, Entity, 20000);
    // Fully reserve the array since we never push new entities after this.
    em->entities.len = em->entities.cap;

    EntityManagerReset(em);
}

LCORE void EntitySystemReset() { EntityManagerReset(GetEntityManager()); }

static Entity* EntityFromId(EntityId id) {
    EntityManager* em = GetEntityManager();
    Entity* entity = ArrGetPtr(&em->entities, id.idx);
    return TestFlag(entity->flags, EFLAG_ACTIVE) && EntityIdEq(entity->id, id) ? entity : NULL;
}

static Entity* NextFreeEntity() {
    EntityManager* em = GetEntityManager();
    // Always have a null entity at position 0
    for (isize i = NULLID.idx + 1; i < ArrLen(&em->entities); ++i) {
        Entity* entity = ArrGetPtr(&em->entities, i);
        if (!TestFlag(entity->flags, EFLAG_ACTIVE)) {
            return entity;
        }
    }
    return NULL;
}

static void EntityInitGameplayFields(Entity* entity) {}

LCORE Entity* AllocateEntity() {
    Entity* e = NextFreeEntity();
    Assert(e != NULL);
    Entity zero = {.id = e->id};
    *e = zero;
    e->flags = EFLAG_ACTIVE;
    e->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    e->orient = Mat4Identity();

    // Physics stuff.
    {
        // entity->v = Vec3Zero();
        EntitySetMass(e, 0.0f);  // Set infinite mass
    }

    PolyhedronInit(&e->shape);
    GraphicsInfoMemset(&e->graphics);
    FlagOn(e->flags, EFLAG_DRAW);

    {
        e->down_id = NULLID;
        e->right_id = NULLID;
        e->up_id = NULLID;
    }

    EntityInitGameplayFields(e);
    return e;
}

// Mark entity as being deactivated at the next cleanup. We do it this way so that we have
// precise control over when entities go out of scope. Mentally its easy to program against
// All entities that are active during a frame stay active throughout the frame until the end.
LCORE void SoftDeactivateEntity(Entity* e) {
    if (e) {
        e->inactive_to_be = true;
    }
}

// Sweep through the entities to be deactivated, and mark them as inactive.
static void DeactivateEntities() {
    EntityManager* em = GetEntityManager();
    // Always have a null entity at position 0
    for (isize i = NULLID.idx + 1; i < ArrLen(&em->entities); ++i) {
        Entity* entity = ArrGetPtr(&em->entities, i);
        if (TestFlag(entity->flags, EFLAG_ACTIVE) && entity->inactive_to_be) {
            FlagOff(entity->flags, EFLAG_ACTIVE);
        }
    }
}

LCORE bool IncrementEntity(Entity** entity_ptr) {
    Entity* e = *entity_ptr;

    // Remember the null handle is at idx 0.
    i32 start_idx = NULLID.idx + 1;
    if (e != NULL) {
        start_idx = e->id.idx + 1;
    }

    EntityManager* em = GetEntityManager();
    for (i32 i = start_idx; i < ArrLen(&em->entities); ++i) {
        Entity* entity = ArrGetPtr(&em->entities, i);
        if (TestFlag(entity->flags, EFLAG_ACTIVE)) {
            *entity_ptr = entity;
            return true;
        }
    }

    return false;
}

static Mat4 EntityGetParentMat(Entity* e) {
    Mat4 parent_mat = Mat4Identity();
    while (!EntityIdEq(e->up_id, NULLID)) {
        Entity* parent = EntityFromId(e->up_id);
        parent_mat = Mat4Multiply(Mat4SVRT(parent->scale, parent->orient, parent->pos), parent_mat);
        e = parent;
    }

    return parent_mat;
}

LCORE Mat4 EntityGetMat(Entity* e) {
    Mat4 parent_mat = EntityGetParentMat(e);
    Mat4 local_mat = Mat4SVRT(e->scale, e->orient, e->pos);
    return Mat4Multiply(parent_mat, local_mat);
}

////////////////////////////////////////////////////
typedef struct {
    EntityId id0;
    EntityId id1;

    f32 depth;  // positive value denoting how far you would need to push obj a along the contact normal to depenetrate.
    Vec3 normal;  // For most collision processing, we assume that this points from id1 -> id0 (or b -> a depending on
                  // terminology).
} CollisionInfo;

typedef struct {
    CollisionInfo* data;
    isize len;
    isize cap;
} CollisionArr;

////////////////////////////////////////////////////
// S:UI

static Vec4 COLORS[] = {
    RGBLit(50.0f, 50.0f, 50.0f),
    RGBLit(123.0f, 123.0f, 123.0f),
    RGBLit(54.0f, 173.0f, 105.0f),
    RGBLit(226.0f, 214.0f, 181.0f),
};

typedef enum {
    UI_SIZE_TEXT = 0,
    UI_SIZE_PCT_OF_PARENT,
    UI_SIZE_PIXEL,
} UISizeKind;

#define UISizeText() ((UISize){.kind = UI_SIZE_TEXT})
#define UISizePctParent(vv) ((UISize){.kind = UI_SIZE_PCT_OF_PARENT, .value = (vv)})
#define UISizePx(px) ((UISize){.kind = UI_SIZE_PIXEL, .value = (px)})

typedef struct {
    UISizeKind kind;
    f32 value;
} UISize;

typedef enum {
    UI_FLAG_NONE = 0,
    UI_FLAG_CLICKABLE = 1 << 0,
    UI_FLAG_HIDE = 1 << 1,  // Note that by default we draw boxes.
    // i.e. a button.
    UI_FLAG_ABLE_TO_BE_CLICKED = 1 << 2,
} UIFlag;

#define UITestFlag(flags, flag) ((flags) & (flag))

typedef enum {
    UI_TEXT_ALIGN_X_NONE = 0,
    UI_TEXT_ALIGN_X_CENTER,
    UI_TEXT_ALIGN_X_LEFT,
} UITextAlignX;
typedef enum {
    UI_TEXT_ALIGN_Y_NONE = 0,
    UI_TEXT_ALIGN_Y_MIDDLE,
    UI_TEXT_ALIGN_Y_BOTTOM,
} UITextAlignY;

typedef struct {
    UITextAlignX x;
    UITextAlignY y;
} UITextAlign;

typedef struct {
    UITextAlign* data;
    isize len;
    isize cap;
} UITextAlignArr;

typedef struct {
    LFont* data;
    isize len;
    isize cap;
} LFontArr;

typedef enum {
    UI_LAYOUT_DOWN = 0,
    UI_LAYOUT_RIGHT,
} UILayoutDirection;

typedef struct {
    UILayoutDirection* data;
    isize len;
    isize cap;
} UILayoutDirectionArr;

typedef struct {
    Vec4 color_bg;
    Vec4 color_bg_hover;
    Vec4 color_bg_down;
    Vec4 color_bg_unclickable;
    Vec4 color_text;

    UITextAlign align;
} UIBoxStyle;

typedef struct {
    isize id;
} UIBoxId;

static UIBoxId NULL_UIBOXID = {.id = 0};
#define UIBoxIdEq(a, b) ((a).id == (b).id)
#define UIBoxIdIsNull(a) (UIBoxIdEq(a, NULL_UIBOXID))

typedef struct {
    UIBoxId id;

    // The box's center should be in world position.
    // TODO: Longer term I should probably think about just storing an offset to parent and a dimension
    // Then you recalculate the world position each frame from the parent (root of tree will get actual screen top left
    // as position coordinate). But because we recreate all boxes for the UI each frame, might not be necessary to do
    // this at the moment.
    AABB2D box;

    // Note that this should be a *negative* offset from the top left. i.e. top left + this = next layout position.
    // TODO: I should potentially change to make everything based off of the bottom left as I think the math will be
    // less confusing.
    Vec2 cursor_top_left_offset;  // Where the next point to layout a ui element inside of the box is.

    Str8 text;
    TextBox textbox;  // Not positioned. i.e. the center here holds no meaning other than an offset.

    UIBoxStyle style;  // Contains color info to be used when rendering the box.
} UIBox;

#define UIBoxGetCursor(bbox) (Vec2Add(AABB2DTopLeft((bbox)->box), (bbox)->cursor_top_left_offset))

typedef struct {
    UIBox* data;
    isize len;
    isize cap;
} UIBoxArr;

typedef enum {
    UI_THEME_NONE = 0,

    UI_THEME_WINDOW_BG_COLOR,

    UI_THEME_BUTTON_BG_COLOR,
    UI_THEME_BUTTON_BG_HOVER_COLOR,
    UI_THEME_BUTTON_BG_DOWN_COLOR,
    UI_THEME_BUTTON_BG_UNCLICKABLE_COLOR,
    UI_THEME_BUTTON_TEXT_COLOR,

    UI_THEME_LABEL_BG_COLOR,
    UI_THEME_LABEL_TEXT_COLOR,

    UI_THEME_COUNT,
} UIThemeKind;

typedef struct {
    UIThemeKind idx;
    Vec4 color_prev;
} UIThemeColorOverride;

typedef struct {
    UIThemeColorOverride* data;
    isize len;
    isize cap;
} UIThemeColorOverrideArr;

typedef struct {
    Vec4 colors[UI_THEME_COUNT];

    UIThemeColorOverrideArr color_overrides;

    UITextAlign align;
} UITheme;

#define ColorHover(color) (Lighten((color), 0.15f))
#define ColorDown(color) (Darken((color), 0.12f))
#define ColorUnclickable(color) (Darken((color), 0.12f))

typedef struct {
    LFontArr fonts;
    Vec2Arr spacings;
    UILayoutDirectionArr layout_dirs;
    UIBoxArr parents;

    UITheme theme;

    UIBoxId next_id;
    UIBoxId active_id;

    GameInput* input;  // Reset each frame.
} UIState;

typedef struct {
    bool clicked;
    bool down;
    bool hover;
} UIAction;

#define UIGetFont(ui) (ArrTail(&(ui)->fonts))
#define UIPushFont(ui, ffont) (ArrPush(&(ui)->fonts, ffont))

#define UIPushSpacing(ui, sspacing) (ArrPush(&(ui)->spacings, sspacing))

#define UIGetLayoutDirection(ui) (ArrTail(&(ui)->layout_dirs))
#define UIPushLayoutDirection(ui, llayout) (ArrPush(&(ui)->layout_dirs, (llayout)))
#define UIPopLayoutDirection(ui) (ArrPop(&(ui)->layout_dirs))
#define UILayoutDirection(ui, llayout) Scoped(UIPushLayoutDirection(ui, llayout), UIPopLayoutDirection(ui))

#define UIGetThemeColor(ui, uikind) ((ui)->theme.colors[(uikind)])
LCORE void UIPushThemeColor(UIState* ui, UIThemeKind kind, Vec4 color) {
    UIThemeColorOverride override = {.idx = kind, .color_prev = ui->theme.colors[kind]};
    ArrPush(&ui->theme.color_overrides, override);
    ui->theme.colors[kind] = color;
}
LCORE void UIPopThemeColor(UIState* ui) {
    UIThemeColorOverride override = ArrPop(&ui->theme.color_overrides);
    ui->theme.colors[override.idx] = override.color_prev;
}
#define UIThemeColor(ui, uikind, color) Scoped(UIPushThemeColor((ui), (uikind), (color)), UIPopThemeColor((ui)))

#define UIGetThemeTextAlign(ui) ((ui)->theme.align)

#define UIGetParent(ui) (ArrTailR(&(ui)->parents))
#define UIPushParent(ui, pparent) (ArrPush(&(ui)->parents, (pparent)))
#define UIPopParent(ui) (ArrPop(&(ui)->parents))
#define UIParent(ui, pparent) Scoped(UIPushParent((ui), (pparent)), UIPopParent((ui)))

static UIBoxId UIStateNextId(UIState* ui) {
    UIBoxId id = ui->next_id;
    ui->next_id = (UIBoxId){.id = id.id + 1};
    return id;
}

// Set the current parent's cursor offset to pos.
LCORE void UISetCursor(UIState* ui, Vec2 pos) {
    UIBox* root = UIGetParent(ui);
    // TODO: We should potentially add some checks or clamping around setting a cursor outside of the parent's window.
    root->cursor_top_left_offset = pos;
}

static void UIStateInit(UIState* ui, Arena* perm, f32 width, f32 height, GameInput* input) {
    InitArr(&ui->spacings, perm, Vec2, 1);

    InitArr(&ui->fonts, perm, LFont, 3);
    UIPushFont(ui, GetLFontClosest(10.0f));
    InitArr(&ui->layout_dirs, perm, UILayoutDirection, 2);
    UIPushLayoutDirection(ui, UI_LAYOUT_DOWN);

    // Make sure first id is not the null id.
    ui->next_id = (UIBoxId){.id = NULL_UIBOXID.id + 1};

    InitArr(&ui->parents, perm, UIBox, 10);
    AABB2D parent_box = AABB2DFromMinMax(Vec2Lit(0, 0), Vec2Lit(width, height));
    UIBox root_parent = {.box = parent_box, .cursor_top_left_offset = Vec2Zero(), .id = UIStateNextId(ui)};
    UIPushParent(ui, root_parent);

    {
        UITheme theme_default = {0};
        InitArr(&theme_default.color_overrides, perm, UIThemeColorOverride, 10);

        for (isize i = 0; i < ArraySizeOf(theme_default.colors); ++i) {
            theme_default.colors[i] = LBLACK;
        }

        theme_default.colors[UI_THEME_WINDOW_BG_COLOR] = COLORS[0];

        theme_default.colors[UI_THEME_BUTTON_BG_COLOR] = COLORS[3];
        theme_default.colors[UI_THEME_BUTTON_BG_HOVER_COLOR] = ColorHover(COLORS[3]);
        theme_default.colors[UI_THEME_BUTTON_BG_DOWN_COLOR] = ColorDown(COLORS[3]);
        theme_default.colors[UI_THEME_BUTTON_BG_UNCLICKABLE_COLOR] = ColorUnclickable(COLORS[3]);
        theme_default.colors[UI_THEME_BUTTON_TEXT_COLOR] = LBLACK;

        theme_default.colors[UI_THEME_LABEL_BG_COLOR] = Vec4Lit(0.0f, 0.0f, 0.0f, 0.0f);
        theme_default.colors[UI_THEME_LABEL_TEXT_COLOR] = LBLACK;

        ui->theme = theme_default;
    }

    ui->input = input;
}

static void UIBegin(UIState* ui, Arena* perm, GameInput* input, f32 width, f32 height, LShader shader,
                    RenderPassLoadAction load_action, RenderTarget target) {
    Mat4 view = Mat4Identity();
    Mat4 proj = Mat4Ortho(0, width, 0, height, -1.0f, 1.0f);

    BeginShaderPass(shader, load_action, view, proj, target);
    UIStateInit(ui, perm, width, height, input);
}

static void UIEnd(UIState* ui) { EndShaderPass(); }

#define UI2D(ui, perm, input, width, height, shader, load_action, target) \
    Scoped(UIBegin((ui), (perm), (input), (width), (height), (shader), (load_action), (target)), UIEnd((ui)))

static UIBox MakeUIBox(UIState* ui, UISize width, UISize height, Str8 text, UIBoxStyle style) {
    UIBox* parent = UIGetParent(ui);
    Vec2 top_left = UIBoxGetCursor(parent);
    LFont font = UIGetFont(ui);

    Vec2 parent_dims = AABB2DDims(parent->box);

    TextBox textbox = LFontMeasureStr8Quad(font, text);
    Vec2 textbox_dims = AABB2DDims(textbox.aabb);

    UISize semantic_sizes[] = {width, height};
    Vec2 resolved_dims = {0};
    for (isize i = 0; i < ArraySizeOf(semantic_sizes); ++i) {
        switch (semantic_sizes[i].kind) {
            case UI_SIZE_TEXT: {
                resolved_dims.f[i] = textbox_dims.f[i];
                break;
            }
            case UI_SIZE_PCT_OF_PARENT: {
                resolved_dims.f[i] = semantic_sizes[i].value * parent_dims.f[i];
                break;
            }
            case UI_SIZE_PIXEL: {
                // Do I need to clamp this to parent dims?
                resolved_dims.f[i] = semantic_sizes[i].value;
                break;
            }
            default: {
                PlatformLog(LLOG_ERROR, "UNKNOWN UI Size Kind: %d", semantic_sizes[i].kind);
                Assert(0);
            }
        }
    }

    Vec2 box_min = Vec2Lit(top_left.x, top_left.y - resolved_dims.y);
    Vec2 box_max = Vec2Lit(top_left.x + resolved_dims.x, top_left.y);

    AABB2D aabb = AABB2DFromMinMax(box_min, box_max);
    UIBox box = {.box = aabb, .text = text, .textbox = textbox, .id = UIStateNextId(ui)};

    // Update parent pos.
    Vec2 box_dims = AABB2DDims(box.box);
    UILayoutDirection dir = UIGetLayoutDirection(ui);
    switch (dir) {
        case UI_LAYOUT_DOWN: {
            parent->cursor_top_left_offset.y -= box_dims.y;
            // Because its a negative offset, check with parent negative.
            parent->cursor_top_left_offset.y = f32max(parent->cursor_top_left_offset.y, -parent_dims.y);
            break;
        }
        case UI_LAYOUT_RIGHT: {
            parent->cursor_top_left_offset.x += box_dims.x;
            parent->cursor_top_left_offset.x = f32min(parent->cursor_top_left_offset.x, parent_dims.x);
            break;
        }
        default: {
            PlatformLog(LLOG_ERROR, "Unknown Layout Direction: %d", dir);
            Assert(0);
        }
    }

    box.style = style;

    return box;
}

static void DrawUIBoxEx(UIState* ui, UIBox* box, UIFlag flags, UIAction action) {
    if (UITestFlag(flags, UI_FLAG_HIDE)) {
        return;
    }

    // Draw backing box.
    {
        Vec4 box_color = box->style.color_bg;
        if (UITestFlag(flags, UI_FLAG_ABLE_TO_BE_CLICKED)) {
            if (UITestFlag(flags, UI_FLAG_CLICKABLE)) {
                if (action.down) {
                    box_color = box->style.color_bg_down;
                } else if (action.hover) {
                    box_color = box->style.color_bg_hover;
                }
            } else {
                box_color = box->style.color_bg_unclickable;
            }
        }

        DrawQuad2D(box->box.c, AABB2DWidth(box->box), AABB2DHeight(box->box), Mat4Identity(), box_color);
    }

    if (Str8Len(box->text)) {
        LFont font = UIGetFont(ui);

        Vec2 box_dims = AABB2DDims(box->box);

        Vec2 bottom_left = AABB2DBottomLeft(box->box);
        Vec2 textbox_center_offset = Vec2Zero();

        switch (box->style.align.x) {
            case UI_TEXT_ALIGN_X_LEFT: {
                break;
            }
            case UI_TEXT_ALIGN_X_CENTER: {
                textbox_center_offset.x = (0.5f * box_dims.x) - box->textbox.aabb.c.x;
                break;
            }
            case UI_TEXT_ALIGN_X_NONE:
            default: {
                PlatformLog(LLOG_ERROR, "Unknown text align x: %d", box->style.align.x);
                Assert(0);
            }
        }

        switch (box->style.align.y) {
            case UI_TEXT_ALIGN_Y_BOTTOM: {
                break;
            }
            case UI_TEXT_ALIGN_Y_MIDDLE: {
                // TODO: this might not actually be vertically centered like how most engines do it? It seems like there
                // is some concept of font height which I don't want to focus on at the moment.
                textbox_center_offset.y = (0.5f * box_dims.y) - box->textbox.aabb.c.y;
                break;
            }
            case UI_TEXT_ALIGN_Y_NONE:
            default: {
                PlatformLog(LLOG_ERROR, "Unknown text align y: %d", box->style.align.y);
                Assert(0);
            }
        }

        textbox_center_offset = Vec2Add(textbox_center_offset, box->textbox.aabb.c);
        Vec2 baseline_offset = Vec2Add(textbox_center_offset, box->textbox.baseline_offset);
        Vec2 text_pos = Vec2Add(bottom_left, baseline_offset);
        DrawStr8Ex(font, text_pos, box->style.color_text, box->text);
    }
}

// TODO: I think text alignment should come from the style potentially
static UIBox UIBoxEx(UIState* ui, Str8 text, UIFlag flags, UISize semantic_width, UISize semantic_height,
                     UIBoxStyle style, UIAction* action_out) {
    UIBox box = MakeUIBox(ui, semantic_width, semantic_height, text, style);

    GameControllerState* controller = GetController(ui->input, 0);

    // Note that the box should be in screen space already.
    Vec2 normal = {0};
    f32 depth = 0.0f;
    bool collides = TestPoint2DVsAABB2D(controller->cursor_pos, box.box, &normal, &depth);

    UIAction action = {0};
    // TODO: There is a potential issue here in that by the time we've checked the down action here,
    // we might have triggered gameplay actions as well since this logic check happens after the
    // update step. Ideally we decouple the UI logic detection/gameplay state update and the render.
    // But they are coupled for the time being.
    if (collides && UITestFlag(flags, UI_FLAG_CLICKABLE)) {
        // Prevent us from activating two adjacent buttonsa t the same time. This can happen occasionally due to
        // floating point math.
        if (UIBoxIdIsNull(ui->active_id) || UIBoxIdEq(ui->active_id, box.id)) {
            ui->active_id = box.id;
            action.hover = true;
            action.clicked = controller->action_down.pressed;
            action.down = controller->action_down.down;
        }
    }

    DrawUIBoxEx(ui, &box, flags, action);

    *action_out = action;
    return box;
}

static UIBox MakeUIWindow(UIState* ui, UISize width, UISize height) {
    UIBoxStyle style = {
        .color_bg = UIGetThemeColor(ui, UI_THEME_WINDOW_BG_COLOR),
    };
    UIAction action = {0};
    UIBox box = UIBoxEx(ui, Str8Zero(), UI_FLAG_NONE, width, height, style, &action);
    return box;
}

// Need to define variable and call UIParent in two steps or can run into bugs around macro expansion due to using
// incorrect parent in MakeUIWindow for sizing and positioning.
// CONCAT is needed because ##__LINE__ doesn't actually expand LINE
#define UIWindow(ui, width, height)                                                 \
    UIBox CONCAT(_ui_container_, __LINE__) = MakeUIWindow((ui), (width), (height)); \
    UIParent((ui), CONCAT(_ui_container_, __LINE__))

static UIAction UIButtonEx(UIState* ui, Str8 text, UIFlag flags, UISize semantic_width, UISize semantic_height) {
    UIBoxStyle style = {
        .color_bg = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_COLOR),
        .color_bg_hover = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_HOVER_COLOR),
        .color_bg_down = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_DOWN_COLOR),
        .color_bg_unclickable = UIGetThemeColor(ui, UI_THEME_BUTTON_BG_UNCLICKABLE_COLOR),
        .color_text = UIGetThemeColor(ui, UI_THEME_BUTTON_TEXT_COLOR),
        .align =
            {
                .x = UI_TEXT_ALIGN_X_CENTER,
                .y = UI_TEXT_ALIGN_Y_MIDDLE,
            },
    };

    UIAction action = {0};
    UIBoxEx(ui, text, flags | UI_FLAG_ABLE_TO_BE_CLICKED, semantic_width, semantic_height, style, &action);
    return action;
}

LCORE UIAction UIButton(UIState* ui, Str8 text, UIFlag flags) {
    return UIButtonEx(ui, text, flags, UISizePctParent(1.0f), UISizeText());
}

LCORE UIAction UILabel(UIState* ui, Str8 text, UIFlag flags) {
    UIBoxStyle style = {
        .color_bg = UIGetThemeColor(ui, UI_THEME_LABEL_BG_COLOR),
        .color_text = UIGetThemeColor(ui, UI_THEME_LABEL_TEXT_COLOR),
        .align =
            {
                .x = UI_TEXT_ALIGN_X_LEFT,
                .y = UI_TEXT_ALIGN_Y_MIDDLE,
            },
    };

    UIAction action = {0};
    UIBoxEx(ui, text, flags & (~UI_FLAG_ABLE_TO_BE_CLICKED), UISizePctParent(1.0f), UISizeText(), style, &action);
    return action;
}

////////////////////////////////////////////////////

typedef struct {
    Vec3 eye;  // world space position of the camera.
    Vec3 target;
    Vec3 up;
} LCamera;

// static Mat4 LCameraGetView(LCamera c) {
//     return Mat4Invert(Mat4Multiply(Mat4TranslateV(c.eye), Mat4RotateEuler(c.orient)));
// }

// Returns the matrix that when applied will put the origin in the same position and orientation of the camera.
LCORE Mat4 CameraGetViewMat(LCamera camera) { return Mat4LookAt(camera.eye, camera.target, camera.up); }

LCORE Mat4 GetCameraMat(Vec3 eye, Vec3 orient) {
    return Mat4Invert(Mat4Multiply(Mat4TranslateV(eye), Mat4RotateEuler(orient)));
}

// I think all unused.
#define Grid2WorldEx(pos, w, h, d) (Vec3Lit((pos).x * (w), (pos).y * (h), (pos).z * (d)))
static const f32 GRID_WIDTH = 16.0f;
static const f32 GRID_HEIGHT = 16.0f;
static const f32 GRID_DEPTH = 1.0f;
#define Grid2World(pos) (Grid2WorldEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

#define World2GridEx(pos, w, h, d) (Vec3Lit((u32)((pos).x / (w)), (u32)((pos).y / (h)), (u32)((pos).z) / (d)))
#define World2Grid(pos) (World2GridEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

// 1 meter = N pixels scale factor. i.e. how many pixels are in 1 meter.
#define PPM (1.0f)
#define MPP (1.0f / PPM)
#define Meter2Px(m) ((m) * PPM)
#define Px2Meter(px) ((px) * MPP)

typedef struct {
    f32 dt;
    f32 accumulated;
} FrameTime;

////////////////////////////////////////////////////

typedef enum {
    SUIT_NONE = 0,
    SUIT_A,
    SUIT_B,
    SUIT_C,
    SUIT_D,
    SUIT_COUNT,
} Suit;

////////////////////////////////////////////////////

#define GRID_NUM_ROWS 16
#define GRID_NUM_COLS 8  // 10

typedef struct {
    i32 r;
    i32 c;
    i32 height;
} TileId;

static const TileId NULL_TILEID = {.r = -1, .c = -1, .height = -1};

typedef struct {
    TileId* data;
    isize len;
    isize cap;
} TileIdArr;

#define TileIdArrIter(arr, it) for (TileId* it = ArrGetR(arr, 0); it <= ArrTailR(arr); ++it)

#define TileIdLit(row, col) ((TileId){.r = (row), .c = (col), .height = 0})
#define TileIdFromVec3(v) ((TileId){.r = (v).y, .c = (v).x, .height = (v).z})
#define TileIdtoVec3(id) (Vec3Lit((id).c, (id).r, (id).height))
#define TileIdEq(a, b) (((a).r == (b).r) && ((a).c == (b).c) && ((a).height == (b).height))

typedef struct {
    i32 points;
    Suit suit;
    Vec4 base_color;
} TileEffects;

typedef struct {
    // Reminder that this pos refers to the *center* of the tile.
    Vec3 pos;
    TileId id;
    bool occupied;
    TileEffects effects;
} Tile;

typedef struct {
    Vec3 pos;
    Vec3 scale;
    Mat4 rot;

    Mat4 grid_to_world;
    Mat4 world_to_grid;

    isize nrows;
    isize ncols;
    Tile tiles[GRID_NUM_ROWS][GRID_NUM_COLS];
} Grid;

#define GridValidTileId(grid, id) ((id).r >= 0 && (id).r < (grid)->nrows && (id).c >= 0 && (id).c < (grid)->ncols)

LCORE Tile* GridMaybeTileFromId(Grid* grid, TileId id) {
    bool valid_idx = (id.r >= 0 && id.r < grid->nrows && id.c >= 0 && id.c < grid->ncols);
    return valid_idx ? &grid->tiles[id.r][id.c] : NULL;
}

LCORE Tile* GridTileFromId(Grid* grid, TileId id) {
    Tile* tile = GridMaybeTileFromId(grid, id);
    Assert(tile);
    return tile;
}

LCORE Tile* GridMaybeGetTile(Grid* grid, isize r, isize c) {
    return GridMaybeTileFromId(grid, (TileId){.r = r, .c = c, .height = 0});
}

LCORE Tile* GridGetTile(Grid* grid, isize r, isize c) {
    Tile* tile = GridMaybeGetTile(grid, r, c);
    Assert(tile);
    return tile;
}

LCORE void GridInit(Grid* grid, f32 renderw_world, f32 renderh_world) {
    static Grid ZERO = {0};

    *grid = ZERO;

    Vec2 grid_tile_dims = Vec2Lit(Px2Meter(10.0f), Px2Meter(10.0f));

    i32 ncols = GRID_NUM_COLS;
    i32 grid_width = grid_tile_dims.x * ncols;
    i32 grid_origin_x = -0.5f * grid_width;

    i32 nrows = GRID_NUM_ROWS;
    i32 grid_height = grid_tile_dims.y * nrows;
    i32 grid_origin_y = (0.5f * renderh_world) + (0.5f * grid_tile_dims.y) - grid_height;

    Vec3 grid_origin = Vec3Lit(grid_origin_x, grid_origin_y, 0.0f);
    for (i32 r = 0; r < nrows; ++r) {
        for (i32 c = 0; c < ncols; ++c) {
            Tile* tile = &grid->tiles[r][c];
            tile->pos = Vec3Lit(c, r, 0);
            tile->id = (TileId){.r = r, .c = c, .height = 0};
        }
    }

    grid->ncols = ncols;
    grid->nrows = nrows;
    grid->pos = grid_origin;
    grid->scale = Vec3Lit(grid_tile_dims.x, grid_tile_dims.y, grid_tile_dims.x);  // Important to make uniform scale.
    grid->rot = Mat4Identity();
    grid->grid_to_world = Mat4SVRT(grid->scale, grid->rot, grid->pos);
    grid->world_to_grid = Mat4Invert(grid->grid_to_world);
}

LCORE Vec3 GridGetTilePosBL(Grid* grid, TileId tileid) {
    Vec3 pos = TileIdtoVec3(tileid);
    pos.x -= 0.5f;
    pos.y -= 0.5f;
    return pos;
}

LCORE Vec3 GridGetTilePosBR(Grid* grid, TileId tileid) {
    Vec3 pos = TileIdtoVec3(tileid);
    pos.x += 0.5f;
    pos.y -= 0.5f;
    return pos;
}

LCORE Vec3 GridPosToWorld(Grid* grid, Vec3 pos) { return Vec3WTransform(pos, grid->grid_to_world); }

// Returns a reference to the tile a cursor is over, otherwise returns NULL if cursor outside of grid.
LCORE TileId GridGetCursorHoverTileId(Grid* grid, GameControllerState* controller) {
    Vec3 cursor_pos_grid = Vec3WTransform(Vec2ToVec3(controller->cursor_pos_world), grid->world_to_grid);
    // PlatformLog(LLOG_INFO, "cursor pos world %f %f", controller->cursor_pos_world.x, controller->cursor_pos_world.y);
    // PlatformLog(LLOG_INFO, "cursor pos grid %f %f %f", cursor_pos_grid.x, cursor_pos_grid.y, cursor_pos_grid.z);

    // In grid space, the integer coordinate like [0, 1] is the center of a grid position with the remaining space in
    // that tile spanning +/- 0.5 along each axis. So to get the actual tile coordinate of the tile the cursor is over
    // (if it is over one), we round each axis coordinate to nearest integer.
    cursor_pos_grid.x = f32round(cursor_pos_grid.x);
    cursor_pos_grid.y = f32round(cursor_pos_grid.y);
    cursor_pos_grid.z = f32round(cursor_pos_grid.z);

    if (cursor_pos_grid.x < 0 || cursor_pos_grid.x >= grid->ncols || cursor_pos_grid.y < 0 ||
        cursor_pos_grid.y >= grid->nrows) {
        // PlatformLog(LLOG_INFO, "Cursor <%f, %f> outside of grid, no tile selected", cursor_pos_grid.x,
        // cursor_pos_grid.y);
        return (TileId){.r = -1, .c = -1, .height = -1};
    }

    // PlatformLog(LLOG_INFO, "Tile r=%f, c=%f selected", cursor_pos_grid.y, cursor_pos_grid.x);
    return TileIdFromVec3(cursor_pos_grid);
}

// Returns a reference to the tile a cursor is over, otherwise returns NULL if cursor outside of grid.
LCORE Tile* GridGetCursorHoverTile(Grid* grid, GameControllerState* controller) {
    TileId tileid = GridGetCursorHoverTileId(grid, controller);
    return GridMaybeTileFromId(grid, tileid);
}

// LCORE Entity* GridGetEntityAtTile(Grid* grid, Tile* tile) {
//     if (tile == NULL) {
//         return NULL;
//     }

//     Entity* e = EntityFromId(tile->occupant);
//     return e;
// }

// LCORE Entity* GridGetEntityAtTileId(Grid* grid, TileId tile) {
//     return GridGetEntityAtTile(grid, GridTileFromId(grid, tile));
// }

typedef enum {
    PT_Z = 0,
    PT_S,
    PT_L,
    PT_J,
    PT_T,
    PT_I,
    PT_O,
    PT_COUNT,
} PieceType;

typedef enum {
    PR_0 = 0,
    PR_90,
    PR_180,
    PR_270,
    PR_COUNT,
} PieceRot;

typedef struct {
    PieceType type;
    PieceRot rot_idx;
    TileIdArr block_offsets;
    TileEffects effects;
} Piece;

LCORE TileIdArr PieceGetOffsets(PieceType type, PieceRot rot_idx) {
    static TileId OFFSETS[PT_COUNT][PR_COUNT][4] = {
        // Z block.
        // XX
        //  AX
        {
            {TileIdLit(1, -1), TileIdLit(1, 0), TileIdLit(0, 0), TileIdLit(0, 1)},    // Rot 0
            {TileIdLit(1, 1), TileIdLit(0, 1), TileIdLit(0, 0), TileIdLit(-1, 0)},    // Rot 90
            {TileIdLit(-1, 1), TileIdLit(-1, 0), TileIdLit(0, 0), TileIdLit(0, -1)},  // Rot 180
            {TileIdLit(-1, -1), TileIdLit(0, -1), TileIdLit(0, 0), TileIdLit(1, 0)},  // Rot 270
        },
        // S block.
        //  XX
        // XA
        {
            {TileIdLit(1, 1), TileIdLit(1, 0), TileIdLit(0, 0), TileIdLit(0, -1)},    // Rot 0
            {TileIdLit(-1, 1), TileIdLit(0, 1), TileIdLit(0, 0), TileIdLit(1, 0)},    // Rot 90
            {TileIdLit(-1, -1), TileIdLit(-1, 0), TileIdLit(0, 0), TileIdLit(0, 1)},  // Rot 180
            {TileIdLit(1, -1), TileIdLit(0, -1), TileIdLit(0, 0), TileIdLit(-1, 0)},  // Rot 270
        },

        // L block.
        //   X
        // XAX
        {
            {TileIdLit(0, -1), TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(1, 1)},    // Rot 0
            {TileIdLit(1, 0), TileIdLit(0, 0), TileIdLit(-1, 0), TileIdLit(-1, 1)},   // Rot 90
            {TileIdLit(0, 1), TileIdLit(0, 0), TileIdLit(0, -1), TileIdLit(-1, -1)},  // Rot 180
            {TileIdLit(-1, 0), TileIdLit(0, 0), TileIdLit(1, 0), TileIdLit(1, -1)},   // Rot 270
        },

        // J block.
        // X
        // XAX
        {
            {TileIdLit(1, -1), TileIdLit(0, -1), TileIdLit(0, 0), TileIdLit(0, 1)},   // Rot 0
            {TileIdLit(1, 1), TileIdLit(1, 0), TileIdLit(0, 0), TileIdLit(-1, 0)},    // Rot 90
            {TileIdLit(-1, 1), TileIdLit(0, 1), TileIdLit(0, 0), TileIdLit(0, -1)},   // Rot 180
            {TileIdLit(-1, -1), TileIdLit(-1, 0), TileIdLit(0, 0), TileIdLit(1, 0)},  // Rot 270
        },

        // T block.
        //  X
        // XAX
        {
            {TileIdLit(0, -1), TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(1, 0)},   // Rot 0
            {TileIdLit(1, 0), TileIdLit(0, 0), TileIdLit(-1, 0), TileIdLit(0, 1)},   // Rot 90
            {TileIdLit(0, 1), TileIdLit(0, 0), TileIdLit(0, -1), TileIdLit(-1, 0)},  // Rot 180
            {TileIdLit(-1, 0), TileIdLit(0, 0), TileIdLit(1, 0), TileIdLit(0, -1)},  // Rot 270
        },

        // I block. Because for I block the anchor point moves from rotation to rotation
        // we need to be careful so that all rotation states are specified to a consistent anchor point
        // which might mean that the anchor point is not being occupied by a block.
        // XAXX
        {
            {TileIdLit(0, -1), TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(0, 2)},      // Rot 0
            {TileIdLit(1, 1), TileIdLit(0, 1), TileIdLit(-1, 1), TileIdLit(-2, 1)},     // Rot 90
            {TileIdLit(-1, 2), TileIdLit(-1, 1), TileIdLit(-1, 0), TileIdLit(-1, -1)},  // Rot 180
            {TileIdLit(-2, 0), TileIdLit(-1, 0), TileIdLit(0, 0), TileIdLit(1, 0)},     // Rot 270
        },

        // O block.
        // AX
        // XX
        {
            {TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(-1, 0), TileIdLit(-1, 1)},  // Rot 0
            {TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(-1, 0), TileIdLit(-1, 1)},  // Rot 90
            {TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(-1, 0), TileIdLit(-1, 1)},  // Rot 180
            {TileIdLit(0, 0), TileIdLit(0, 1), TileIdLit(-1, 0), TileIdLit(-1, 1)},  // Rot 270
        },
    };

    TileIdArr arr = {
        .data = OFFSETS[type][rot_idx],
        .len = ArraySizeOf(OFFSETS[type][rot_idx]),
        .cap = ArraySizeOf(OFFSETS[type][rot_idx]),
    };
    return arr;
}

LCORE TileId GridCalculatePiecePlacementAnchor(Grid* grid, Piece piece, TileId anchor) {
    isize r_candidate = -1;
    for (isize r = anchor.r; r >= 0; --r) {
        bool can_place = true;
        for (isize i = 0; i < ArrLen(&piece.block_offsets); ++i) {
            TileId offset = ArrGet(&piece.block_offsets, i);
            Tile* t = GridMaybeGetTile(grid, r + offset.r, anchor.c + offset.c);
            if (!t) {
                can_place = false;
                break;
            } else if (t->occupied) {
                can_place = false;
                break;
            }
        }

        if (!can_place) {
            break;
        }

        r_candidate = r;
    }

    Assert(r_candidate < grid->nrows && r_candidate >= 0);
    return TileIdLit(r_candidate, anchor.c);
}

////////////////////////////////////////////////////

#define RELIC_LIST                                                                            \
    RELICX(RELIC_SUIT_A_MULT, "+1 mult when all blocks in line are of suit A")                \
    RELICX(RELIC_SUIT_B_MULT, "+1 mult when all blocks in line are of suit B")                \
    RELICX(RELIC_SUIT_C_MULT, "+1 mult when all blocks in line are of suit C")                \
    RELICX(RELIC_SUIT_D_MULT, "+1 mult when all blocks in line are of suit D")                \
    RELICX(RELIC_SUIT_A_CHIPS, "+50 CHIPS when all blocks in line are of suit A")             \
    RELICX(RELIC_SUIT_B_CHIPS, "+50 CHIPS when all blocks in line are of suit B")             \
    RELICX(RELIC_SUIT_C_CHIPS, "+50 CHIPS when all blocks in line are of suit C")             \
    RELICX(RELIC_SUIT_D_CHIPS, "+50 CHIPS when all blocks in line are of suit D")             \
    RELICX(RELIC_PLUS_MULT_CONSECTUTIVE, "+X mult for the number of consecutive line clears") \
    RELICX(RELIC_FIRST_CLEAR_MULT, "+3 mult for the first clear each round")                  \
    RELICX(RELIC_CLEAR_COUNT_MULT, "+mult equal to the number of line in a clear")            \
    RELICX(RELIC_ALL_SUIT_MULT, "+1 mult if line cleared contains all suits")                 \
    RELICX(RELIC_ALL_SUIT_CHIPS, "+25 chips if line cleared contains all suits")

typedef enum {
    RELIC_NONE = 0,
#define RELICX(name, desc) name,
    RELIC_LIST
#undef RELICX
        RELIC_COUNT,
} RelicId;

TypedefArr(RelicId);

typedef struct {
    Str8 name;
    Str8 description;
} Relic;

static Relic RELICS[] = {{.name = Str8Lit("NULL"), .description = Str8Lit("NULL")},
#define RELICX(nname, desc) {.name = Str8Lit(#nname), .description = Str8Lit(desc)},
                         RELIC_LIST
#undef RELICX
};

#define RelicGet(id) &RELICS[(id)]

typedef struct {
    RelicId id;
    bool owned;
    f32 activated_time;  // timestamp the relic was last activated.
} RelicState;

TypedefArr(RelicState);

typedef struct {
    RelicState state[RELIC_COUNT];
    RelicId ids_buffer[RELIC_COUNT];
    RelicIdArr ids;
} Relics;

void RelicsInit(Relics* relics) {
    static Relics RELICS_ZERO = {0};
    *relics = RELICS_ZERO;
    for (isize i = 0; i < ArraySizeOf(relics->state); ++i) {
        relics->state[i].id = (RelicId)i;
    }

    InitArrS(&relics->ids, relics->ids_buffer, ArraySizeOf(relics->ids_buffer));
}

void RelicsAdd(Relics* relics, RelicId id) {
    RelicState* state = &relics->state[id];
    Assert(!state->owned);
    // The activated time is a bit of a hack to ensure we don't display them as activated on startup/when added
    RelicState new_state = {.id = id, .owned = true, .activated_time = -100.0f};
    *state = new_state;
    ArrPush(&relics->ids, id);
}

#define RelicsHas(relics, id) (relics)->state[(id)].owned
#define RelicsOwnedCount(relics) ArrLen(&(relics)->ids)
#define RelicsStateGet(relics, i) &(relics)->state[ArrGet(&(relics)->ids, (i))]

typedef struct {
    Vec3 pos;
    Piece piece;
} Card;

typedef struct {
    Card* data;
    isize len;
    isize cap;
} CardArr;

////////////////////////////////////////////////////

typedef struct {
    TileId block_id;
} ScoreEvent;

TypedefArr(ScoreEvent);

typedef struct {
    ScoreEvent events[1024];

    isize head_idx;
    isize tail_idx;
    isize cap;
} ScoreLog;

static void ScoreLogInit(ScoreLog* log) {
    static ScoreLog ZERO_SCORE_LOG = {0};
    *log = ZERO_SCORE_LOG;
    log->cap = ArraySizeOf(log->events);
    log->head_idx = 1;
    log->tail_idx = 0;
}

static void ScoreLogBlockScored(ScoreLog* log, TileId block_id) {
    log->head_idx = (log->head_idx + 1) % log->cap;
    log->events[log->head_idx] = (ScoreEvent){.block_id = block_id};
    Assert(log->head_idx != log->tail_idx);
}

#define ScoreLogHead(sl) (&(sl)->events[(sl)->head_idx])

typedef struct {
    Grid grid;

    Piece active_piece;
    TileId anchor;
    TileId placement_anchor;

    i32 score_target;
    i32 total_score;
    i32 num_cleared;
    i32 cleared_streak;
    i32 blocks_placed_since_last_clear;
    i32 max_blocks_placed_since_last_clear;
    i32 trick_base_score;
    i32 trick_mult_score;
    i32 num_hands;

    Card hand_buffer[6];
    CardArr hand;

    Card deck_buffer[SUIT_COUNT * PT_COUNT];
    CardArr deck;

    Relics relics;

    Mat4 hand_to_world;
    Vec2 card_scale;
    isize selection_idx;

    i32 renderw_world;
    i32 renderh_world;

    ScoreLog score_log;
} Battle;

static const isize NO_CARD_SELECTED_IDX = -1;

// Assume position zero is sentinel.
#define BattleHandIter(arr, it) for (Card* it = ArrGetR(arr, 0); it <= ArrTailR(arr); ++it)

static const Vec4 BLOCK_PURPLE = RGBLit(138, 92, 245);
static const Vec4 BLOCK_GREEN = RGBLit(72, 207, 134);
static const Vec4 BLOCK_ORANGE = RGBLit(255, 159, 67);
static const Vec4 BLOCK_RED = RGBLit(245, 92, 135);
static Vec4 BLOCK_COLORS[SUIT_COUNT] = {LBLACK, BLOCK_PURPLE, BLOCK_GREEN, BLOCK_ORANGE, BLOCK_RED};

static Piece MakePiece(PieceType type, PieceRot rot, Suit suit) {
    Piece piece = {0};
    piece.type = type;
    piece.rot_idx = rot;
    piece.block_offsets = PieceGetOffsets(piece.type, piece.rot_idx);

    piece.effects.points = 1;
    piece.effects.suit = suit;
    piece.effects.base_color = BLOCK_COLORS[suit];
    return piece;
}

static i32 CalculateOffsetHeight(TileIdArr block_offsets) {
    i32 max_height = I32_MIN;
    for (isize i = 0; i < ArrLen(&block_offsets); ++i) {
        TileId offset = ArrGet(&block_offsets, i);
        max_height = i32max(offset.r, max_height);
    }
    Assert(max_height <= 100);
    Assert(max_height >= -100);
    return max_height;
}

static void BattleSetActivePiece(Battle* battle) {
    PlatformLog(LLOG_INFO, "Setting active piece idx=%d", battle->selection_idx);

    Card* card = ArrGetPtr(&battle->hand, battle->selection_idx);
    battle->active_piece = card->piece;
    i32 max_height = CalculateOffsetHeight(battle->active_piece.block_offsets);
    Assert(max_height < battle->grid.nrows);
    battle->anchor.r = (battle->grid.nrows - 1) - max_height;
    battle->anchor.c = (i32)(battle->grid.ncols / 2);  // Always reset to the middle.
    battle->placement_anchor = GridCalculatePiecePlacementAnchor(&battle->grid, battle->active_piece, battle->anchor);
}

// Draw up to max handsize
static void BattleDrawToFull(Battle* battle) {
    // if (ArrLen(&battle->hand) == 0) {
    //     Card nullcard = {0};
    //     nullcard.piece = MakePiece(PT_O, PR_0, SUIT_NONE);
    //     nullcard.piece.effects.base_color = LWHITE;
    //     ArrPush(&battle->hand, nullcard);
    // }

    for (isize i = ArrLen(&battle->hand); i < ArrCap(&battle->hand) && ArrLen(&battle->deck) > 0; ++i) {
        Card card = ArrPop(&battle->deck);
        i32 r = i / 2;
        i32 c = i % 2;
        card.pos = Vec3Lit(c, r, 0);
        ArrPush(&battle->hand, card);
    }
}

static void BattleInit(Battle* battle, i32 renderw_world, i32 renderh_world, i32 score_target) {
    static Battle BATTLE_ZERO = {0};
    *battle = BATTLE_ZERO;

    battle->renderw_world = renderw_world;
    battle->renderh_world = renderh_world;
    GridInit(&battle->grid, battle->renderw_world, battle->renderh_world);

    // hand init deck init
    {
        battle->hand_to_world = Mat4SVRT(Vec3Lit(Px2Meter(30.0f), Px2Meter(30.0f), Px2Meter(1.0f)), Mat4Identity(),
                                         Vec3Lit(Px2Meter(80.0f), Px2Meter(0.0f), Px2Meter(0.0f)));
        battle->card_scale = Vec2Lit(Px2Meter(20.0f), Px2Meter(20.0f));

        InitArrS(&battle->deck, battle->deck_buffer, ArraySizeOf(battle->deck_buffer));
        i32 pieces_per_suit = ArrCap(&battle->deck) / SUIT_COUNT;
        for (Suit suit = SUIT_A; suit < SUIT_COUNT; ++suit) {
            for (isize i = 0, piece_type = 0; i < pieces_per_suit; ++i, piece_type = (piece_type + 1) % PT_COUNT) {
                Card card = {0};
                card.piece = MakePiece(piece_type, PR_0, suit);
                ArrPush(&battle->deck, card);
            }
        }

        // shuffle deck (crude approach)
        for (isize shuffles = 0; shuffles < 10; ++shuffles) {
            for (isize i = 0; i < ArrLen(&battle->deck); ++i) {
                for (isize j = 0; j < 3; ++j) {
                    isize dest_idx = i32randab(0, ArrLen(&battle->deck) - 1);
                    Card* dest = ArrGetPtr(&battle->deck, dest_idx);
                    Card temp = *dest;
                    Card* source = ArrGetPtr(&battle->deck, i);
                    *dest = *source;
                    *source = temp;
                }
            }
        }

        InitArrS(&battle->hand, battle->hand_buffer, ArraySizeOf(battle->hand_buffer));
        BattleDrawToFull(battle);
        battle->num_hands = 5;

        battle->selection_idx = NO_CARD_SELECTED_IDX;
    }

    {
        RelicsInit(&battle->relics);
        for (isize i = 0; i < 3; ++i) {
            RelicId id = {0};
            i32 count = 0;
            do {
                id = i32randab(1, RELIC_COUNT - 1);
                ++count;
            } while (RelicsHas(&battle->relics, id) && count < 50);
            Assert(!RelicsHas(&battle->relics, id));
            RelicsAdd(&battle->relics, id);
        }
    }

    battle->score_target = score_target;

    ScoreLogInit(&battle->score_log);
}

// TODO: I should maybe make this BattleTick essentially and have all logic driven in a flat function here.
static void BattlePlacePiece(Battle* battle, FrameTime time) {
    Grid* grid = &battle->grid;
    Relics* relics = &battle->relics;

    // Place piece.
    {
        TileId placement_anchor = battle->placement_anchor;
        Piece* active_piece = &battle->active_piece;
        for (isize i = 0; i < ArrLen(&active_piece->block_offsets); ++i) {
            TileId offset = ArrGet(&active_piece->block_offsets, i);
            Tile* tile = GridGetTile(grid, placement_anchor.r + offset.r, placement_anchor.c + offset.c);
            tile->occupied = true;
            tile->effects = active_piece->effects;
        }
    }

    // Score placement
    {
        isize cleared_row_start = grid->nrows;
        i32 num_cleared_rows = 0;
        for (isize r = 0; r < grid->nrows; ++r) {
            bool cleared = true;
            for (isize c = 0; c < grid->ncols; ++c) {
                Tile* tile = GridGetTile(grid, r, c);
                if (!tile->occupied) {
                    cleared = false;
                    break;
                }
            }

            if (cleared) {
                ++num_cleared_rows;
                cleared_row_start = i32min(cleared_row_start, r);
            }
        }

        // Score trick.
        if (num_cleared_rows) {
            i32 block_score = 0;
            i32 mult = 0;
            for (isize i = 0; i < num_cleared_rows; ++i) {
                isize r = cleared_row_start + i;
                Assert(r < grid->nrows);

                Tile* first = GridGetTile(grid, r, 0);
                Suit suit = first->effects.suit;
                i32 all_suit_mask = 0;
                for (isize c = 0; c < grid->ncols; ++c) {
                    Tile* tile = GridGetTile(grid, r, c);
                    Assert(tile->occupied);
                    block_score += tile->effects.points;
                    suit = suit == tile->effects.suit ? suit : SUIT_NONE;
                    all_suit_mask |= 1 << tile->effects.suit;

                    ScoreLogBlockScored(&battle->score_log, tile->id);
                }

                ++mult;

                for (isize ri = 0; ri < RelicsOwnedCount(relics); ++ri) {
                    RelicState* relic_state = RelicsStateGet(relics, ri);
                    bool activated = false;
                    switch (relic_state->id) {
                        case RELIC_SUIT_A_MULT: {
                            if (suit == SUIT_A) {
                                activated = true;
                                ++mult;
                            }
                            break;
                        }
                        case RELIC_SUIT_B_MULT: {
                            if (suit == SUIT_B) {
                                activated = true;
                                ++mult;
                            }
                            break;
                        }
                        case RELIC_SUIT_C_MULT: {
                            if (suit == SUIT_C) {
                                activated = true;
                                ++mult;
                            }
                            break;
                        }
                        case RELIC_SUIT_D_MULT: {
                            if (suit == SUIT_D) {
                                activated = true;
                                ++mult;
                            }
                            break;
                        }
                        case RELIC_SUIT_A_CHIPS: {
                            if (suit == SUIT_A) {
                                activated = true;
                                block_score += 50;
                            }
                            break;
                        }
                        case RELIC_SUIT_B_CHIPS: {
                            if (suit == SUIT_B) {
                                activated = true;
                                block_score += 50;
                            }
                            break;
                        }
                        case RELIC_SUIT_C_CHIPS: {
                            if (suit == SUIT_C) {
                                activated = true;
                                block_score += 50;
                            }
                            break;
                        }
                        case RELIC_SUIT_D_CHIPS: {
                            if (suit == SUIT_D) {
                                activated = true;
                                block_score += 50;
                            }
                            break;
                        }
                        case RELIC_PLUS_MULT_CONSECTUTIVE: {
                            if (battle->cleared_streak) {
                                activated = true;
                                mult += battle->cleared_streak;
                            }
                            break;
                        }
                        case RELIC_FIRST_CLEAR_MULT: {
                            if (battle->num_cleared == 0) {
                                activated = true;
                                mult += 3;
                            }
                            break;
                        }
                        case RELIC_CLEAR_COUNT_MULT: {
                            // Little hacky in that we are relying on the fact that we are looping over the cleared
                            // rows. A simpler impl would be to just += num_cleared_rows
                            ++mult;
                            activated = true;
                            break;
                        }
                        case RELIC_ALL_SUIT_MULT: {
                            static i32 ALL_SUIT_MASK = (1 << SUIT_A) | (1 << SUIT_B) | (1 << SUIT_C) | (1 << SUIT_D);
                            if (all_suit_mask == ALL_SUIT_MASK) {
                                activated = true;
                                mult += 1;
                            }
                            break;
                        }
                        case RELIC_ALL_SUIT_CHIPS: {
                            static i32 ALL_SUIT_MASK = (1 << SUIT_A) | (1 << SUIT_B) | (1 << SUIT_C) | (1 << SUIT_D);
                            if (all_suit_mask == ALL_SUIT_MASK) {
                                activated = true;
                                block_score += 20;
                            }
                            break;
                        }
                        default: {
                            PlatformLog(LLOG_ERROR, "Unknown owned relic: %d", relic_state);
                            Assert(0);
                        }
                    }

                    relic_state->activated_time = activated ? time.accumulated : 0.0f;
                }
            }

            // Score trick.
            // Don't reset after scoring so that UI can display for the moment.
            battle->trick_base_score = 0;
            battle->trick_mult_score = 0;
            battle->trick_base_score += block_score;
            battle->trick_mult_score += mult;

            battle->total_score += battle->trick_base_score * battle->trick_mult_score;
        }

        // Metrics
        {
            battle->blocks_placed_since_last_clear =
                num_cleared_rows > 0 ? 0 : battle->blocks_placed_since_last_clear + 1;
            battle->max_blocks_placed_since_last_clear =
                i32max(battle->max_blocks_placed_since_last_clear, battle->blocks_placed_since_last_clear);
            battle->num_cleared += num_cleared_rows;
            battle->cleared_streak = num_cleared_rows > 0 ? battle->cleared_streak + 1 : 0;
        }

        // Zero out the cleared rows.
        for (isize i = 0; i < num_cleared_rows; ++i) {
            isize r = cleared_row_start + i;
            Assert(r < grid->nrows);
            for (isize c = 0; c < grid->ncols; ++c) {
                Tile* tile = GridGetTile(grid, r, c);
                Assert(tile->occupied);
                tile->occupied = false;
            }
        }

        // Shift down everything above the cleared lines.
        for (isize r = cleared_row_start + num_cleared_rows; r < grid->nrows; ++r) {
            for (isize c = 0; c < grid->ncols; ++c) {
                Tile* source = GridGetTile(grid, r, c);
                Tile* dest = GridGetTile(grid, r - num_cleared_rows, c);

                dest->occupied = source->occupied;
                dest->effects = source->effects;
                source->occupied = false;
            }
        }
    }

    // Remove card from hand.
    {
        for (isize i = battle->selection_idx; i + 1 < ArrLen(&battle->hand); ++i) {
            Card* to = ArrGetPtr(&battle->hand, i);
            Card* from = ArrGetPtr(&battle->hand, i + 1);
            // copying the entire card copies the pos which will lead to visual oddities. Should think about a better
            // way for this.
            to->piece = from->piece;
        }

        ArrPop(&battle->hand);
        battle->selection_idx = NO_CARD_SELECTED_IDX;
    }

    BattleDrawToFull(battle);

    // Hand over + score hand.
    if (ArrLen(&battle->hand) <= 0) {
        if (battle->total_score >= battle->score_target) {
            PlatformLog(LLOG_INFO, "Score Target Reached - Round Win!");
            BattleInit(battle, battle->renderw_world, battle->renderh_world, battle->score_target * 1.5f);
        } else {
            // TODO: Detect round win or game over.
            PlatformLog(LLOG_INFO, "Score Target Missed - Round Loss!");
            Assert(0);
        }
    }
}

// TODO: Combine with rotate function. move + rotate.
static void BattleMoveActivePiece(Battle* battle, bool right) {
    Piece* active_piece = &battle->active_piece;
    i32 move_dir = right ? 1 : -1;

    TileId new_anchor = TileIdLit(battle->anchor.r, battle->anchor.c + move_dir);
    for (isize i = 0; i < ArrLen(&active_piece->block_offsets); ++i) {
        TileId offset = ArrGet(&active_piece->block_offsets, i);
        TileId tile = TileIdLit(new_anchor.r + offset.r, new_anchor.c + offset.c);
        if (!GridValidTileId(&battle->grid, tile)) {
            new_anchor = battle->anchor;
            break;
        }
    }

    battle->anchor = new_anchor;
    battle->placement_anchor = GridCalculatePiecePlacementAnchor(&battle->grid, *active_piece, battle->anchor);
}

static void BattleRotateActivePiece(Battle* battle, bool clockwise) {
    Piece* active_piece = &battle->active_piece;
    isize dir = clockwise ? 1 : -1;

    PieceRot next_rot = ((active_piece->rot_idx + dir) + PR_COUNT) % PR_COUNT;
    Assert(next_rot >= 0 && next_rot < PR_COUNT);
    TileIdArr next_block_offsets = PieceGetOffsets(active_piece->type, next_rot);
    i32 next_rot_max_height = CalculateOffsetHeight(next_block_offsets);
    TileId next_anchor = TileIdLit((battle->grid.nrows - 1) - next_rot_max_height, battle->anchor.c);

    for (isize i = 0; i < ArrLen(&next_block_offsets); ++i) {
        TileId offset = ArrGet(&next_block_offsets, i);
        TileId tile = TileIdLit(next_anchor.r + offset.r, next_anchor.c + offset.c);
        if (!GridValidTileId(&battle->grid, tile)) {
            next_rot = active_piece->rot_idx;
            next_block_offsets = active_piece->block_offsets;
            next_anchor = battle->anchor;
            break;
        }
    }

    active_piece->rot_idx = next_rot;
    active_piece->block_offsets = next_block_offsets;
    battle->anchor = next_anchor;
    battle->placement_anchor = GridCalculatePiecePlacementAnchor(&battle->grid, *active_piece, battle->anchor);
}

typedef struct {
    TileId block_scored;
} BattleRenderState;

static void ComputeBattleRenderState(BattleRenderState* brs, ScoreLog* log) {
    brs->block_scored = ScoreLogHead(log)->block_id;
}

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef enum {
    SCENE_DEFAULT = 0,
    SCENE_MAIN,
    SCENE_SHOP,
    SCENE_DEFAULT_3D,
    SCENE_COUNT,
} SceneId;

typedef enum {
    GP_NONE = 0,
    GP_NEW_ROUND,
    GP_PLAYING,
    GP_SHOP,
    GP_COUNT,
} GamePhase;

typedef struct GameState {
    /////////////////////////////////////////
    // Core fields
    Arena perm;
    Arena temp;

    bool debug;
    bool update;
    bool render;
    bool game_over;
    bool init;  // True on first frame of scene, tells scene to run any init code.

    // TODO: Move this to RenderState and move a reference to RSTATE here. Or just get rid of this and have a
    // GetScreen/RenderWidth() function that reads RSTATE.
    u32 renderw;
    u32 renderh;
    f32 renderw_world;  // in meters
    f32 renderh_world;  // in meters
    u32 screenw;
    u32 screenh;

    bool debug_ever;

    f32 dt;     // frame delta time.
    f32 timer;  // THIS IS REALLY ACCUMULATED FRAME TIME ACROSS THE ENTIRE PROGRAM
    FrameTime frame_time;
    u32 nframe;            // num frame we are on.
    f32 frame_times[256];  // The incremental dts of each frame produced. its a ring buffer. used for fps count.
    i32 fts_tail;
    i32 fts_head;
    u32 fps;
    i32 target_fps;        // render fps we are targetting.
    f32 fixed_dt;          // fixed dt we will update the simulation with for stability.
    f32 accumulated_time;  // Time we have yet to simulate. THIS IS NOT ACCUMULATED FRAME TIME ACROSS THE ENTIRE LIFE OF
                           // THE PROGRAM.

    void (*scenefuncs[MAX_SCENES])(struct GameState*, GameInput* input);
    u32 scenefuncssize;
    SceneId sceneid;
    SceneId nextsceneid;

    LCamera camera;

    /////////////////////////////////////////
    // Scene fields

    GamePhase phase;
    i32 score_target;

    Battle battle;

} GameState;

static GameState GState;

#define GetGameState() (&GState)

#define GameStateGetGrid(gstate) (&(gstate)->battle.grid)

typedef void (*SceneTickFunc)(GameState* gstate);

i32 GameGetTargetFps() { return GetGameState()->target_fps; }

// Converts a screen space position (assumes origin bottom left) to a 2d position in world space.
Vec2 GameScreenToWorld2D(Vec2 pos_screen) {
    GameState* gstate = GetGameState();
    RenderState* rstate = GetRenderState();

    // PlatformLog(LLOG_INFO, "Pos Screen %f %f", pos_screen.x, pos_screen.y);

    Vec2 viewport_dims = Vec2Lit(rstate->screen_target.width, rstate->screen_target.height);
    Vec2 viewport_origin = Vec2Zero();

    // Transform from screen space to clip space.
    // https://registry.khronos.org/OpenGL-Refpages/gl4/html/glViewport.xhtml
    Vec3 pos_clip;
    pos_clip.x = ((pos_screen.x - viewport_origin.x) * (2.0f / viewport_dims.x)) - 1.0f;
    pos_clip.y = ((pos_screen.y - viewport_origin.y) * (2.0f / viewport_dims.y)) - 1.0f;
    pos_clip.z = 0.0f;
    // PlatformLog(LLOG_INFO, "Pos clip %f %f %f", pos_clip.x, pos_clip.y, pos_clip.z);

    // Transform from clip to camera space and from camera to world.
    // TODO: I should auto derive these matrices from render or game state potentially? The values hardcoded here could
    // become out of sync if I change the setup in the game tick rendering
    Mat4 inv_proj = Mat4Invert(Mat4Ortho(-0.5f * gstate->renderw_world, 0.5f * gstate->renderw_world,
                                         -0.5f * gstate->renderh_world, 0.5f * gstate->renderh_world, -100.0f, 100.0f));
    Mat4 inv_view = Mat4Invert(CameraGetViewMat(gstate->camera));
    Mat4 inv = Mat4Multiply(inv_view, inv_proj);
    Vec3 pos_world = Vec3WTransform(pos_clip, inv);

    return Vec3ToVec2(pos_world);
}

// Resets global state/system state that contains per scene info. Experimental approach
// for use in scene switching as opposed to putting all this state into GameState object
// and passing that around. The globalness of these systems is kind of convenient at the
// moment.
static void GameStateLoadScene(GameState* gstate, SceneId sceneid) {
    Assert(sceneid < MAX_SCENES);

    EntitySystemInit(&gstate->perm);

    // Signal to scene update func to run init.
    gstate->init = true;
    gstate->update = true;
    gstate->debug = false;
    gstate->debug_ever = false;
    gstate->timer = 0.0f;
    gstate->nframe = 0;

    gstate->sceneid = sceneid;
    gstate->nextsceneid = gstate->sceneid;
}

static const Vec2 GRIDR = {.x = GRID_WIDTH, .y = GRID_HEIGHT};

typedef enum {
    PS_NONE = 0,
    PS_COUNT,
} PlayerState;

LCORE void EntityCollideWorld(Entity* ea, CollisionArr* collisions) {
    if (PolyhedronIsNull(&ea->shape)) {
        return;
    }
    Polyhedron pa = PolyhedronTransform(ea->shape, ea->scale, ea->orient, ea->pos);

    for (Entity* eb = ea; IncrementEntity(&eb);) {
        if (ea == eb || PolyhedronIsNull(&eb->shape)) {
            continue;
        }

        Polyhedron pb = PolyhedronTransform(eb->shape, eb->scale, eb->orient, eb->pos);

        CollisionInfo info = {.id0 = ea->id, .id1 = eb->id};
        bool collides = TestPolyhedronVsPolyhedron(&pa, &pb, &info.normal, &info.depth);
        if (collides) {
            // if (TestFlag(ea->flags, EFLAG_PROJECTILE) || TestFlag(eb->flags, EFLAG_PROJECTILE)) {
            //     PlatformLog(LLOG_INFO, "Collides id0=%d id1=%d", info.id0.idx, info.id1.idx);
            // }
            // if (TestFlag(ea->flags, EFLAG_PROJECTILE) && TestFlag(eb->flags, EFLAG_PROJECTILE)) {
            //     breakpoint();
            // }
            // TODO: Should I just continue on here? At the moment we detect all collisions, but need to
            // verify that works if for say we collide with a wall that has multiple geometry entities
            // inside of it
            ArrPush(collisions, info);
        }
    }
}

static void SceneMainTick(GameState* gstate, GameInput* input) {
    f32 dt = gstate->dt;

    // Grid* grid = GameStateGetGrid(gstate);

    GameControllerState* controller = GetController(input, 0);
    Battle* battle = &gstate->battle;
    Grid* grid = &gstate->battle.grid;

    if (gstate->init) {
        gstate->phase = GP_NEW_ROUND;
        gstate->score_target = 10;
        BattleInit(&gstate->battle, gstate->renderw_world, gstate->renderh_world, gstate->score_target);
    }

    if (gstate->update) {
        switch (gstate->phase) {
            case GP_NEW_ROUND: {
                BattleInit(&gstate->battle, gstate->renderw_world, gstate->renderh_world, gstate->score_target);
                gstate->phase = GP_PLAYING;
                break;
            }
            case GP_PLAYING: {
                // Movement and rotation.
                {
                    if (controller->move_left.pressed) {
                        PlatformLog(LLOG_INFO, "move left");
                        BattleMoveActivePiece(battle, false);
                    }
                    if (controller->move_right.pressed) {
                        PlatformLog(LLOG_INFO, "move right");
                        BattleMoveActivePiece(battle, true);
                    }

                    if (controller->action_down.pressed) {
                        PlatformLog(LLOG_INFO, "rotate clockwise");
                        BattleRotateActivePiece(battle, true);
                    }

                    if (controller->action_right.pressed) {
                        PlatformLog(LLOG_INFO, "rotate counterclockwise");
                        BattleRotateActivePiece(battle, false);
                    }
                }

                if (controller->action_left.pressed && battle->selection_idx != NO_CARD_SELECTED_IDX) {
                    BattlePlacePiece(battle, gstate->frame_time);
                }

                // Update hand
                {
                    isize selection_to_be = NO_CARD_SELECTED_IDX;
                    for (isize i = 0; i < ArrLen(&battle->hand); ++i) {
                        Card* card = ArrGetPtr(&battle->hand, i);
                        Vec3 quad_pos = Vec3WTransform(card->pos, battle->hand_to_world);
                        AABB2D box = AABB2DFromWidthHeight(Vec3ToVec2(quad_pos), battle->card_scale);
                        Vec2 normal = {0};
                        f32 depth = 0.0f;
                        bool collides =
                            TestPoint2DVsAABB2D(Vec3ToVec2(controller->cursor_pos_world), box, &normal, &depth);
                        if (collides && controller->action_left.pressed) {
                            // PlatformLog(LLOG_INFO, "mouse collides hand %d", i);
                            selection_to_be = i;
                        }
                    }

                    if (selection_to_be != NO_CARD_SELECTED_IDX && battle->selection_idx == NO_CARD_SELECTED_IDX) {
                        battle->selection_idx = selection_to_be;
                        BattleSetActivePiece(battle);
                    }

                    if (controller->right_shoulder.pressed) {
                        battle->selection_idx = NO_CARD_SELECTED_IDX;
                        // Causes issues with trying to set active piece while we have nothing selected. I should maybe
                        // consider instead not having active piece live as data, but instead just always select it
                        // given the idx. BattleSetActivePiece(battle);
                    }
                }

                if (battle->total_score >= battle->score_target) {
                    gstate->phase = GP_SHOP;
                    battle->score_target += 5;
                }

                break;
            }
            case GP_SHOP: {
                if (controller->action_down.pressed) {
                    gstate->phase = GP_NEW_ROUND;
                }
                break;
            }
            default: {
                PlatformLog(LLOG_INFO, "UNKNOWN UPDATE GAME PHASE %d", gstate->phase);
                Assert(0);
            }
        }
    }

    if (gstate->render) {
        // TODO: Focus on orienting UI to be more like balatro. Have a play area that is reserved for changing UI
        // elements (grid, shop, etc), have a UI overlay that is static and does not fixed. Score on the left, relics on
        // top, cards on the bottom (?). As the "scenes transition", just the middle swappable area moves
        Render(dt) {
            // Entities/Game World
            {
                // Reminder that using the _world variables here ensures we convert from world units back to pixel units
                // correctly.
                Mat4 view = CameraGetViewMat(gstate->camera);
                Vec2 half_render_dims = Vec2Lit(0.5f * gstate->renderw_world, 0.5f * gstate->renderh_world);
                Mat4 proj = Mat4Ortho(-half_render_dims.x, half_render_dims.x, -half_render_dims.y, half_render_dims.y,
                                      -100.0f, 100.0f);
                RenderPassLoadAction load_action = DefaultLoadAction();
                load_action.clear_color = LPURPLE;
                load_action.clear_color_flag = true;
                BeginShaderPass(GetDefaultShader(), load_action, view, proj, GetDefaultRenderTarget());

                switch (gstate->phase) {
                    case GP_NEW_ROUND: {
                        break;
                    }
                    case GP_PLAYING: {
                        // Draw grid
                        {
                            Vec4 GRAY1 = RGBLit(70, 70, 72);
                            Vec4 GRAY2 = RGBLit(82, 82, 84);

                            Mat4 grid_to_world = grid->grid_to_world;
                            i32 color_idx = 0;
                            for (i32 r = 0; r < grid->nrows; ++r) {
                                for (i32 c = 0; c < grid->ncols; ++c, ++color_idx) {
                                    Tile* tile = GridGetTile(grid, r, c);
                                    Vec3 pos = Vec3WTransform(tile->pos, grid_to_world);
                                    Vec3 scale = Vec3Transform(Vec3LitAll(1.0f), grid_to_world);
                                    Vec4 color = color_idx % 2 == 0 ? GRAY1 : GRAY2;
                                    DrawQuad(pos, scale.x, scale.y, Mat4Identity(), color);

                                    if (tile->occupied) {
                                        Vec4 piece_color = tile->effects.base_color;
                                        DrawQuad(pos, scale.x, scale.y, Mat4Identity(), piece_color);
                                    }
                                }

                                // Offset each row.
                                ++color_idx;
                            }

                            // Draw active piece.
                            if (battle->selection_idx != NO_CARD_SELECTED_IDX) {
                                TileId anchor = battle->anchor;
                                Piece* active_piece = &battle->active_piece;
                                for (isize i = 0; i < ArrLen(&active_piece->block_offsets); ++i) {
                                    TileId offset = ArrGet(&active_piece->block_offsets, i);
                                    Tile* tile = GridGetTile(grid, anchor.r + offset.r, anchor.c + offset.c);
                                    Vec3 pos = Vec3WTransform(tile->pos, grid_to_world);
                                    Vec3 scale = Vec3Transform(Vec3LitAll(1.0f), grid_to_world);
                                    DrawQuad(pos, scale.x, scale.y, Mat4Identity(), active_piece->effects.base_color);
                                }

                                // Draw placement preview viz
                                TileId placement_anchor = battle->placement_anchor;
                                for (isize i = 0; i < ArrLen(&active_piece->block_offsets); ++i) {
                                    TileId offset = ArrGet(&active_piece->block_offsets, i);
                                    Tile* tile =
                                        GridGetTile(grid, placement_anchor.r + offset.r, placement_anchor.c + offset.c);
                                    Vec3 pos = Vec3WTransform(tile->pos, grid_to_world);
                                    Vec3 scale = Vec3Transform(Vec3LitAll(1.0f), grid_to_world);
                                    Vec4 color = active_piece->effects.base_color;
                                    color.a = 0.3f;
                                    DrawQuad(pos, scale.x, scale.y, Mat4Identity(), color);
                                }
                            }
                        }

                        // Draw hand
                        {
                            f32 piece_scale = Px2Meter(5.0f);
                            BattleHandIter(&battle->hand, card) {
                                Vec3 quad_pos = Vec3WTransform(card->pos, battle->hand_to_world);
                                DrawQuad(quad_pos, battle->card_scale.x, battle->card_scale.y, Mat4Identity(), LGRAY);

                                // Draw piece inside card.
                                for (isize j = 0; j < ArrLen(&card->piece.block_offsets); ++j) {
                                    TileId offset = ArrGet(&card->piece.block_offsets, j);
                                    Vec3 pos = Vec3Lit(offset.c * piece_scale, offset.r * piece_scale, 0.0f);
                                    pos = Vec3Add(pos, quad_pos);
                                    DrawQuad(pos, piece_scale, piece_scale, Mat4Identity(),
                                             card->piece.effects.base_color);
                                }
                            }
                        }
                        break;
                    }
                    case GP_SHOP: {
                        break;
                    }
                    default: {
                        PlatformLog(LLOG_ERROR, "Unknown game phase for rendering: %d", gstate->phase);
                        Assert(0);
                    }
                }

                EndShaderPass();
            }

            // Draw the texture framebuffer to screen quad.
            {
                // We aren't specifying a view or proj matrix, since we are specifying the coordinates in clip space
                // directly.
                Mat4 view = Mat4Identity();
                Mat4 proj = Mat4Identity();
                RenderPassLoadAction load_action = {
                    .clear_color = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f),
                    .clear_color_flag = true,
                    .depth_test = false,
                };
                BeginShaderPass(GetEffectsShader(), load_action, view, proj, GetScreenRenderTarget());

                // DrawCmd* cmd = AddDrawMeshCmd();
                // // DrawMeshCmdInit(cmd);
                // cmd->meshcmd.meshid = GetMesh(M_QUAD);
                // // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since the
                // // quad is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
                // cmd->meshcmd.modelm = Mat4ScaleV(Vec3Lit(2.0f, 2.0f, 1.0f));
                // // cmd->meshcmd.color = LRED;  // For debug.
                // RenderTarget game_target = GetDefaultRenderTarget();
                // cmd->meshcmd.tx.id = game_target.txid;
                // cmd->meshcmd.tx.width = game_target.width;
                // cmd->meshcmd.tx.height = game_target.height;

                RenderTarget game_target = GetDefaultRenderTarget();
                LTexture game_texture = {
                    .id = game_target.txid, .width = game_texture.width, .height = game_target.height};
                // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since the
                // quad is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
                // iirc this is specific to how this effects shader works.
                DrawLTextureProUV(game_texture, Vec2Zero(), Vec2Lit(1.0f, 1.0f), Vec2Zero(), Vec2Lit(2.0f, 2.0f), 0.0f,
                                  LCLEAR);

                EndShaderPass();
            }

            // UI.
            {
                UIState ui = {0};
                RenderPassLoadAction load_action = {
                    .clear_color_flag = false,
                    .depth_test = true,
                };
                UI2D(&ui, &gstate->temp, input, PlatformScreenWidth(), PlatformScreenHeight(), GetDefaultShader(),
                     load_action, GetScreenRenderTarget()) {
                    UIPushFont(&ui, GetLFontClosest(40.0f));
                    UIPushThemeColor(&ui, UI_THEME_LABEL_TEXT_COLOR, LWHITE);
                    UIWindow(&ui, UISizePctParent(0.3f), UISizePctParent(1.0f)) {
                        {
                            char buffer[256] = {0};
                            snprintf(buffer, ArraySizeOf(buffer) - 1, "Target: %d", battle->score_target);
                            UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        }
                        {
                            char buffer[256] = {0};
                            snprintf(buffer, ArraySizeOf(buffer) - 1, "Total Score: %d", battle->total_score);
                            UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        }
                        {
                            char buffer[256] = {0};
                            snprintf(buffer, ArraySizeOf(buffer) - 1, "Trick Score: %d X %d", battle->trick_base_score,
                                     battle->trick_mult_score);
                            UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        }
                        {
                            char buffer[256] = {0};
                            snprintf(buffer, ArraySizeOf(buffer) - 1, "Deck Size: %lld", ArrLen(&battle->deck));
                            UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        }
                        // UILabel(&ui, Str8Lit("space"), UI_FLAG_HIDE);
                        // {
                        //     char buffer[256] = {0};
                        //     snprintf(buffer, ArraySizeOf(buffer) - 1, "Blocks Since Clear: %d",
                        //              battle->blocks_placed_since_last_clear);
                        //     UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        // }
                        // {
                        //     char buffer[256] = {0};
                        //     snprintf(buffer, ArraySizeOf(buffer) - 1, "Max Blocks Since Clear: %d",
                        //              battle->max_blocks_placed_since_last_clear);
                        //     UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        // }
                        // {
                        //     char buffer[256] = {0};
                        //     snprintf(buffer, ArraySizeOf(buffer) - 1, "Total Cleared: %d", battle->num_cleared);
                        //     UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        // }
                        // {
                        //     char buffer[256] = {0};
                        //     snprintf(buffer, ArraySizeOf(buffer) - 1, "Cleared Streak: %d", battle->cleared_streak);
                        //     UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
                        // }

                        // Draw relics
                        UIThemeColor(&ui, UI_THEME_WINDOW_BG_COLOR, LRED) {
                            static f32 RELIC_TEXT_ANIMATION_TIME = 1.0f;
                            UIWindow(&ui, UISizePctParent(1.0f), UISizePctParent(1.0f)) {
                                for (isize i = 0; i < RelicsOwnedCount(&battle->relics); ++i) {
                                    RelicState* relic_state = RelicsStateGet(&battle->relics, i);
                                    Relic* relic = RelicGet(relic_state->id);

                                    f32 activated_dt =
                                        f32clamp(gstate->timer - relic_state->activated_time, 0.0f, 1.0f);
                                    f32 activated_t = activated_dt / RELIC_TEXT_ANIMATION_TIME;
                                    Vec4 color = Vec4Lerp(LYELLOW, LWHITE, activated_t);
                                    UIThemeColor(&ui, UI_THEME_LABEL_TEXT_COLOR, color) {
                                        UILabel(&ui, relic->name, UI_FLAG_NONE);
                                    }
                                }
                            }
                        }
                    }
                }

                {
                    // Reminder that using the _world variables here ensures we convert from world units back to pixel
                    // units
                    // correctly.
                    Mat4 view = Mat4Identity();
                    Mat4 proj = Mat4Ortho(0, PlatformScreenWidth(), 0, PlatformScreenHeight(), -1.0f, 1.0f);
                    BeginShaderPass(GetDefaultShader(), load_action, view, proj, GetScreenRenderTarget());

                    EndShaderPass();
                }
            }
        }
    }
}

static void SceneShopTick(GameState* gstate, GameInput* input) {
    // static isize RELICS_SELECTION_IDX = -1;

    // f32 dt = gstate->dt;

    // // Grid* grid = GameStateGetGrid(gstate);

    // GameControllerState* controller = GetController(input, 0);

    // if (gstate->init) {
    //     RELICS_SELECTION_IDX = -1;
    // }

    // if (gstate->update) {
    // }

    // if (gstate->render) {
    //     Render(dt) {
    //         // Entities/Game World
    //         {
    //             // Reminder that using the _world variables here ensures we convert from world units back to pixel
    //             units
    //             // correctly.
    //             Mat4 view = CameraGetViewMat(gstate->camera);
    //             Vec2 half_render_dims = Vec2Lit(0.5f * gstate->renderw_world, 0.5f * gstate->renderh_world);
    //             Mat4 proj = Mat4Ortho(-half_render_dims.x, half_render_dims.x, -half_render_dims.y,
    //             half_render_dims.y,
    //                                   -100.0f, 100.0f);
    //             RenderPassLoadAction load_action = DefaultLoadAction();
    //             load_action.clear_color = LPURPLE;
    //             load_action.clear_color_flag = true;
    //             BeginShaderPass(GetDefaultShader(), load_action, view, proj, GetDefaultRenderTarget());

    //             {
    //                 Vec2 relic_scale = Vec2Lit(Px2Meter(30.0f), Px2Meter(50.0f));
    //                 Mat4 relic_to_world = Mat4SVRT(Vec3Lit(Px2Meter(40.0f), Px2Meter(40.0f), Px2Meter(1.0f)),
    //                                                Mat4Identity(), Vec3Lit(0, 0, 0));
    //                 for (isize i = 0; i < ArraySizeOf(RELICS); ++i) {
    //                     Vec3 quad_pos = Vec3Lit(i, 0, 0);
    //                     quad_pos = Vec3WTransform(quad_pos, relic_to_world);
    //                     DrawQuad(quad_pos, relic_scale.x, relic_scale.y, Mat4Identity(), LGRAY);
    //                 }
    //             }

    //             EndShaderPass();
    //         }

    //         // Draw the texture framebuffer to screen quad.
    //         {
    //             // We aren't specifying a view or proj matrix, since we are specifying the coordinates in clip space
    //             // directly.
    //             Mat4 view = Mat4Identity();
    //             Mat4 proj = Mat4Identity();
    //             RenderPassLoadAction load_action = {
    //                 .clear_color = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f),
    //                 .clear_color_flag = true,
    //                 .depth_test = false,
    //             };
    //             BeginShaderPass(GetEffectsShader(), load_action, view, proj, GetScreenRenderTarget());

    //             // DrawCmd* cmd = AddDrawMeshCmd();
    //             // // DrawMeshCmdInit(cmd);
    //             // cmd->meshcmd.meshid = GetMesh(M_QUAD);
    //             // // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since
    //             the
    //             // // quad is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
    //             // cmd->meshcmd.modelm = Mat4ScaleV(Vec3Lit(2.0f, 2.0f, 1.0f));
    //             // // cmd->meshcmd.color = LRED;  // For debug.
    //             // RenderTarget game_target = GetDefaultRenderTarget();
    //             // cmd->meshcmd.tx.id = game_target.txid;
    //             // cmd->meshcmd.tx.width = game_target.width;
    //             // cmd->meshcmd.tx.height = game_target.height;

    //             RenderTarget game_target = GetDefaultRenderTarget();
    //             LTexture game_texture = {
    //                 .id = game_target.txid, .width = game_texture.width, .height = game_target.height};
    //             // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since the
    //             // quad is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
    //             // iirc this is specific to how this effects shader works.
    //             DrawLTextureProUV(game_texture, Vec2Zero(), Vec2Lit(1.0f, 1.0f), Vec2Zero(), Vec2Lit(2.0f, 2.0f),
    //             0.0f,
    //                               LCLEAR);

    //             EndShaderPass();
    //         }

    //         // UI.
    //         {
    //             UIState ui = {0};
    //             RenderPassLoadAction load_action = {
    //                 .clear_color_flag = false,
    //                 .depth_test = true,
    //             };
    //             UI2D(&ui, &gstate->temp, input, PlatformScreenWidth(), PlatformScreenHeight(), GetDefaultShader(),
    //                  load_action, GetScreenRenderTarget()) {
    //                 UIPushFont(&ui, GetLFontClosest(40.0f));
    //                 UIPushThemeColor(&ui, UI_THEME_LABEL_TEXT_COLOR, LWHITE);
    //                 UIWindow(&ui, UISizePctParent(0.3f), UISizePctParent(1.0f)) {
    //                     {
    //                         char buffer[256] = {0};
    //                         snprintf(buffer, ArraySizeOf(buffer) - 1, "Target: %d", 100);
    //                         UILabel(&ui, Str8FromC(buffer), UI_FLAG_NONE);
    //                     }
    //                 }
    //             }

    //             {
    //                 // Reminder that using the _world variables here ensures we convert from world units back to
    //                 pixel
    //                 // units
    //                 // correctly.
    //                 Mat4 view = Mat4Identity();
    //                 Mat4 proj = Mat4Ortho(0, PlatformScreenWidth(), 0, PlatformScreenHeight(), -1.0f, 1.0f);
    //                 BeginShaderPass(GetDefaultShader(), load_action, view, proj, GetScreenRenderTarget());

    //                 EndShaderPass();
    //             }
    //         }
    //     }
    // }
}

////////////////////////////////////////////////////////////

static void TestL();

void GameInit() {
    GameState* gstate = &GState;
    TestL();

    memset(gstate, 0, sizeof(GameState));

    // Initialize dyanmic memory pool.
    isize cap = GB(4);
    byte* buffer = PlatformAllocate(cap);
    gstate->perm = NewArena(buffer, cap);
    byte* temp_buffer = New(&gstate->perm, GB(1), byte);
    gstate->temp = NewArena(temp_buffer, GB(1));

    gstate->renderw = 320.0f;
    gstate->renderh = 180.0f;
    // Reminder, the way we convert from world units back to screen units for rendering is by using these for the
    // projection matrix.
    gstate->renderw_world = Px2Meter(gstate->renderw);
    gstate->renderh_world = Px2Meter(gstate->renderh);

    gstate->target_fps = 60;
    gstate->fixed_dt = Hz(2.0f * gstate->target_fps);

    u32 scale = 6;
    PlatformInit(scale * gstate->renderw, scale * gstate->renderh, gstate->target_fps);

    isize sound_cap = GB(2);
    byte* sound_buffer = New(&gstate->perm, sound_cap, byte);

    PlatformInitAudio(sound_buffer, sound_cap);
    InitFonts();
    ParticleSystemInit();

    {
        Arena render_state_arena = NewSubArena(&gstate->perm, MB(1));
        RenderStateInit(gstate->renderw_world, gstate->renderh_world, &render_state_arena);
        Arena render_data_arena = NewSubArena(&gstate->perm, MB(10));
        RenderDataInit(&render_data_arena);
    }

    LoadSounds();

    {
        // glEnable(GL_DEPTH_TEST);

        // Allow blending to support drawing textures with transparency.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Register scenes.
    {
        u32 idx = 0;
        gstate->scenefuncs[SCENE_MAIN] = SceneMainTick;
        gstate->scenefuncs[SCENE_SHOP] = SceneShopTick;
        Assert(SCENE_COUNT < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, SCENE_MAIN);

    gstate->fts_tail = 1;
    gstate->fts_head = 0;

    gstate->camera.up = Vec3Lit(0.0f, 1.0f, 0.0f);
    gstate->camera.eye = Vec3Lit(0.0f, 0.0f, 10.0f);
}

void GameTick(f32 dt, GameInput* input) {
    GameState* gstate = &GState;
    gstate->dt = dt;
    gstate->timer += dt;
    gstate->frame_time.dt = dt;
    gstate->frame_time.accumulated = gstate->timer;

    gstate->screenw = PlatformScreenWidth();
    gstate->screenh = PlatformScreenHeight();

    {
        static bool ENABLE_DEBUG_KEYMAP = true;

        if (ENABLE_DEBUG_KEYMAP) {
            bool debugkey = PlatformKeyDown(LKEY_LCONTROL) && PlatformKeyPressed(LKEY_1);
            if (debugkey) {
                gstate->debug = !gstate->debug;
                gstate->update = !gstate->debug;
                gstate->debug_ever = true;
                PlatformLog(LLOG_INFO, "Debug %s", gstate->debug ? "On" : "Off");
            }
            if (gstate->debug) {
                bool updatekey = PlatformKeyPressed(LKEY_U);
                if (updatekey) {
                    gstate->update = !gstate->update;
                }
            }
        }
    }

    // Important this happens before the scene tick as we set init=false here and need to make sure the first
    // tick of the scene is with init=false.
    if (gstate->nextsceneid != gstate->sceneid) {
        GameStateLoadScene(gstate, gstate->nextsceneid);
    }

    {
        gstate->update = false;
        gstate->render = false;
        gstate->accumulated_time += dt;

        // This is a follow up to the zeroing input hack we do in the update loop. Need to pass in original input
        // to render frame because interactive UI component logic lives in render step for the time being.
        // TODO: Ideally this goes away either when we back out the zeroing hack in the update step or UI component
        // response logic is moved into update loop (requires some sort of state tracking to then be able to
        // draw the UI components in the render step)
        GameInput orig_input = *input;

        i32 update_tick_count = 0;

        // https://medium.com/%40tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
        // https://gameprogrammingpatterns.com/game-loop.html#play-catch-up
        while (gstate->accumulated_time >= gstate->fixed_dt) {
            ++update_tick_count;

            // Update simulation with fixed delta time.
            gstate->update = true;
            gstate->dt = gstate->fixed_dt;
            gstate->scenefuncs[gstate->sceneid](gstate, input);
            gstate->init = false;
            gstate->accumulated_time -= gstate->fixed_dt;

            // HACK: Zero out input after first tick so that we only consume the input for a single simulation frame.
            // This will likely totally break things like controller movement (e.g. we will detect moving one sim frame
            // and then standing still the next). So I should consider a better alternative at some point.
            // TODO: Lets try the following approach. Add back in half_transition count + ended down to button state.
            // Then before (or maybe after?) each sim tick, we consume only button input. E.g. if a button was detected
            // as being pressed it would then become unpressed (but down dependning on half transition count). Need to
            // potentially think about ordering here as if we do it this way where we consume input, does this mean the
            // sim sees the input "backwards" in time?
            // UPDATE: I've fixed the cursor handling, but we should still make this more robust by consuming
            // crontroller button presses probably.
            for (isize i = 0; i < ArraySizeOf(input->controllers); ++i) {
                for (isize j = 0; j < ArraySizeOf(input->controllers[i].buttons); ++j) {
                    GameButtonState button = {0};
                    input->controllers[i].buttons[j] = button;
                }

                for (isize j = 0; j < ArraySizeOf(input->controllers[i].moves); ++j) {
                    GameButtonState move = {0};
                    input->controllers[i].moves[j] = move;
                }
            }
            DeactivateEntities();
        }

        // Render
        // Potentially we might want to interpolate here, but seems complicated.
        gstate->dt = 0.0f;
        gstate->update = false;
        gstate->render = true;
        gstate->scenefuncs[gstate->sceneid](gstate, &orig_input);
        gstate->init = false;

        if (false) {
            PlatformLog(LLOG_WARN, "Simulated %d times for frame %d", update_tick_count, gstate->nframe);
        }
    }

    ++gstate->nframe;

    // FPS count
    // TODO: Bug if spamming enter on first couple screens. head == tail.
    if (false) {
        Assert(gstate->fts_tail != gstate->fts_head);  // TODO: just like emit a warning log here or something.
        i32 len = ArraySizeOf(gstate->frame_times);
        i32 insert_idx = gstate->fts_tail;
        gstate->frame_times[insert_idx] = gstate->timer;
        ++gstate->fts_tail;
        gstate->fts_tail %= len;

        // The FPS is how many frames we've shown in the last second interval.
        // We only count frames who's timestamp is within a second of the most recent timestamp.
        // Could probably do this instead by storing delta times, but easier to think about in terms
        // of absolute timestamps (with dts I worry about off by one errors).
        i32 cur = insert_idx;
        u32 fps = 0;
        for (; ((cur + len - 1) % len) != gstate->fts_head && (gstate->timer - gstate->frame_times[cur]) <= 1.0f;) {
            cur = (cur + len - 1) % len;
            ++fps;
        }

        gstate->fts_head = cur;
        gstate->fps = fps;
        // PlatformLog(LLOG_INFO, "fps: %d", gstate->fps);
    }
}

////////////////////////////////////////////////////////////

static bool TestFail = false;

static void TAssert(bool result, const char* msg) {
    if (!result) {
        PlatformLog(LLOG_ERROR, "Failed: %s", msg);
    }

    TestFail |= !result;
}

static void TestMath() {
    // Test Vec2.
    {
        TAssert(Vec2Eq(Vec2Lit(1.0f, 1.0f), Vec2Lit(1.0f, 1.0f)), "Vec2Eq");
        TAssert(Vec2Len(Vec2Lit(1.0f, 0.0)) == 1.0f, "Vec2Len");
        TAssert(Vec2Len(Vec2Zero()) == 0.0f, "Vec2Len Zero");
        TAssert(Vec2Eq(Vec2Add(Vec2Zero(), Vec2Lit(1.0f, 1.0f)), Vec2Lit(1.0f, 1.0f)), "Vec2Add");
        TAssert(Vec2Eq(Vec2Sub(Vec2Zero(), Vec2Lit(1.0f, 1.0f)), Vec2Lit(-1.0f, -1.0f)), "Vec2Sub");
        TAssert(Vec2Eq(Vec2Scale(Vec2Lit(1.0f, 1.0f), 2.0f), Vec2Lit(2.0f, 2.0f)), "Vec2Scale");
        TAssert(Vec2Eq(Vec2Clamp(Vec2Zero(), Vec2Zero(), Vec2Zero()), Vec2Zero()), "Vec2Clamp");
        TAssert(f32eq(Vec2Dot(Vec2Lit(1.0f, 0.0f), Vec2Lit(1.0f, 0.0f)), 1.0f), "Vec2Dot");
        TAssert(Vec2Eq(Vec2Normalize(Vec2Lit(3.0f, 0.0f)), Vec2Lit(1.0f, 0.0f)), "Vec2Normalize");
        TAssert(Vec2Eq(Vec2Cross(Vec2Lit(1.0f, 0.0f)), Vec2Lit(0.0f, -1.0f)), "Vec2Cross");
    }

    // Test Vec3.
    {
        TAssert(Vec3EqPro(Vec3Lit(1.0f, 1.0f, 1.0f), Vec3Lit(1.0f, 1.0f, 1.0f), 0.001), "Vec3EqPro");
        TAssert(Vec3Eq(Vec3Lit(1.0f, 1.0f, 0.9998f), Vec3Lit(1.0f, 1.0f, 1.0f)), "Vec3EqPro approx");

        TAssert(f32eq(Vec3Len(Vec3Lit(0.0f, 0.0f, 0.0f)), 0.0f), "Vec3Len 0");
        char* msg[] = {"Vec3Len x", "Vec3Len y", "Vec3Len z"};
        for (u32 i = 0; i < 3; ++i) {
            Vec3 v = Vec3Zero();
            v.f[i] = 1.0f;
            TAssert(f32eq(Vec3Len(v), 1.0f), msg[i]);
        }

        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.0f, 0.0f, 0.0f)), Vec3Zero()), "Vec3Normalize 0");
        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.5f, 0.0f, 0.0f)), Vec3Lit(1.0f, 0.0f, 0.0f)), "Vec3Normalize x");
        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.0f, 199.0f, 0.0f)), Vec3Lit(0.0f, 1.0f, 0.0f)), "Vec3Normalize y");
        TAssert(Vec3Eq(Vec3Normalize(Vec3Lit(0.0f, 0.0f, 1.0f)), Vec3Lit(0.0f, 0.0f, 1.0f)), "Vec3Normalize z");

        TAssert(f32eq(Vec3Dot(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f)), 0.0f), "Vec3Dot |_");
        TAssert(f32eq(Vec3Dot(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 0.0f, 1.0f)), 1.0f), "Vec3Dot ||");
        TAssert(f32eq(Vec3Dot(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 0.0f, -1.0f)), -1.0f), "Vec3Dot neg ||");

        TAssert(f32eq(Vec3Angle(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(0.0f, 0.0f, 1.0f)), 0.0f), "Vec3Angle ||");

        TAssert(Vec3Eq(Vec3RotateAxis(Vec3Lit(0.0f, 0.0f, 1.0f), Vec3Lit(1.0f, 0.0f, 0.0f), Deg2Rad(-90.0f)),
                       Vec3Lit(0.0f, 1.0f, 0.0f)),
                "Vec3RotateAxis z->y");

        // Vec3Cross
        {
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 1.0f, 0.0f)), Vec3Lit(0.0f, 0.0f, 1.0f)),
                    "Vec3Cross X x Y");
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(0.0f, 1.0f, 0.0f), Vec3Lit(1.0f, 0.0f, 0.0f)), Vec3Lit(0.0f, 0.0f, -1.0f)),
                    "Vec3Cross Y x X");
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f)), Vec3Lit(0.0f, -1.0f, 0.0f)),
                    "Vec3Cross X x Z");

            // Cross product of two parallel vectors should result in a zero vector.
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(2.0f, 0.0f, 0.0f)), Vec3Zero()),
                    "Vec3Cross Parallel X");

            // Cross product of two opposite vectors should result in a zero vector.
            TAssert(Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 0.0f, 0.0f), Vec3Lit(-1.0f, 0.0f, 0.0f)), Vec3Zero()),
                    "Vec3Cross Opposite X");

            // Cross product of arbitrary vectors.
            TAssert(
                Vec3Eq(Vec3Cross(Vec3Lit(1.0f, 2.0f, 3.0f), Vec3Lit(4.0f, 5.0f, 6.0f)), Vec3Lit(-3.0f, 6.0f, -3.0f)),
                "Vec3Cross Arbitrary");
        }
    }

    // Test Mat4
    {
        // Mat4ScaleXYZ
        {
            // Test uniform scaling.
            Mat4 scaleUniform = Mat4ScaleXYZ(2.0f, 2.0f, 2.0f);
            Mat4 expectedUniform =
                Mat4Lit(2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleUniform, expectedUniform), "Mat4ScaleXYZ Uniform");

            // Test non-uniform scaling.
            Mat4 scaleNonUniform = Mat4ScaleXYZ(3.0f, 2.0f, 1.0f);
            Mat4 expectedNonUniform =
                Mat4Lit(3.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleNonUniform, expectedNonUniform), "Mat4ScaleXYZ Non-Uniform");

            // Test identity scaling (scaling by 1 on all axes).
            Mat4 scaleIdentity = Mat4ScaleXYZ(1.0f, 1.0f, 1.0f);
            Mat4 expectedIdentity =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleIdentity, expectedIdentity), "Mat4ScaleXYZ Identity");

            // Test scaling with zero on one axis.
            Mat4 scaleZeroX = Mat4ScaleXYZ(0.0f, 1.0f, 1.0f);
            Mat4 expectedZeroX =
                Mat4Lit(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(scaleZeroX, expectedZeroX), "Mat4ScaleXYZ Zero X");
        }

        // Test Mat4TranslateV
        {
            // Test translating by a positive vector.
            Vec3 translatePos = Vec3Lit(3.0f, 4.0f, 5.0f);
            Mat4 translationMatrixPos = Mat4TranslateV(translatePos);
            Mat4 expectedPos =
                Mat4Lit(1.0f, 0.0f, 0.0f, 3.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f, 0.0f, 1.0f, 5.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixPos, expectedPos), "Mat4TranslateV Positive Vector");

            // Test translating by a negative vector.
            Vec3 translateNeg = Vec3Lit(-1.0f, -2.0f, -3.0f);
            Mat4 translationMatrixNeg = Mat4TranslateV(translateNeg);
            Mat4 expectedNeg = Mat4Lit(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -2.0f, 0.0f, 0.0f, 1.0f, -3.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixNeg, expectedNeg), "Mat4TranslateV Negative Vector");

            // Test translating by a zero vector (no translation).
            Vec3 translateZero = Vec3Lit(0.0f, 0.0f, 0.0f);
            Mat4 translationMatrixZero = Mat4TranslateV(translateZero);
            Mat4 expectedZero =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixZero, expectedZero), "Mat4TranslateV Zero Vector");

            // Test translating by a large vector.
            Vec3 translateLarge = Vec3Lit(1000.0f, 2000.0f, 3000.0f);
            Mat4 translationMatrixLarge = Mat4TranslateV(translateLarge);
            Mat4 expectedLarge = Mat4Lit(1.0f, 0.0f, 0.0f, 1000.0f, 0.0f, 1.0f, 0.0f, 2000.0f, 0.0f, 0.0f, 1.0f,
                                         3000.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(translationMatrixLarge, expectedLarge), "Mat4TranslateV Large Vector");
        }

        // Mat4RotateX
        {
            // 1. Test 90-degree rotation (π/2 radians) around the X-axis
            Mat4 rotation90 = Mat4RotateX(LPI / 2.0f);
            Mat4 expected90 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation90, expected90), "Mat4RotateX 90 degrees");

            // 2. Test 180-degree rotation (π radians) around the X-axis
            Mat4 rotation180 = Mat4RotateX(LPI);
            Mat4 expected180 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation180, expected180), "Mat4RotateX 180 degrees");

            // 3. Test 0-degree rotation (0 radians), should produce the identity matrix
            Mat4 rotation0 = Mat4RotateX(0.0f);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation0, expected0), "Mat4RotateX 0 degrees");

            // 4. Test 33-degree rotation (approx. 0.57596 radians) around the X-axis
            f32 angle33 = 33.0f * (LPI / 180.0f);
            Mat4 rotation33 = Mat4RotateX(angle33);
            Mat4 expected33 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.83867f, -0.54464f, 0.0f, 0.0f, 0.54464f, 0.83867f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(rotation33, expected33), "Mat4RotateX 33 degrees");
        }

        // Mat4RotateY
        {
            // Test 1: Rotate by 0 degrees (identity matrix)
            Mat4 result0 = Mat4RotateY(0.0f);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result0, expected0));

            // Test 2: Rotate by 90 degrees around Y-axis
            Mat4 result90 = Mat4RotateY(LPI / 2);
            Mat4 expected90 = Mat4Lit(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result90, expected90));

            // Test 3: Rotate by 180 degrees around Y-axis
            Mat4 result180 = Mat4RotateY(LPI);
            Mat4 expected180 = Mat4Lit(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result180, expected180));

            // Test 4: Rotate by 33 degrees around Y-axis
            float radians33 = 33.0f * (LPI / 180.0f);  // Convert degrees to radians
            Mat4 result33 = Mat4RotateY(radians33);
            Mat4 expected33 = Mat4Lit(0.83867f, 0.0f, 0.54464f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -0.54464f, 0.0f, 0.83867f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result33, expected33));
        }

        // Mat4RotateZ
        {
            // Test 1: Rotate by 0 degrees (identity matrix)
            Mat4 result0 = Mat4RotateZ(0.0f);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result0, expected0));

            // Test 2: Rotate by 90 degrees around Z-axis
            Mat4 result90 = Mat4RotateZ(LPI / 2);
            Mat4 expected90 = Mat4Lit(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result90, expected90));

            // Test 3: Rotate by 180 degrees around Z-axis
            Mat4 result180 = Mat4RotateZ(LPI);
            Mat4 expected180 = Mat4Lit(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result180, expected180));

            // Test 4: Rotate by 33 degrees around Z-axis
            float radians33 = 33.0f * (LPI / 180.0f);  // Convert degrees to radians
            Mat4 result33 = Mat4RotateZ(radians33);
            Mat4 expected33 = Mat4Lit(0.83867f, -0.54464f, 0.0f, 0.0f, 0.54464f, 0.83867f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                      0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result33, expected33));
        }

        // Mat4RotateAxis
        {
            // Test 1: Rotate by 0 degrees around an arbitrary axis (should return the identity matrix)
            Vec3 axis1 = Vec3Lit(1.0f, 1.0f, 1.0f);
            Mat4 result0 = Mat4Rotate(0.0f, axis1);
            Mat4 expected0 =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result0, expected0));

            // Test 2: Rotate by 90 degrees around the X-axis
            Vec3 axis2 = Vec3Lit(1.0f, 0.0f, 0.0f);
            Mat4 result90 = Mat4Rotate(M_PI / 2, axis2);
            Mat4 expected90 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result90, expected90));

            // Test 3: Rotate by 180 degrees around the Y-axis
            Vec3 axis3 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 result180 = Mat4Rotate(M_PI, axis3);
            Mat4 expected180 = Mat4Lit(-1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                       0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result180, expected180));

            // Test 4: Rotate by 33 degrees around an arbitrary axis (1, 1, 1)
            Vec3 axis4 = Vec3Lit(1.0f, 1.0f, 1.0f);
            float radians33 = 33.0f * (M_PI / 180.0f);  // Convert degrees to radians
            Mat4 result33 = Mat4Rotate(radians33, axis4);
            Mat4 expected33 =
                Mat4Lit(0.89244705f, -0.26067102f, 0.36822397f, 0.0f, 0.36822397f, 0.89244705f, -0.26067102f, 0.0f,
                        -0.26067102f, 0.36822397f, 0.89244705f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            assert(Mat4Eq(result33, expected33));
        }

        // Mat4Multiply
        {
            // Test case 1: Multiplying with identity matrix
            Mat4 identity = Mat4Identity();
            Mat4 matrix = Mat4Lit(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f,
                                  14.0f, 15.0f, 16.0f);
            Mat4 result1 = Mat4Multiply(matrix, identity);
            TAssert(Mat4Eq(result1, matrix), "Mat4Multiply - Multiplying with identity should return the same matrix");

            // Test case 2: Multiplying two arbitrary matrices
            Mat4 a = Mat4Lit(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
                             15.0f, 16.0f);
            Mat4 b = Mat4Lit(16.0f, 15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f,
                             2.0f, 1.0f);
            Mat4 expected2 = Mat4Lit(80.0f, 70.0f, 60.0f, 50.0f, 240.0f, 214.0f, 188.0f, 162.0f, 400.0f, 358.0f, 316.0f,
                                     274.0f, 560.0f, 502.0f, 444.0f, 386.0f);
            Mat4 result2 = Mat4Multiply(a, b);
            TAssert(Mat4Eq(result2, expected2), "Mat4Multiply - Multiplying two arbitrary matrices");

            // Test case 3: Multiplying by a scaling matrix
            Mat4 scale = Mat4ScaleXYZ(2.0f, 3.0f, 4.0f);
            Mat4 expected3 = Mat4Lit(2.0f, 6.0f, 12.0f, 4.0f, 10.0f, 18.0f, 28.0f, 8.0f, 18.0f, 30.0f, 44.0f, 12.0f,
                                     26.0f, 42.0f, 60.0f, 16.0f);
            Mat4 result3 = Mat4Multiply(matrix, scale);
            TAssert(Mat4Eq(result3, expected3), "Mat4Multiply - Multiplying by a scaling matrix");

            // Test case 4: Multiplying by a rotation matrix (e.g., a 90-degree rotation around Z-axis)
            Mat4 rotation = Mat4RotateZ(1.5708f);  // ~90 degrees in radians
            Mat4 initial =
                Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            Mat4 expected4 = Mat4Lit(0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f);
            Mat4 result4 = Mat4Multiply(initial, rotation);
            TAssert(Mat4Eq(result4, expected4),
                    "Mat4Multiply - Multiplying by a 90-degree rotation matrix around Z-axis");
        }

        // Mat4Perspective
        {
            // Test case 1: Basic perspective matrix with 45-degree FOV and 16:9 aspect ratio
            Mat4 result1 = Mat4Perspective(Deg2Rad(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
            Mat4 expected1 = Mat4Lit(1.3579952f, 0.0f, 0.0f, 0.0f, 0.0f, 2.4142137f, 0.0f, 0.0f, 0.0f, 0.0f, -1.002002f,
                                     -0.2002002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result1, expected1), "Mat4Perspective - 45-degree FOV, 16:9 aspect ratio");

            // Test case 2: Perspective matrix with square aspect ratio (45-degree FOV, 1:1 aspect ratio)
            Mat4 result2 = Mat4Perspective(Deg2Rad(45.0f), 1.0f, 0.1f, 100.0f);
            Mat4 expected2 = Mat4Lit(2.4142137f, 0.0f, 0.0f, 0.0f, 0.0f, 2.4142137f, 0.0f, 0.0f, 0.0f, 0.0f, -1.002002f,
                                     -0.2002002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result2, expected2), "Mat4Perspective - 45-degree FOV, 1:1 aspect ratio");

            // Test case 3: Perspective matrix with wide aspect ratio (60-degree FOV, 21:9 aspect ratio)
            Mat4 result3 = Mat4Perspective(Deg2Rad(60.0f), 21.0f / 9.0f, 0.1f, 100.0f);
            Mat4 expected3 = Mat4Lit(0.7423, 0.0f, 0.0f, 0.0f, 0.0f, 1.73205f, 0.0f, 0.0f, 0.0f, 0.0f, -1.002002f,
                                     -0.2002002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result3, expected3), "Mat4Perspective - 60-degree FOV, 21:9 aspect ratio");

            // Test case 4: Perspective matrix with small near and large far planes (30-degree FOV, 4:3 aspect
            // ratio)
            Mat4 result4 = Mat4Perspective(Deg2Rad(30.0f), 4.0f / 3.0f, 0.01f, 1000.0f);
            Mat4 expected4 = Mat4Lit(2.79904f, 0.0f, 0.0f, 0.0f, 0.0f, 3.73205f, 0.0f, 0.0f, 0.0f, 0.0f, -1.00002f,
                                     -0.0200002f, 0.0f, 0.0f, -1.0f, 0.0f);
            TAssert(Mat4Eq(result4, expected4),
                    "Mat4Perspective - 30-degree FOV, 4:3 aspect ratio, small near/large far");
        }

        // Mat4LookAt
        {
            // Test Case 1: Simple Case - Looking down the -Z axis from the origin
            Vec3 eye1 = Vec3Lit(0.0f, 0.0f, 0.0f);
            Vec3 target1 = Vec3Lit(0.0f, 0.0f, -1.0f);
            Vec3 up1 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 expected1 = Mat4Identity();
            TAssert(Mat4Eq(Mat4LookAt(eye1, target1, up1), expected1), "Mat4LookAt look down -z from origin");

            // Test Case 2: Offset Eye Position - Looking at the origin from (0, 0, 5)
            Vec3 eye2 = Vec3Lit(0.0f, 0.0f, 5.0f);
            Vec3 target2 = Vec3Lit(0.0f, 0.0f, 0.0f);
            Vec3 up2 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 expected2 = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -5.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(Mat4LookAt(eye2, target2, up2), expected2),
                    "Mat4LookAt Looking at the origin from (0, 0, 5)");

            // Test Case 3: Tilted Up Vector - Looking at the origin from (5, 5, 5) with a tilted up vector
            Vec3 eye3 = Vec3Lit(5.0f, 5.0f, 5.0f);
            Vec3 target3 = Vec3Lit(0.0f, 0.0f, 0.0f);
            Vec3 up3 = Vec3Lit(0.0f, 1.0f, 0.0f);
            Mat4 expected3 = Mat4Lit(0.707107f, 0.0f, -0.707107f, 0.0f, -0.408248f, 0.816497f, -0.408248f, 0.0f,
                                     0.57735f, 0.57735f, 0.57735f, -8.66025f, 0.0f, 0.0f, 0.0f, 1.0f);
            TAssert(Mat4Eq(Mat4LookAt(eye3, target3, up3), expected3),
                    "Mat4LookAt Looking at the origin from (5, 5, 5) with a tilted up vector");
        }

        // Mat4Rotate x Mat4Multiply
        {
            f32 target = Deg2Rad(90.0f);
            Vec3 axis = Vec3Lit(0.0f, -1.0f, 0.0f);
            Mat4 orient = Mat4Identity();
            Mat4 expected = Mat4Lit(0.0f, 0.0f, -1.0f, 0.0f,  //
                                    0.0f, 1.0f, 0.0f, 0.0f,   //
                                    1.0f, 0.0f, 0.0f, 0.0f,   //
                                    0.0f, 0.0f, 0.0f, 1.0f);
            f32 step = target / 128.0f;
            f32 rotation = 0.0f;
            for (; rotation + step <= target; rotation += step) {
                Mat4 r = Mat4Rotate(step, axis);
                orient = Mat4Multiply(r, orient);
            }

            f32 rest = target - rotation;
            if (rest > 0.0f) {
                Mat4 r = Mat4Rotate(rest, axis);
                orient = Mat4Multiply(r, orient);
            }
            TAssert(Mat4Eq(orient, expected), "Integration Turn Left");
        }
    }

    // Test Mat2
    {
        {
            // Test uniform scaling.
            Mat4 m4 = Mat4Lit(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f,
                              15.0f, 16.0f);
            Mat2 m2 = Mat4ToMat2(m4);
            TAssert(Vec2Eq(Vec2Lit(1, 5), m2.v.a), "Mat4ToMat2");
            TAssert(Vec2Eq(Vec2Lit(2, 6), m2.v.b), "Mat4ToMat2");
        }
    }

    // Test Quat
    {
        // QuatFromAxisAngle
        {
            {
                Quat res = QuatFromAxisAngle(Vec3Lit(1.0f, 0.0f, 0.0f), Deg2Rad(90.0f));
                Quat expected = QuatLit(0.7071068f, 0.0f, 0.0f, 0.7071068f);
                TAssert(QuatEq(res, expected), "QuatFromAxisAngle 90 degrees about x axis");
            }
            {
                Quat res = QuatFromAxisAngle(Vec3Lit(0.0f, 1.0f, 0.0f), Deg2Rad(180.0f));
                Quat expected = QuatLit(0.0f, 1.0f, 0.0f, 0.0f);
                TAssert(QuatEq(res, expected), "QuatFromAxisAngle 180 degrees about y axis");
            }
            {
                Quat res = QuatFromAxisAngle(Vec3Lit(0.57735027f, 0.57735027f, 0.57735027f), Deg2Rad(45.0f));
                Quat expected = QuatLit(0.2214466f, 0.2214466f, 0.2214466f, 0.9238795f);
                TAssert(QuatEq(res, expected), "QuatFromAxisAngle 45 degrees about diagonal axis");
            }
        }

        // QuatLen
        {
            {
                f32 res = QuatLen(QuatLit(1.0f, 2.0f, 3.0f, 4.0f));
                TAssert(f32eq(res, 5.4772256f), "QuatLen");
            }
        }

        // QuatMult
        {
            {
                Quat q1 = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);        // Identity quaternion
                Quat q2 = QuatLit(0.7071f, 0.0f, 0.0f, 0.7071f);  // 90° rotation around X
                Quat res = QuatMult(q1, q2);
                TAssert(QuatEq(res, q2), "QuatMult with identity");
            }
            {
                Quat q1 = QuatLit(0.7071f, 0.0f, 0.0f, 0.7071f);  // 90° rotation around X
                Quat q2 = QuatLit(0.0f, 0.7071f, 0.0f, 0.7071f);  // 90° rotation around Y
                Quat res = QuatMult(q2, q1);
                Quat expected = QuatLit(0.5f, 0.5f, 0.5f, 0.5f);  // 120° rotation around (1,1,1)
                TAssert(QuatEq(res, expected), "QuatMult multiple rotations");
            }
            {
                Quat q1 = QuatLit(0.0f, 0.7071f, 0.0f, 0.7071f);   // 90° rotation around Y
                Quat q2 = QuatLit(0.0f, -0.7071f, 0.0f, 0.7071f);  // Inverse of q1
                Quat res = QuatMult(q2, q1);
                Quat expected = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);  // identity
                TAssert(QuatEq(res, expected), "QuatMult qq^-1 = Identity");
            }
            {
                Quat q1 = QuatLit(0.3f, 0.5f, 0.1f, 0.8f);
                Quat q2 = QuatLit(0.4f, -0.2f, 0.7f, 0.5f);
                Quat res = QuatMult(q2, q1);
                Quat expected = QuatLit(0.84f, -0.08f, 0.35f, 0.31f);
                TAssert(QuatEq(res, expected), "QuatMult non trivial");
            }
        }

        // QuatTransform
        {
            {
                Quat q = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);  // Identity quaternion
                Vec3 v = Vec3Lit(1.0f, 2.0f, 3.0f);
                Vec3 res = QuatTransform(q, v);
                Vec3 expected = Vec3Lit(1.0f, 2.0f, 3.0f);
                TAssert(Vec3Eq(res, expected), "QuatTransform identity");
            }
            {
                Quat q = QuatLit(0.0f, 0.0f, 0.7071f, 0.7071f);  // rotate 90 about z
                Vec3 v = Vec3Lit(1.0f, 0.0f, 0.0f);
                Vec3 res = QuatTransform(q, v);
                Vec3 expected = Vec3Lit(0.0f, 1.0f, 0.0f);
                TAssert(Vec3Eq(res, expected), "QuatTransform 90 deg about z axis");
            }
            {
                Quat q = QuatLit(0.0f, 0.3827f, 0.0f, 0.9239f);  // rotate about y axis by 45 deg
                Vec3 v = Vec3Lit(1.0f, 2.0f, 3.0f);
                Vec3 res = QuatTransform(q, v);
                Vec3 expected = Vec3Lit(2.8284f, 2.0f, 1.4142f);
                TAssert(Vec3Eq(res, expected), "QuatTransform 45 deg about y axis");
            }
        }

        // Quat2Mat4
        {
            {
                Quat q = QuatLit(0.0f, 0.0f, 0.0f, 1.0f);  // Identity quaternion
                Mat4 res = Quat2Mat4(q);
                Mat4 expected = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f);
                TAssert(Mat4Eq(res, expected), "Quat2Mat4 identity");
            }
            {
                Quat q = QuatLit(1.0f, 0.0f, 0.0f, 0.0f);  // 180 around x
                Mat4 res = Quat2Mat4(q);
                Mat4 expected = Mat4Lit(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
                                        0.0f, 0.0f, 1.0f);
                TAssert(Mat4Eq(res, expected), "Quat2Mat4 180 around x-axis");
            }
            {
                Quat q = QuatLit(0.2971f, 0.4952f, 0.1981f, 0.7924f);
                Mat4 res = Quat2Mat4(q);
                Mat4 expected = Mat4Lit(0.4312f, -0.0196f, 0.9022f, 0.0f, 0.6084f, 0.7450f, -0.2744f, 0.0f, -0.6666f,
                                        0.6668f, 0.3330f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
                TAssert(Mat4Eq(res, expected), "Quat2Mat4 complicated");
            }
        }
    }
}

static void TestMemory() {
    // NewArena
    {
        {
            byte buffer[1] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            TAssert(a.beg != NULL, "NewArena(1) beg not null");
            TAssert(a.end != a.beg, "NewArena(1) end != beg");
            TAssert(a.end == a.beg + 1, "NewArena(1) end = beg + 1");
            TAssert(a.end - a.beg == 1, "NewArena(1) end - beg == 1");
        }

        {
            byte buffer[1234] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            TAssert(a.end == a.beg + 1234, "NewArena(1234) end = beg + 1234");
            TAssert(a.end - a.beg == 1234, "NewArena(1234) end - beg == 1234");
        }
    }

    // New
    {
        {
            byte buffer[1] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            u8* begprev = a.beg;
            u8* b = New(&a, 1, u8);
            *b = 10;

            TAssert(b == begprev, "NewArena(1) + New(1, u8): b == begprev");
            TAssert(a.beg == a.end, "NewArena(1) + New(1, u8): beg == end");
            TAssert(*b == 10, "NewArena(1) + New(1, u8): *b == 10");
        }

        {
            byte buffer[10] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));
            u8* begprev = a.beg;
            u8* b = New(&a, 5, u8);
            b[0] = 10;
            b[1] = 20;
            b[2] = 30;
            b[3] = 40;
            b[4] = 50;
            TAssert(b == begprev, "NewArena(10) + New(5, u8): b == begprev");
            TAssert(a.beg != a.end, "NewArena(10) + New(5, u8): beg != end");
            TAssert(a.end - a.beg == 5, "NewArena(10) + New(5, u8): end - beg == 5");
            TAssert(b[0] == 10, "NewArena(10) + New(5, u8): b[0]");
            TAssert(b[1] == 20, "NewArena(10) + New(5, u8): b[1]");
            TAssert(b[2] == 30, "NewArena(10) + New(5, u8): b[2]");
            TAssert(b[3] == 40, "NewArena(10) + New(5, u8): b[3]");
            TAssert(b[4] == 50, "NewArena(10) + New(5, u8): b[4]");

            u8* begprev2 = a.beg;
            u8* b2 = New(&a, 5, u8);
            b2[0] = 60;
            b2[1] = 70;
            b2[2] = 80;
            b2[3] = 90;
            b2[4] = 100;
            TAssert(b2 == begprev2, "NewArena(10) + New(5, u8): b2 == begprev2");
            TAssert(a.beg == a.end, "NewArena(10) + New(5, u8): beg == end");
            TAssert(b[0] == 10, "NewArena(10) + New(5, u8) + New(5, u8): b[0]");
            TAssert(b[1] == 20, "NewArena(10) + New(5, u8) + New(5, u8): b[1]");
            TAssert(b[2] == 30, "NewArena(10) + New(5, u8) + New(5, u8): b[2]");
            TAssert(b[3] == 40, "NewArena(10) + New(5, u8) + New(5, u8): b[3]");
            TAssert(b[4] == 50, "NewArena(10) + New(5, u8) + New(5, u8): b[4]");
            TAssert(b2[0] == 60, "NewArena(10) + New(5, u8) + New(5, u8): b2[0]");
            TAssert(b2[1] == 70, "NewArena(10) + New(5, u8) + New(5, u8): b2[1]");
            TAssert(b2[2] == 80, "NewArena(10) + New(5, u8) + New(5, u8): b2[2]");
            TAssert(b2[3] == 90, "NewArena(10) + New(5, u8) + New(5, u8): b2[3]");
            TAssert(b2[4] == 100, "NewArena(10) + New(5, u8) + New(5, u8): b2[4]");
        }
    }

    // PushS
    {
        {
            byte buffer[100] = {0};
            Arena a = NewArena(buffer, ArraySizeOf(buffer));

            struct {
                i32* data;
                isize len;
                isize cap;
            } ii = {0};
            i32* data = New(&a, 5, i32);
            InitArrS(&ii, data, 5);
            TAssert(ii.len == 0, "PushS(int): len == 0");

            *PushS(&ii) = 0;
            *PushS(&ii) = 10;
            *PushS(&ii) = 20;
            *PushS(&ii) = 30;
            *PushS(&ii) = 40;
            for (i32 i = 0; i < ii.len; ++i) {
                TAssert(ii.data[i] == i * 10, "PushS: data matches");
            }
            TAssert(ii.len == 5, "PushS(int): len == 5");
            TAssert(ii.cap == 5, "PushS(int): cap == 5");
        }
    }
}

static void TestRand() {
    // f32rand01
    {
        for (i32 i = 0; i < 1000; ++i) {
            f32 r = f32rand01();
            TAssert(r >= 0.0f && r <= 1.0f, "0.0f <= f32rand01() <= 1.0f");
        }
    }

    // f32randab
    {
        // no diff.
        {
            for (i32 i = 0; i < 1000; ++i) {
                f32 r = f32randab(1000.0f, 1000.0f);
                TAssert(f32eq(r, 1000.0f), "f32randab(1000, 1000) ~= 1000f");
            }
        }

        // small diff.
        {
            for (i32 i = 0; i < 1000; ++i) {
                f32 r = f32randab(100.0f, 101.0f);
                TAssert(r >= 100.0f && r <= 101.0f, "100.0f <= f32randab(100, 101) <= 101f");
            }
        }
    }
}

static void TestL() {
    PlatformLog(LLOG_INFO, "Running Tests");
    TestMath();
    TestMemory();
    TestRand();

    if (TestFail) {
        PlatformLog(LLOG_ERROR, "Failed tests!");
        exit(1);
    }

    PlatformLog(LLOG_INFO, "Tests Pass");
}
