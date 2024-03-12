#ifndef UNITY_BUILD
#include <stdint.h>

#include "entity.c"
#include "level.c"
#include "raylib.h"
#endif

static Color Colors[7] = {GRAY, RED, BLUE, GREEN, YELLOW, PINK, ORANGE};
static Color FaceColors[6] = {RED, BLUE, GREEN, YELLOW, PINK, ORANGE};

static Color GetTopFaceColor(Mesh* mesh, Quaternion rotation) {
    Vector3 up = {0.0f, 1.0f, 0.0f};
    for (uint32_t i = 0; i < (sizeof(FaceColors) / sizeof(FaceColors[0])); ++i) {
        uint32_t offset = i * 4 * 3;
        Vector3 normal = {mesh->normals[offset + 0], mesh->normals[offset + 1], mesh->normals[offset + 2]};
        normal = Vector3RotateByQuaternion(normal, rotation);
        float angle = Vector3Angle(normal, up);
        if (fabs(angle) <= 1e-5) {
            return FaceColors[i];
        }
    }

    assert(0);
}

static void PlayerInit(Entity* player, Vector3 position) {
    player->position = position;
    player->orientation = QuaternionIdentity();
    player->model = GenCubeModel(RED);
    EntitySetFlag(player, EF_PLAYER_CONTROLLED);

    Texture texture = GenCubeTexture2(FaceColors);

    // Remap the texture coordinates for all of the faces of the cube such that each face has a different color.
    player->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    Mesh* mesh = &player->model.meshes[0];
    for (uint32_t i = 0; i < 6; ++i) {
        uint32_t tidx = i * 2 * 4;
        const float face_width = 128.0f;
        float min = (i * face_width);
        float max = min + face_width;
        float min_u = min / texture.width;
        float max_u = max / texture.width;
        mesh->texcoords[tidx + 0] = min_u;
        mesh->texcoords[tidx + 1] = 0.0f;
        mesh->texcoords[tidx + 2] = max_u;
        mesh->texcoords[tidx + 3] = 0.0f;
        mesh->texcoords[tidx + 4] = max_u;
        mesh->texcoords[tidx + 5] = 1.0f;
        mesh->texcoords[tidx + 6] = min_u;
        mesh->texcoords[tidx + 7] = 1.0f;
    }
    // TODO: This feels hacky, but it works. Just forget the mesh buffer ids and reupload.
    mesh->vaoId = 0;
    mesh->vboId = NULL;
    UploadMesh(mesh, false);
}

static void EntityPlayerControlled(Entity* entity, float delta_time) {
    Vector3 direction = {0};
    if (IsKeyPressed(KEY_UP)) {
        direction = Vector3Add(direction, (Vector3){0, 0, -CubeSideLength});
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        direction = Vector3Add(direction, (Vector3){CubeSideLength, 0, 0});
    }
    if (IsKeyPressed(KEY_DOWN)) {
        direction = Vector3Add(direction, (Vector3){0, 0, CubeSideLength});
    }
    if (IsKeyPressed(KEY_LEFT)) {
        direction = Vector3Add(direction, (Vector3){-CubeSideLength, 0, 0});
    }

    Vector3 position = Vector3Add(entity->position, direction);
    BoundingBox bb = GetBoundingBox(position);
    LevelData* level = GetCurLevel();
    for (uint32_t i = 0; i < LevelDataNumEntities(level); ++i) {
        if (i == entity->idx) {
            continue;
        }
        Entity* e2 = LevelDataGetEntity(level, i);
        BoundingBox e2_bb = EntityGetBoundingBox(e2);
        if (CheckCollisionBoxes(bb, e2_bb)) {
            if (EntityTestFlag(e2, EF_BLOCK_IS_PUSHABLE)) {
                e2->position = Vector3Add(e2->position, direction);
            } else {
                position = entity->position;
                break;
            }
        }
    }

    entity->position = position;
}

static void EntityPlayerControlled2(Entity* entity, float delta_time) {
    static float T = 0;
    static Quaternion StartOrientation;
    static Quaternion EndOrientation;
    static Vector3 StartPosition;
    static Vector3 EndPosition;

    switch (entity->player_state) {
        case PS_IDLE: {
            if (IsKeyPressed(KEY_UP)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, -90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){0, 0, -CubeSideLength});
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, -90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){CubeSideLength, 0, 0});
            }
            if (IsKeyPressed(KEY_DOWN)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){1, 0, 0}, 90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){0, 0, CubeSideLength});
            }
            if (IsKeyPressed(KEY_LEFT)) {
                entity->player_state = PS_WALKING;
                T = 0;
                StartOrientation = entity->orientation;
                StartPosition = entity->position;
                EndOrientation =
                    QuaternionMultiply(QuaternionFromAxisAngle((Vector3){0, 0, 1}, 90 * DEG2RAD), entity->orientation);
                EndPosition = Vector3Add(entity->position, (Vector3){-CubeSideLength, 0, 0});
            }

            break;
        }
        case PS_WALKING: {
            T += delta_time;
            float t = T / 0.15;
            if (t >= 1.0f) {
                entity->player_state = PS_IDLE;
                entity->orientation = EndOrientation;
                entity->position = EndPosition;
            } else {
                entity->orientation = QuaternionSlerp(StartOrientation, EndOrientation, t);
                entity->position = Vector3Lerp(StartPosition, EndPosition, t);
            }

            Color top_color = GetTopFaceColor(&entity->model.meshes[0], StartOrientation);

            Entity* goal = LevelDataGetGoal(&CurLevel);
            BoundingBox player_bb = GetBoundingBox(entity->position);

            for (uint32_t i = 0; i < LevelDataNumEntities(&CurLevel); ++i) {
                Entity* entity2 = LevelDataGetEntity(&CurLevel, i);
                if (entity2 == entity || entity2 == goal) {
                    continue;
                }

                Color e2_color = entity2->block_color;
                BoundingBox e2_bb = GetBoundingBox(entity2->position);
                if (CheckCollisionBoxes(player_bb, e2_bb) &&
                    (top_color.r != e2_color.r || top_color.g != e2_color.g || top_color.b != e2_color.b ||
                     top_color.a != e2_color.a) &&
                    !EntityTestFlag(entity2, EF_HUNT_PLAYER)) {
                    entity->player_state = PS_IDLE;
                    entity->position = StartPosition;
                    entity->orientation = StartOrientation;
                }
            }

            break;
        }
        default: {
            exit(0);
        }
    }

    if (false) {
        PrintMatrix(QuaternionToMatrix(entity->orientation));
    }
}