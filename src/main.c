#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>

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

// Configurable visuals/gameplay
static int enableParticleEffects = 1; // 1: show particles, 0: disable
static float cascadeSpeedPxPerSec = 600.0f; // tile fall speed in px/s

// Simple particle system for match effects
typedef struct Particle {
    float x;
    float y;
    float vx;
    float vy;
    float size;
    float age;
    float life;
    unsigned char r, g, b, a;
    int active;
} Particle;

#define MAX_PARTICLES 4096
static Particle particles[MAX_PARTICLES];
// Burst events (logical cell positions) to spawn particles next frame when we know offsets
typedef struct BurstEvent {
    int row;
    int col;
    int tileType;
    int active;
} BurstEvent;

#define MAX_BURST_EVENTS 512
static BurstEvent burstEvents[MAX_BURST_EVENTS];

static void resetParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = 0;
}

static void resetBurstEvents(void)
{
    for (int i = 0; i < MAX_BURST_EVENTS; i++) burstEvents[i].active = 0;
}

static void spawnParticle(float x, float y, Color color)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].active = 1;
            particles[i].x = x;
            particles[i].y = y;
            const float speed = (float)GetRandomValue(30, 140) / 100.0f; // 0.3..1.4
            const float angle = (float)GetRandomValue(0, 360) * (3.1415926f / 180.0f);
            particles[i].vx = cosf(angle) * speed * (float)tileSize * 2.0f;
            particles[i].vy = sinf(angle) * speed * (float)tileSize * 2.0f;
            particles[i].size = (float)GetRandomValue(6, 12);
            particles[i].age = 0.0f;
            particles[i].life = (float)GetRandomValue(300, 600) / 1000.0f; // 0.3..0.6s
            particles[i].r = color.r;
            particles[i].g = color.g;
            particles[i].b = color.b;
            particles[i].a = 255;
            return;
        }
    }
}

static void spawnBurstAtCell(int row, int col, int offsetX, int offsetY, int tileType)
{
    const int cx = offsetX + gridMargin + col * (tileSize + tileSpacing) + tileSize / 2;
    const int cy = offsetY + gridMargin + row * (tileSize + tileSpacing) + tileSize / 2;
    const Color base = tileColors[tileType % 4];
    const int count = 14;
    for (int i = 0; i < count; i++) spawnParticle((float)cx, (float)cy, base);
}

static void enqueueBurstEvent(int row, int col, int tileType)
{
    for (int i = 0; i < MAX_BURST_EVENTS; i++) {
        if (!burstEvents[i].active) {
            burstEvents[i].active = 1;
            burstEvents[i].row = row;
            burstEvents[i].col = col;
            burstEvents[i].tileType = tileType;
            return;
        }
    }
}

static void processBurstEvents(int offsetX, int offsetY)
{
    for (int i = 0; i < MAX_BURST_EVENTS; i++) {
        if (!burstEvents[i].active) continue;
        spawnBurstAtCell(burstEvents[i].row, burstEvents[i].col, offsetX, offsetY, burstEvents[i].tileType);
        burstEvents[i].active = 0;
    }
}

static void updateAndDrawParticles(float dt)
{
    const float gravity = 900.0f; // pixels/s^2
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        particles[i].age += dt;
        if (particles[i].age >= particles[i].life) {
            particles[i].active = 0;
            continue;
        }
        particles[i].vy += gravity * dt * 0.5f;
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;

        float t = particles[i].age / particles[i].life;
        unsigned char alpha = (unsigned char)(255.0f * (1.0f - t));
        Color c = { particles[i].r, particles[i].g, particles[i].b, alpha };
        DrawCircleV((Vector2){ particles[i].x, particles[i].y }, particles[i].size * (1.0f - 0.7f * t), c);
    }
}

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

// Forward declarations for collapse helpers used before their definitions
static void enqueueBurstsForMarked(const int tiles[gridRows][gridColumns], const int marked[gridRows][gridColumns]);
static void prepareCollapseAndAssign(int tiles[gridRows][gridColumns], const int marked[gridRows][gridColumns]);
// Animation flag must be visible to resolveBoard()
static int isAnimating = 0;

