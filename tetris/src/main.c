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
     {},
     {},
     {}},
    // CELL_O.
    {{
         {true, true, false, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {},
     {},
     {}},
    // CELL_T.
    {{
         {false, true, false, false},
         {true, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {},
     {},
     {}},
    // CELL_S.
    {{
         {false, true, true, false},
         {true, true, false, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {},
     {},
     {}},
    // CELL_Z.
    {{
         {true, true, false, false},
         {false, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {},
     {},
     {}},
    // CELL_J.
    {{
         {true, false, false, false},
         {true, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {},
     {},
     {}},
    // CELL_L.
    {{
         {false, false, true, false},
         {true, true, true, false},
         {false, false, false, false},
         {false, false, false, false},
     },
     {},
     {},
     {}},
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
    incomming_piece_active = true;
    Cell type = (Cell)GetRandomValue(CELL_EMPTY + 1, CELL_COUNT - 1);
    incomming_piece = (Piece){.type = type, .origin = {0, 5}, .rotation_state = 0};

    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static void MovePiece(Vector2 direction) {
    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);

    incomming_piece.origin = Vector2Add(incomming_piece.origin, direction);

    // Verify we did not collide with something before marking the grid.
    bool valid = true;
    for (size_t i = 0; i < 4; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            if (!(*mask)[i][j]) {
                continue;
            }

            size_t x = incomming_piece.origin.x + i;
            size_t y = incomming_piece.origin.y + j;
            if (x < 0 || y < 0 || x >= GRID_NUM_ROWS || y >= GRID_NUM_COLS || grid[x][y]) {
                valid = false;
                break;
            }
        }
    }

    if (!valid) {
        incomming_piece.origin = Vector2Subtract(incomming_piece.origin, direction);
    }

    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static bool CheckPieceIsBlockedVertical() {
    bool mask[5][4];
    memset(mask, false, 5 * 4 * sizeof(bool));
    for (size_t j = 0; j < 4; ++j) {
        // Mark the top of the collumn as being needed to check. Really this covers if the
        // incomming piece is an I.
        mask[4][j] = true;
        // NOTE: int vs size_t important because size_t is unsigned and will wrap around.
        for (int i = 3; i >= 0; --i) {
            if (rotation_states[incomming_piece.type][incomming_piece.rotation_state][i][j]) {
                break;
            }
            mask[i][j] = true;
            mask[i + 1][j] = false;
        }
        // In case there were no part of piece in this column, just wipe the mask for the column.
        mask[0][j] = false;
    }

    for (size_t i = 0; i < 5; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            size_t x = incomming_piece.origin.x + i;
            size_t y = incomming_piece.origin.y + j;
            if (!mask[i][j] || y >= GRID_NUM_COLS) {
                continue;
            } else if (x >= GRID_NUM_ROWS || grid[x][y] != CELL_EMPTY) {
                return true;
            }
        }
    }

    return false;
}

static void ResolveVerticalMovement(float delta_time) {
    if (incomming_piece_blocked) {
        incomming_piece_active = false;
    } else {
        vertical_movement_cooldown_ms -= delta_time;
        vertical_movement_cooldown_ms = vertical_movement_cooldown_ms <= 0.0f ? 0.0f : vertical_movement_cooldown_ms;

        bool user_triggered_fall = vertical_movement_cooldown_ms <= 0.0f && IsKeyDown(KEY_DOWN);

        fall_timer -= delta_time;
        bool should_fall = fall_timer <= 0.0f || user_triggered_fall;

        if (should_fall) {
            fall_timer = 1.0f;
            MovePiece((Vector2){.x = 1.0f, .y = 0.0f});
            vertical_movement_cooldown_ms =
                user_triggered_fall ? VERTICAL_MOVEMENT_COOLDOWN_MS_INIT : vertical_movement_cooldown_ms;
            // TODO: I don't believe I need this since we always check is blocked after horizontal movement and before
            // vertical movement.
            // incomming_piece_blocked = CheckPieceIsBlockedVertical();
        }
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
    incomming_piece_blocked = CheckPieceIsBlockedVertical();

    ResolveVerticalMovement(delta_time);
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