#include <raylib.h>
#include <raymath.h>

#define WIDTH 800
#define HEIGHT 600

#define PLAYER_SIZE 50
#define PLAYER_SPEED 300.0f

#define MAX_BULLETS 100
#define BULLET_SPEED 600.0f
#define BULLET_SIZE 8.0f

#define MAX_ENEMIES 50
#define ENEMY_RADIUS 20.0f

typedef struct Bullet {
    Vector2 pos;
    Vector2 vel;
    bool active;
} Bullet;

typedef struct Enemy {
    Vector2 pos;
    float radius;
    float speed;
    bool active;
} Enemy;

typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_GAMEOVER
} GameState;

// --------------------------------------------------
// Spawn one enemy
// --------------------------------------------------
Enemy SpawnEnemy(float speed)
{
    Enemy e = { 0 };
    e.radius = ENEMY_RADIUS;
    e.speed = speed;
    
    // Spawn from edges for better gameplay
    int side = GetRandomValue(0, 3);
    switch(side) {
        case 0: // Top
            e.pos.x = GetRandomValue(0, WIDTH);
            e.pos.y = -ENEMY_RADIUS;
            break;
        case 1: // Right
            e.pos.x = WIDTH + ENEMY_RADIUS;
            e.pos.y = GetRandomValue(0, HEIGHT);
            break;
        case 2: // Bottom
            e.pos.x = GetRandomValue(0, WIDTH);
            e.pos.y = HEIGHT + ENEMY_RADIUS;
            break;
        case 3: // Left
            e.pos.x = -ENEMY_RADIUS;
            e.pos.y = GetRandomValue(0, HEIGHT);
            break;
    }
    
    e.active = true;
    return e;
}

// --------------------------------------------------
// Spawn a wave
// --------------------------------------------------
void SpawnWave(Enemy enemies[], int wave, int *alive)
{
    int count = Clamp(wave * 3, 1, MAX_ENEMIES);
    
    // Only spawn new enemies in empty slots
    for (int i = 0, spawned = 0; i < MAX_ENEMIES && spawned < count; i++)
    {
        if (!enemies[i].active)
        {
            enemies[i] = SpawnEnemy(80.0f + wave * 20.0f);
            spawned++;
        }
    }
    
    // Count how many are now active
    *alive = 0;
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (enemies[i].active) (*alive)++;
}

// --------------------------------------------------
// Find closest enemy
// --------------------------------------------------
int FindClosestEnemy(Enemy enemies[], Vector2 pos)
{
    int index = -1;
    float bestDist = 999999;

    for (int i = 0; i < MAX_ENEMIES; i++)
    {
        if (!enemies[i].active) continue;

        float d = Vector2Distance(pos, enemies[i].pos);
        if (d < bestDist)
        {
            bestDist = d;
            index = i;
        }
    }
    return index;
}

// --------------------------------------------------
// Reset game to initial state
// --------------------------------------------------
void ResetGame(
    Rectangle *player,
    Bullet bullets[],
    Enemy enemies[],
    int *wave,
    int *enemiesAlive,
    int *playerHealth,
    int *score
)
{
    *player = (Rectangle){
        WIDTH / 2.0f - PLAYER_SIZE / 2,
        HEIGHT / 2.0f - PLAYER_SIZE / 2,
        PLAYER_SIZE,
        PLAYER_SIZE
    };

    for (int i = 0; i < MAX_BULLETS; i++)
        bullets[i].active = false;
    
    for (int i = 0; i < MAX_ENEMIES; i++)
        enemies[i].active = false;

    *wave = 1;
    *playerHealth = 5;
    *score = 0;

    SpawnWave(enemies, *wave, enemiesAlive);
}

