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
// s:unit

// 1 meter = N pixels scale factor. i.e. how many pixels are in 1 meter.
// TODO: I'm not really sure I should be using this in a whole lot of places. Its sort of insiduously worked its way
// into a fair amount of different places. I think maybe I should just author art assets in world units?
static f32 PPM = 1.0f;
#define MPP (1.0f / PPM)
#define Meter2Px(m) ((m) * PPM)
#define Px2Meter(px) ((px) * MPP)

////////////////////////////////////////////////////////////
// S:Color

typedef Vec4 RGBA;

#define RGBALit(r, g, b, a) (Vec4Lit((r) / 255.0f, (g) / 255.0f, (b) / 255.0f, (a)))
#define RGBLit(r, g, b) (RGBALit((r), (g), (b), 1.0f))

static const Vec4 LCLEAR = Vec4Lit(1.0f, 1.0f, 1.0f, 1.0f);
static const Vec4 LHIDDEN = Vec4Lit(1.0f, 1.0f, 1.0f, 0.0f);
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

typedef enum {
    FILTER_NONE = 0,
    FILTER_NEAREST = GL_NEAREST,
    FILTER_LINEAR = GL_LINEAR,
} LTextureFilter;

typedef struct {
    TextureId id;
    i32 width;
    i32 height;

    LTextureFilter filter;
    byte* data;
} LTexture;

TypedefArr(LTexture);

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
    tx.filter = filter;
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
    LTexture textures_buffer[MAX_TEXTURES];
    LTextureArr textures;
} TextureCache;

static TextureCache TCACHE;

typedef enum TextureLabel { TX_DEFAULT = 0, TX_ALL, TX_COUNT } TextureLabel;

static LTexture GetTexture(TextureLabel label) {
    TextureCache* tcache = &TCACHE;
    return ArrGet(&tcache->textures, label);
}

#define GetTexturePtr(label) (ArrGetPtr(&TCACHE.textures, (label)))

#define TextureBottomLeft(tx) (Vec3Lit(-0.5f * (tx).width, -0.5f * (tx).height, 0.0f))

