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

// Match-3 helpers
static int detectMatches(const int tiles[gridRows][gridColumns], int marked[gridRows][gridColumns])
{
    // Clear marked
    int totalMarked = 0;
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridColumns; c++) {
            marked[r][c] = 0;
        }
    }

    // Horizontal runs
    for (int r = 0; r < gridRows; r++) {
        int runStart = 0;
        while (runStart < gridColumns) {
            const int type = tiles[r][runStart];
            int runEnd = runStart + 1;
            while (runEnd < gridColumns && tiles[r][runEnd] == type) {
                runEnd++;
            }
            const int runLength = runEnd - runStart;
            if (type >= 0 && runLength >= 3) {
                for (int c = runStart; c < runEnd; c++) {
                    if (!marked[r][c]) { marked[r][c] = 1; totalMarked++; }
                }
            }
            runStart = runEnd;
        }
    }

    // Vertical runs
    for (int c = 0; c < gridColumns; c++) {
        int runStart = 0;
        while (runStart < gridRows) {
            const int type = tiles[runStart][c];
            int runEnd = runStart + 1;
            while (runEnd < gridRows && tiles[runEnd][c] == type) {
                runEnd++;
            }
            const int runLength = runEnd - runStart;
            if (type >= 0 && runLength >= 3) {
                for (int r = runStart; r < runEnd; r++) {
                    if (!marked[r][c]) { marked[r][c] = 1; totalMarked++; }
                }
            }
            runStart = runEnd;
        }
    }

    return totalMarked;
}

static void removeMatchesAndCollapse(int tiles[gridRows][gridColumns], const int marked[gridRows][gridColumns])
{
    for (int c = 0; c < gridColumns; c++) {
        int writeRow = gridRows - 1;
        for (int r = gridRows - 1; r >= 0; r--) {
            if (!marked[r][c]) {
                tiles[writeRow][c] = tiles[r][c];
                writeRow--;
            }
        }
        // Fill remaining with new random tiles at the top
        for (int r = writeRow; r >= 0; r--) {
            tiles[r][c] = GetRandomValue(0, 3);
        }
    }
}

static int resolveBoard(int tiles[gridRows][gridColumns])
{
    int totalCleared = 0;
    while (1) {
        int marked[gridRows][gridColumns];
        const int cleared = detectMatches(tiles, marked);
        if (cleared <= 0) break;
        totalCleared += cleared;
        removeMatchesAndCollapse(tiles, marked);
    }
    return totalCleared;
}

static void initBoardNoMatches(int tiles[gridRows][gridColumns])
{
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridColumns; c++) {
            while (1) {
                int t = GetRandomValue(0, 3);
                int invalid = 0;
                // Check horizontal: ensure not forming 3 with the 2 to the left
                if (c >= 2 && tiles[r][c-1] == t && tiles[r][c-2] == t) invalid = 1;
                // Check vertical: ensure not forming 3 with the 2 above
                if (!invalid && r >= 2 && tiles[r-1][c] == t && tiles[r-2][c] == t) invalid = 1;
                if (!invalid) { tiles[r][c] = t; break; }
            }
        }
    }
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1920, 1080, "Open Match");
    // Normalize initial window/mouse mapping to avoid pre-resize mismatch
    SetWindowSize(1920, 1080);
    SetMouseOffset(0, 0);
    SetMouseScale(1.0f, 1.0f);
    SetTargetFPS(60);

    // Initialize grid with 4 different tile types (0..3) without initial matches
    srand((unsigned int)time(NULL));
    int tileTypeByCell[gridRows][gridColumns];
    initBoardNoMatches(tileTypeByCell);

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
                // If dragged outside and over an adjacent tile, attempt swap
                if (movedOutsideOriginal && hoveredRow != -1 && hoveredCol != -1) {
                    const int drow = hoveredRow - pressedRow;
                    const int dcol = hoveredCol - pressedCol;
                    const int manhattan = (drow < 0 ? -drow : drow) + (dcol < 0 ? -dcol : dcol);
                    if (manhattan == 1) {
                        // Try swap
                        int tmp = tileTypeByCell[pressedRow][pressedCol];
                        tileTypeByCell[pressedRow][pressedCol] = tileTypeByCell[hoveredRow][hoveredCol];
                        tileTypeByCell[hoveredRow][hoveredCol] = tmp;

                        // Validate swap by resolving matches; revert if no matches cleared
                        const int cleared = resolveBoard(tileTypeByCell);
                        if (cleared <= 0) {
                            // Revert swap
                            tmp = tileTypeByCell[pressedRow][pressedCol];
                            tileTypeByCell[pressedRow][pressedCol] = tileTypeByCell[hoveredRow][hoveredCol];
                            tileTypeByCell[hoveredRow][hoveredCol] = tmp;
                        }
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


