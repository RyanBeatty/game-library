#ifndef UNITY_BUILD
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Decls.
typedef struct Entity Entity;
static void EntityPlayerControlled(Entity* entity, float delta_time);

/////////////////////////////////////////////////////////////////////////////

const int ScreenWidth = 800;
const int ScreenHeight = 450;

static const float CubeSideLength = 2.0f;

static void PrintMatrix(Matrix m) {
    float16 mv = MatrixToFloatV(m);
    for (int i = 0; i < 16; ++i) {
        if (i % 4 == 0) {
            printf("\n");
        }
        printf("%f ", mv.v[i]);
    }
    printf("\n");
}

#define ORIGIN ((Vector3){0, 0, 0})

static Vector3 World2Grid(Vector3 position) {
    position.x = floorf(position.x / CubeSideLength);
    position.y = floorf(position.y / CubeSideLength);
    position.z = floorf(position.z / CubeSideLength);
    return position;
}

static Vector3 GridToWorld(Vector3 position) {
    position.x *= CubeSideLength;
    position.y *= CubeSideLength;
    position.z *= CubeSideLength;
    return position;
}

typedef enum PlayerState {
    PS_IDLE = 0,
    PS_WALKING,

    PS_COUNT
} PlayerState;

typedef enum EntityFlag {
    EF_NONE = 0,
    EF_TRANSPARENT_ON_COLLISION,
    EF_LEVEL_SUCCESS_ON_COLLISION,
    EF_PLAYER_CONTROLLED,
    EF_HUNT_PLAYER,
    EF_PUSHABLE,
    EF_WALL,
    EF_COUNT
} EntityFlag;

#define FLAGS_LEN (((int)(EF_COUNT / 8)) == 0 ? 1 : EF_COUNT / 8)

typedef struct Entity {
    uint32_t idx;
    Vector3 position;
    Quaternion orientation;
    uint8_t flags[FLAGS_LEN];

    Color block_color;  // Only used for non player entities.
    PlayerState player_state;

    Model model;
} Entity;

static bool EntityTestFlag(Entity* entity, EntityFlag flag) {
    return entity->flags[flag / 8] & (1 << ((flag % 8) - 1));
}
static void SetFlag(uint8_t flags[FLAGS_LEN], EntityFlag flag) { flags[flag / 8] |= (1 << ((flag % 8) - 1)); }
static void UnsetFlag(uint8_t flags[FLAGS_LEN], EntityFlag flag) { flags[flag / 8] &= (0 << ((flag % 8) - 1)); }
static void EntitySetFlag(Entity* entity, EntityFlag flag) { SetFlag(entity->flags, flag); }
static void EntityUnsetFlag(Entity* entity, EntityFlag flag) { UnsetFlag(entity->flags, flag); }

static void EntityUpdateTransform(Entity* entity) {
    // Remember matrices are applied left to right order, so the rotation is first.
    entity->model.transform =
        MatrixMultiply(QuaternionToMatrix(entity->orientation),
                       MatrixTranslate(entity->position.x, entity->position.y, entity->position.z));
}

struct {
    Color key;
    Texture value;
}* TextureCache = NULL;

