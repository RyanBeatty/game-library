#ifndef UNITY_BUILD
#include <stdbool.h>

#include "entity.c"
#include "level.c"
#include "raylib.h"
#include "stb_ds.h"

#endif

static void EntityLevelSuccessOnCollision(Entity* entity, float delta_time) {
    // Check for collision with goal.
    // TODO: Maybe rethink this since this causes problems depending on the size of the bounding box. for
    // example, if we make the bounding box the same size as cube, then we will accidentally collide if we move
    // to a square next to goal. Maybe instead we should convert entity positions to a "grid space" where each
    // grid spot has a unique id and use that to check collisions.
    Entity* player = LevelDataGetPlayer(&CurLevel);
    BoundingBox player_bb = EntityGetBoundingBox(player);
    BoundingBox goal_bb = EntityGetBoundingBox(entity);
    if (CheckCollisionBoxes(player_bb, goal_bb)) {
        CurLevel.finished = true;
    }
}

static void EntityTransparentOnCollision(Entity* entity, float delta_time) {
    Entity* player = LevelDataGetPlayer(&CurLevel);
    BoundingBox e_bb = GetBoundingBox(entity->position);
    BoundingBox p_bb = GetBoundingBox(player->position);

    unsigned char alpha = 255;
    if (CheckCollisionBoxes(e_bb, p_bb)) {
        Color p_color = entity->block_color;
        Color e_color = entity->block_color;
        if ((e_color.r == p_color.r || e_color.g == p_color.g || e_color.b == p_color.b)) {
            alpha = 255 / 2;
        } else {
            alpha = 255;
        }
    }
    entity->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color.a = alpha;
}

// TODO: Fully implement this once I shift grid to be on
static void EntityHuntPlayer(Entity* entity, float delta_time) {
    Entity* player = LevelDataGetPlayer(&CurLevel);

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) {
        struct {
            Vector3 key;
            Vector3 value;
        }* parents = NULL;
        Vector3 end = World2Grid(player->position);
        Vector3 start = World2Grid(entity->position);
        hmput(parents, start, start);

        Vector3 queue[128];
        uint32_t head = 0;
        uint32_t tail = 1;
        queue[head] = start;

        // BFS to find next move.
        for (; head < tail; ++head) {
            Vector3 curr = queue[head];
            if (Vector3Equals(curr, end)) {
                break;
            }

            // Remember, everything in grid coords here so offsets all 1 instead of cube size.
            Vector3 dirs[4] = {{0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {1, 0, 0}};
            for (uint32_t i = 0; i < 4; ++i) {
                Vector3 child = Vector3Add(curr, dirs[i]);
                if (hmgeti(parents, child) == -1) {
                    bool hit = false;
                    for (uint32_t k = 0; k < LevelDataNumEntities(&CurLevel); ++k) {
                        Entity* e2 = LevelDataGetEntity(&CurLevel, k);
                        if (e2 == entity || e2 == player) {
                            continue;
                        }

                        if (Vector3Equals(child, World2Grid(e2->position))) {
                            hit = true;
                            break;
                        }
                    }

                    hmput(parents, child, curr);
                    if (!hit) {
                        assert(tail < 128);
                        queue[tail] = child;
                        ++tail;
                    }
                }
            }
        }

        ptrdiff_t idx = hmgeti(parents, end);
        assert(idx != -1);
        Vector3 next_move = end;
        Vector3 parent = parents[idx].value;
        while (!Vector3Equals(parent, start)) {
            next_move = parent;
            parent = hmget(parents, next_move);
        }

        // Reached player, signal level failed.
        // TOOD: Bug here. If player and hunter try to move to same square, player will not because of the way we
        // interpolate its position during movement. Need to figure this out in a way that doesn't mess with the
        // animation.
        if (Vector3Equals(next_move, end)) {
            CurLevel.failed = true;
            return;
        }

        next_move = GridToWorld(next_move);
        entity->position = next_move;
    }

    // TODO: Maybe improve this. Should think about centrallizing collision response since I want to respond differently
    // depending on what entity types are colliding. Its a bit hard otherwise to deal with this logic since I have to
    // spread the collision response logic across multiple update functions. Having the logic be spread in update
    // functions can be convenient though.
    if (Vector3Equals(World2Grid(entity->position), World2Grid(player->position))) {
        CurLevel.failed = true;
    }
}

static void EntityUpdate(Entity* entity, float delta_time) {
    if (EntityTestFlag(entity, EF_TRANSPARENT_ON_COLLISION)) {
        EntityTransparentOnCollision(entity, delta_time);
    }
    if (EntityTestFlag(entity, EF_LEVEL_SUCCESS_ON_COLLISION)) {
        EntityLevelSuccessOnCollision(entity, delta_time);
    }
    if (EntityTestFlag(entity, EF_PLAYER_CONTROLLED)) {
        EntityPlayerControlled(entity, delta_time);
    }
    if (EntityTestFlag(entity, EF_HUNT_PLAYER)) {
        EntityHuntPlayer(entity, delta_time);
    }
}