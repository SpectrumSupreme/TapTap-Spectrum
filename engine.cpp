/**
 * @file engine.cpp
 * @brief Man vs Math - Custom C++ WebAssembly Game Engine
 * @architecture Data-Driven Entity-Component system with Emscripten bridging.
 * * Features:
 * - Cross-compiled to WebAssembly (WASM) via Emscripten.
 * - Data-Driven Design: Live hot-reloading of game variables via config.json.
 * - Custom Physics: Euler integration for gravity and velocity.
 * - Collision Detection: Axis-Aligned Bounding Box (AABB) implementation.
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include "json.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

using namespace std;
using json = nlohmann::json;

// ==========================================================
// 1. ENGINE CORE & MATH UTILS
// ==========================================================
enum GameState { PLAYING, GAME_OVER, VICTORY };

/**
 * @brief Axis-Aligned Bounding Box (AABB) Collision Detection
 * Highly optimized standard for 2D rectangular hitboxes.
 */
bool CheckCollision(SDL_Rect a, SDL_Rect b) {
    if (a.y + a.h <= b.y) return false;
    if (a.y >= b.y + b.h) return false;
    if (a.x + a.w <= b.x) return false;
    if (a.x >= b.x + b.w) return false;
    return true;
}

struct Projectile {
    float x, y, vx, vy;
    int width, height;
    bool active;
    int damage;
    SDL_Rect GetRect() { return {(int)x, (int)y, width, height}; }
};

// ==========================================================
// 2. THE ENTITY CLASS (Data-Driven Object)
// ==========================================================
class Entity {
public:
    // Kinematics
    float x, y;
    int width, height;
    float velocityX, velocityY;
    bool hasGravity, onGround;
    
    // Stats
    int hp, maxHp;
    
    // Rendering & Animation
    SDL_Texture *texture;
    int frameWidth, frameHeight;
    int currentFrame, maxFrames, currentRow, animationSpeed;
    Uint32 lastFrameTime;

    Entity(float startX, float startY, int w, int h, bool gravity, int health, int fWidth, int fHeight, int mFrames, int animRow) {
        x = startX; y = startY; width = w; height = h;
        hasGravity = gravity; onGround = false;
        velocityX = 0.0f; velocityY = 0.0f;
        hp = health; maxHp = health; texture = nullptr;
        frameWidth = fWidth; frameHeight = fHeight;
        maxFrames = mFrames; currentRow = animRow; 
        currentFrame = 0; animationSpeed = 80; lastFrameTime = SDL_GetTicks();
    }

    void LoadTexture(SDL_Renderer *renderer, const char *filePath) {
        SDL_Surface *tempSurface = IMG_Load(filePath);
        if (tempSurface != NULL) {
            texture = SDL_CreateTextureFromSurface(renderer, tempSurface);
            SDL_FreeSurface(tempSurface);
        }
    }

    /**
     * @brief Resolves position based on current velocity and gravity (Euler Integration).
     * @param floorY The Y-coordinate of the ground boundary to prevent infinite falling.
     */
    void UpdatePhysics(float floorY) {
        x += velocityX;
        if (hasGravity) {
            velocityY += 0.8f; // Gravity constant
            if (velocityY > 15.0f) velocityY = 15.0f; // Terminal velocity
            y += velocityY;
            
            // Floor collision response
            if (y + height >= floorY) { 
                y = floorY - height; 
                velocityY = 0.0f; 
                onGround = true; 
            } else { 
                onGround = false; 
            }
        } else {
            y += velocityY;
            // Screen clamping for non-gravity entities (like flying bosses)
            if (y < 0) y = 0; 
            if (y > 600 - height) y = 600 - height;
        }
    }

    void Render(SDL_Renderer *renderer, bool isInvincible) {
        // I-Frame flashing logic
        if (isInvincible && SDL_GetTicks() % 100 < 50) return;
        
        if (texture != nullptr && hp > 0) {
            // Sprite sheet animation tick
            if (SDL_GetTicks() - lastFrameTime > animationSpeed) {
                currentFrame++;
                if (currentFrame >= maxFrames) currentFrame = 0; 
                lastFrameTime = SDL_GetTicks();
            }
            SDL_Rect srcRect = {currentFrame * frameWidth, currentRow * frameHeight, frameWidth, frameHeight};
            SDL_Rect destRect = {(int)x, (int)y, width, height};
            SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
        }
    }
    
