// main.cpp
#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <emscripten/fetch.h>  // Required for web requests (Emscripten only)

// =====================================================
// Global fetch-related variables
// =====================================================
#define MAX_WORDS 5
#define MAX_WORD_LEN 64

char EnemyWords[MAX_WORDS][MAX_WORD_LEN];
int WordCount = 0;
bool DataFetched = false;
bool FetchFailed = false;
bool FetchInProgress = false;

// =====================================================
// JSON parsing (for simple JSON arrays like ["one","two","three"])
// =====================================================
void ParseJSONWords(const char *json) {
    WordCount = 0;
    const char *p = json;
    while (*p && WordCount < MAX_WORDS) {
        const char *start = strchr(p, '\"');
        if (!start) break;
        const char *end = strchr(start + 1, '\"');
        if (!end) break;
        size_t len = end - (start + 1);
        if (len >= MAX_WORD_LEN) len = MAX_WORD_LEN - 1;
        strncpy(EnemyWords[WordCount], start + 1, len);
        EnemyWords[WordCount][len] = '\0';
        WordCount++;
        p = end + 1;
    }
}

// =====================================================
// Fetch Callbacks
// =====================================================
void OnFetchSuccess(emscripten_fetch_t *fetch) {
    char *buf = (char*)malloc(fetch->numBytes + 1);
    memcpy(buf, fetch->data, fetch->numBytes);
    buf[fetch->numBytes] = '\0';
    ParseJSONWords(buf);
    free(buf);
    emscripten_fetch_close(fetch);
    FetchInProgress = false;
    DataFetched = true;
}

void OnFetchError(emscripten_fetch_t *fetch) {
    emscripten_fetch_close(fetch);
    FetchInProgress = false;
    FetchFailed = true;
}

void StartFetch() {
    if (FetchInProgress) return;
    FetchInProgress = true;
    FetchFailed = false;

    const char *url = "https://random-word-api.herokuapp.com/word?number=3";

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = OnFetchSuccess;
    attr.onerror = OnFetchError;
    attr.userData = NULL;

    emscripten_fetch(&attr, url);
}