static void removeMatchesAndCollapse(int tiles[gridRows][gridColumns], const int marked[gridRows][gridColumns])
{
    // Enqueue bursts and then prepare collapse with fall offsets
    enqueueBurstsForMarked(tiles, marked);
    prepareCollapseAndAssign(tiles, marked);
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
        // Signal animation start
        isAnimating = 1;
        break; // play this step; next steps trigger after tiles land
    }
    return totalCleared;
}

// Falling animation state
static float fallOffset[gridRows][gridColumns]; // negative values animate up to 0
// pixels per second; overridden by cascadeSpeedPxPerSec config
static const float FALL_SPEED = 1200.0f;

static void clearFallOffsets(void)
{
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridColumns; c++) {
            fallOffset[r][c] = 0.0f;
        }
    }
}

static int anyTilesFalling(void)
{
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridColumns; c++) {
            if (fallOffset[r][c] < -0.001f) return 1;
        }
    }
    return 0;
}

static void enqueueBurstsForMarked(const int tiles[gridRows][gridColumns], const int marked[gridRows][gridColumns])
{
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridColumns; c++) {
            if (marked[r][c]) enqueueBurstEvent(r, c, tiles[r][c]);
        }
    }
}

// Prepare collapse result into-place and set fall offsets so tiles visually drop
static void prepareCollapseAndAssign(int tiles[gridRows][gridColumns], const int marked[gridRows][gridColumns])
{
    const int cellH = tileSize + tileSpacing;
    for (int r = 0; r < gridRows; r++) {
        for (int c = 0; c < gridColumns; c++) fallOffset[r][c] = 0.0f;
    }
    for (int c = 0; c < gridColumns; c++) {
        int writeRow = gridRows - 1;
        for (int r = gridRows - 1; r >= 0; r--) {
            if (!marked[r][c]) {
                int srcRow = r;
                int dstRow = writeRow;
                tiles[dstRow][c] = tiles[srcRow][c];
                int fallCells = dstRow - srcRow;
                fallOffset[dstRow][c] = -((float)fallCells * (float)cellH);
                writeRow--;
            }
        }
        for (int dstRow = writeRow; dstRow >= 0; dstRow--) {
            tiles[dstRow][c] = GetRandomValue(0, 3);
            int steps = (writeRow - dstRow + 1);
            fallOffset[dstRow][c] = -((float)steps * (float)cellH);
        }
    }
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

    resetParticles();
    resetBurstEvents();
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

        // Draw grid of tiles (include fall offsets during animation)
        for (int row = 0; row < gridRows; row++) {
            for (int col = 0; col < gridColumns; col++) {
                const int x = offsetX + gridMargin + col * (tileSize + tileSpacing);
                int y = offsetY + gridMargin + row * (tileSize + tileSpacing);
                if (fallOffset[row][col] < 0.0f) {
                    y += (int)fallOffset[row][col];
                }
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

        // Process any burst events enqueued by matches and update/draw particles
        if (enableParticleEffects) {
            processBurstEvents(offsetX, offsetY);
        } else {
            // clear any queued bursts if effects are disabled
            resetBurstEvents();
        }
        const float dt = GetFrameTime();
        if (enableParticleEffects) {
            updateAndDrawParticles(dt);
        }

        // Advance fall animation and trigger next cascade step when finished
        if (isAnimating) {
            int allDone = 1;
            for (int r = 0; r < gridRows; r++) {
                for (int c = 0; c < gridColumns; c++) {
                    if (fallOffset[r][c] < 0.0f) {
                        const float speed = (cascadeSpeedPxPerSec > 0.0f) ? cascadeSpeedPxPerSec : FALL_SPEED;
                        fallOffset[r][c] += speed * dt;
                        if (fallOffset[r][c] > 0.0f) fallOffset[r][c] = 0.0f;
                        allDone = 0;
                    }
                }
            }
            if (allDone) {
                isAnimating = 0;
                // After tiles settled, check for further matches
                (void)resolveBoard(tileTypeByCell);
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