// --------------------------------------------------
// UI Button (Simplified version)
// --------------------------------------------------
bool Button(Rectangle rect, const char *text)
{
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rect);
    
    Color bgColor = hover ? DARKGRAY : GRAY;
    if (!hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse, rect))
        bgColor = DARKGRAY;

    // Draw button background
    DrawRectangleRec(rect, bgColor);
    
    // Draw button border
    DrawRectangleLinesEx(rect, 2, WHITE);
    
    // Center text
    int fontSize = 20;
    int textWidth = MeasureText(text, fontSize);
    DrawText(
        text,
        rect.x + rect.width / 2 - textWidth / 2,
        rect.y + rect.height / 2 - fontSize / 2,
        fontSize,
        WHITE
    );

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// --------------------------------------------------
// Draw Health Bar
// --------------------------------------------------
void DrawHealthBar(int health, int maxHealth)
{
    int barWidth = 200;
    int barHeight = 20;
    int x = WIDTH - barWidth - 10;
    int y = 10;
    
    // Background
    DrawRectangle(x, y, barWidth, barHeight, DARKGRAY);
    
    // Health fill
    float healthPercent = (float)health / (float)maxHealth;
    int fillWidth = (int)(barWidth * healthPercent);
    Color healthColor;
    if (healthPercent > 0.6f) healthColor = GREEN;
    else if (healthPercent > 0.3f) healthColor = YELLOW;
    else healthColor = RED;
    
    DrawRectangle(x, y, fillWidth, barHeight, healthColor);
    DrawRectangleLines(x, y, barWidth, barHeight, WHITE);
    
    // Health text
    DrawText(TextFormat("HP: %d/%d", health, maxHealth), x + 5, y + 2, 15, WHITE);
}

