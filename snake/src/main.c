#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define GridNumRows (50)
#define GridNumCols (50)
#define GridNumCells (GridNumRows * GridNumCols)
#define CellSizePixels (10)
#define ScreenWidth (GridNumCols * CellSizePixels)
#define ScreenHeight (GridNumRows * CellSizePixels)

#define MoveUp (-GridNumCols)
#define MoveDown (GridNumCols)
#define MoveLeft (-1)
#define MoveRight (1)

typedef enum Cell {
    CELL_EMPTY = 0,
    CELL_SNAKE = 1,
    CELL_FOOD = 2,

    CELL_COUNT
} Cell;

typedef struct Snake {
    size_t body[GridNumRows * GridNumCols];
    size_t size;
    size_t direction;
} Snake;

static void SnakeInit(Snake* snake) {
    memset(snake->body, 0, GridNumCells * sizeof(size_t));
    size_t start = (GridNumCells / 2) + (GridNumCols / 2);
    snake->body[0] = start;
    snake->body[1] = start + 1;
    snake->size = 2;
    snake->direction = MoveUp;
}

typedef struct Grid {
    Cell cells[GridNumRows * GridNumCols];
} Grid;

typedef struct Food {
    size_t location;
    size_t is_spawned;
} Food;

static void FoodInit(Food* food, const Snake snake) {
    size_t location;
    bool done = true;
    do {
        location = GetRandomValue(0, GridNumCells - 1);
        for (size_t i = 0; i < snake.size; ++i) {
            if (snake.body[i] == location) {
                done = false;
                break;
            }
        }
    } while (!done);

    food->location = location;
    food->is_spawned = true;
}

static void GridInit(Grid* grid, const Snake snake, const Food food) {
    memset(grid->cells, CELL_EMPTY, GridNumCells * sizeof(Cell));

    for (size_t i = 0; i < snake.size; ++i) {
        size_t body = snake.body[i];
        grid->cells[body] = CELL_SNAKE;
    }

    grid->cells[food.location] = CELL_FOOD;
}

static void DrawGameGrid(Grid grid) {
    int c = 0;
    for (size_t i = 0; i < GridNumRows; ++i) {
        for (size_t j = 0; j < GridNumCols; ++j, ++c) {
            Color color = WHITE;
            Cell cell = grid.cells[(i * GridNumRows) + j];
            if (cell == CELL_EMPTY) {
                color = BLACK;
            } else if (cell == CELL_SNAKE) {
                color = GREEN;
            } else if (cell == CELL_FOOD) {
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

    DrawFPS(10, 10);
    EndDrawing();
}

static void UpdateSnake(Snake* snake, Grid* grid, Food* food) {
    if (IsKeyDown(KEY_UP) && (snake->direction != MoveDown)) {
        snake->direction = MoveUp;
    } else if (IsKeyDown(KEY_DOWN) && (snake->direction != MoveUp)) {
        snake->direction = MoveDown;
    } else if (IsKeyDown(KEY_LEFT) && (snake->direction != MoveRight)) {
        snake->direction = MoveLeft;
    } else if (IsKeyDown(KEY_RIGHT) && (snake->direction != MoveLeft)) {
        snake->direction = MoveRight;
    }

    size_t old_tail = snake->body[snake->size - 1];

    for (size_t i = snake->size - 1; i > 0; --i) {
        snake->body[i] = snake->body[i - 1];
    }
    snake->body[0] += snake->direction;

    if (snake->body[0] == food->location) {
        food->is_spawned = false;
        snake->body[snake->size] = old_tail;
        ++snake->size;
    }
}

static void UpdateFood(Food* food, Snake* snake) {
    if (!food->is_spawned) {
        FoodInit(food, *snake);
    }
}

static void UpdateGrid(Grid* grid, const Snake* snake, const Food* food) { GridInit(grid, *snake, *food); }

static void Update(float delta_time, Snake* snake, Grid* grid, Food* food) {
    UpdateSnake(snake, grid, food);
    UpdateFood(food, snake);
    UpdateGrid(grid, snake, food);
}

int main(void) {
    InitWindow(ScreenWidth, ScreenHeight, "Game");

    Snake snake;
    SnakeInit(&snake);

    Food food;
    FoodInit(&food, snake);

    Grid grid;
    GridInit(&grid, snake, food);

    SetTargetFPS(10);

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        Update(delta_time, &snake, &grid, &food);
        Draw(grid);
    }

    CloseWindow();

    return 0;
}