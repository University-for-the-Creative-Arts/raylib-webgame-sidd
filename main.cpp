#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>

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
    };

    InitWindow(screenWidth, screenHeight, "Raylib Single Enemy Stomp & Score");
    SetTargetFPS(60);

    ResetGame();

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

        if (GameOver) {
            DrawText("GAME OVER", screenWidth / 2 - 150, screenHeight / 2 - 40, 40, RED);
            DrawText("Press ENTER to restart", screenWidth / 2 - 180, screenHeight / 2 + 20, 20, DARKGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}