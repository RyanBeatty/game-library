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

typedef enum TextureLabel { TX_DEFAULT = 0, TX_MAIL, TX_MAILBOX, TX_COUNT } TextureLabel;

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

    {
        tcache->textures[TX_MAIL] = TextureFromFile("assets/textures/mail.png", FILTER_NEAREST);
        ++tcache->txlen;
    }

    {
        tcache->textures[TX_MAILBOX] = TextureFromFile("assets/textures/mailbox.png", FILTER_NEAREST);
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

typedef enum {
    CMD_NONE = 0,
    CMD_DRAW_MESH,
    CMD_CLEAR,
    CMD_COUNT,
} DrawCmdType;

typedef struct {
    DrawCmdType type;

    union {
        // Draws a mesh.
        struct {
            LMesh meshid;
            Mat4 modelm;  // converts from model space to world space.
            Vec4 color;
            Vec2 uvoffset;  // offset of the bottom left corner/origin in texture space (0, 0)
            Vec2 uvscale;   // width/height in texture space of the Aserect to sample inside the texture [0, 1]
            LTexture tx;
        } meshcmd;

        // Clears buffer with color.
        struct {
            Vec4 color;
        } clearcmd;
    };

    // Pass arbitrary shader data (ex. like new uniforms to set) via this member.
    // Mostly exists as convenience when iterating on new shader ideas.
    void* extra;
} DrawCmd;

// Assumes the struct has been memset already.
LCORE void DrawMeshCmdInit(DrawCmd* cmd) {
    cmd->type = CMD_DRAW_MESH;
    cmd->meshcmd.color = LCLEAR;
    cmd->meshcmd.modelm = Mat4Identity();
    cmd->meshcmd.uvoffset = Vec2Lit(0.0f, 0.0f);
    cmd->meshcmd.uvscale = Vec2Lit(1.0f, 1.0f);
    cmd->meshcmd.tx = GetTexture(TX_DEFAULT);
}

#define GLSL(src) "#version 330 core\n" #src

typedef u32 ShaderId;

// Forward decls to be able to define enable shader func. This is somewhat akward, maybe consider
// trying to do what the File Pilot guy does and auto gen header files. That might be work tho.
struct ShaderPass;
typedef struct ShaderPass ShaderPass;
struct LShader;
typedef struct LShader LShader;

typedef void (*EnableShaderFunc)(LShader*, ShaderPass*);
typedef void (*SetUniformsFunc)(LShader*, DrawCmd*);

struct LShader {
    ShaderId pid;  // program id.
    u32 vid;
    u32 fid;

    EnableShaderFunc enable_func;
    SetUniformsFunc set_uniforms_func;
};

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

// No typedef because it is forward declared above.
struct ShaderPass {
    ShaderId _sid;
    LShader shader;

    ///////////////////////
    // Uniform data that should be set once for a pass here.
    Mat4 view;
    Mat4 proj;
    ///////////////////////

    Vec2 resolution;
    f32 dt;
    f32 timer;
    i32 nframe;

    u32 clen;
    u32 ccap;
    DrawCmd cmds[MAX_CMDS];
};

LCORE void EnableDefaultShader(LShader* shader, ShaderPass* pass) {
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

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&pass->resolution.f);
    glUniform1f(F_TIMER, pass->timer);

    glUniform1i(F_NFRAME, pass->nframe);
}

LCORE void DefaultShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, cmd->meshcmd.tx.width, cmd->meshcmd.tx.height);
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

LCORE void EnableLineShader(LShader* shader, ShaderPass* pass) {
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

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&pass->resolution.f);
    glUniform1f(F_TIMER, pass->timer);

    glUniform1i(F_NFRAME, pass->nframe);
}

LCORE void LineShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, cmd->meshcmd.tx.width, cmd->meshcmd.tx.height);
}

LCORE LShader GetLineShader() {
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
        out vec2 fPosLocal;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
           fPosLocal = vPos.xy;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor;
        in vec2 fTextCoords;
        in vec2 fPosLocal;

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

            float alpha_mask = 1.0;

            // {
            //     float u = fTextCoords.x;
            //     // aa radius. This represents the radius of a circle that determines how much alpha falloff
            //     // the line pixels experience as our u coordinate goes from inside the line to outside the line.
            //     // we are fully within the line in the range [r, 1-r] and outside the line outside of that range.
            //     // Note that this assumes that the drawn line quad is oriented such that the u coordinate of each
            //     // fragment lies along the width axis of the quad.
            //     float r = 0.3;
            //     // Construct a piecewise smoothing function that transitions smoothly from [0, 1] between the
            //     // domain of [0, r], stays at 1 from [r, 1-r] and transitions smoothly decreasing from [1, 0]
            //     // in the domain [1-r, r].
            //     alpha_mask = smoothstep(0.0, 1.0, u / r) - 
            //                     smoothstep(0.0, 1.0, (max(u - 1 + r, 0.0)) / r);
            // }

            {
                // Calculates AA in local space of the quad model. Assumes the quad has width of 0.5 in local space
                // and that we are using an orthographic projection (I think due to perspective correct interpolation of 
                // vertex attribute concerns.)
                float x = abs(fPosLocal.x);  // pass in our vertex position in local space.
                float edge = 0.5;            // Assume the quad model has edge vertex of 0.5
                float w = 2.0 * fwidth(x);         // Get size of 1 pixel in screen space
                alpha_mask *= 1.0 - smoothstep(edge - w, edge, x);  // Apply alpha blending of 1 pixel width at edge.
            
                // Do the same for AA the end edges of the line. Note that we use y for fwidth
                // which I'm not 100% sure is correct.
                float y = abs(fPosLocal.y);
                float h = fwidth(y);  // Get ~size of 1 pixel in screen space in the y direction.
                alpha_mask *= 1.0 - smoothstep(edge - h, edge, y);
            }

            FragColor.a *= alpha_mask;
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableLineShader;
        SHADER.set_uniforms_func = LineShaderSetUniforms;
    }

    return SHADER;
}

LCORE void EnableNoiseShader(LShader* shader, ShaderPass* pass) {
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

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);

    glUniform2fv(F_RESOLUTION, 1, (const f32*)&pass->resolution.f);
    glUniform1f(F_TIMER, pass->timer);

    glUniform1i(F_NFRAME, pass->nframe);
}