// =====================================================
// MAIN GAME
// =====================================================
int main() {
    srand(time(nullptr));

    const int screenWidth = 960;
    const int screenHeight = 600;
    const int PlayerWidth = 50;
    const int PlayerHeight = 50;
    const int EnemySize = 50;

    Rectangle ground = {0, 550, (float)screenWidth, 50};
    Rectangle leftWall = {0, 0, 50, (float)screenHeight};
    Rectangle rightWall = {screenWidth - 50, 0, 50, (float)screenHeight};
    Rectangle ceiling = {0, 0, (float)screenWidth, 50};

    int PlayerSpeed = 5;
    float PlayerVelocityY = 0.0f;
    const float Gravity = 0.8f;
    const float JumpForce = -15.0f;
    bool IsGrounded = true;

    Rectangle platforms[3] = {
        {200, 450, 200, 10},
        {500, 350, 150, 10},
        {300, 250, 250, 10}
    };

    int PlayerPosX, PlayerPosY, PrevPlayerPosY;
    bool GameOver = false;
    int Score = 0;

    float EnemyX, EnemyY;
    float EnemySpeed;
    bool EnemyAlive = true;
    double EnemyRespawnTime = 0;

    auto ResetGame = [&]() {
        PlayerPosX = screenWidth / 2;
        PlayerPosY = ground.y - PlayerHeight;
        PrevPlayerPosY = PlayerPosY;
        PlayerVelocityY = 0.0f;
        IsGrounded = true;
        GameOver = false;
        Score = 0;

        EnemyX = 50 + rand() % (screenWidth - 100);
        EnemyY = ground.y - EnemySize;
        EnemySpeed = 1.5f;
        EnemyAlive = true;
        EnemyRespawnTime = 0;

        // 🎯 Use fetched data to affect enemy behavior dynamically
        if (DataFetched && WordCount > 0) {
            EnemySpeed = 1.5f + (float)WordCount; // faster with more fetched words
        }
    };

    InitWindow(screenWidth, screenHeight, "Game");
    SetTargetFPS(60);

    ResetGame();
    StartFetch(); // Fetch data once at the start

    while (!WindowShouldClose()) {
        if (!GameOver) {
            PrevPlayerPosY = PlayerPosY;

            if (IsKeyDown(KEY_D)) PlayerPosX += PlayerSpeed;
            if (IsKeyDown(KEY_A)) PlayerPosX -= PlayerSpeed;

            PlayerVelocityY += Gravity;
            PlayerPosY += (int)PlayerVelocityY;

            IsGrounded = false;

            if (PlayerPosY + PlayerHeight >= ground.y) {
                PlayerPosY = ground.y - PlayerHeight;
                PlayerVelocityY = 0;
                IsGrounded = true;
            }

            for (int i = 0; i < 3; i++) {
                bool withinX = PlayerPosX + PlayerWidth > platforms[i].x &&
                               PlayerPosX < platforms[i].x + platforms[i].width;

                bool crossingDown = PrevPlayerPosY + PlayerHeight <= platforms[i].y &&
                                    PlayerPosY + PlayerHeight >= platforms[i].y;

                if (withinX && crossingDown && PlayerVelocityY >= 0) {
                    PlayerPosY = platforms[i].y - PlayerHeight;
                    PlayerVelocityY = 0;
                    IsGrounded = true;
                }
            }

            if (IsGrounded && IsKeyPressed(KEY_SPACE)) {
                PlayerVelocityY = JumpForce;
                IsGrounded = false;
            }

            if (PlayerPosX < leftWall.width) PlayerPosX = leftWall.width;
            if (PlayerPosX + PlayerWidth > rightWall.x) PlayerPosX = rightWall.x - PlayerWidth;
            if (PlayerPosY < ceiling.height) {
                PlayerPosY = ceiling.height;
                PlayerVelocityY = 0;
            }

            Rectangle playerRect = {(float)PlayerPosX, (float)PlayerPosY, (float)PlayerWidth, (float)PlayerHeight};

            if (EnemyAlive) {
                float dx = PlayerPosX - EnemyX;
                float dy = PlayerPosY - EnemyY;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > 0) {
                    EnemyX += (dx / dist) * EnemySpeed;
                    EnemyY += (dy / dist) * EnemySpeed;
                }

                Rectangle enemyRect = {EnemyX, EnemyY, EnemySize, EnemySize};

                if (CheckCollisionRecs(playerRect, enemyRect)) {
                    bool stomp = PrevPlayerPosY + PlayerHeight <= EnemyY && PlayerVelocityY > 0;
                    if (stomp) {
                        EnemyAlive = false;
                        EnemyRespawnTime = GetTime() + 5.0;
                        PlayerVelocityY = JumpForce / 2;
                        Score++;
                        EnemySpeed += 0.3f; // speed up after stomp
                    } else {
                        GameOver = true;
                    }
                }
            } else {
                if (GetTime() >= EnemyRespawnTime) {
                    EnemyX = 50 + rand() % (screenWidth - 100);
                    EnemyY = ground.y - EnemySize;
                    EnemyAlive = true;
                }
            }
        } else {
            if (IsKeyPressed(KEY_ENTER)) ResetGame();
        }

        // =====================================================
        // DRAW
        // =====================================================
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRec(ground, BLACK);
        DrawRectangleRec(leftWall, BLACK);
        DrawRectangleRec(rightWall, BLACK);
        DrawRectangleRec(ceiling, BLACK);
        for (int i = 0; i < 3; i++) DrawRectangleRec(platforms[i], BLACK);

        DrawRectangle(PlayerPosX, PlayerPosY, PlayerWidth, PlayerHeight, BLUE);

        if (EnemyAlive) DrawRectangle((int)EnemyX, (int)EnemyY, EnemySize, EnemySize, RED);

        DrawText(TextFormat("Score: %d", Score), 20, 20, 30, WHITE);

        // 🛰️ Display fetch data state
        if (FetchInProgress) {
            DrawText("Fetching data from API...", 20, 60, 20, ORANGE);
        } else if (DataFetched) {
            DrawText("Fetched Words:", 20, 60, 20, DARKGRAY);
            for (int i = 0; i < WordCount; i++) {
                DrawText(EnemyWords[i], 40, 90 + i * 20, 20, GRAY);
            }
        } else if (FetchFailed) {
            DrawText("Failed to fetch API data!", 20, 60, 20, RED);
        }

        if (GameOver) {
            DrawText("GAME OVER", screenWidth / 2 - 150, screenHeight / 2 - 40, 40, RED);
            DrawText("Press ENTER to restart", screenWidth / 2 - 180, screenHeight / 2 + 20, 20, DARKGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