    // Generates a tightened hurtbox for fairer gameplay
    SDL_Rect GetRect() { return {(int)x + 15, (int)y + 10, width - 30, height - 10}; }
};

// ==========================================================
// 3. GAME CONTEXT (Memory Sandbox for Emscripten)
// ==========================================================
/**
 * @brief Encapsulates the entire engine state.
 * Required to avoid global variables while passing state to Emscripten's 
 * async browser loop (emscripten_set_main_loop_arg).
 */
struct GameContext {
    SDL_Renderer* renderer;
    SDL_Texture* bgTexture;
    GameState currentState;
    
    Entity* player;
    Entity* boss;
    Entity* platform;
    
    vector<Projectile> playerBullets;
    vector<Projectile> bossBullets;
    
    json config;
    std::string pWeapon, bMove, bPhase2;
    int bossTimer, playerIFrames, chargeFrames, autoFireTimer, earthquakeTimer;
    bool isCharging, gameIsRunning;
};

// ==========================================================
// 4. MAIN ENGINE LOOP (Ticked by Browser's requestAnimationFrame)
// ==========================================================
void MainLoopStep(void* arg) {
    GameContext* ctx = (GameContext*)arg;
    SDL_Event event;

    // ------------------------------------------------------
    // INPUT & EVENT POLLING
    // ------------------------------------------------------
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) ctx->gameIsRunning = false;

        // DATA-DRIVEN HOT RELOADING: 
        // Instantly parse JSON and inject new behavior without recompiling C++.
        if (event.type == SDL_MOUSEBUTTONDOWN && (ctx->currentState == GAME_OVER || ctx->currentState == VICTORY)) {
            std::ifstream reloadFile("config.json");
            if (reloadFile.is_open()) {
                ctx->config = json::parse(reloadFile);
                reloadFile.close();
            }

            // Rehydrate Player
            ctx->player->hp = ctx->config["player"]["hp"];
            ctx->player->x = ctx->config["player"]["spawn_x"];
            ctx->player->y = ctx->config["player"]["spawn_y"];
            ctx->player->hasGravity = ctx->config["player"].value("has_gravity", true);
            ctx->pWeapon = ctx->config["player"].value("weapon", "charge_shot");

            // Rehydrate Boss
            ctx->boss->hp = ctx->config["boss"]["hp"];
            ctx->boss->x = ctx->config["boss"]["spawn_x"];
            ctx->boss->y = ctx->config["boss"]["spawn_y"];
            ctx->boss->velocityX = ctx->config["boss"]["speed_x"];
            ctx->boss->velocityY = ctx->config["boss"]["speed_y"];
            ctx->bMove = ctx->config["boss"].value("ai_movement", "bounce");
            ctx->bPhase2 = ctx->config["boss"].value("phase2_skill", "earthquake");
            
            // Clean up memory and reset states
            ctx->playerBullets.clear(); ctx->bossBullets.clear();
            ctx->bossTimer = 0; ctx->playerIFrames = 0; ctx->earthquakeTimer = 0; ctx->chargeFrames = 0;
            ctx->currentState = PLAYING;
        }
    }

    // ------------------------------------------------------
    // GAME LOGIC & STATE MACHINE
    // ------------------------------------------------------
    if (ctx->currentState == PLAYING) {
        if (ctx->playerIFrames > 0) ctx->playerIFrames--;

        // 1. Player Kinematics
        const Uint8 *keyState = SDL_GetKeyboardState(NULL);
        ctx->player->velocityX = 0.0f;
        if (keyState[SDL_SCANCODE_A]) ctx->player->velocityX = -5.0f;
        if (keyState[SDL_SCANCODE_D]) ctx->player->velocityX = 5.0f;
        
        if (ctx->player->hasGravity) {
            if (keyState[SDL_SCANCODE_SPACE] && ctx->player->onGround) ctx->player->velocityY = -16.0f;
        } else {
            ctx->player->velocityY = 0.0f;
            if (keyState[SDL_SCANCODE_W]) ctx->player->velocityY = -5.0f;
            if (keyState[SDL_SCANCODE_S]) ctx->player->velocityY = 5.0f;
        }

        // Clamp to screen bounds
        if (ctx->player->x < 0) ctx->player->x = 0;
        if (ctx->player->x > 800 - ctx->player->width) ctx->player->x = 800 - ctx->player->width;

        // 2. Weapon Systems (Vector-based aiming)
        int mouseX, mouseY;
        Uint32 mouseState = SDL_GetMouseState(&mouseX, &mouseY);
        
        if (ctx->pWeapon == "charge_shot") {
            if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) {
                ctx->isCharging = true;
                if (ctx->chargeFrames < 60) ctx->chargeFrames++; 
            } else if (ctx->isCharging) {
                float dx = mouseX - (ctx->player->x + 32); float dy = mouseY - (ctx->player->y + 32);
                float length = std::sqrt(dx * dx + dy * dy);
                if (length > 0) {
                    float vx = (dx / length) * 15.0f; float vy = (dy / length) * 15.0f;
                    // Dynamically scale projectile size and damage based on charge time
                    ctx->playerBullets.push_back({ctx->player->x + 32, ctx->player->y + 32, vx, vy, 10 + (ctx->chargeFrames / 2), 10 + (ctx->chargeFrames / 2), true, 10 + ctx->chargeFrames});
                }
                ctx->isCharging = false; ctx->chargeFrames = 0; 
            }
        } else if (ctx->pWeapon == "auto_rifle") {
            ctx->isCharging = false;
            if (ctx->autoFireTimer > 0) ctx->autoFireTimer--;
            if ((mouseState & SDL_BUTTON(SDL_BUTTON_LEFT)) && ctx->autoFireTimer == 0) {
                float dx = mouseX - (ctx->player->x + 32); float dy = mouseY - (ctx->player->y + 32);
                float length = std::sqrt(dx * dx + dy * dy);
                if (length > 0) {
                    float vx = (dx / length) * 20.0f; float vy = (dy / length) * 20.0f; 
                    ctx->playerBullets.push_back({ctx->player->x + 32, ctx->player->y + 32, vx, vy, 8, 8, true, 5}); 
                    ctx->autoFireTimer = 8; 
                }
            }
        }

        ctx->player->UpdatePhysics(ctx->platform->y);

        // 3. Boss Artificial Intelligence
        if (ctx->bMove == "bounce") {
            if (ctx->boss->x <= 0 || ctx->boss->x >= 800 - ctx->boss->width) ctx->boss->velocityX *= -1.0f;
            if (ctx->boss->y <= 50.0f || ctx->boss->y >= 300.0f) ctx->boss->velocityY *= -1.0f;
        } else if (ctx->bMove == "chase") {
            if (ctx->boss->x < ctx->player->x) ctx->boss->x += std::abs(ctx->boss->velocityX);
            else ctx->boss->x -= std::abs(ctx->boss->velocityX);
            if (ctx->boss->y < ctx->player->y - 100) ctx->boss->y += std::abs(ctx->boss->velocityY);
            else ctx->boss->y -= std::abs(ctx->boss->velocityY);
        }
        ctx->boss->UpdatePhysics(ctx->platform->y);

        // 4. Collision Resolution
        if (CheckCollision(ctx->player->GetRect(), ctx->boss->GetRect()) && ctx->playerIFrames == 0) {
            ctx->player->hp -= 20; ctx->playerIFrames = 60; 
        }

        ctx->bossTimer++;
        if (ctx->bossTimer % 45 == 0) {
            float vx = (ctx->player->x > ctx->boss->x) ? 8.0f : -8.0f; 
            float vy = (rand() % 11) - 5.0f;
            ctx->bossBullets.push_back({ctx->boss->x + 50, ctx->boss->y + 50, vx, vy, 15, 15, true, 10});
        }

        for (auto &b : ctx->playerBullets) {
            if (b.active) {
                b.x += b.vx; b.y += b.vy;
                if (CheckCollision(b.GetRect(), ctx->boss->GetRect()) && ctx->boss->hp > 0) { ctx->boss->hp -= b.damage; b.active = false; }
                if (b.x < 0 || b.x > 800 || b.y < 0 || b.y > 600) b.active = false;
            }
        }

        for (auto &b : ctx->bossBullets) {
            if (b.active) {
                b.x += b.vx; b.y += b.vy;
                if (CheckCollision(b.GetRect(), ctx->player->GetRect()) && ctx->playerIFrames == 0 && ctx->player->hp > 0) {
                    ctx->player->hp -= b.damage; ctx->playerIFrames = 60; b.active = false;
                }
            }
        }

        // Win/Loss Verification
        if (ctx->player->hp <= 0) ctx->currentState = GAME_OVER;
        if (ctx->boss->hp <= 0) ctx->currentState = VICTORY;
    }

    // ------------------------------------------------------
    // RENDERING PIPELINE
    // ------------------------------------------------------
    SDL_SetRenderDrawColor(ctx->renderer, 30, 30, 50, 255);
    SDL_RenderClear(ctx->renderer);

    // Render Background
    if (ctx->bgTexture != nullptr) {
        SDL_RenderCopy(ctx->renderer, ctx->bgTexture, NULL, NULL); 
    }

    // Render Platform & Phase 2 Environmental Hazards
    bool earthquakeWarning = false; bool earthquakeActive = false;
    ctx->platform->Render(ctx->renderer, false);

    if (ctx->bPhase2 == "earthquake" && ctx->currentState == PLAYING && ctx->boss->hp > 0 && ctx->boss->hp <= ctx->boss->maxHp / 2) {
        ctx->earthquakeTimer++;
        int cycle = ctx->earthquakeTimer % 300; 
        if (cycle > 200 && cycle < 280) earthquakeWarning = true; 
        else if (cycle >= 280 && cycle < 300) {
            earthquakeActive = true; 
            // Punish the player if they fail to jump
            if (ctx->player->onGround && ctx->playerIFrames == 0) { ctx->player->hp -= 30; ctx->playerIFrames = 60; }
        }
    }

    if (earthquakeActive || earthquakeWarning) {
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
        if (earthquakeActive) SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 180);
        else SDL_SetRenderDrawColor(ctx->renderer, 255, 140, 0, 150);
        SDL_Rect platOverlay = {(int)ctx->platform->x, (int)ctx->platform->y, ctx->platform->width, ctx->platform->height};
        SDL_RenderFillRect(ctx->renderer, &platOverlay);
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);
    }

    // Render Entities
    ctx->player->Render(ctx->renderer, ctx->playerIFrames > 0);
    ctx->boss->Render(ctx->renderer, false);

    // Render Charging Aura
    if (ctx->isCharging && ctx->pWeapon == "charge_shot") {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 255, 100);
        SDL_Rect aura = {(int)ctx->player->x - ctx->chargeFrames / 4, (int)ctx->player->y - ctx->chargeFrames / 4,
                         ctx->player->width + ctx->chargeFrames / 2, ctx->player->height + ctx->chargeFrames / 2};
        SDL_RenderDrawRect(ctx->renderer, &aura);
    }

    // Render HUD (Health Bars)
    if (ctx->player->hp > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 0, 255, 0, 255);
        SDL_Rect pHp = {(int)ctx->player->x, (int)ctx->player->y - 15, (ctx->player->hp * 64) / ctx->player->maxHp, 8};
        SDL_RenderFillRect(ctx->renderer, &pHp);
    }
    if (ctx->boss->hp > 0) {
        SDL_SetRenderDrawColor(ctx->renderer, 255, 0, 0, 255);
        SDL_Rect bHp = {(int)ctx->boss->x, (int)ctx->boss->y - 20, (ctx->boss->hp * 100) / ctx->boss->maxHp, 10};
        SDL_RenderFillRect(ctx->renderer, &bHp);
    }

    // Render Projectiles
    SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);
    for (auto &b : ctx->playerBullets) if (b.active) { SDL_Rect r = b.GetRect(); SDL_RenderFillRect(ctx->renderer, &r); }
    
    SDL_SetRenderDrawColor(ctx->renderer, 255, 100, 0, 255);
    for (auto &b : ctx->bossBullets) if (b.active) { SDL_Rect r = b.GetRect(); SDL_RenderFillRect(ctx->renderer, &r); }

    // Render Screen State Overlays
    if (ctx->currentState == GAME_OVER) {
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(ctx->renderer, 100, 0, 0, 180);
        SDL_Rect fullScreen = {0, 0, 800, 600}; SDL_RenderFillRect(ctx->renderer, &fullScreen); SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);
    } else if (ctx->currentState == VICTORY) {
        SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(ctx->renderer, 255, 215, 0, 120);
        SDL_Rect fullScreen = {0, 0, 800, 600}; SDL_RenderFillRect(ctx->renderer, &fullScreen); SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_NONE);
    }

    SDL_RenderPresent(ctx->renderer);
}

