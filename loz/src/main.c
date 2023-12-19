#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

static const int ppu = 16;

static const int OriginalWidth = 256;
static const int OriginalHeight = 240;
static const int ScalingFactor = 2;
#define SCALE(x) ((x)*ScalingFactor)
#define ScalingFactor (2)
#define RenderWidth (OriginalWidth * ScalingFactor)
#define RenderHeight (OriginalHeight * ScalingFactor)

#define OriginalWidthTiles (OriginalWidth / ppu)
#define OriginalHeightTiles (OriginalHeight / ppu)

static Texture desert_atlas;

static const uint8_t start_screen[11][16] = {
    {54, 54, 54, 54, 54, 54, 54, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 54, 54, 54, 12, 54, 55, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 54, 54, 55, 23, 23, 23, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 54, 55, 23, 23, 23, 23, 23, 23, 54, 54, 54, 54, 54, 54, 54},
    {54, 55, 23, 23, 23, 23, 23, 23, 23, 53, 54, 54, 54, 54, 54, 54},
    {23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23},
    {52, 52, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 52, 52},
    {54, 54, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 54, 54},
    {54, 54, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 54, 54},
    {54, 54, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 54, 54},
    {54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54},
};

static void TileMapDrawDebug() {
    int idx = 0;
    for (size_t i = 0; i < OriginalHeightTiles; ++i, ++idx) {
        for (size_t j = 0; j < OriginalWidthTiles; ++j, ++idx) {
            Color color = idx % 2 == 0 ? BLACK : RED;
            DrawRectangleV((Vector2){SCALE(j * ppu), SCALE(i * ppu)},
                           (Vector2){SCALE((j * ppu) + ppu), SCALE((i * ppu) + ppu)}, color);
        }
    }
}

static void TileMapDraw() {
    int offset = ppu * 4;
    for (size_t i = 0; i < OriginalHeightTiles; ++i) {
        for (size_t j = 0; j < OriginalWidthTiles; ++j) {
            uint8_t tile_idx = start_screen[i][j];
            int x = (tile_idx % 8) * ppu;
            int y = (tile_idx / 8) * ppu;
            DrawTexturePro(
                desert_atlas, (Rectangle){.x = x, .y = y, .width = ppu, .height = ppu},
                (Rectangle){
                    .x = SCALE(j * ppu), .y = SCALE((i * ppu) + offset), .width = SCALE(ppu), .height = SCALE(ppu)},
                (Vector2){0, 0}, 0, WHITE);
        }
    }
}

static void GameInit() {
    InitWindow(RenderWidth, RenderHeight, "Game");
    SetTargetFPS(60);

    desert_atlas = LoadTexture("assets/textures/desert_atlas.png");
    assert(IsTextureReady(desert_atlas));
    // SetTextureFilter(desert_atlas, TE)
}

static void GameUpdate() {}

static void GameDraw() {
    BeginDrawing();

    ClearBackground(BLACK);

    if (false) {
        TileMapDrawDebug();
    }

    TileMapDraw();

    EndDrawing();
}

static void GameLoop() {
    float delta_time = GetFrameTime();
    GameUpdate();
    GameDraw();
}

static void GameClose() { CloseWindow(); }

int main(void) {
    GameInit();
    while (!WindowShouldClose()) {
        GameLoop();
    }
    GameClose();

    return 0;
}