// --------------------------------------------------
// Main
// --------------------------------------------------
int main(void)
{
    GameState state = STATE_MENU;
    InitWindow(WIDTH, HEIGHT, "Simple Wave Shooter");
    SetTargetFPS(60);

    // Game objects
    Rectangle player = {
        WIDTH / 2.0f - PLAYER_SIZE / 2,
        HEIGHT / 2.0f - PLAYER_SIZE / 2,
        PLAYER_SIZE,
        PLAYER_SIZE
    };

    Bullet bullets[MAX_BULLETS] = { 0 };
    Enemy enemies[MAX_ENEMIES] = { 0 };

    // Game variables
    int wave = 1;
    int enemiesAlive = 0;
    int playerHealth = 5;
    int maxHealth = 5;
    int score = 0;
    
    float fireRate = 0.25f;
    float fireTimer = 0.0f;
    
    // Invincibility frames
    float invincibilityTimer = 0.0f;
    bool isInvincible = false;
    float hitFlashTimer = 0.0f;
    
    SpawnWave(enemies, wave, &enemiesAlive);

    // --------------------------------------------------
    // Game loop
    // --------------------------------------------------
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        BeginDrawing();
        ClearBackground(BLACK);

        // =========================
        // MENU
        // =========================
        if (state == STATE_MENU)
        {
            // Title
            DrawText("WAVE SHOOTER", WIDTH / 2 - MeasureText("WAVE SHOOTER", 40) / 2, 120, 40, YELLOW);
            DrawText("Survive as long as you can!", WIDTH / 2 - MeasureText("Survive as long as you can!", 20) / 2, 180, 20, WHITE);
            
            // Controls info
            DrawText("CONTROLS:", WIDTH / 2 - 200, 220, 18, GRAY);
            DrawText("WASD or ARROWS - Move", WIDTH / 2 - 200, 240, 16, LIGHTGRAY);
            DrawText("SPACE or MOUSE - Auto-aim shoot", WIDTH / 2 - 200, 260, 16, LIGHTGRAY);
            
            // Buttons
            if (Button((Rectangle){WIDTH/2 - 100, 300, 200, 50}, "START GAME"))
            {
                ResetGame(&player, bullets, enemies, &wave, &enemiesAlive, &playerHealth, &score);
                state = STATE_GAME;
            }

            if (Button((Rectangle){WIDTH/2 - 100, 370, 200, 50}, "EXIT GAME"))
            {
                break;
            }
        }

        // =========================
        // GAME
        // =========================
        else if (state == STATE_GAME)
        {
            // Update timers
            fireTimer -= dt;
            invincibilityTimer -= dt;
            hitFlashTimer -= dt;
            
            if (invincibilityTimer <= 0.0f)
                isInvincible = false;

            Vector2 playerCenter = {
                player.x + player.width / 2,
                player.y + player.height / 2
            };

            // ---- Player movement ----
            if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) player.x -= PLAYER_SPEED * dt;
            if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) player.x += PLAYER_SPEED * dt;
            if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) player.y -= PLAYER_SPEED * dt;
            if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) player.y += PLAYER_SPEED * dt;

            // Keep player in bounds
            player.x = Clamp(player.x, 0, WIDTH - player.width);
            player.y = Clamp(player.y, 0, HEIGHT - player.height);

            // ---- Shooting ----
            int target = FindClosestEnemy(enemies, playerCenter);

            if ((IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_LEFT_BUTTON)) && fireTimer <= 0 && target != -1)
            {
                Vector2 dir = Vector2Normalize(
                    Vector2Subtract(enemies[target].pos, playerCenter)
                );

                for (int i = 0; i < MAX_BULLETS; i++)
                {
                    if (!bullets[i].active)
                    {
                        bullets[i].active = true;
                        bullets[i].pos = playerCenter;
                        bullets[i].vel = Vector2Scale(dir, BULLET_SPEED);
                        fireTimer = fireRate;
                        break;
                    }
                }
            }

            // ---- Bullet updates ----
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active) continue;
                
                bullets[i].pos = Vector2Add(bullets[i].pos, Vector2Scale(bullets[i].vel, dt));
                
                // Remove bullets that go off screen
                if (bullets[i].pos.x < -BULLET_SIZE || bullets[i].pos.x > WIDTH + BULLET_SIZE ||
                    bullets[i].pos.y < -BULLET_SIZE || bullets[i].pos.y > HEIGHT + BULLET_SIZE)
                {
                    bullets[i].active = false;
                }
            }

            // ---- Enemy movement & collision ----
            int currentAlive = 0;
            for (int i = 0; i < MAX_ENEMIES; i++)
            {
                if (!enemies[i].active) continue;
                
                currentAlive++;
                
                // Move enemy toward player
                Vector2 dir = Vector2Normalize(Vector2Subtract(playerCenter, enemies[i].pos));
                enemies[i].pos = Vector2Add(enemies[i].pos, Vector2Scale(dir, enemies[i].speed * dt));
                
                // Enemy collision with player
                Rectangle enemyRect = {
                    enemies[i].pos.x - enemies[i].radius,
                    enemies[i].pos.y - enemies[i].radius,
                    enemies[i].radius * 2,
                    enemies[i].radius * 2
                };
                
                if (CheckCollisionRecs(player, enemyRect) && !isInvincible)
                {
                    playerHealth--;
                    invincibilityTimer = 1.0f; // 1 second invincibility
                    isInvincible = true;
                    hitFlashTimer = 0.1f;
                    
                    // Push player away slightly
                    Vector2 pushDir = Vector2Normalize(Vector2Subtract(playerCenter, enemies[i].pos));
                    player.x += pushDir.x * 30;
                    player.y += pushDir.y * 30;
                }
                
                // Bullet collision with enemies
                for (int b = 0; b < MAX_BULLETS; b++)
                {
                    if (!bullets[b].active) continue;
                    
                    if (CheckCollisionCircleRec(enemies[i].pos, enemies[i].radius, 
                                               (Rectangle){bullets[b].pos.x - BULLET_SIZE/2, 
                                                          bullets[b].pos.y - BULLET_SIZE/2, 
                                                          BULLET_SIZE, BULLET_SIZE}))
                    {
                        bullets[b].active = false;
                        enemies[i].active = false;
                        score += 100 * wave; // More points for higher waves
                        break;
                    }
                }
            }
            enemiesAlive = currentAlive;

            // ---- Wave progression ----
            if (enemiesAlive <= 0)
            {
                wave++;
                score += 500 * (wave - 1); // Bonus for completing wave
                SpawnWave(enemies, wave, &enemiesAlive);
            }

            // ---- Death ----
            if (playerHealth <= 0)
            {
                state = STATE_GAMEOVER;
            }

            // ---- Draw game objects ----
            
            // Draw player with hit flash effect
            Color playerColor = BLUE;
            if (isInvincible && fmod(GetTime() * 10, 2.0f) < 1.0f)
                playerColor = ColorAlpha(BLUE, 0.3f);
            else if (hitFlashTimer > 0)
                playerColor = RED;
                
            DrawRectangleRec(player, playerColor);
            DrawRectangleLinesEx(player, 2, WHITE);
            
            // Draw player direction indicator
            if (target != -1)
            {
                DrawLineV(playerCenter, enemies[target].pos, ColorAlpha(RED, 0.3f));
            }

            // Draw enemies
            for (int i = 0; i < MAX_ENEMIES; i++)
            {
                if (!enemies[i].active) continue;
                
                // Pulsing effect for enemies
                float pulse = sin(GetTime() * 3.0f + i) * 0.1f + 1.0f;
                DrawCircleV(enemies[i].pos, enemies[i].radius * pulse, GREEN);
                DrawCircleLines((int)enemies[i].pos.x, (int)enemies[i].pos.y, enemies[i].radius, DARKGREEN);
            }

            // Draw bullets
            for (int i = 0; i < MAX_BULLETS; i++)
            {
                if (!bullets[i].active) continue;
                
                // Bullet trail effect
                Vector2 trailPos = Vector2Subtract(bullets[i].pos, Vector2Scale(Vector2Normalize(bullets[i].vel), 10.0f));
                DrawLineEx(trailPos, bullets[i].pos, 3.0f, ColorAlpha(RED, 0.5f));
                
                // Bullet head
                DrawCircleV(bullets[i].pos, BULLET_SIZE/2, RED);
            }

            // ---- Draw UI ----
            // Health bar
            DrawHealthBar(playerHealth, maxHealth);
            
            // Wave info
            DrawRectangle(10, 10, 150, 60, ColorAlpha(BLACK, 0.5f));
            DrawRectangleLines(10, 10, 150, 60, WHITE);
            DrawText(TextFormat("WAVE: %d", wave), 20, 20, 20, YELLOW);
            DrawText(TextFormat("ENEMIES: %d", enemiesAlive), 20, 45, 15, LIGHTGRAY);
            
            // Score
            DrawRectangle(WIDTH/2 - 100, 10, 200, 30, ColorAlpha(BLACK, 0.5f));
            DrawRectangleLines(WIDTH/2 - 100, 10, 200, 30, WHITE);
            DrawText(TextFormat("SCORE: %d", score), WIDTH/2 - MeasureText(TextFormat("SCORE: %d", score), 20)/2, 15, 20, WHITE);
            
            // Controls hint
            DrawText("Hold SPACE or LEFT MOUSE to auto-aim", 10, HEIGHT - 30, 15, ColorAlpha(WHITE, 0.7f));
        }

        // =========================
        // GAME OVER
        // =========================
        else if (state == STATE_GAMEOVER)
        {
            // Game over screen with fade effect
            DrawRectangle(0, 0, WIDTH, HEIGHT, ColorAlpha(BLACK, 0.7f));
            DrawText("GAME OVER", WIDTH / 2 - MeasureText("GAME OVER", 50) / 2, 120, 50, RED);
            
            // Stats
            DrawText(TextFormat("FINAL SCORE: %d", score), WIDTH / 2 - MeasureText(TextFormat("FINAL SCORE: %d", score), 30) / 2, 200, 30, YELLOW);
            DrawText(TextFormat("WAVES SURVIVED: %d", wave), WIDTH / 2 - MeasureText(TextFormat("WAVES SURVIVED: %d", wave), 25) / 2, 240, 25, WHITE);
            
            // Buttons
            if (Button((Rectangle){WIDTH/2 - 100, 300, 200, 50}, "PLAY AGAIN"))
            {
                ResetGame(&player, bullets, enemies, &wave, &enemiesAlive, &playerHealth, &score);
                state = STATE_GAME;
            }

            if (Button((Rectangle){WIDTH/2 - 100, 370, 200, 50}, "MAIN MENU"))
            {
                state = STATE_MENU;
            }
            
            if (Button((Rectangle){WIDTH/2 - 100, 440, 200, 50}, "EXIT GAME"))
            {
                break;
            }
        }

        // FPS counter
        DrawFPS(WIDTH - 80, 10);

        EndDrawing();
    }
  
    CloseWindow();
    return 0;
}