LCORE void NoiseShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;
    static i32 F_TEXTURE0_RESOLUTION = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
        F_TEXTURE0_RESOLUTION = glGetUniformLocation(shader->pid, "Texture0Resolution");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
    glUniform2f(F_TEXTURE0_RESOLUTION, cmd->meshcmd.tx.width, cmd->meshcmd.tx.height);
}

LCORE LShader GetNoiseShader() {
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
        out vec3 fWorldPos;

        void main() {
           fTextCoords = vUVOffset + (vUVScale * vTextCoords);
           gl_Position = vProj * vView * vModel * vec4(vPos.x, vPos.y, vPos.z, 1.0);
           fColor = vUColor;
           fWorldPos = (vModel * vec4(fWorldPos, 1.0)).xyz;
        }
    );
    // clang-format on

    // clang-format off
    static const char* FSRC = GLSL(
        in vec4 fColor; 
        in vec2 fTextCoords;
        in vec3 fWorldPos;

        uniform sampler2D texture0;
        uniform vec2 Texture0Resolution;
        uniform vec2 resolution;
        uniform float timer;
        uniform int NFrame;

        out vec4 FragColor;

        // https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
        uint pcg_hash(uint x) {
            uint state = x * 747796405u + 2891336453u;
            uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
            return (word >> 22u) ^ word;
        }

        float uv2whitenoise(vec2 uv, ivec2 dims) {
            ivec2 texel = ivec2(floor(uv * dims));
            uint seed = uint((texel.x * dims.y) + texel.y);
            return float(pcg_hash(seed)) * (1.0 / 4294967295.0);
        }

        float whitenoise(vec3 value) {
            // Produce a random value by dotting it with a random direction vector.
            // The resulting random value will be how close our input vector is to the direciton vector
            // multiplied by magnitudes.
            float random = dot(value, vec3(2.971, 49.149, 0));
            // Random could be a large number since our vectors are assumed to be unit vectors
            // clamp to -1 to 1 range with sin function. This also gives our random value a wave like
            // property. then to avoid tiling, multiply with a very large random number to essentially
            // convert our small number to range [0, large number] which should eliminate tiling.
            return fract(sin(random) * 143758.5453);
        }

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
            float noise = whitenoise(vec3(fTextCoords, 1.0));
            // Remember cant use texture dims as some textures are 1 pixel.
            float noisehash = uv2whitenoise(fTextCoords, ivec2(10000, 10000));
            FragColor = vec4(vec3(noisehash), 1.0);
            if (noisehash == 0.0) {
                FragColor = vec4(1, 0, 0, 1);
            }
        }
    );
    // clang-format on

    static bool INIT = false;
    static LShader SHADER = {0};

    if (!INIT) {
        INIT = true;
        LShaderInit(&SHADER, VSRC, FSRC);
        SHADER.enable_func = EnableNoiseShader;
        SHADER.set_uniforms_func = NoiseShaderSetUniforms;
    }

    return SHADER;
}

LCORE void EnableEffectsShader(LShader* shader, ShaderPass* pass) {
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

    glUniformMatrix4fv(V_VIEW, 1, GL_FALSE, (const f32*)&pass->view.m);
    glUniformMatrix4fv(V_PROJ, 1, GL_FALSE, (const f32*)&pass->proj.m);
}

LCORE void EffectsShaderSetUniforms(LShader* shader, DrawCmd* cmd) {
    static bool INIT = false;
    static i32 V_MODEL = 0;
    static i32 V_UVCOLOR = 0;
    static i32 V_UVSCALE = 0;
    static i32 V_UVOFFSET = 0;

    Assert(cmd->type == CMD_DRAW_MESH);
    if (!INIT) {
        INIT = true;

        V_MODEL = glGetUniformLocation(shader->pid, "vModel");
        V_UVCOLOR = glGetUniformLocation(shader->pid, "vUColor");
        V_UVOFFSET = glGetUniformLocation(shader->pid, "vUVOffset");
        V_UVSCALE = glGetUniformLocation(shader->pid, "vUVScale");
    }

    glUniformMatrix4fv(V_MODEL, 1, GL_FALSE, (const f32*)&cmd->meshcmd.modelm.m);
    glUniform4fv(V_UVCOLOR, 1, (const f32*)&cmd->meshcmd.color.f);
    glUniform2fv(V_UVOFFSET, 1, (const f32*)&cmd->meshcmd.uvoffset.f);
    glUniform2fv(V_UVSCALE, 1, (const f32*)&cmd->meshcmd.uvscale.f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd->meshcmd.tx.id);
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

static void ShaderPassInit(ShaderPass* pass, LShader shader) {
    memset(pass, 0, sizeof(ShaderPass));
    pass->shader = shader;
    pass->view = Mat4Identity();
    pass->proj = Mat4Identity();

    pass->clen = 0;
    pass->ccap = MAX_CMDS;
}

static DrawCmd* ShaderPassAddCmd(ShaderPass* pass) {
    Assert(pass->clen < pass->ccap);
    DrawCmd* cmd = &pass->cmds[pass->clen++];
    memset(cmd, 0, sizeof(DrawCmd));
    return cmd;
}

static void ShaderPassExecute(ShaderPass* pass) {
    LShader shader = pass->shader;
    Assert(shader.enable_func != NULL);
    Assert(shader.set_uniforms_func != NULL);

    shader.enable_func(&shader, pass);

    for (u32 i = 0; i < pass->clen; ++i) {
        DrawCmd* cmd = &pass->cmds[i];
        switch (cmd->type) {
            case CMD_DRAW_MESH: {
                shader.set_uniforms_func(&shader, cmd);

                glDrawElements(GL_TRIANGLES, cmd->meshcmd.meshid.ntriangles * 3, GL_UNSIGNED_INT,
                               (void*)(cmd->meshcmd.meshid.ibufferidx * sizeof(GLuint)));
                break;
            };

            case CMD_CLEAR: {
                Vec4 color = cmd->clearcmd.color;
                glClearColor(color.r, color.g, color.b, color.a);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                break;
            }

            default: {
                Assert(false);
                break;
            };
        }
    }
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

    RenderTarget target;
} RenderState;

static RenderState RSTATE;
#define GetRenderState() (&RSTATE)

// #define CurShaderPass() (&RSTATE.passes[RSTATE.passidx])
static ShaderPass* CurShaderPass() {
    Assert(RSTATE.passidx != PASSNULL);
    return &RSTATE.passes[RSTATE.passidx];
}

static void RenderStateInit(u32 width, u32 height) {
    RenderState* rstate = &RSTATE;

    memset(rstate, 0, sizeof(RenderTarget));
    rstate->target = CreateRenderTarget(width, height);
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
}

static ShaderPass* BeginShaderPass(LShader shader) {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen < rstate->passcap);
    Assert(rstate->passidx == PASSNULL);

    rstate->passidx = rstate->passlen++;
    ShaderPass* pass = &rstate->passes[rstate->passidx];
    ShaderPassInit(pass, shader);
    pass->resolution = Vec2Lit(rstate->target.width, rstate->target.height);
    pass->dt = rstate->dt;
    pass->timer = rstate->timer;
    pass->nframe = rstate->nframe;
    return pass;
}