static void TextureCacheInit() {
    static TextureCache TEXTURE_CACHE_ZERO = {0};
    TextureCache* tcache = &TCACHE;
    *tcache = TEXTURE_CACHE_ZERO;
    InitArrS(&tcache->textures, tcache->textures_buffer, ArraySizeOf(tcache->textures_buffer));

    // Default black texture.
    LTexture default_tx = {0};
    default_tx.width = 1;
    default_tx.height = 1;
    glGenTextures(1, &default_tx.id);
    glBindTexture(GL_TEXTURE_2D, default_tx.id);

    // The texture will be all white. Important to do this so that we can properly blend
    // texture colors and uniform frag color.
    GLubyte transparentPixel[] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, transparentPixel);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (isize i = 0; i < ArrCap(&tcache->textures); ++i) {
        ArrPush(&tcache->textures, default_tx);
    }

    {
        *ArrGetPtr(&tcache->textures, TX_ALL) = TextureFromFile("assets/textures/all.png", FILTER_LINEAR);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// s:sprite

// Ideally a runtime sprite should contain only the info/units needed to render or do game operations on a sprite.
// That means ideally all units are either in uv space for rendering or world units for positioning.
typedef struct {
    LTexture tx;

    // The dims are the size of the sprite in *world units*. Useful if we need to position other things relative to
    // sprites.
    Vec2 dims;
    // For when *positioning* the sprite visually. Note that this is in *pixels* not world units. If you were to
    // visually draw a texture attached to an object at world (x, y), then you need to do (x - px2world(offset_px.x))
    // for instance.
    // Note that (0, 0) will mean that the sprite is drawn at the center of the backing quad.
    // TODO: We don't use this yet. one problem when trying to factor this into drawing is ordering: if you shift the
    // origin first and then scale, the origin shift is actually scaled as well. So ideally you do Trans(pos) * Rot *
    // Trans(pivot) * Scale so that you can rotate around your origin. But we need to make some changes to the DrawQuad
    // api if this were to be the case.
    Vec2 origin_offset;

    // UV space coordinates for texture sampling (computed once when loaded).
    // Reminder that in general when we describe some *area* of a texture, we want to be covering
    // the *edges* (not the centers) of all of the texels within that area.
    Vec2 minuv;
    Vec2 dimsuv;
} Sprite;

typedef enum {
    SBB_FULL = 0,  // bb = width, height of texture
    SBB_CLOSEST_FIT,
} SpriteBoundingBox;

// Note that min and dims here are in pixels within the image space of the image.
LCORE Sprite SpriteFromMinDims(LTexture tx, Vec2 min, Vec2 dims) {
    Sprite sprite = {.tx = tx, .dims = Vec2Scale(dims, MPP)};

    // Calculate the dimensions of a texel. Texel size is dependant on the dimensions of the overall texture.
    Vec2 texel_dims = Vec2Lit(1.0f / tx.width, 1.0f / tx.height);

    sprite.minuv = Vec2Lit(min.x / sprite.tx.width, min.y / sprite.tx.height);
    sprite.dimsuv = Vec2Lit(dims.x / sprite.tx.width, dims.y / sprite.tx.height);

    // For now I think we only really want to do this when we are nearest filtering. for linear filtering, we might just
    // want to stick with sampling at the edges.
    if (tx.filter == FILTER_NEAREST) {
        // Inset the starting minuv by half a texel so that instead of sampling the left edge of the texel we will
        // sample near the center of the texel. this seems to prevent texture bleeding as sampling near the boundary can
        // cause the sampler to sample into the next adjacent texel rather than the one we want to sample.
        sprite.minuv.x += 0.5f * texel_dims.x;
        sprite.minuv.y += 0.5f * texel_dims.y;

        // Because we've inset the starting texel to sample near the first texel's center and that we want to sample
        // nearer to texel centers in general, we decrease the uv dimensions by 1 full texel so we can sample from left
        // texel center to right texel center for example.
        sprite.dimsuv.x -= texel_dims.x;
        sprite.dimsuv.y -= texel_dims.y;
    }
    return sprite;
}

LCORE Sprite SpriteFromMinMax(LTexture tx, Vec2 min, Vec2 max) {
    // think more about if adding 1,1 here is correct. I think it is.
    Vec2 dims = Vec2Sub(Vec2Add(max, Vec2Lit(1.0f, 1.0f)), min);
    return SpriteFromMinDims(tx, min, dims);
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

// // Address center, and all 4 corners of an aseprite pixel. We consider the aseprite coord
// // given in the UI to be the top left of the pixel.
#define AsepritePxCtr(px) (Vec2Lit((px).x + 0.5f, (px).y + 0.5f))
#define AsepritePxBL(px) (Vec2Lit((px).x, (px).y + 1.0f))
#define AsepritePxBR(px) (Vec2Lit((px).x + 1.0f, (px).y + 1.0f))
#define AsepritePxTL(px) (px)
#define AsepritePxTR(px) (Vec2Lit((px).x + 1.0f, (px).y))

// This is converting from aseprite image space to our engine image space (not world space).
// This essentially moves the origin from aseprite top left corner to bottom left corner. Why do we do this?
// Its because when we import the texture/png we flip the Y axis there.
// NOTE: It may seem like we need to subtract one, but we really don't (trust me)
// remember we are converting continuous pixel coordinates not pixel indices. It might feel like hey the last px in
// aseprite ui is actually (width-1, height-1) but thats the top left corner of that pixel. We could address the bottom
// left corner and we'd end up at the y origin.
// READ THIS IF CONFUSED: All of this gets less confusing if I just: look at the Aserpite UI, see what pixel index I
// see, see what corner/pos of the pixel I actually want to address, and then wrap that pixel index in a call to our
// PX*() conversion functions before passing to this function.
#define AsepriteToEngineImage(tx, apos) (Vec2Lit((apos).x, (tx).height - (apos).y))

// TODO: Examine use of pixel conversion here. I think I want to be authoring my art assets 1-1 with logical resolution.
// Then I can avoid the pixel conversions here since I will have authored in logical resolution
static inline Vec3 AsepriteToEngineWorld(Vec2 render_dims, Vec2 apos, f32 z) {
    return Vec3Lit(Px2Meter(apos.x - 0.5f * render_dims.x), Px2Meter(render_dims.y - apos.y - 0.5f * render_dims.y), z);
}

// TODO: Examine use of pixel conversion here. I think I want to be authoring my art assets 1-1 with logical resolution.
// Then I can avoid the pixel conversions here since I will have authored in logical resolution
static inline Vec2 AsepriteToEngineWorld2D(Vec2 render_dims, Vec2 apos) {
    return Vec3ToVec2(AsepriteToEngineWorld(render_dims, apos, 0.0f));
}

// Rectangular coordinates of some rectangle as seen via the aseprite ui. There are all in aseprite sprite space. (0, 0)
// in top left.
typedef struct {
    // The origin of aseprite is the *top left corner* of the first sprite. This should essentially jsut be the number I
    // see in aseprite UI when I hover over the pixel with the marquee tool.
    Vec2 top_left;
    Vec2 dims;

    // Note that this does not necessarily mean the top left, or any corner. you can address any portion of an aseprite
    // pixel. But essentially this is the pixel (relative to the top left corner of our sprite) that we will pivot
    // against/becomes our sprite's origin.
    Vec2 pivot_offset;
} AsepriteSprite;

#define AsepriteLit(tl, dms, pv) ((AsepriteSprite){.top_left = (tl), .dims = (dms), .pivot_offset = (pv)})
#define AsepriteCenteredLit(tl, dms) (AsepriteLit(tl, dms, Vec2Scale(dms, 0.5f)))

static Sprite SpriteFromAseprite(LTexture tx, AsepriteSprite rect) {
    Vec2 top_left = AsepriteToEngineImage(tx, rect.top_left);
    Vec2 bottom_left = Vec2Lit(top_left.x, top_left.y - rect.dims.y);
    // Vec2 top_right = Vec2Add(bottom_left, rect.dims);
    // Sprite sprite = SpriteFromMinMax(tx, bottom_left, top_right);
    Sprite sprite = SpriteFromMinDims(tx, bottom_left, rect.dims);

    // Remember that pivot_offset is a direction defined with respect to the aseprite coordinate system min, so we can
    // just add this to our engine coordinate system min and get the same point.
    Vec2 pivot_point = Vec2Add(bottom_left, rect.pivot_offset);
    Vec2 sprite_origin = Vec2Add(bottom_left, Vec2Scale(rect.dims, 0.5f));
    // This direction is intentional as essentially we need the displacement that moves the pivot point to the origin,
    // so we go from pivot -> origin.
    sprite.origin_offset = Vec2Scale(Vec2Sub(sprite_origin, pivot_point), MPP);
    return sprite;
}

#define SPRITE_LIST                                                                                \
    SPRITEX(BLOCK, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(64, 1)), Vec2Lit(36, 36)))             \
    SPRITEX(BLOCK_BG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(105, 7)), Vec2Lit(36, 36)))         \
    SPRITEX(BLOCK_FG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(154, 8)), Vec2Lit(32, 32)))         \
    SPRITEX(BOARD_BG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(5, 45)), Vec2Lit(282, 248)))        \
    SPRITEX(BUTTON_BG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(10, 314)), Vec2Lit(100, 59)))      \
    SPRITEX(SCORE_PANEL_BG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(294, 8)), Vec2Lit(109, 351))) \
    SPRITEX(SCORE_BG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(20, 400)), Vec2Lit(105, 71)))       \
    SPRITEX(SCORE_WORD_BG, AsepriteCenteredLit(AsepritePxTL(Vec2Lit(144, 419)), Vec2Lit(99, 32)))