// ==========================================================
// 5. ENGINE BOOTSTRAP
// ==========================================================
int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);
    SDL_Window *window = SDL_CreateWindow("Man vs Math: Web Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Initial JSON Hydration
    std::ifstream configFile("config.json");
    if (!configFile.is_open()) {
        cout << "CRITICAL ERROR: config.json not found! Check --preload-file flags." << endl;
        return -1;
    }
    json config = json::parse(configFile);
    configFile.close();

    // Instantiate Entities from parsed JSON data
    Entity player(
        config["player"]["spawn_x"], config["player"]["spawn_y"], config["player"]["width"], config["player"]["height"],
        config["player"].value("has_gravity", true), config["player"]["hp"],
        config["player"]["frame_w"], config["player"]["frame_h"], config["player"]["max_frames"], config["player"]["anim_row"]
    );
    std::string pSprite = config["player"]["sprite"]; player.LoadTexture(renderer, pSprite.c_str());

    Entity boss(
        config["boss"]["spawn_x"], config["boss"]["spawn_y"], config["boss"]["width"], config["boss"]["height"],
        false, config["boss"]["hp"], config["boss"]["frame_w"], config["boss"]["frame_h"], config["boss"]["max_frames"], config["boss"]["anim_row"]
    );
    boss.velocityX = config["boss"]["speed_x"]; boss.velocityY = config["boss"]["speed_y"];
    std::string bSprite = config["boss"]["sprite"]; boss.LoadTexture(renderer, bSprite.c_str());

    Entity platform(
        config["platform"]["spawn_x"], config["platform"]["spawn_y"], config["platform"]["width"], config["platform"]["height"],
        false, config["platform"]["hp"], config["platform"]["frame_w"], config["platform"]["frame_h"], config["platform"]["max_frames"], config["platform"]["anim_row"]
    );
    std::string platSprite = config["platform"]["sprite"]; platform.LoadTexture(renderer, platSprite.c_str());

    // Setup Background
    SDL_Texture* bgTex = nullptr;
    if (config.contains("background")) {
        std::string bgSprite = config["background"].value("sprite", "./images/background.png");
        SDL_Surface* bgSurf = IMG_Load(bgSprite.c_str());
        if (bgSurf != NULL) {
            bgTex = SDL_CreateTextureFromSurface(renderer, bgSurf);
            SDL_FreeSurface(bgSurf);
        }
    }

    // Bind state to the GameContext sandbox
    GameContext ctx;
    ctx.renderer = renderer; ctx.bgTexture = bgTex; ctx.currentState = PLAYING;
    ctx.player = &player; ctx.boss = &boss; ctx.platform = &platform;
    ctx.config = config;
    ctx.pWeapon = config["player"].value("weapon", "charge_shot");
    ctx.bMove = config["boss"].value("ai_movement", "bounce");
    ctx.bPhase2 = config["boss"].value("phase2_skill", "earthquake");
    ctx.bossTimer = 0; ctx.playerIFrames = 0; ctx.chargeFrames = 0; ctx.autoFireTimer = 0; ctx.earthquakeTimer = 0;
    ctx.isCharging = false; ctx.gameIsRunning = true;

    // Cross-Platform Loop Setup
    #ifdef __EMSCRIPTEN__
        // Hands control over to the browser's JavaScript engine
        emscripten_set_main_loop_arg(MainLoopStep, &ctx, 60, 1);
    #else
        // Fallback for native desktop testing
        while (ctx.gameIsRunning) { MainLoopStep(&ctx); SDL_Delay(16); }
        if (ctx.bgTexture) SDL_DestroyTexture(ctx.bgTexture);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); IMG_Quit(); SDL_Quit();
    #endif

    return 0;
}