static void EndShaderPass() {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passidx != PASSNULL);
    rstate->passidx = PASSNULL;
}

// Execute shader passes and produce render to screen.
static void EndRender() {
    RenderState* rstate = &RSTATE;

    Assert(rstate->passidx == PASSNULL);

    // Begin.
    {
        // Render draw commands to render target.
        glBindFramebuffer(GL_FRAMEBUFFER, rstate->target.fbid);
        // IMPORTANT.
        glViewport(0, 0, rstate->target.width, rstate->target.height);
        // I forget exactly why we want depth testing. I think maybe has to do with not drawing portions of
        // sprites that are blank? maybe thats just alpha blending.
        glEnable(GL_DEPTH_TEST);
        // Pass depth test if incomming_depth <= stored_depth. This is necessary to ensure we still draw
        // sprites which have the same z value which so far seems like a desirable thing.
        glDepthFunc(GL_LEQUAL);

        // TODO: turn this into a draw command using unions.
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // TODO: move this to shader pass data.
        glBindVertexArray(VAO);
    }

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

    for (u32 i = 0; i < rstate->passlen; ++i) {
        ShaderPassExecute(&rstate->passes[i]);
    }

    // Draw the render target texture to screen as a quad.
    // TODO: This is very messy. I should be able to do this using draw commands to change framebuffers.
    // This would require an inversion of how framebuffers work though. They would need to also contain shader
    // pass commands. Or maybe I should just have every shader pass store what framebuffer they are drawing to
    // But this would require some duplication in code. If I have two shaders that process draw commands
    // I'd need to duplicate ClearBuffer() draw command logic.
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // IMPORTANT.
        glViewport(0, 0, PlatformScreenWidth(), PlatformScreenHeight());
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ShaderPass pass;
        ShaderPassInit(&pass, GetEffectsShader());
        pass.dt = rstate->dt;
        pass.timer = rstate->timer;
        pass.resolution = Vec2Lit(PlatformScreenWidth(), PlatformScreenHeight());
        DrawCmd* cmd = ShaderPassAddCmd(&pass);
        DrawMeshCmdInit(cmd);
        cmd->meshcmd.meshid = GetMesh(M_QUAD);
        // Since we only set a scale matrix, essentially we are specifying this quad in clip space. since the quad
        // is from -0.5, 0.5, we mult by 2 to get -1, 1 so we cover the entire screen.
        cmd->meshcmd.modelm = Mat4ScaleV(Vec3Lit(2.0f, 2.0f, 1.0f));
        // cmd->meshcmd.color = LRED;  // For debug.
        cmd->meshcmd.tx.id = rstate->target.txid;
        cmd->meshcmd.tx.width = rstate->target.width;
        cmd->meshcmd.tx.height = rstate->target.height;
        ShaderPassExecute(&pass);
    }
}

static DrawCmd* AddCmd() {
    RenderState* rstate = &RSTATE;
    Assert(rstate->passlen != 0);
    DrawCmd* cmd = ShaderPassAddCmd(CurShaderPass());
    return cmd;
}

static DrawCmd* AddDrawMeshCmd() {
    DrawCmd* cmd = AddCmd();
    DrawMeshCmdInit(cmd);
    return cmd;
}

LCORE DrawCmd* ClearBuffer(Vec4 color) {
    DrawCmd* cmd = AddCmd();
    cmd->type = CMD_CLEAR;
    cmd->clearcmd.color = color;
    return cmd;
}

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

LCORE void DrawAABB(Vec3 center, Vec3 radius, Vec4 color) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_CUBE);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Scale(radius, 2.0f), Mat4Identity(), center);
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

// This will return the dimensions of the quad that would be necessary to span the entire text (i.e. if somehow
// the text was baked into a texture, what quad dims would be needed to draw the texture).
LCORE Vec2 LFontMeasureText(LFont font, char* text, i32 len) {
    Vec2 tl = Vec2Zero();
    Vec2 br = Vec2Zero();

    Vec2 baseline_pos = Vec2Zero();

    for (i32 i = 0; i < len; ++i) {
        stbtt_aligned_quad quad = {0};
        // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
                           &quad, true);

        tl.x = f32min(tl.x, quad.x0);
        tl.y = f32min(tl.y, quad.y0);

        br.x = f32max(br.x, quad.x1);
        br.y = f32max(br.y, quad.y1);
    }

    Vec2 dims = Vec2Sub(br, tl);
    return dims;
}

LCORE Vec2 LFontMeasureStr8(LFont font, Str8 str) { return LFontMeasureText(font, str.data, str.len); }

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
    SOUND_SORT,
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
    // https://sfxr.me/#57uBnWaP2P9Tqnb9U7C2ZWuqF7Yn8sRtHgc3tYYGh7QCv86Wu2sF1kFkPh5bvtBQ9iaPWGSLgdqsvbkvpHCN1wscN8HpzxFZ1iBxiQhoGz63dAo4uDr85wAZm
    SOUNDS[SOUND_SORT] = PlatformLoadSound("assets/sounds/sort.wav");
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