static Texture GenCubeTexture(Color color) {
    Image image = GenImageColor(128, 128, color);
    // Horizontal lines.
    const int thickness = 5;
    ImageDrawRectangle(&image, 0, 0, 128, thickness, BLACK);
    ImageDrawRectangle(&image, 0, 128 - thickness, 128, thickness, BLACK);
    // Vertical lines.
    ImageDrawRectangle(&image, 0, 0, thickness, 128, BLACK);
    ImageDrawRectangle(&image, 128 - thickness, 0, thickness, 128, BLACK);

    Texture texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

// TODO: This is only used to generate the player cube texture. Maybe we can combine this with normal texture gen above
// since the above is just a special case of this method where all the colors are the same.
static Texture GenCubeTexture2(Color colors[6]) {
    const int width = 128;
    const int height = 128;
    const int edge_thickness = 5;
    const int num_colors = 6;
    Image image = GenImageColor(width * num_colors, height, BLACK);
    for (uint32_t i = 0; i < num_colors; ++i) {
        // Fill with color for face.
        ImageDrawRectangle(&image, i * width, 0, width, height, colors[i]);

        // Horizontal lines.
        ImageDrawRectangle(&image, i * width, 0, width, edge_thickness, BLACK);
        ImageDrawRectangle(&image, i * width, height - edge_thickness, width, edge_thickness, BLACK);
        // Vertical lines.
        ImageDrawRectangle(&image, i * width, 0, edge_thickness, height, BLACK);
        ImageDrawRectangle(&image, (i * width) + width - edge_thickness, 0, edge_thickness, height, BLACK);
    }

    Texture texture = LoadTextureFromImage(image);
    UnloadImage(image);
    return texture;
}

static Texture GetTextureForColor(Color color) {
    ptrdiff_t idx = hmgeti(TextureCache, color);
    if (idx == -1) {
        Texture texture = GenCubeTexture(color);
        hmput(TextureCache, color, texture);
        return texture;
    }
    return TextureCache[idx].value;
}

static Model GenCubeModel(Color color) {
    Model model = LoadModelFromMesh(GenMeshCube(CubeSideLength, CubeSideLength, CubeSideLength));
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = GetTextureForColor(color);
    return model;
}

// TODO: Texture coords aren't correct. black lines aren't showing. fix later.
static Mesh GenMeshHunter() {
    Mesh mesh = {0};
    mesh.triangleCount = 1;
    mesh.vertexCount = mesh.triangleCount * 3;
    mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));   // 3 vertices, 3 coordinates each
    mesh.texcoords = (float*)MemAlloc(mesh.vertexCount * 2 * sizeof(float));  // 3 vertices, 2 coordinates each
    mesh.normals = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));    // 3 vertices, 3 coordinates each

    float radius = CubeSideLength / 2.0f;

    // v1 - Rmemeber center needs to be at origin.
    mesh.vertices[0] = -radius;
    mesh.vertices[1] = 0;
    mesh.vertices[2] = radius;
    mesh.normals[0] = 0;
    mesh.normals[1] = 1;
    mesh.normals[2] = 0;
    mesh.texcoords[0] = 0;
    mesh.texcoords[1] = 0;

    // v2
    mesh.vertices[3] = radius;
    mesh.vertices[4] = 0;
    mesh.vertices[5] = radius;
    mesh.normals[3] = 0;
    mesh.normals[4] = 1;
    mesh.normals[5] = 0;
    mesh.texcoords[2] = 0.5f;
    mesh.texcoords[3] = 1.0f;

    // v3
    mesh.vertices[6] = 0;
    mesh.vertices[7] = 0;
    mesh.vertices[8] = -radius;
    mesh.normals[6] = 0;
    mesh.normals[7] = 1;
    mesh.normals[8] = 0;
    mesh.texcoords[4] = 1;
    mesh.texcoords[5] = 0;

    // Upload mesh data from CPU (RAM) to GPU (VRAM) memory
    UploadMesh(&mesh, false);

    return mesh;
}

static Model GenHunterModel(Color color) {
    Model model = LoadModelFromMesh(GenMeshHunter());
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = GetTextureForColor(color);
    return model;
}

static BoundingBox EntityGetBoundingBox(Entity* entity) {
    // NOTE: Make the bounding box smaller than the cube for now so that we don't accidentally collide two boxes if they
    // move next to eachother.
    float radius = CubeSideLength / 4.0f;
    Vector3 min = {-radius, -radius, -radius};
    Vector3 max = {radius, radius, radius};
    // TODO: Why doesn't this work??
    // min = Vector3RotateByQuaternion(min, entity->orientation);
    // max = Vector3RotateByQuaternion(max, entity->orientation);
    min = Vector3Add(min, entity->position);
    max = Vector3Add(max, entity->position);
    BoundingBox bb = {.min = min, .max = max};
    return bb;
}

static BoundingBox GetBoundingBox(Vector3 position) {
    // NOTE: Make the bounding box smaller than the cube for now so that we don't accidentally collide two boxes if they
    // move next to eachother.
    float radius = CubeSideLength / 4.0f;
    BoundingBox bb = {.min = Vector3Add(position, (Vector3){-radius, -radius, -radius}),
                      .max = Vector3Add(position, (Vector3){radius, radius, radius})};
    return bb;
}

static RayCollision GetRayCollisionEntity(Ray ray, Entity* entity) {
    assert(entity->model.meshCount != 0);
    EntityUpdateTransform(entity);
    return GetRayCollisionMesh(ray, entity->model.meshes[0], entity->model.transform);
}

static void EntityTransformMesh(Entity* entity, Matrix transform) {
    assert(entity->model.meshCount == 1);
    Mesh* mesh = &entity->model.meshes[0];
    for (uint32_t i = 0; i < mesh->vertexCount; ++i) {
        uint32_t offset = i * 3;
        Vector3 vertex = {mesh->vertices[offset], mesh->vertices[offset + 1], mesh->vertices[offset + 2]};
        vertex = Vector3Transform(vertex, transform);
        mesh->vertices[offset] = vertex.x;
        mesh->vertices[offset + 1] = vertex.y;
        mesh->vertices[offset + 2] = vertex.z;
    }

    for (uint32_t i = 0; i < mesh->vertexCount; ++i) {
        uint32_t offset = i * 3;
        Vector3 vertex = {mesh->normals[offset], mesh->normals[offset + 1], mesh->normals[offset + 2]};
        vertex = Vector3Transform(vertex, transform);
        mesh->normals[offset] = vertex.x;
        mesh->normals[offset + 1] = vertex.y;
        mesh->normals[offset + 2] = vertex.z;
    }

    // TODO: Do I need to handle texcoords or tangents??
    // TODO: This feels hacky, but it works. Just forget the mesh buffer ids and reupload.
    mesh->vaoId = 0;
    mesh->vboId = NULL;
    UploadMesh(mesh, false);
}
