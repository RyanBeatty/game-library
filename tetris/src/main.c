#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define GRID_NUM_ROWS (20)
#define GRID_NUM_COLS (10)
#define GRID_NUM_CELLS (GRID_NUM_ROWS * GRID_NUM_COLS)
#define CELL_WIDTH (25)
#define ScreenWidth (GRID_NUM_COLS * CELL_WIDTH)
#define ScreenHeight (GRID_NUM_ROWS * CELL_WIDTH)
#define LATERAL_MOVEMEMENT_COOLDOWN_MS_INIT (0.07f)
#define VERTICAL_MOVEMENT_COOLDOWN_MS_INIT (0.07f)

typedef bool PieceState[4][4];

typedef enum Cell {
    CELL_EMPTY = 0,
    CELL_I,
    CELL_O,
    CELL_T,
    CELL_S,
    CELL_Z,
    CELL_J,
    CELL_L,

    CELL_COUNT
} Cell;

typedef Cell Grid[GRID_NUM_ROWS][GRID_NUM_ROWS];

typedef struct Piece {
    Cell type;
    Vector2 origin;
    size_t rotation_state;
} Piece;

//////////////////////////////////////////////////////////
// Global state.
static Grid grid;
static Piece incomming_piece;
static Piece ghost_piece;
static float fall_timer;
static bool incomming_piece_blocked;
static bool incomming_piece_active;
static float lateral_movement_cooldown_ms;
static float vertical_movement_cooldown_ms;

static Piece held_piece;
static bool can_hold;

