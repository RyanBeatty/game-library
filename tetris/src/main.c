#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define GRID_NUM_ROWS (20)
#define GRID_NUM_COLS (10)
#define GRID_NUM_CELLS (GRID_NUM_ROWS * GRID_NUM_COLS)
#define CELL_WIDTH (25)
#define ScreenWidth (GRID_NUM_COLS * CELL_WIDTH)
#define ScreenHeight (GRID_NUM_ROWS * CELL_WIDTH)

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
    Vector2 offsets[4];
    size_t rotation_state;
} Piece;

//////////////////////////////////////////////////////////
// Global state.
static Grid grid;
static Piece incomming_piece;
static float fall_timer;

static PieceState rotation_states[CELL_COUNT][4] = {
    {},
    {},
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
    {},
    {},
    {},
    {},
    {},
};

// static PieceState vertical_blocking_states[CELL_COUNT][4] = {
//     {},
//     {},
//     // CELL_O.
//     {{
//          {false, false, false, false},
//          {false, false, false, false},
//          {true, true, false, false},
//          {true, true, false, false},
//      },
//      {},
//      {},
//      {}},
//     {},
//     {},
//     {},
//     {},
//     {},
// };

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
    incomming_piece =
        (Piece){.type = CELL_O, .origin = {0, 5}, .offsets = {{0, 0}, {0, 1}, {1, 1}, {1, 0}}, .rotation_state = 0};

    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static void MovePiece(Vector2 direction) {
    PieceState *mask = &rotation_states[incomming_piece.type][incomming_piece.rotation_state];
    GridMarkMask(*mask, incomming_piece.origin, CELL_EMPTY);

    // TODO: Need to make the wall collisions more robust.
    incomming_piece.origin = Vector2Add(incomming_piece.origin, direction);
    incomming_piece.origin.y = Clamp(incomming_piece.origin.y, 0.0f, GRID_NUM_COLS - 3);

    GridMarkMask(*mask, incomming_piece.origin, incomming_piece.type);
}

static void GameInit() {
    InitWindow(ScreenWidth, ScreenHeight, "Game");
    SetTargetFPS(60);

    GridInit();
    SpawnPiece();

    fall_timer = 1.0f;
}

static void Update(float delta_time) {
    fall_timer -= delta_time;
    bool should_fall = fall_timer <= 0.0f;

    if (should_fall) {
        fall_timer = 1.0f;
        MovePiece((Vector2){.x = 1.0f, .y = 0.0f});
    }

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