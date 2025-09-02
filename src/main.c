#include "raylib.h"
#include <stdlib.h>
#include <time.h>

static const int gridColumns = 9;
static const int gridRows = 9;
static const int tileSize = 64;
static const int tileSpacing = 4;
static const int gridMargin = 20;

static const Color tileColors[4] = {
    RED,
    GREEN,
    BLUE,
    YELLOW
};

int main(void)
{
    // Compute window dimensions based on grid geometry
    const int gridWidth = gridColumns * tileSize + (gridColumns - 1) * tileSpacing;
    const int gridHeight = gridRows * tileSize + (gridRows - 1) * tileSpacing;
    const int windowWidth = gridMargin * 2 + gridWidth;
    const int windowHeight = gridMargin * 2 + gridHeight;

    InitWindow(windowWidth, windowHeight, "Match-3 Prototype");
    SetTargetFPS(60);

    // Initialize grid with 4 different tile types (0..3)
    srand((unsigned int)time(NULL));
    int tileTypeByCell[gridRows][gridColumns];
    for (int row = 0; row < gridRows; row++) {
        for (int col = 0; col < gridColumns; col++) {
            tileTypeByCell[row][col] = GetRandomValue(0, 3);
        }
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){ 24, 24, 32, 255 });

        // Draw grid of tiles
        for (int row = 0; row < gridRows; row++) {
            for (int col = 0; col < gridColumns; col++) {
                const int x = gridMargin + col * (tileSize + tileSpacing);
                const int y = gridMargin + row * (tileSize + tileSpacing);
                const int tileType = tileTypeByCell[row][col];
                const Color color = tileColors[tileType % 4];
                DrawRectangle(x, y, tileSize, tileSize, color);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}


