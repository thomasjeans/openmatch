#include "raylib.h"
#include <stdlib.h>
#include <time.h>

enum { gridColumns = 9, gridRows = 9 };
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "Open Match");
    // Normalize initial window/mouse mapping to avoid pre-resize mismatch
    SetWindowSize(1920, 1080);
    SetMouseOffset(0, 0);
    SetMouseScale(1.0f, 1.0f);
    SetTargetFPS(60);

    // Initialize grid with 4 different tile types (0..3)
    srand((unsigned int)time(NULL));
    int tileTypeByCell[gridRows][gridColumns];
    for (int row = 0; row < gridRows; row++) {
        for (int col = 0; col < gridColumns; col++) {
            tileTypeByCell[row][col] = GetRandomValue(0, 3);
        }
    }

    // Compute board dimensions (tiles + spacing + margins)
    const int gridWidth = gridColumns * tileSize + (gridColumns - 1) * tileSpacing;
    const int gridHeight = gridRows * tileSize + (gridRows - 1) * tileSpacing;
    const int boardWidth = gridMargin * 2 + gridWidth;
    const int boardHeight = gridMargin * 2 + gridHeight;

    // Track the tile that is currently pressed/dragged (if any)
    int pressedRow = -1;
    int pressedCol = -1;
    int movedOutsideOriginal = 0; // becomes true once cursor leaves original tile rect while pressed

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground((Color){ 24, 24, 32, 255 });

        // Compute offsets to keep the board centered for the current window size
        const int windowWidth = GetScreenWidth();
        const int windowHeight = GetScreenHeight();
        const int offsetX = (windowWidth - boardWidth) / 2;
        const int offsetY = (windowHeight - boardHeight) / 2;

        // Determine mouse position (use same coordinate space as drawing)
        const Vector2 mouse = GetMousePosition();

        // Compute hovered tile using exact same calculation as drawing
        int hoveredRow = -1;
        int hoveredCol = -1;

        // Find which tile the mouse is over by checking each tile's exact rectangle
        for (int row = 0; row < gridRows && hoveredRow == -1; row++) {
            for (int col = 0; col < gridColumns && hoveredCol == -1; col++) {
                // Calculate exact tile position (same as drawing code)
                const int tileX = offsetX + gridMargin + col * (tileSize + tileSpacing);
                const int tileY = offsetY + gridMargin + row * (tileSize + tileSpacing);

                // Check if mouse is within this tile's bounds (inclusive of edges)
                if (mouse.x >= tileX && mouse.x < tileX + tileSize &&
                    mouse.y >= tileY && mouse.y < tileY + tileSize) {
                    hoveredRow = row;
                    hoveredCol = col;
                }
            }
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (hoveredRow != -1 && hoveredCol != -1) {
                pressedRow = hoveredRow;
                pressedCol = hoveredCol;
                movedOutsideOriginal = 0;
            }
        }

        // If mouse is already down and no pressed tile tracked yet, capture hovered now
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && pressedRow == -1 && pressedCol == -1) {
            if (hoveredRow != -1 && hoveredCol != -1) {
                pressedRow = hoveredRow;
                pressedCol = hoveredCol;
                movedOutsideOriginal = 0;
            }
        }

        // Update drag state based on mouse leaving the original tile rect
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && pressedRow != -1 && pressedCol != -1) {
            const int pressedX = offsetX + gridMargin + pressedCol * (tileSize + tileSpacing);
            const int pressedY = offsetY + gridMargin + pressedRow * (tileSize + tileSpacing);
            if ((mouse.x < pressedX) || (mouse.x >= pressedX + tileSize) ||
                (mouse.y < pressedY) || (mouse.y >= pressedY + tileSize)) {
                movedOutsideOriginal = 1;
            }
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (pressedRow != -1 && pressedCol != -1) {
                // On release, if dragged outside and over adjacent tile, perform swap
                if (movedOutsideOriginal && hoveredRow != -1 && hoveredCol != -1) {
                    const int drow = hoveredRow - pressedRow;
                    const int dcol = hoveredCol - pressedCol;
                    if ((drow*drow + dcol*dcol == 1) || (drow == 0 && (dcol == 1 || dcol == -1)) || (dcol == 0 && (drow == 1 || drow == -1))) {
                        // Manhattan distance 1 → adjacent
                        int tmp = tileTypeByCell[pressedRow][pressedCol];
                        tileTypeByCell[pressedRow][pressedCol] = tileTypeByCell[hoveredRow][hoveredCol];
                        tileTypeByCell[hoveredRow][hoveredCol] = tmp;
                    }
                }
            }
            pressedRow = -1;
            pressedCol = -1;
            movedOutsideOriginal = 0;
        }

        // Draw grid of tiles
        for (int row = 0; row < gridRows; row++) {
            for (int col = 0; col < gridColumns; col++) {
                const int x = offsetX + gridMargin + col * (tileSize + tileSpacing);
                const int y = offsetY + gridMargin + row * (tileSize + tileSpacing);
                const int tileType = tileTypeByCell[row][col];
                const Color color = tileColors[tileType % 4];
                const bool isPressedTile = (row == pressedRow && col == pressedCol && IsMouseButtonDown(MOUSE_BUTTON_LEFT));
                if (isPressedTile) {
                    if (movedOutsideOriginal) {
                        // While dragging outside original tile, skip drawing here; we'll draw on top at cursor
                    } else {
                        const float scale = 1.14f;
                        const float cx = (float)x + (float)tileSize * 0.5f;
                        const float cy = (float)y + (float)tileSize * 0.5f;
                        Rectangle rec = { cx, cy, (float)tileSize * scale, (float)tileSize * scale };
                        Vector2 origin = { rec.width * 0.5f, rec.height * 0.5f };
                        Rectangle shadowRec = rec; shadowRec.x += 4.0f; shadowRec.y += 6.0f;
                        DrawRectanglePro(shadowRec, origin, 0.0f, (Color){ 0, 0, 0, 120 });
                        DrawRectanglePro(rec, origin, 0.0f, color);
                    }
                } else {
                    DrawRectangle(x, y, tileSize, tileSize, color);
                }
            }
        }

        // If dragging, render the pressed tile centered under cursor on top
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && pressedRow != -1 && pressedCol != -1 && movedOutsideOriginal) {
            const int tileType = tileTypeByCell[pressedRow][pressedCol];
            const Color color = tileColors[tileType % 4];
            const float scale = 1.14f;
            Rectangle rec = { (float)mouse.x, (float)mouse.y, (float)tileSize * scale, (float)tileSize * scale };
            Vector2 origin = { rec.width * 0.5f, rec.height * 0.5f };
            Rectangle shadowRec = rec; shadowRec.x += 4.0f; shadowRec.y += 6.0f;
            DrawRectanglePro(shadowRec, origin, 0.0f, (Color){ 0, 0, 0, 120 });
            DrawRectanglePro(rec, origin, 0.0f, color);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}


