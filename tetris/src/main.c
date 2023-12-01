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
static float fall_timer;
static bool incomming_piece_blocked;
static bool incomming_piece_active;
static float lateral_movement_cooldown_ms;
static float vertical_movement_cooldown_ms;

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
    static Cell type = CELL_I;
    incomming_piece_active = true;
    // Cell type = (Cell)GetRandomValue(CELL_EMPTY + 1, CELL_COUNT - 1);
    // Cell type = CELL_T;
    ++type;
    incomming_piece = (Piece){.type = type, .origin = {0, 5}, .rotation_state = 0};

    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static bool GridCheckMaskBlocked(PieceState mask, Vector2 origin) {
    bool valid = true;
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            if (!mask[i][j]) {
                continue;
            }

            size_t x = origin.x + i;
            size_t y = origin.y + j;
            if (x < 0 || y < 0 || x >= GRID_NUM_ROWS || y >= GRID_NUM_COLS || grid[x][y]) {
                valid = false;
                break;
            }
        }
    }
    return valid;
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

    // TODO: Check if can perform rotation/wallkicks.
    incomming_piece.rotation_state = (incomming_piece.rotation_state + 1) % 4;
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

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetTargetFPS(60);

    fall_timer = 1.0f;
    incomming_piece_blocked = false;
    incomming_piece_active = false;
    lateral_movement_cooldown_ms = 0.0f;
    vertical_movement_cooldown_ms = 0.0f;

    GridInit();
    SpawnPiece();
}

static void GridClearLines() {
    if (incomming_piece_active) {
        return;
    }

    for (int i = GRID_NUM_ROWS - 1; i >= 0; --i) {
        bool line_clear = true;
        for (size_t j = 0; j < GRID_NUM_COLS; ++j) {
            if (grid[i][j] == CELL_EMPTY) {
                line_clear = false;
                break;
            }
        }

        if (line_clear) {
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
}

static void Update(float delta_time) {
    if (!incomming_piece_active) {
        SpawnPiece();
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
    GridClearLines();
}

static void GridDraw() {
    size_t color_count = 0;
    for (size_t i = 0; i < GRID_NUM_ROWS; ++i, ++color_count) {
        for (size_t j = 0; j < GRID_NUM_COLS; ++j, ++color_count) {
            Color color;
            Cell cell = grid[i][j];
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
            // Color color = color_count % 2 == 0 ? BLACK : RED;
            DrawRectangle(j * CELL_WIDTH, i * CELL_WIDTH, CELL_WIDTH, CELL_WIDTH, color);
        }
    }
}

static void Draw() {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    GridDraw(&grid);

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