static PieceState rotation_states[CELL_COUNT][4] = {
    // CELL_EMPTY.
    {},
    // CELL_I.
    {{
         {false, false, false, false},
         {true, true, true, true},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {false, false, true, false},
         {false, false, true, false},
         {false, false, true, false},
         {false, false, true, false},
     },
     {
         {false, false, false, false},
         {false, false, false, false},
         {true, true, true, true},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {false, true, false, false},
         {false, true, false, false},
         {false, true, false, false},
     }},
    // CELL_O.
    {{
         {true, true, false, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {true, true, false, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {true, true, false, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {true, true, false, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     }},
    // CELL_T.
    {{
         {false, true, false, false},
         {true, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {false, true, true, false},
         {false, true, false, false},
         {false, false, false, false},
     },
     {
         {false, false, false, false},
         {true, true, true, false},
         {false, true, false, false},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {true, true, false, false},
         {false, true, false, false},
         {false, false, false, false},
     }},
    // CELL_S.
    {{
         {false, true, true, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {false, true, true, false},
         {false, false, true, false},
         {false, false, false, false},
     },
     {
         {false, false, false, false},
         {false, true, true, false},
         {true, true, false, false},
         {false, false, false, false},
     },
     {
         {true, false, false, false},
         {true, true, false, false},
         {false, true, false, false},
         {false, false, false, false},
     }},
    // CELL_Z.
    {{
         {true, true, false, false},
         {false, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {false, false, true, false},
         {false, true, true, false},
         {false, true, false, false},
         {false, false, false, false},
     },
     {
         {false, false, false, false},
         {true, true, false, false},
         {false, true, true, false},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {true, true, false, false},
         {true, false, false, false},
         {false, false, false, false},
     }},
    // CELL_J.
    {{
         {true, false, false, false},
         {true, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {false, true, true, false},
         {false, true, false, false},
         {false, true, false, false},
         {false, false, false, false},
     },
     {
         {false, false, false, false},
         {true, true, true, false},
         {false, false, true, false},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {false, true, false, false},
         {true, true, false, false},
         {false, false, false, false},
     }},
    // CELL_L.
    {{
         {false, false, true, false},
         {true, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {
         {false, true, false, false},
         {false, true, false, false},
         {false, true, true, false},
         {false, false, false, false},
     },
     {
         {false, false, false, false},
         {true, true, true, false},
         {true, false, false, false},
         {false, false, false, false},
     },
     {
         {true, true, false, false},
         {false, true, false, false},
         {false, true, false, false},
         {false, false, false, false},
     }},
};

// Wall kick displacements are defined in the forward direction. i.e. the wall kick data for position 0
// is for going from 0 -> 1
static Vector2 kick_offset_table[CELL_COUNT][4][5] = {
    // CELL_EMPTY.
    {{}, {}, {}, {}},
    // CELL_I.
    {
        {{0, 0}, {0, -2}, {0, 1}, {1, -2}, {-2, 1}},
        {{0, 0}, {0, -1}, {0, -2}, {-2, -1}, {1, 2}},
        {{0, 0}, {0, 2}, {0, -1}, {-1, 2}, {2, -1}},
        {{0, 0}, {0, 1}, {0, -2}, {2, 1}, {-1, -2}},
    },
    // CELL_O.
    {
        {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
        {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    },
    // CELL_T.
    {
        {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
        {{0, 0}, {0, 1}, {-1, -1}, {-2, 0}, {-2, 1}},
        {{0, 0}, {0, -1}, {-1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
    },
    // CELL_S.
    {
        {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
        {{0, 0}, {0, 1}, {-1, -1}, {-2, 0}, {-2, 1}},
        {{0, 0}, {0, -1}, {-1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
    },
    // CELL_Z.
    {
        {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
        {{0, 0}, {0, 1}, {-1, -1}, {-2, 0}, {-2, 1}},
        {{0, 0}, {0, -1}, {-1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
    },
    // CELL_J.
    {
        {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
        {{0, 0}, {0, 1}, {-1, -1}, {-2, 0}, {-2, 1}},
        {{0, 0}, {0, -1}, {-1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
    },
    // CELL_L.
    {
        {{0, 0}, {0, -1}, {-1, -1}, {2, 0}, {2, -1}},
        {{0, 0}, {0, 1}, {-1, -1}, {-2, 0}, {-2, 1}},
        {{0, 0}, {0, -1}, {-1, 1}, {2, 0}, {2, 1}},
        {{0, 0}, {0, -1}, {1, -1}, {-2, 0}, {-2, -1}},
    },
};

//////////////////////////////////////////////////////////

static void GridInit() { memset(grid, CELL_EMPTY, GRID_NUM_CELLS * sizeof(Cell)); }

static void GridMarkMask(PieceState mask, Vector2 origin, Cell type) {
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            if (!mask[i][j]) {
                continue;
            }
            size_t x = origin.x + i;
            size_t y = origin.y + j;
            grid[x][y] = type;
        }
    }
}

static void SpawnPiece() {
    // static Cell type = CELL_I;
    can_hold = true;
    incomming_piece_active = true;
    Cell type = (Cell)GetRandomValue(CELL_EMPTY + 1, CELL_COUNT - 1);
    incomming_piece = (Piece){.type = type, .origin = {0, 5}, .rotation_state = 0};
    // ++type;

    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static bool GridCheckMaskBlocked(PieceState mask, Vector2 origin) {
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            if (!mask[i][j]) {
                continue;
            }

            size_t x = origin.x + i;
            size_t y = origin.y + j;
            if (x < 0 || y < 0 || x >= GRID_NUM_ROWS || y >= GRID_NUM_COLS || grid[x][y]) {
                return false;
            }
        }
    }
    return true;
}

static void MovePiece(Vector2 direction) {
    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);

    // Verify we did not collide with something before marking the grid.
    Vector2 new_origin = Vector2Add(incomming_piece.origin, direction);
    bool can_move = GridCheckMaskBlocked(*mask, new_origin);
    if (can_move) {
        incomming_piece.origin = new_origin;
    }

    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static void RotatePiece() {
    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);

    size_t new_rotation_state = (incomming_piece.rotation_state + 1) % 4;
    Vector2 new_origin = incomming_piece.origin;
    mask = &rotation_states[incomming_piece.type][new_rotation_state];
    Vector2 *kicks = kick_offset_table[incomming_piece.type][incomming_piece.rotation_state];
    for (size_t i = 0; i < 5; ++i) {
        Vector2 kick = kicks[i];
        new_origin = Vector2Add(incomming_piece.origin, kick);
        if (GridCheckMaskBlocked(*mask, new_origin)) {
            incomming_piece.rotation_state = new_rotation_state;
            incomming_piece.origin = new_origin;
            break;
        }
    }

    mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static bool CheckPieceIsBlockedVertical() {
    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);
    Vector2 new_origin = Vector2Add(incomming_piece.origin, (Vector2){.x = 1.0f, .y = 0.0f});
    bool can_move = GridCheckMaskBlocked(*mask, new_origin);
    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
    return !can_move;
}

static void ResolveVerticalMovement(float delta_time) {
    vertical_movement_cooldown_ms -= delta_time;
    vertical_movement_cooldown_ms = vertical_movement_cooldown_ms <= 0.0f ? 0.0f : vertical_movement_cooldown_ms;

    bool land_block = IsKeyPressed(KEY_SPACE);
    bool user_triggered_fall = land_block || (vertical_movement_cooldown_ms <= 0.0f && IsKeyDown(KEY_DOWN));

    fall_timer -= delta_time;
    bool should_fall = fall_timer <= 0.0f || user_triggered_fall;

    if (should_fall) {
        fall_timer = 1.0f;
        incomming_piece_active = !(incomming_piece_blocked || land_block);
        for (size_t i = land_block ? GRID_NUM_ROWS : 1; i > 0; --i) {
            // NOTE: This move could fail if the piece is blocked, thats okay.
            MovePiece((Vector2){.x = 1.0f, .y = 0.0f});
        }
        vertical_movement_cooldown_ms =
            user_triggered_fall ? VERTICAL_MOVEMENT_COOLDOWN_MS_INIT : vertical_movement_cooldown_ms;
    }
}

static void CalculateGhostPiece() {
    if (!incomming_piece_active) {
        return;
    }

    ghost_piece = incomming_piece;

    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);

    Vector2 new_origin = ghost_piece.origin;
    do {
        ghost_piece.origin = new_origin;
        new_origin = Vector2Add(ghost_piece.origin, (Vector2){.x = 1, .y = 0});
    } while (GridCheckMaskBlocked(*mask, new_origin));

    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetTargetFPS(60);

    fall_timer = 1.0f;
    incomming_piece_blocked = false;
    incomming_piece_active = false;
    lateral_movement_cooldown_ms = 0.0f;
    vertical_movement_cooldown_ms = 0.0f;

    // We init with CELL_EMPTY to denote that nothing is being held at the moment.
    held_piece = (Piece){.origin = (Vector2){0, 0}, .rotation_state = 0, .type = CELL_EMPTY};
    can_hold = true;

    GridInit();
    SpawnPiece();
}

static void GridClearLines() {
    if (incomming_piece_active) {
        return;
    }

    for (int i = GRID_NUM_ROWS - 1; i >= 0;) {
        bool line_clear = true;
        for (size_t j = 0; j < GRID_NUM_COLS; ++j) {
            if (grid[i][j] == CELL_EMPTY) {
                line_clear = false;
                break;
            }
        }

        if (!line_clear) {
            --i;
            continue;
        }

        for (int r = i; r > 0; --r) {
            for (size_t j = 0; j < GRID_NUM_COLS; ++j) {
                grid[r][j] = grid[r - 1][j];
            }
        }
        for (size_t j = 0; j < GRID_NUM_COLS; ++j) {
            grid[0][j] = CELL_EMPTY;
        }
    }
}

static void Update(float delta_time) {
    if (!incomming_piece_active) {
        SpawnPiece();
    }

    if (can_hold && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_C))) {
        // TODO: All this grid mask marking stuff is kind of tedious. Consider cleaning up at some point.
        Piece tmp = incomming_piece;
        PieceState *mask = &rotation_states[tmp.type][tmp.rotation_state];
        GridMarkMask(*mask, tmp.origin, CELL_EMPTY);

        SpawnPiece();
        mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
        GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);

        incomming_piece.type = held_piece.type == CELL_EMPTY ? incomming_piece.type : held_piece.type;
        held_piece = tmp;
        vertical_movement_cooldown_ms = VERTICAL_MOVEMENT_COOLDOWN_MS_INIT;
        can_hold = false;

        mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
        GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
    }

    lateral_movement_cooldown_ms -= delta_time;
    lateral_movement_cooldown_ms = lateral_movement_cooldown_ms <= 0.0f ? 0.0f : lateral_movement_cooldown_ms;
    if (lateral_movement_cooldown_ms <= 0.0f) {
        lateral_movement_cooldown_ms = LATERAL_MOVEMEMENT_COOLDOWN_MS_INIT;
        // TODO: Need to make movement a bit slower. Like need to buildup the lateral speed.
        Vector2 lateral_movement = (Vector2){.x = 0, .y = 0};
        if (IsKeyDown(KEY_RIGHT)) {
            lateral_movement = Vector2Add(lateral_movement, (Vector2){.x = 0.0f, .y = 1.0f});
        }
        if (IsKeyDown(KEY_LEFT)) {
            lateral_movement = Vector2Add(lateral_movement, (Vector2){.x = 0.0f, .y = -1.0f});
        }

        MovePiece(lateral_movement);
    }

    if (IsKeyPressed(KEY_UP)) {
        RotatePiece();
    }

    incomming_piece_blocked = CheckPieceIsBlockedVertical();

    ResolveVerticalMovement(delta_time);
    CalculateGhostPiece();
    GridClearLines();
}

static Color MapColor(Cell cell) {
    Color color;
    if (cell == CELL_EMPTY) {
        color = BLACK;
    } else if (cell == CELL_I) {
        color = (Color){.r = 0, .g = 255, .b = 255, .a = 255};
    } else if (cell == CELL_O) {
        color = YELLOW;
    } else if (cell == CELL_T) {
        color = PURPLE;
    } else if (cell == CELL_S) {
        color = GREEN;
    } else if (cell == CELL_Z) {
        color = RED;
    } else if (cell == CELL_J) {
        color = BLUE;
    } else if (cell == CELL_L) {
        color = ORANGE;
    }
    return color;
}

static void GridDraw() {
    size_t color_count = 0;
    for (size_t i = 0; i < GRID_NUM_ROWS; ++i, ++color_count) {
        for (size_t j = 0; j < GRID_NUM_COLS; ++j, ++color_count) {
            Cell cell = grid[i][j];
            Color color = MapColor(cell);
            // Color color = color_count % 2 == 0 ? BLACK : RED;
            DrawRectangle(j * CELL_WIDTH, i * CELL_WIDTH, CELL_WIDTH, CELL_WIDTH, color);
        }
    }
}

static void DrawGhostPiece() {
    Color color = MapColor(ghost_piece.type);
    PieceState *mask = &rotation_states[ghost_piece.type][ghost_piece.rotation_state];

    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            if (!(*mask)[i][j]) {
                continue;
            }

            size_t x = ghost_piece.origin.x + i;
            size_t y = ghost_piece.origin.y + j;
            DrawRectangleLines(y * CELL_WIDTH, x * CELL_WIDTH, CELL_WIDTH, CELL_WIDTH, color);
        }
    }
}

static void Draw() {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    GridDraw(&grid);
    DrawGhostPiece();

    EndDrawing();
}

static void GameClose() { CloseWindow(); }

int main(void) {
    GameInit();
    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Update(delta_time);
        Draw();
    }
    GameClose();
    return 0;
}