// SPRITEX(BLOCK2, AsepriteLit(AsepritePxTL(Vec2Lit(64, 1)), Vec2Lit(36, 36), Vec2Lit(36, 36)))

typedef enum {
    SPRITE_DEFAULT = 0,
#define SPRITEX(tag, rect) SPRITE_##tag,
    SPRITE_LIST
#undef SPRITEX
        SPRITE_COUNT,
} SpriteTag;

static Sprite* GetSprite(SpriteTag tag) {
    static bool ONCE = false;
    static Sprite SPRITES[SPRITE_COUNT] = {0};
    if (!ONCE) {
        ONCE = true;
        LTexture tx = GetTexture(TX_ALL);

        SPRITES[SPRITE_DEFAULT] = DefaultSprite();

#define SPRITEX(tag, rect) SPRITES[SPRITE_##tag] = SpriteFromAseprite(tx, (rect));
        SPRITE_LIST
#undef SPRITEX
    }

    return &SPRITES[tag];
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

    TextureCacheInit();
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

        // This technique assumes LINEAR filtering on the texture in question. This works by using nearest sampling if we are sampling near the center of a texel
        // and then switching to using LINEAR sampling when we sample near a texel's border. This will cause some artifacts in the final render where at the edges
        // of a sprite/texture we might be rendering colors that do not actually appear in the original art asset. However at large enough resolutions, these
        // artifacts are hard to spot. The alternative seems to be snapping draw positions to nearest pixels but that seems to break down if we want to rotate/scale
        // continuously at non integer scales.
        vec2 iq_uv(vec2 uv, ivec2 txdims) {
            vec2 px = uv * txdims;
            vec2 seam = floor(px + 0.5);
            vec2 dudv = fwidth(px);
            px = seam + clamp((px - seam) / dudv, -0.5, 0.5);
            return px / txdims;
        }

        void main() {
            ivec2 txdims = ivec2(Texture0Resolution.xy);
            vec2 uv = iq_uv(fTextCoords, txdims);
            vec4 txColor = texture(texture0, uv);
            // vec4 txColor = texture(texture0, fTextCoords);


            // Crucial for having depth buffer + blending enabled. when I write quads with textures with some tansparent
            // parts. We want to discard those transparent parts else those values will get written to the depth buffer.
            // if (txColor.a < 0.01) {
            //     discard;
            // }
            // For blending to work here properly, its important that either texture is specified or texture
            // is default (all black) and a uniform color is specified. also 2d needs to be drawn after 3d.
            FragColor = txColor * fColor;

            // Debug helpers

            // output what texture coords we sample as the frag color.
            // i.e. output the actual pixel (normalized to 0-1) that we sampled from the texture. multiply by txdims to get the actual
            // pixel index
            // vec2 texelPos = fTextCoords.xy * txdims;
            // vec2 nearestTexel = floor(texelPos + 0.5);
            // FragColor = vec4(nearestTexel / txdims, 0.0, 1.0);
            
            // FragColor = fColor; // Useful to debug texture sampling issues.
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

// This will intialize an offscreen (game target) and a screen target. They should both be the same dimensions
// The game can upscale by rendering internally at smaller world units but to the larger offscreen buffer
// which will naturally upscale during rasterization.
static void RenderStateInit(Arena* perm) {
    RenderState* rstate = &RSTATE;

    memset(rstate, 0, sizeof(RenderTarget));
    rstate->game_target = CreateRenderTarget(PlatformScreenWidth(), PlatformScreenHeight());

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

LCORE DrawCmd* DrawQuadEx(Vec3 center, f32 width, f32 height, Mat4 orient, Vec4 color, LTexture* tx) {
    DrawCmd* cmd = AddDrawMeshCmd();
    cmd->meshcmd.meshid = GetMesh(M_QUAD);
    cmd->meshcmd.modelm = Mat4SVRT(Vec3Lit(width, height, 1.0f), orient, center);
    cmd->meshcmd.color = color;
    cmd->meshcmd.tx = *tx;
    cmd->meshcmd.uvscale = Vec2Lit(1.0f, 1.0f);
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

LCORE DrawCmd* DrawSpritePro(Sprite sprite, Vec2 dest_pos, Vec2 dest_dims, f32 rotation, Vec4 color) {
    return DrawLTextureProUV(sprite.tx, sprite.minuv, sprite.dimsuv, dest_pos, dest_dims, rotation, color);
}

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawSpriteEx(Sprite sprite, Vec3 pos, Mat4 orient, f32 scale, Vec4 tint) {
    // By default scale of 1 will mean draw the sprite with same width/height
    DrawCmd* cmd = DrawQuad(pos, sprite.dims.x * scale, sprite.dims.y * scale, orient, tint);
    cmd->meshcmd.tx = sprite.tx;
    cmd->meshcmd.uvoffset = sprite.minuv;
    cmd->meshcmd.uvscale = sprite.dimsuv;
    return cmd;
}

// I assume this will mostly be used for prototyping and debugging since entities will have dynamic positions.
LCORE DrawCmd* DrawSprite(Sprite sprite, Vec3 pos, Mat4 orient, f32 scale) {
    return DrawSpriteEx(sprite, pos, orient, scale, LCLEAR);
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

LCORE void GetScaledStbtBakedQuad(const stbtt_bakedchar* chardata, int pw, int ph, int char_index, float* xpos,
                                  float* ypos, stbtt_aligned_quad* q) {
    /*
        GetBakedQuad will return a quad that has draw coordinates relative to the passed in baseline. In order to
       downscale this baked quad (for instance in order to make text that gets upscaled by rasterizer (e.g. by
       projection only) look nicer), we need to calculate the baked quad relative to origin. Then we can downscale the
       draw position coordinates to get the correctly sized quad to draw. We also need to adjust the baseline's
       xadvancement so that text quads remain correctly positioned relative to eachother.
    */
    Vec2 baseline = Vec2Zero();

    // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
    // NOTE: Although we pass pointers to the baseline_pos, it doesn't look like GetBakedQuad actually modifies it.
    stbtt_GetBakedQuad(chardata, pw, ph, char_index, &baseline.x, &baseline.y, q, true);

    // Remember why we do this essentially: When we render the game, things will get upscaled N times since we render
    // the world in logical units which will get upscaled to the full screen. We use baked font textures to draw text,
    // one per font size. For a given font size, if we try to upscale the text, it will look like shit because upscaling
    // will introduce artifacts. Essentially we said we wanted to use fontsize X, but when our final render is written
    // to screen, we really should have used fontsize X*Upscale where Upscale = N. So instead what we do is when we say
    // "draw text with fontsize X", we actually choose fontsize X * Upscale to use for drawing. Then we *downscale* that
    // by *Upscale* so that our downscale factor essentially cancels out the upscale factor in the final render. E.g. (X
    // * Upscale / Downscale) * Upscale (remembering two upscales, we in code upscale by some factor and then render to
    // screen upscale) == X * Upscale (since Downscale == Upscale)
    f32 downscale = 1.0f / 3.0f;
    q->x0 *= downscale;
    q->x1 *= downscale;
    q->y0 *= downscale;
    q->y1 *= downscale;

    q->x0 += *xpos;
    q->x1 += *xpos;
    q->y0 += *ypos;
    q->y1 += *ypos;

    *xpos += baseline.x * downscale;
}

LCORE Vec2 DrawLTextEx(LFont font, char* text, i32 len, Vec2 baseline_pos, Vec4 color) {
    for (i32 i = 0; i < len; ++i) {
        stbtt_aligned_quad quad = {0};
        // // The y position passed in here is the baseline of the glpyhs (i.e. the common line they will sit on)
        // // NOTE: Although we pass pointers to the baseline_pos, it doesn't look like GetBakedQuad actually modifies
        // it. stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x,
        // &baseline_pos.y,
        //                    &quad, true);
        GetScaledStbtBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x,
                               &baseline_pos.y, &quad);

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
        // Debugging for testing if we can combat texture upscaling making resulting end text look like shit
        DrawLTextureProUV(font.tx, quvoffset, quvdims, qorigin, qdims, 0.0f, color);
    }
    return baseline_pos;
}

// Reminder that pos is baseline pos of the text.
LCORE void DrawStr8(LFont font, Vec2 pos, Vec4 color, Str8 str) { DrawLTextEx(font, str.data, str.len, pos, color); }

typedef struct {
    AABB2D aabb;  // bl at origin. quad to use for drawing the text.
    // This gives the vector going from center of the aabb to baseline pos. So if you add to center, you get baseline
    // pos.
    // NOTE: This means that if you want to draw this centered somewhere, take the pos you want to be centered around
    // (which might not be aabb.c, it might be some bg quad center) and add baseline_offset to that - that resulting
    // position is where you should draw the text from
    Vec2 baseline_offset;  // offset from the center of where the baseline exists. i.e. where to draw text from such
    // that it will be centered in the quad.

    // Useful to keep track of the font we measured the text with.
    LFont font;
    Str8 text;
} TextBox;

static Vec2 TextBoxGetDrawPos(TextBox box, Vec2 pos) { return Vec2Add(pos, box.baseline_offset); }

static void DrawTextBox(TextBox* textbox, Vec2 pos, Vec4 tint) {
    Vec2 text_pos = TextBoxGetDrawPos(*textbox, pos);
    return DrawStr8(textbox->font, text_pos, tint, textbox->text);
}

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
        // stbtt_GetBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x, &baseline_pos.y,
        //                    &quad, true);
        GetScaledStbtBakedQuad(font.cdata, font.tx.width, font.tx.height, text[i] - 32, &baseline_pos.x,
                               &baseline_pos.y, &quad);

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
    TextBox tbox = {.aabb = aabb, .baseline_offset = Vec2Sub(baseline, aabb.c), .font = font, .text = str};
    return tbox;
}

////////////////////////////////////////////////////////////
// S:Sound

#define SOUND_LIST                                               \
    SOUNDX(BEEP, "assets/sounds/beep.wav")                       \
    SOUNDX(CARD_FOCUS, "assets/sounds/card_focus.wav")           \
    SOUNDX(CARD_SELECT, "assets/sounds/card_select.wav")         \
    SOUNDX(CARD_DESELECT, "assets/sounds/card_deselect.wav")     \
    SOUNDX(PIECE_MOVE, "assets/sounds/piece_move.wav")           \
    SOUNDX(PIECE_PLACE, "assets/sounds/piece_place.wav")         \
    SOUNDX(UI_BUTTON_CLICK, "assets/sounds/ui_button_click.wav") \
    SOUNDX(TILE_POP, "assets/sounds/tile_pop.wav")               \
    SOUNDX(CARD_DRAW, "assets/sounds/card_draw.wav")

typedef enum {
    SOUND_NONE = 0,
#define SOUNDX(tag, filename) SOUND_##tag,
    SOUND_LIST
#undef SOUNDX
        SOUND_COUNT
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

#define SOUNDX(tag, filename) SOUNDS[SOUND_##tag] = PlatformLoadSound(filename);
    SOUND_LIST
#undef SOUNDX
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
// s:easing

// typedef enum {
//     ET_NONE = 0,     // No easing, error!
//     ET_SMOOTH_LERP,  // Uses exponential damped lerp: https://www.youtube.com/watch?v=YJB1QnEmlTs
//     ET_IN_EXPO,
//     ET_OUT_BACK,
// } EasingType;

LCORE f32 EaseOutBack(f32 t) {
    // ease out back
    f32 c1 = 1.70158;
    f32 c3 = c1 + 1.0f;
    t = 1.0f + (c3 * f32pow(t - 1.0f, 3.0f) + (c1 * f32pow(t - 1.0f, 2.0f)));
    return t;
}
LCORE f32 EaseOutCubic(f32 t) { return 1.0f - f32pow(1.0f - t, 3.0f); }

// LCORE Easing EaseSmoothLerp(f32 rate) { return (Easing){.type = ET_SMOOTH_LERP, .lerp = {.rate = rate}}; }

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

    f32 active_dt;

    // TODO: I think unused at the moment.
    EntityId down_id;   // Child
    EntityId right_id;  // Neighbor/sibling
    EntityId up_id;     // Parent

    /////////////////////////////////////////////////////////////////
    // Gameplay fields.
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
// S:UI2

typedef struct {
    Vec2 pos;   // center of rect
    Vec2 dims;  // full width and height of dims
} Rect;

static Rect RectFromBLAndDims(Vec2 bl, Vec2 dims) {
    return (Rect){.pos = Vec2Lit(bl.x + 0.5f * dims.x, bl.y + 0.5f * dims.y), .dims = dims};
}

static Rect RectFromCenterAndDims(Vec2 center, Vec2 dims) { return (Rect){.pos = center, .dims = dims}; }

// apos - is the *non* AsepritePxBL coordints. essentially the coords you see on scren when you hover in the UI
// adims - the full pixel dimensions of the rect as you see in the aseprite UI
static Rect RectFromAsepriteBLAndDims(Vec2 apos, Vec2 adims, Vec2 render_dims) {
    Vec2 bl = Vec3ToVec2(AsepriteToEngineWorld(render_dims, AsepritePxBL(apos), 0.0f));
    Vec2 dims = Vec2Lit(Px2Meter(adims.x), Px2Meter(adims.y));
    return RectFromBLAndDims(bl, dims);
}

typedef struct {
    // One might worry about collisions here when using a hash as a means for checking for equality (indeed I worried
    // about this), but you have to remember the birthday paradox. with a 2^64 keyspace, you would need ~5.1 billion
    // distint ui keys before you have a ~50% chance of collision. With 10k unique keys, you have a one in roughly 370
    // *billion* chance of collision, so I think we are safe here.
    u64 key;
} UIBoxKey;

static UIBoxKey UIBOXKEY_NULL = {0};

static bool UIBoxKeyEq(UIBoxKey a, UIBoxKey b) { return a.key == b.key; }

static u64 Str8HashU64(Str8 str) {
    u64 h = 0x100;
    for (isize i = 0; i < str.len; ++i) {
        // Deals with potential signed char issues.
        h ^= str.data[i] & 255;
        // Multiply by a large prime to scatter us across the u64 space.
        h *= 1111111111111111111;
    }
    return h;
}

static isize Str8GetSplitIdx(Str8 text) {
    // Calculate what text to display. Use ## as dilineator.
    // For ##, everything in the string gets hashed, but only things before ## get hashed
    // ex. Hello##world, only will display Hello
    // ##FOO - displays no text
    //
    // For ###, only things from ### and beyond are hashed. Same text behavior apply as above.

    for (isize i = 0; i < text.len - 1; ++i) {
        if (text.data[i] == '#' && text.data[i + 1] == '#') {
            return i;
        }
    }

    return -1;
}

static Str8 Str8GetHashPart(Str8 text) {
    isize split_idx = Str8GetSplitIdx(text);
    isize hash_start = split_idx > 0 && split_idx + 2 < text.len && text.data[split_idx + 2] == '#' ? split_idx : 0;
    Str8 hash = {0};
    hash.data = text.data + hash_start;
    hash.len = text.len - hash_start;
    return hash;
}

static Str8 Str8GetDisplayPart(Str8 text) {
    isize split_idx = Str8GetSplitIdx(text);
    isize display_end = split_idx != -1 ? split_idx : text.len;
    Str8 display = {0};
    display.data = text.data;
    // If we aren't displaying the full string because we have ## or ###,
    display.len = display_end;
    return display;
}

static UIBoxKey UIBoxKeyFromStr8(Str8 str) {
    Str8 hash_part = Str8GetHashPart(str);
    return (UIBoxKey){.key = Str8HashU64(hash_part)};
}

static u64 UIBoxKeyHash(UIBoxKey key) { return key.key; }

typedef struct UIBox UIBox;
struct UIBox {
    Vec2 pos;
    Vec2 dims;  // Full width and height

    TextBox textbox;  // Not positioned. i.e. the center here holds no meaning other than an offset.

    UIBoxKey key;
    UIBox* hash_next;
};

TypedefArr(UIBox);

typedef UIBox* UIBoxPtr;

typedef struct {
    Rect rect;

    f32 fontsize;
    Vec4 text_tint;

    Sprite* sprite;
    Vec4 tint;
} UIBoxVisuals;

#define UIBoxVisualsZero() ((UIBoxVisuals){.tint = LCLEAR})

typedef struct {
    bool first_hot;  // true on the first frame we were hot.
    bool hot;
    bool active;
} UIBoxAction;

typedef struct {
    UIBox* first;
} UIBoxHashSlot;

TypedefArr(UIBoxHashSlot);

typedef struct {
    // TODO: Is persistent storage of the boxes really necessary? I'm currently not doing any layout algorithm.
    UIBoxArr boxes;
    UIBoxHashSlotArr slots;

    // hovered.
    UIBoxKey hot_key;
    UIBoxKey hot_key_to_be;
    UIBoxKey hot_key_prev;
    UIBoxKey active_key;

    GameInput* input;
} UIState;

UIState UI_STATE;
#define GetUIState() (&UI_STATE)
#define UIGetHot() (GetUIState()->hot)

static void UIStateInit(Arena* perm) {
    static UIState UI_STATE_ZERO = {0};

    UIState* ui = GetUIState();
    *ui = UI_STATE_ZERO;

    isize cap = 10;
    InitArr(&ui->boxes, perm, UIBox, cap);
    InitArr(&ui->slots, perm, UIBoxHashSlot, cap);
    ui->slots.len = ui->slots.cap;

    ui->hot_key = UIBOXKEY_NULL;
    ui->hot_key_to_be = UIBOXKEY_NULL;
    ui->hot_key_prev = UIBOXKEY_NULL;
    ui->active_key = UIBOXKEY_NULL;
}

LCORE void UIBegin(GameInput* input) {
    UIState* ui = GetUIState();

    ui->input = input;
}

LCORE void UIEnd() {
    UIState* ui = GetUIState();

    ui->hot_key_prev = ui->hot_key;
    ui->hot_key = ui->hot_key_to_be;
    ui->hot_key_to_be = UIBOXKEY_NULL;
    ui->active_key = UIBOXKEY_NULL;
}

#define DoUI(input) Scoped(UIBegin((input)), UIEnd())

static void UISetHotToBe(UIBox* box) {
    UIState* ui = GetUIState();
    ui->hot_key_to_be = box->key;
}
static bool UIBoxIsHot(UIBox* box) { return UIBoxKeyEq(box->key, GetUIState()->hot_key); }
static bool UIBoxWasHot(UIBox* box) { return UIBoxKeyEq(box->key, GetUIState()->hot_key_prev); }

static void UISetActive(UIBox* box) {
    Assert(UIBoxIsHot(box));
    UIState* ui = GetUIState();
    ui->active_key = box->key;
}

static UIBox* UIBoxFromKey(UIBoxKey key) {
    UIState* ui = GetUIState();

    u64 hash = UIBoxKeyHash(key);
    isize idx = hash % ArrCap(&ui->slots);
    UIBoxHashSlot* slot = ArrGetPtr(&ui->slots, idx);

    // Linear probe to check if we have in ht.
    for (UIBox* cur = slot->first; cur != NULL; cur = cur->hash_next) {
        if (UIBoxKeyEq(key, cur->key)) {
            return cur;
        }
    }

    // Need to make a new box as we have no existing in hash table.
    UIBox* box = ArrReserve(&ui->boxes);
    box->key = key;
    box->hash_next = slot->first;
    slot->first = box;
    return box;
}

LCORE UIBoxAction DoUIBox(Str8 text, UIBoxVisuals* visuals) {
    UIState* ui = GetUIState();

    LFont font = GetLFontClosest(visuals->fontsize);

    UIBoxKey key = UIBoxKeyFromStr8(text);
    UIBox* box = UIBoxFromKey(key);
    box->pos = visuals->rect.pos;
    box->dims = visuals->rect.dims;
    Str8 display_text = Str8GetDisplayPart(text);
    box->textbox = LFontMeasureStr8Quad(font, display_text);

    GameControllerState* controller = GetController(ui->input, 0);
    // TODO: At some point we might want to do collision in local space of aabb.
    AABB2D aabb = AABB2DFromWidthHeight(box->pos, box->dims);
    Vec2 normal = {0};
    f32 depth = 0.0f;
    // Our choice of using cursor_pos_world sort of depends on our projection matrix. i.e. if we
    // switch to rendering ui in screen space, then this will need to change
    bool collides = TestPoint2DVsAABB2D(Vec3ToVec2(controller->cursor_pos_world), aabb, &normal, &depth);

    UIBoxAction action = {0};
    if (collides) {
        UISetHotToBe(box);
        if (UIBoxIsHot(box)) {
            action.hot = true;
            action.first_hot = !UIBoxWasHot(box);
            if (controller->action_left.pressed) {
                action.active = true;
                UISetActive(box);
            }
        }
    }

    // Draw
    {
        Sprite* sprite = visuals->sprite ? visuals->sprite : GetSprite(SPRITE_DEFAULT);
        DrawSpritePro(*sprite, box->pos, box->dims, 0.0f, visuals->tint);

        if (box->textbox.text.len) {
            Vec4 text_tint = f32eq(visuals->text_tint.a, 0.0f) ? LBLACK : visuals->text_tint;
            DrawTextBox(&box->textbox, box->pos, text_tint);
        }
    }

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

typedef struct {
    f32 dt;           // per frame time
    f32 accumulated;  // total accumulated time across program.
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

static Vec2 GetTileStep() {
    Sprite* sprite = GetSprite(SPRITE_BLOCK_BG);
    return Vec2Lit(sprite->dims.x - 2.0f, sprite->dims.y - 2.0f);
}

////////////////////////////////////////////////////

#define GRID_NUM_ROWS 7
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
    // Reminder that this pos refers to the *center* of the tile.
    Vec3 pos;
    TileId id;
} Tile;

typedef struct {
    Vec3 pos;  // Where we visually draw the center of the grid's bg sprite.
    Vec3 scale;
    Mat4 rot;

    Vec3 grid_origin_offset;  // offset to the grid's bottom left hand corner/tile space start.

    Vec3 pos_start;
    Vec3 pos_end;
    Vec3 pos_live;
    f32 pos_duration;
    f32 pos_elapsed;

    // Mat4 grid_to_world;
    // Mat4 world_to_grid;

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

// grid_pos is the grid center and where we should draw visually the board.
// origin offset is the offset from center to start of tile space 0, 0, 0. and where gameplay logic should operate on.
LCORE void GridInit(Grid* grid, Vec3 grid_pos, Vec3 grid_origin_offset) {
    static Grid ZERO = {0};

    *grid = ZERO;

    i32 ncols = GRID_NUM_COLS;
    i32 nrows = GRID_NUM_ROWS;
    for (i32 r = 0; r < nrows; ++r) {
        for (i32 c = 0; c < ncols; ++c) {
            Tile* tile = &grid->tiles[r][c];
            tile->pos = Vec3Lit(c, r, 0);
            tile->id = (TileId){.r = r, .c = c, .height = 0};
        }
    }

    Vec2 grid_tile_dims = GetTileStep();
    Sprite* grid_sprite = GetSprite(SPRITE_BOARD_BG);

    grid->nrows = nrows;
    grid->ncols = ncols;
    grid->pos = grid_pos;
    grid->grid_origin_offset = grid_origin_offset;
    grid->scale = Vec3Lit(grid_tile_dims.x, grid_tile_dims.y, grid_tile_dims.x);  // Important to make uniform scale.
    grid->rot = Mat4Identity();

    grid->pos_start = Vec3Lit(grid_pos.x, grid_pos.y + grid_sprite->dims.y, grid_pos.z);
    grid->pos_end = grid->pos;
    grid->pos_live = grid->pos_start;
    grid->pos_duration = 0.8f;
}

static Mat4 GridGetGridToWorld(Grid* grid) {
    return Mat4SVRT(grid->scale, grid->rot, Vec3Add(grid->pos_live, grid->grid_origin_offset));
}
static Mat4 GridGetWorldToGrid(Grid* grid) { return Mat4Invert(GridGetGridToWorld(grid)); }

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

LCORE Vec3 GridPosToWorld(Grid* grid, Vec3 pos) { return Vec3WTransform(pos, GridGetGridToWorld(grid)); }

// Returns a reference to the tile a cursor is over, otherwise returns NULL if cursor outside of grid.
LCORE TileId GridGetCursorHoverTileId(Grid* grid, GameControllerState* controller) {
    Mat4 world_to_grid = GridGetWorldToGrid(grid);
    Vec3 cursor_pos_grid = Vec3WTransform(Vec2ToVec3(controller->cursor_pos_world), world_to_grid);
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

////////////////////////////////////////////////////

#define MAX_SCENES 10

typedef enum {
    SCENE_DEFAULT = 0,
    SCENE_MAIN,
    SCENE_DEFAULT_3D,
    SCENE_COUNT,
} SceneId;

typedef enum {
    GP_NONE = 0,
    GP_NEW_ROUND,
    GP_PLAYING,
    GP_SHOP,
    GP_GAME_OVER,
    GP_NEW_GAME,
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
    // TODO: Rethink the naming for some of this stuff, way too confusing when you have multiple projections
    Vec2 render_dims;
    // The scaling factor we will scale up the game render texture to the screen texture.
    // TODO: We might want to somehow keep screenw/h and this inline. Or just make sure we letterbox or something
    f32 render_to_screen_scale;

    bool debug_ever;

    f32 dt;     // frame delta time.
    f32 timer;  // THIS IS REALLY ACCUMULATED FRAME TIME ACROSS THE ENTIRE PROGRAM
    FrameTime frame_time;
    u32 nframe;  // num frame we are on.
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
    Mat4 world_proj;  // The projection of world space onto the render viewport. i.e. it should map logical world units
                      // to the edges of the viewport (which may be larger than the world dimensions)

    /////////////////////////////////////////
    // Scene fields

    GamePhase phase;
    GamePhase phase_to_be;
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
    // TODO: Potentially if we are doing pixel perfect rendering (e.g. snapping camera pos + draw call positions at
    // render time), we need to make sure that we use the pixel perfect camera to unconvert from screen to world here as
    // well.
    Mat4 inv_proj = Mat4Invert(gstate->world_proj);
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

static void GamePhaseChange(GameState* gstate) {
    switch (gstate->phase_to_be) {
        case GP_NONE: {
            break;
        }
        case GP_NEW_ROUND: {
            break;
        }
        case GP_PLAYING: {
            break;
        }
        case GP_SHOP: {
            break;
        }
        case GP_GAME_OVER: {
            break;
        }
        case GP_NEW_GAME: {
            break;
        }
        default: {
            PlatformLog(LLOG_ERROR, "Unknown phase change phase: %d", gstate->phase_to_be);
            Assert(0);
        }
    }

    gstate->phase = gstate->phase_to_be == GP_NONE ? gstate->phase : gstate->phase_to_be;
    gstate->phase_to_be = GP_NONE;
}

static void SceneMainTick(GameState* gstate, GameInput* input) {
    f32 dt = gstate->dt;

    // Grid* grid = GameStateGetGrid(gstate);

    GameControllerState* controller = GetController(input, 0);

    if (gstate->init) {
        gstate->phase_to_be = GP_NEW_GAME;
        // We need to do an initial one of these here because we init and then render on the first call to tick()
        GamePhaseChange(gstate);
    }

    if (gstate->update) {
    }

    if (gstate->render) {
        Render(dt) {
            // Entities/Game World
            {
                RenderPassLoadAction load_action = DefaultLoadAction();
                load_action.clear_color = RGBLit(137, 119, 201);
                load_action.clear_color_flag = true;
                BeginShaderPass(GetDefaultShader(), load_action, CameraGetViewMat(gstate->camera), gstate->world_proj,
                                GetDefaultRenderTarget());

                // DrawQuad(Vec3Zero(), 20, 20, Mat4Identity(), LBLUE);
                // AABB2D box = AABB2DFromWidthHeight(Vec2Zero(), Vec2Lit(20, 20));

                // Vec2 point = controller->cursor_pos_world;
                // Vec2 normal = {0};
                // f32 depth = 0.0f;
                // bool collides = TestPoint2DVsAABB2D(point, box, &normal, &depth);
                // if (collides) {
                //     PlatformLog(LLOG_INFO, "collides");
                // }

                {
                    static Vec3 POS = Vec3Lit(100, 0, 0);
                    if (PlatformKeyDown(LKEY_LEFT)) {
                        POS.x -= 0.5f;
                    }
                    if (PlatformKeyDown(LKEY_RIGHT)) {
                        POS.x += 0.5f;
                    }
                    Sprite* sprite = GetSprite(SPRITE_SCORE_PANEL_BG);
                    f32 scale = f32lerp(0.72f, 1.35f, f32sin(gstate->timer));
                    DrawSprite(*sprite, POS, Mat4Identity(), scale);
                    breakpoint();
                }

                {
                    Sprite* sprite = GetSprite(SPRITE_BUTTON_BG);
                    f32 scale = f32lerp(0.90f, 1.35f, f32sin(gstate->timer));
                    Mat4 rot = Mat4RotateZ(f32lerp(Deg2Rad(20.0f), Deg2Rad(-20.0f), f32sin(gstate->timer * 0.5f)));
                    Vec3 pos = Vec3Lit(-100, 0, 0);
                    pos.x += f32lerp(-2.3, 3.2, f32sin(gstate->timer * 0.6f));
                    pos.y += f32lerp(-10.32, 10.456, f32sin(gstate->timer * 0.3f));
                    DrawSprite(*sprite, pos, rot, scale);
                    breakpoint();
                }

                DoUI(input) {
                    {
                        static UIBoxVisuals VISUALS = {0};
                        static bool VISUALS_INIT = false;

                        if (!VISUALS_INIT) {
                            VISUALS_INIT = true;
                            VISUALS = UIBoxVisualsZero();
                            VISUALS.sprite = GetSprite(SPRITE_BUTTON_BG);
                        }

                        VISUALS.rect = RectFromCenterAndDims(Vec2Zero(), Vec2Lit(100, 59));
                        VISUALS.fontsize = 16.0f * 3.0f;

                        UIBoxAction action = DoUIBox(Str8Lit("Play Hand"), &VISUALS);
                        if (action.hot) {
                            VISUALS.sprite = GetSprite(SPRITE_BLOCK_BG);
                            if (action.first_hot) {
                                PlatformPlaySound(GetSound(SOUND_UI_BUTTON_CLICK), 0);
                            }
                            if (action.active) {
                                PlatformPlaySound(GetSound(SOUND_BEEP), 0);
                            }
                        } else {
                            VISUALS.sprite = GetSprite(SPRITE_BUTTON_BG);
                        }
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
        }
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
    gstate->perm = NewArena(buffer, cap);
    byte* temp_buffer = New(&gstate->perm, GB(1), byte);
    gstate->temp = NewArena(temp_buffer, GB(1));

    gstate->render_dims = Vec2Lit(640, 360);
    gstate->render_to_screen_scale = 3.0f;

    gstate->target_fps = 60;
    gstate->fixed_dt = Hz(2.0f * gstate->target_fps);

    Vec2 screen_dims = Vec2Scale(gstate->render_dims, gstate->render_to_screen_scale);
    PlatformInit(screen_dims.x, screen_dims.y, gstate->target_fps);

    isize sound_cap = GB(2);
    byte* sound_buffer = New(&gstate->perm, sound_cap, byte);

    PlatformInitAudio(sound_buffer, sound_cap);
    InitFonts();
    ParticleSystemInit();

    {
        Arena render_state_arena = NewSubArena(&gstate->perm, MB(1));
        RenderStateInit(&render_state_arena);
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
        Assert(SCENE_COUNT < MAX_SCENES);
        gstate->scenefuncssize = idx;
    }
    GameStateLoadScene(gstate, SCENE_MAIN);

    // Camera and proj init
    {
        gstate->camera.up = Vec3Lit(0.0f, 1.0f, 0.0f);
        gstate->camera.eye = Vec3Lit(0.0f, 0.0f, 10.0f);

        // The projection that the game should render to. Maps logical world units onto the edges of the viewport (which
        // will upscale naturally to the size of the viewport)
        {
            Vec2 half_render_dims = Vec2Scale(gstate->render_dims, 0.5f);
            gstate->world_proj = Mat4Ortho(-half_render_dims.x, half_render_dims.x, -half_render_dims.y,
                                           half_render_dims.y, -100.0f, 100.0f);
        }
    }

    UIStateInit(&gstate->perm);
}

void GameTick(f32 dt, GameInput* input) {
    GameState* gstate = &GState;
    gstate->dt = dt;
    gstate->timer += dt;
    gstate->frame_time.dt = dt;
    gstate->frame_time.accumulated = gstate->timer;

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