LCORE Particle* SpawnParticle(Vec3 p, Vec2 dims, f32 lifetime, RGBA color) {
    Assert(lifetime > 0.0f);
    Particle* particle = AllocateParticle();
    particle->p = p;
    particle->scale = Vec3Lit(dims.x, dims.y, 1.0f);
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
        // particle->color.a = f32lerp(1.0f, 0.1f, particle->lifedt / particle->lifetime);

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
    return LModelFromAnimation(AnimationFromSprite(sprite), pos, orient, scale, tint);
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

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawLModel(LModel model, Vec3 pos, Mat4 orient, Vec3 scale) {
    return DrawLModelEx(model, 0, pos, orient, scale);
}

typedef enum {
    ET_NONE = 0,  // No easing, track the target value exactly.
    ET_LERP       // Uses exponential damped lerp: https://www.youtube.com/watch?v=YJB1QnEmlTs
} EasingType;

typedef struct {
    EasingType type;

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

static void EasingSetLerp(Easing* easing, f32 rate) {
    easing->type = ET_LERP;
    easing->lerp.rate = rate;
}

static Vec3 EaseVec3(Vec3 cur, Vec3 target, f32 dt, Easing easing) {
    switch (easing.type) {
        case ET_NONE: {
            cur = target;
            break;
        }
        case ET_LERP: {
            f32 T = 1.0f - f32pow(easing.lerp.rate, dt);
            cur = Vec3Lerp(cur, target, T);
            break;
        }
        default: {
            Assert(0);
        }
    }
    return cur;
}

typedef struct {
    Vec3 pos;
    Vec3 scale;
    Mat4 orient;

    Easing pos_easing;
    Easing scale_easing;

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

// LCORE void GraphicsInfoSetShape(GraphicsInfo* graphics, LMesh mesh, Vec3 scale, Vec4 color) {
//     graphics->mid = mesh;
//     graphics->scale = scale;
//     graphics->txinfo.tint = color;
// }

////////////////////////////////////////////////////////////

typedef enum { HAMPER_NONE = 0, HAMPER_LEFT, HAMPER_UP, HAMPER_RIGHT, HAMPER_COUNT } HamperType;

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

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.

    EntityId next_mail;
    EntityId next_worker;

    f32 work_cooldown_dt;
    f32 work_cooldown_sec;
} Entity;

#define ENTITY_CAP 1024
#define ENTITY_INACTIVE_TO_BE_CAP 20

typedef struct {
    struct {
        Entity data[ENTITY_CAP];
        isize len;
        isize cap;
    } entities;
} EntityManager;

static EntityManager EntityM = {0};

#define GetEntityManager() &EntityM

#define GetNumEntities() (EntityM.cap)

// TODO: Move these
#define EntitySetMass(e, m) ((e)->mass_inv = (m) != 0.0f ? 1.0f / (m) : 0.0f)
#define EntityGetMass(e) ((e)->mass_inv != 0.0f ? 1.0f / (e)->mass_inv : 0.0f)
#define EntityGetMassInv(e) ((e)->mass_inv)

static void EntitySystemInit() {
    EntityManager* em = &EntityM;
    memset(em, 0, sizeof(*em));
    em->entities.cap = ArraySizeOf(em->entities.data);
    em->entities.len = em->entities.cap;

    for (isize i = 0; i < ArrLen(&em->entities); ++i) {
        Entity* entity = ArrGetPtr(&em->entities, i);
        entity->id.idx = i;
        // Set generation to be 1 so that we don't accidentally reference some entity
        // with default value of 0.
        entity->id.generation = 1;
    }
}

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

LCORE Entity* AllocateEntity() {
    Entity* entity = NextFreeEntity();
    Assert(entity != NULL);
    Entity zero = {.id = entity->id};
    *entity = zero;
    entity->flags = EFLAG_ACTIVE;
    entity->scale = Vec3Lit(1.0f, 1.0f, 1.0f);
    entity->orient = Mat4Identity();

    // Physics stuff.
    {
        // entity->v = Vec3Zero();
        EntitySetMass(entity, 0.0f);  // Set infinite mass
    }

    PolyhedronInit(&entity->shape);
    GraphicsInfoMemset(&entity->graphics);
    FlagOn(entity->flags, EFLAG_DRAW);
    return entity;
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

#define Grid2WorldEx(pos, w, h, d) (Vec3Lit((pos).x * (w), (pos).y * (h), (pos).z * (d)))

static const f32 GRID_WIDTH = 16.0f;
static const f32 GRID_HEIGHT = 16.0f;
static const f32 GRID_DEPTH = 1.0f;
#define Grid2World(pos) (Grid2WorldEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))

#define World2GridEx(pos, w, h, d) (Vec3Lit((u32)((pos).x / (w)), (u32)((pos).y / (h)), (u32)((pos).z) / (d)))
#define World2Grid(pos) (World2GridEx(pos, GRID_WIDTH, GRID_HEIGHT, GRID_DEPTH))
////////////////////////////////////////////////////

typedef enum {
    PHASE_NONE = 0,
    PHASE_EOD,
    PHASE_PLAYING,
    PHASE_GAMEOVER,
    PHASE_SHOP_OPEN,
} GamePhase;

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef enum {
    SCENE_DEFAULT = 0,
    SCENE_MAIN,
    SCENE_DEFAULT_3D,
    SCENE_COUNT,
} SceneId;

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

    f32 dt;  // frame delta time.
    f32 timer;
    u32 nframe;            // num frame we are on.
    f32 frame_times[256];  // The incremental dts of each frame produced. its a ring buffer. used for fps count.
    i32 fts_tail;
    i32 fts_head;
    u32 fps;
    i32 target_fps;        // render fps we are targetting.
    f32 fixed_dt;          // fixed dt we will update the simulation with for stability.
    f32 accumulated_time;  // Time we have yet to simulate.

    void (*scenefuncs[MAX_SCENES])(struct GameState*, GameInput* input);
    u32 scenefuncssize;
    SceneId sceneid;
    SceneId nextsceneid;

    LCamera camera;

    /////////////////////////////////////////
    // Scene fields

    f32 score;

    GamePhase phase;

    i32 worker_count;
    f32 worker_cost;

    f32 worker_cooldown_sec;
    f32 worker_cooldown_sec_cost;

    EntityId mail_root;

    i32 day;
    i32 num_sorted;
    i32 target_score;

    EntityId worker_root;

    bool shop_open;
} GameState;

static GameState GState;

#define GetGameState() (&GState)

typedef void (*SceneTickFunc)(GameState* gstate);

i32 GameGetTargetFps() { return GetGameState()->target_fps; }

// Converts a screen space position (assumes origin bottom left) to a 2d position in world space.
Vec2 GameScreenToWorld2D(Vec2 pos_screen) {
    GameState* gstate = GetGameState();
    RenderState* rstate = GetRenderState();

    Vec2 viewport_dims = Vec2Lit(rstate->target.width, rstate->target.height);
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

    // PlatformLog(LLOG_INFO, "Pos world %f %f %f", pos_world.x, pos_world.y, pos_world.z);
    return Vec3ToVec2(pos_world);
}

// Resets global state/system state that contains per scene info. Experimental approach
// for use in scene switching as opposed to putting all this state into GameState object
// and passing that around. The globalness of these systems is kind of convenient at the
// moment.
static void GameStateLoadScene(GameState* gstate, SceneId sceneid) {
    Assert(sceneid < MAX_SCENES);

    // TODO: Big todo here. Need to come up with a more elegant way to load/unload scene data.
    // Luckily for megaman I don't think there will be entities that persist across scenes.
    EntitySystemInit();

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

typedef enum {
    SS_NONE = 0,
    SS_COUNT,
} SceneState;

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

static void SceneDefaultTick(GameState* gstate, GameInput* input) {
    f32 dt = gstate->dt;

    if (gstate->init) {
    }

    if (gstate->update) {
    }

    if (gstate->render) {
        BeginRender(dt);

        ShaderPass* pass = BeginShaderPass(GetDefaultShader());
        // Reminder. this will draw stuff in screen space. if you want to draw with pixel scaling using PPM, then use
        // _world sizes from gstate.
        f32 width = gstate->renderw;
        f32 height = gstate->renderh;
        pass->proj = Mat4Ortho(-0.5f * width, 0.5f * width, -0.5f * height, 0.5f * height, -1.0f, 1.0f);

        char msg[] = "DEFAULT SCENE";
        u32 size = strlen(msg);

        LFont font = GetLFontClosest(20.0f);
        DrawLTextEx(font, msg, size, Vec2Zero(), LBLACK);

        EndShaderPass();

        EndRender();
    }
}

static void SceneDefault3DTick(GameState* gstate, GameInput* input) {
    f32 dt = gstate->dt;

    if (gstate->init) {
        gstate->camera.eye = Vec3Lit(50.0f, 50.0f, 50.0f);
        gstate->camera.target = Vec3Zero();
        gstate->camera.up = Vec3Lit(0.0f, 1.0f, 0.0f);
    }

    if (gstate->update) {
    }

    if (gstate->render) {
        BeginRender(dt);

        ShaderPass* pass = BeginShaderPass(GetDefaultShader());
        // Reminder. this will draw stuff in screen space. if you want to draw with pixel scaling using PPM, then use
        // _world sizes from gstate.
        f32 width = gstate->renderw;
        f32 height = gstate->renderh;
        pass->proj = Mat4Ortho(-0.5f * width, 0.5f * width, -0.5f * height, 0.5f * height, -1.0f, 1.0f);

        pass->view = CameraGetViewMat(gstate->camera);
        pass->proj = Mat4Perspective(Deg2Rad(60.0f), (f32)gstate->renderw / (f32)gstate->renderh, 0.1f, 100.0f);

        // DrawQuad(Vec3Zero(), 50, 50, Mat4Identity(), LRED);

        // DrawLSphere(Vec3Zero(), 5.0f, LBLUE);

        // DrawMesh(GetMesh(M_HELLO_CUBE), Vec3Lit(0.0f, 0.0f, 0.0f), Mat4Identity(), Vec3Lit(5, 5, 5), LBLUE);
        DrawMesh(GetMesh(M_HELLO_MONKEY), Vec3Lit(0.0f, 0.0f, 0.0f), Mat4Identity(), Vec3Lit(5, 5, 5), LBLUE);

        EndShaderPass();

        EndRender();
    }
}

// 1 meter = N pixels scale factor.
#define PPM (1.0f)
#define MPP (1.0f / PPM)
#define Meter2Px(m) ((m) * PPM)
#define Px2Meter(px) ((px) * MPP)

static void EntityInitWorker(Entity* e, Vec3 pos, f32 work_cooldown_sec, EntityId next_worker) {
    Vec3 hamper_dims_world = Vec3Lit(Px2Meter(216.0f), Px2Meter(216.0f), 0.0f);

    e->pos = pos;
    PolyHedronSetAABB2DFromWH(&e->shape, Vec2Zero(), Vec2Lit(Px2Meter(10.0f), Px2Meter(10.0f)));
    GraphicsInfoSetLModel(&e->graphics, LModelFromMesh(GetMesh(M_QUAD), GetTexture(TX_MAILBOX), Vec3Zero(),
                                                       Mat4Identity(), Vec3Scale(hamper_dims_world, 2.0f), LCLEAR));
    FlagOn(e->flags, EFLAG_WORKER);
    e->work_cooldown_dt = 0.0f;
    e->work_cooldown_sec = work_cooldown_sec;
    e->next_worker = next_worker;
}

static void SpawnWorker(GameState* gstate, Vec3 pos) {
    Entity* e = AllocateEntity();
    EntityInitWorker(e, pos, gstate->worker_cooldown_sec, gstate->worker_root);
    gstate->worker_root = e->id;
}

static void StartNewDay(GameState* gstate) {
    EntitySystemInit();

    gstate->shop_open = false;
    ++gstate->day;
    gstate->num_sorted = 0;

    gstate->target_score = 1000;

    f32 dt = gstate->dt;

    Vec3 hamper_origin_world = Vec3Lit(Px2Meter(0.0f), Px2Meter(-150.0f), Px2Meter(0.0f));
    Vec3 hamper_dims_world = Vec3Lit(Px2Meter(216.0f), Px2Meter(216.0f), 0.0f);
    Mat4 hamper_to_world =
        Mat4Multiply(Mat4TranslateV(hamper_origin_world), Mat4ScaleV(Vec3Scale(hamper_dims_world, 2.0f)));

    // Spawn mail stack
    {
        gstate->mail_root = NULLID;
        for (i32 i = 0; i < gstate->target_score; ++i) {
            Entity* e = AllocateEntity();
            e->pos = Vec3WTransform(Vec3Lit(0.0f, -0.7f, 0.0f), hamper_to_world);
            // Add a random offset so that it more visually looks like a pile.
            Vec3 offset = Vec3RotateAxis(Vec3Lit(Px2Meter(10.0f) * f32rand01(), 0.0f, 0.0f), Vec3Lit(0.0f, 0.0f, 1.0f),
                                         f32randab(Deg2Rad(0.0f), Deg2Rad(360.0f)));
            e->pos = Vec3Add(e->pos, offset);
            PolyHedronSetAABB2DFromWH(&e->shape, Vec2Zero(), Vec2Lit(Px2Meter(134), Px2Meter(90)));
            GraphicsInfoSetLModel(&e->graphics, LModelFromMesh(GetMesh(M_QUAD), GetTexture(TX_MAIL), Vec3Zero(),
                                                               Mat4Identity(), hamper_dims_world, LCLEAR));
            e->next_mail = gstate->mail_root;
            gstate->mail_root = e->id;
        }
    }

    // Spawn workers
    for (isize i = 0; i < gstate->worker_count; ++i) {
        Vec3 worker_pos = Vec3Lit(i, 0, 0);
        Vec3 pos = Vec3WTransform(worker_pos, hamper_to_world);
        SpawnWorker(gstate, pos);
    }

    // Important or the first couple frames our entities will teleport from origin to starting positions.
    for (Entity* e = NULL; IncrementEntity(&e);) {
        if (TestFlag(e->flags, EFLAG_DRAW)) {
            e->graphics.pos = e->pos;
            e->graphics.orient = e->orient;
            e->graphics.scale = e->scale;
            GraphicsInfoTickAnimation(&e->graphics, dt);
        }
    }

    gstate->phase = PHASE_PLAYING;
}

static void SceneMainTick(GameState* gstate, GameInput* input) {
    f32 dt = gstate->dt;

    if (gstate->init) {
        gstate->score = 0.0f;
        gstate->worker_count = 1;
        gstate->worker_cost = 4.0f;
        gstate->day = -1;
        gstate->worker_cooldown_sec = 1.0f;
        gstate->worker_cooldown_sec_cost = 8.0f;
        gstate->num_sorted = 0;

        StartNewDay(gstate);
        // Sort of hacky but lets start with eod.
        // gstate->phase = PHASE_EOD;

        gstate->camera = (LCamera){.eye = Vec3Lit(Px2Meter(0.0f), Px2Meter(0.0f), Px2Meter(20.0f)),
                                   .target = Vec3Zero(),
                                   .up = Vec3Lit(0.0f, 1.0f, 0.0f)};
    }

    if (gstate->update) {
        // GameControllerState* controller = &input->controllers[0];

        switch (gstate->phase) {
            case PHASE_PLAYING: {
                // Relayout workers
                {
                    Vec3 hamper_origin_world = Vec3Lit(Px2Meter(0.0f), Px2Meter(-150.0f), Px2Meter(0.0f));
                    Vec3 hamper_step_world = Vec3Lit(Px2Meter(216.0f), Px2Meter(216.0f), 0.0f);
                    Mat4 hamper_to_world = Mat4Multiply(Mat4TranslateV(hamper_origin_world),
                                                        Mat4ScaleV(Vec3Scale(hamper_step_world, 2.0f)));

                    i32 NUM_COLS = 3;
                    i32 i = 0;
                    for (Entity* e = EntityFromId(gstate->worker_root); e != NULL;
                         e = EntityFromId(e->next_worker), ++i) {
                        i32 row = i / NUM_COLS;
                        i32 col = i % NUM_COLS;
                        Vec3 worker_pos = Vec3Lit(col, row, 0);
                        e->pos = Vec3WTransform(worker_pos, hamper_to_world);
                        EasingSetLerp(&e->graphics.pos_easing, 0.1f);
                    }
                }

                for (Entity* e = NULL; IncrementEntity(&e);) {
                    if (TestFlag(e->flags, EFLAG_WORKER)) {
                        e->work_cooldown_dt -= dt;
                        e->work_cooldown_dt = f32max(e->work_cooldown_dt, 0.0f);

                        if (EntityIdNotNull(gstate->mail_root)) {
                            if (e->work_cooldown_dt <= 0.0f) {
                                e->work_cooldown_dt = e->work_cooldown_sec;

                                Entity* mail = EntityFromId(gstate->mail_root);
                                Assert(mail != NULL);

                                PlatformPlaySound(GetSound(SOUND_SORT), 0);

                                gstate->score += 1.0f;
                                ++gstate->num_sorted;

                                mail->pos = e->pos;
                                mail->pos.z += 1.0f;
                                EasingSetLerp(&mail->graphics.pos_easing, 0.001f);
                                mail->scale = Vec3Lit(0.1f, 0.1f, 0.1f);
                                EasingSetLerp(&mail->graphics.scale_easing, 0.1f);

                                gstate->mail_root = mail->next_mail;
                            }
                        }
                    }
                }

                if (EntityIdNull(gstate->mail_root)) {
                    gstate->phase = PHASE_GAMEOVER;
                    break;
                }

                break;
            }
            case PHASE_GAMEOVER: {
                break;
            }
            case PHASE_SHOP_OPEN: {
                break;
            }
            case PHASE_EOD: {
                break;
            }
            default: {
                Assert(0);
            }
        }

        for (Entity* e = NULL; IncrementEntity(&e);) {
            if (TestFlag(e->flags, EFLAG_DRAW)) {
                e->graphics.pos = EaseVec3(e->graphics.pos, e->pos, dt, e->graphics.pos_easing);
                e->graphics.orient = e->orient;
                e->graphics.scale = EaseVec3(e->graphics.scale, e->scale, dt, e->graphics.scale_easing);
                GraphicsInfoTickAnimation(&e->graphics, dt);
            }
        }
    }

    if (gstate->render) {
        BeginRender(dt);

        // Render Entities
        {
            ShaderPass* pass = BeginShaderPass(GetDefaultShader());
            pass->view = CameraGetViewMat(gstate->camera);
            // pass->proj = Mat4Perspective(Deg2Rad(60.0f), (f32)gstate->renderw / (f32)gstate->renderh, 0.1f, 100.0f);
            f32 width = gstate->renderw_world;
            f32 height = gstate->renderh_world;
            pass->proj = Mat4Ortho(-0.5f * width, 0.5f * width, -0.5f * height, 0.5f * height, -100.0f, 100.0f);

            for (Entity* e = NULL; IncrementEntity(&e);) {
                if (TestFlag(e->flags, EFLAG_DRAW)) {
                    DrawGraphicsInfo(&e->graphics);
                }
            }

            EndShaderPass();
        }

        // 2D HUD
        {
            ShaderPass* pass = BeginShaderPass(GetDefaultShader());
            f32 width = gstate->renderw;
            f32 height = gstate->renderh;
            pass->proj = Mat4Ortho(0, width, 0, height, -1.0f, 1.0f);

            {
                LFont font = GetLFontClosest(40.0f);

                {
                    Vec2 spacing = Vec2Lit(0.0f, 20.0f);
                    Vec2 pos = Vec2Lit(10.0f, height);

                    {
                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Day: %d", gstate->day);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);
                        pos.y -= dims.y;
                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }

                    {
                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Score: %.2f", gstate->score);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);
                        pos.y -= dims.y;
                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }

                    {
                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "No. Sorted: %d", gstate->num_sorted);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);
                        pos.y -= dims.y;
                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }

                    {
                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Upgrade");
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);

                        pos.y -= dims.y;

                        // TODO: The positioning of this is not correct. this will make sure the quad is centered
                        // relative to the baseline pos (the pos we passed to measuretext) but the text likely has
                        // some chars that extend below the baseline. I think instead I need to have some function
                        // that gives me the text rect relative to a baseline and then position that inside of drawn
                        // rect to make the text centered? Not sure, sort of need to think about what API drawing
                        // text should take. Should it take a position representing a baseline? or a position
                        // representing the center of some rect to draw within? sort of feel like the former and the
                        // later could be a wrapper on top.
                        Vec3 quadpos = Vec3Lit(pos.x + 0.5f * dims.x, pos.y + 0.5f * dims.y, 0.0f);
                        Polyhedron rect;
                        PolyHedronSetAABB2DFromWH(&rect, Vec3ToVec2(quadpos), dims);

                        GameControllerState* controller = &input->controllers[0];
                        Polyhedron cursor;
                        PolyhedronSetCircle(&cursor, controller->cursor_pos, 3.0f);
                        CollisionInfo info = {0};
                        bool collides = TestPolyhedronVsPolyhedron(&cursor, &rect, &info.normal, &info.depth);
                        // TODO: There is a potential issue here in that by the time we've checked the down action
                        // here, we might have triggered gameplay actions as well since this logic check happens
                        // after the update step. Ideally we decouple the UI logic detection/gameplay state update
                        // and the render. But they are coupled for the time being.
                        if (collides && controller->action_down.pressed) {
                            gstate->shop_open = !gstate->shop_open;
                        }

                        DrawQuad(quadpos, dims.x, dims.y, Mat4Identity(), LBLUE);

                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }
                }

                if (gstate->shop_open) {
                    Vec3 center = Vec3Lit(0.5f * width, 0.5f * height, 0.0f);
                    Vec2 parent_dims = Vec2Lit(width * 0.7f, height * 0.7f);
                    DrawQuad(center, parent_dims.x, parent_dims.y, Mat4Identity(), LRED);

                    Vec2 parent_top_left = Vec2Lit(center.x - 0.5f * parent_dims.x, center.y + 0.5f * parent_dims.y);
                    Vec2 pos = Vec2Lit(parent_top_left.x + 10.0f, parent_top_left.y + 0.0f);
                    Vec2 spacing = Vec2Lit(0.0f, 20.0f);

                    {
                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Buy Mailbox: $%.2f", gstate->worker_cost);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);

                        pos.y -= dims.y;

                        // TODO: The positioning of this is not correct. this will make sure the quad is centered
                        // relative to the baseline pos (the pos we passed to measuretext) but the text likely has some
                        // chars that extend below the baseline. I think instead I need to have some function that gives
                        // me the text rect relative to a baseline and then position that inside of drawn rect to make
                        // the text centered? Not sure, sort of need to think about what API drawing text should take.
                        // Should it take a position representing a baseline? or a position representing the center of
                        // some rect to draw within? sort of feel like the former and the later could be a wrapper on
                        // top.
                        Vec3 quadpos = Vec3Lit(pos.x + 0.5f * dims.x, pos.y + 0.5f * dims.y, 0.0f);
                        Polyhedron rect;
                        PolyHedronSetAABB2DFromWH(&rect, Vec3ToVec2(quadpos), dims);

                        GameControllerState* controller = &input->controllers[0];
                        Polyhedron cursor;
                        PolyhedronSetCircle(&cursor, controller->cursor_pos, 3.0f);
                        CollisionInfo info = {0};
                        bool collides = TestPolyhedronVsPolyhedron(&cursor, &rect, &info.normal, &info.depth);
                        // TODO: There is a potential issue here in that by the time we've checked the down action here,
                        // we might have triggered gameplay actions as well since this logic check happens after the
                        // update step. Ideally we decouple the UI logic detection/gameplay state update and the render.
                        // But they are coupled for the time being.
                        if (collides && controller->action_down.pressed) {
                            if (gstate->score >= gstate->worker_cost) {
                                gstate->score -= gstate->worker_cost;
                                ++gstate->worker_count;
                                SpawnWorker(gstate, Vec3Zero());
                                gstate->worker_cost *= 2.0f;
                            }
                        }

                        DrawQuad(quadpos, dims.x, dims.y, Mat4Identity(), LBLUE);

                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }

                    {
                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Increase Worker Speed: $%.2f",
                                 gstate->worker_cooldown_sec_cost);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);

                        pos.y -= dims.y;

                        // TODO: The positioning of this is not correct. this will make sure the quad is centered
                        // relative to the baseline pos (the pos we passed to measuretext) but the text likely has some
                        // chars that extend below the baseline. I think instead I need to have some function that gives
                        // me the text rect relative to a baseline and then position that inside of drawn rect to make
                        // the text centered? Not sure, sort of need to think about what API drawing text should take.
                        // Should it take a position representing a baseline? or a position representing the center of
                        // some rect to draw within? sort of feel like the former and the later could be a wrapper on
                        // top.
                        Vec3 quadpos = Vec3Lit(pos.x + 0.5f * dims.x, pos.y + 0.5f * dims.y, 0.0f);
                        Polyhedron rect;
                        PolyHedronSetAABB2DFromWH(&rect, Vec3ToVec2(quadpos), dims);

                        GameControllerState* controller = &input->controllers[0];
                        Polyhedron cursor;
                        PolyhedronSetCircle(&cursor, controller->cursor_pos, 3.0f);
                        CollisionInfo info = {0};
                        bool collides = TestPolyhedronVsPolyhedron(&cursor, &rect, &info.normal, &info.depth);
                        // TODO: There is a potential issue here in that by the time we've checked the down action here,
                        // we might have triggered gameplay actions as well since this logic check happens after the
                        // update step. Ideally we decouple the UI logic detection/gameplay state update and the render.
                        // But they are coupled for the time being.
                        if (collides && controller->action_down.pressed) {
                            if (gstate->score >= gstate->worker_cooldown_sec_cost) {
                                gstate->score -= gstate->worker_cooldown_sec_cost;
                                gstate->worker_cooldown_sec *= 0.8f;
                                gstate->worker_cooldown_sec_cost *= 2.0f;
                                for (Entity* e = EntityFromId(gstate->worker_root); e;
                                     e = EntityFromId(e->next_worker)) {
                                    e->work_cooldown_sec = gstate->worker_cooldown_sec;
                                }
                            }
                        }

                        DrawQuad(quadpos, dims.x, dims.y, Mat4Identity(), LBLUE);

                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }

                    if (gstate->worker_count >= 6) {
                        f32 cost = 200.0f;

                        pos = Vec2Sub(pos, spacing);

                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Increase Worker Speed: $%.2f", cost);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);

                        pos.y -= dims.y;

                        // TODO: The positioning of this is not correct. this will make sure the quad is centered
                        // relative to the baseline pos (the pos we passed to measuretext) but the text likely has some
                        // chars that extend below the baseline. I think instead I need to have some function that gives
                        // me the text rect relative to a baseline and then position that inside of drawn rect to make
                        // the text centered? Not sure, sort of need to think about what API drawing text should take.
                        // Should it take a position representing a baseline? or a position representing the center of
                        // some rect to draw within? sort of feel like the former and the later could be a wrapper on
                        // top.
                        Vec3 quadpos = Vec3Lit(pos.x + 0.5f * dims.x, pos.y + 0.5f * dims.y, 0.0f);
                        Polyhedron rect;
                        PolyHedronSetAABB2DFromWH(&rect, Vec3ToVec2(quadpos), dims);

                        GameControllerState* controller = &input->controllers[0];
                        Polyhedron cursor;
                        PolyhedronSetCircle(&cursor, controller->cursor_pos, 3.0f);
                        CollisionInfo info = {0};
                        bool collides = TestPolyhedronVsPolyhedron(&cursor, &rect, &info.normal, &info.depth);
                        // TODO: There is a potential issue here in that by the time we've checked the down action here,
                        // we might have triggered gameplay actions as well since this logic check happens after the
                        // update step. Ideally we decouple the UI logic detection/gameplay state update and the render.
                        // But they are coupled for the time being.
                        if (collides && controller->action_down.pressed) {
                            if (gstate->score >= cost) {
                                gstate->score -= cost;
                                cost *= 2.0f;
                            }
                        }

                        DrawQuad(quadpos, dims.x, dims.y, Mat4Identity(), LBLUE);

                        DrawStr8Ex(font, pos, LBLACK, txt);
                    }
                }
            }

            if (gstate->phase == PHASE_GAMEOVER) {
                LFont font = GetLFontClosest(40.0f);

                DrawQuad(Vec3Lit(width * 0.5f, height * 0.5f, 0.0f), width, height, Mat4Identity(), LGRAY);

                {
                    Vec2 spacing = Vec2Lit(0.0f, 0.0f);
                    Vec2 pos = Vec2Lit(width * 0.5f, height * 0.75f);

                    {
                        Str8 txt = Str8Lit("GAME OVER");
                        Vec2 dims = LFontMeasureStr8(font, txt);
                        pos.y -= dims.y;

                        Vec2 p = Vec2Sub(pos, spacing);
                        p.x -= dims.x * 0.5f;
                        DrawStr8Ex(font, p, LBLACK, txt);
                    }

                    {
                        char buffer[256] = {0};
                        snprintf(buffer, ArraySizeOf(buffer) - 1, "Reached The End In %d Days!", gstate->day);
                        Str8 txt = Str8FromC(buffer);
                        Vec2 dims = LFontMeasureStr8(font, txt);
                        pos.y -= dims.y;

                        Vec2 p = Vec2Sub(pos, spacing);
                        p.x -= dims.x * 0.5f;
                        DrawStr8Ex(font, p, LBLACK, txt);
                    }
                }
            }

            EndShaderPass();
        }

        EndRender();
    }
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
    Arena perm = NewArena(buffer, cap);
    gstate->perm = perm;
    byte* temp_buffer = New(&gstate->perm, GB(1), byte);
    gstate->temp = NewArena(temp_buffer, GB(1));

    gstate->renderw = 1920.0f;
    gstate->renderh = 1080.0f;
    gstate->renderw_world = Px2Meter(gstate->renderw);
    gstate->renderh_world = Px2Meter(gstate->renderh);

    gstate->target_fps = 60;
    gstate->fixed_dt = Hz(2.0f * gstate->target_fps);

    u32 scale = 1;
    PlatformInit(scale * gstate->renderw, scale * gstate->renderh, gstate->target_fps);

    isize sound_cap = GB(2);
    byte* sound_buffer = New(&gstate->perm, sound_cap, byte);

    PlatformInitAudio(sound_buffer, sound_cap);
    InitFonts();
    ParticleSystemInit();

    {
        Arena render_arena = NewSubArena(&gstate->perm, MB(10));
        RenderStateInit(gstate->renderw, gstate->renderh);
        RenderDataInit(&render_arena);
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
        gstate->scenefuncs[SCENE_DEFAULT] = SceneDefaultTick;
        gstate->scenefuncs[SCENE_DEFAULT_3D] = SceneDefault3DTick;
        gstate->scenefuncs[SCENE_MAIN] = SceneMainTick;
        Assert(SCENE_COUNT < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, SCENE_MAIN);

    gstate->fts_tail = 1;
    gstate->fts_head = 0;
}

void GameTick(f32 dt, GameInput* input) {
    GameState* gstate = &GState;
    gstate->dt = dt;
    gstate->timer += dt;

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

        // https://medium.com/%40tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
        // https://gameprogrammingpatterns.com/game-loop.html#play-catch-up
        while (gstate->accumulated_time >= gstate->fixed_dt) {
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
