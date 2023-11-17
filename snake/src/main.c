#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define GridNumRows (50)
#define GridNumCols (50)
#define GridNumCells (GridNumRows * GridNumCols)
#define CellSizePixels (10)
#define ScreenWidth (GridNumCols * CellSizePixels)
#define ScreenHeight (GridNumRows * CellSizePixels)

typedef enum Cell {
    CELL_EMPTY = 0,
    CELL_SNAKE = 1,
    CELL_FOOD = 2,

    CELL_COUNT
} Cell;

typedef struct Snake {
    size_t body[GridNumRows * GridNumCols];
    size_t size;
} Snake;

static void SnakeInit(Snake* snake) {
    memset(snake->body, 0, GridNumRows * GridNumCols);
    size_t start = (GridNumCells / 2) + (GridNumCols / 2);
    snake->body[0] = start;
    snake->body[1] = start + 1;
    snake->size = 2;
}

typedef struct Grid {
    Cell cells[GridNumRows * GridNumCols];
} Grid;

static void GridInit(Grid* grid, const Snake snake) {
    memset(grid->cells, CELL_EMPTY, GridNumRows * GridNumCols);

    for (size_t i = 0; i < snake.size; ++i) {
        size_t body = snake.body[i];
        grid->cells[body] = CELL_SNAKE;
    }
}

static void DrawGameGrid(Grid grid) {
    int c = 0;
    for (size_t i = 0; i < GridNumRows; ++i) {
        for (size_t j = 0; j < GridNumCols; ++j, ++c) {
            Color color = WHITE;
            Cell cell = grid.cells[(i * GridNumRows) + j];
            if (cell == CELL_EMPTY) {
                color = WHITE;
            } else if (cell == CELL_SNAKE) {
                color = RED;
            }
            // color = c % 2 == 0 ? BLACK : RED;
            DrawRectangle(j * CellSizePixels, i * CellSizePixels, CellSizePixels, CellSizePixels, color);
            char text[10];
            sprintf(text, "%ld", (i * GridNumRows) + j);
            // DrawText(text, j * CellSizePixels, i * CellSizePixels, CellSizePixels, WHITE);
        }
        ++c;
    }
}

static void Draw(Grid grid) {
    BeginDrawing();

    ClearBackground(RAYWHITE);
    DrawGameGrid(grid);

    EndDrawing();
}

static void Update(float delta_time) {}

int main(void) {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    Snake snake;
    SnakeInit(&snake);

    Grid grid;
    GridInit(&grid, snake);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Update(delta_time);
        Draw(grid);
    }

    CloseWindow();

    return 0;
}