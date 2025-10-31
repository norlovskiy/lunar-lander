#include "CS3113/Entity.h"
#include <string>
#include <vector>
#include <random>
#include <cmath>



enum GameState { START, PLAY, GAME_OVER };



// Global Constants
constexpr int SCREEN_WIDTH  = 1500,
              SCREEN_HEIGHT = 1200,
              FPS           = 120;

constexpr char    BG_COLOUR[] = "#efececff";
constexpr char OUT_OF_FUEL[]        = "Mission failed. \nNo fuel remaining.",
               CRASH_LANDING[]      = "Mission failed. \nCrash landing.",
               SUCCESSFUL_LANDING[] = "Mission successful!";

constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

constexpr float 
                TILE_DIMENSION          = 175.0f,
                GRAVITY                 = 200.0f, 
                SHIP_FORCE              = 500.0f,
                WIN_SPEED               = 100.0f,
                WIN_ANGLE               = 10.0f,
                MAX_FUEL                = 1000.0f,
                FUEL_LOSS               = 5.0f,


                FIXED_TIMESTEP          = 1.0f / 60.0f,
                END_GAME_THRESHOLD      = 600.0f;

 
constexpr int LANDING_POINTS   = 500,
              NUMBER_OF_TILES  = SCREEN_WIDTH/TILE_DIMENSION + 1,
              GROUND_LEVEL     = 800;


// Moving platform constants
constexpr float PLATFORM_SPEED = 180.0f,
                PLATFORM_Y     = ORIGIN.y + GROUND_LEVEL - 4 * TILE_DIMENSION,
                BOUNDARY_X_MIN = ORIGIN.x - 0.35f * SCREEN_WIDTH,
                BOUNDARY_X_MAX = ORIGIN.x + 0.35f * SCREEN_WIDTH;

int   gMovingTileIndex = NUMBER_OF_TILES;
float gPlatformDir     = 1.0f;

// Global Variables 
AppStatus gAppStatus   = RUNNING;
float gPreviousTicks   = 0.0f,
      gTimeAccumulator = 0.0f;


GameState gGameState;
Entity* gShip = nullptr;
Entity *gTiles   = nullptr;

int fuel;
bool fuel_used;
std::string gEndMessage = "";
int gEndPoints = 0;


// Function Declarations
void initialise();
void processInput();
void update();
void render();
void shutdown();
void startNewGame();
void endScreen(const std::string& endMessage, int points);
void setTiles(Entity* gTiles, int NUMBER_OF_TILES);

// Used AI for this multiline text helper so I could pass the entire end message at once
void drawMultilineText(const std::string &text, int posX, int posY, int fontSize, Color color)
{
    const int lineSpacing = fontSize + 8;
    size_t start = 0;
    int line = 0;
    while (start < text.size()) {
        size_t pos = text.find('\n', start);
        std::string lineText = (pos == std::string::npos) ? text.substr(start) : text.substr(start, pos - start);
        DrawText(lineText.c_str(), posX, posY + line * lineSpacing, fontSize, color);
        if (pos == std::string::npos) break;
        start = pos + 1;
        ++line;
    }
}

void initialise()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lunar Lander");

   gGameState = START;
    
   // All textures are OC
    gShip = new Entity(
        {ORIGIN.x - 300.0f, ORIGIN.y - 500.0f}, // position
        {50.0f, 50.0f},                         // scale
        "assets/game/ship.png",
        DISABLED
    );

    fuel = MAX_FUEL;

    gTiles = new Entity[NUMBER_OF_TILES + 1];

    float leftMostX = ORIGIN.x - (NUMBER_OF_TILES * TILE_DIMENSION) / 2.0f;

    // Populate tile array
    for (int i = 0; i < NUMBER_OF_TILES; i++) 
    {
        gTiles[i].setStatus(DISABLED);
        gTiles[i].setScale({TILE_DIMENSION, TILE_DIMENSION});
        gTiles[i].setColliderDimensions({TILE_DIMENSION, TILE_DIMENSION});
        gTiles[i].setPosition({
            leftMostX + i * TILE_DIMENSION, 
            ORIGIN.y + GROUND_LEVEL - 2 * TILE_DIMENSION
        });
        gTiles[i].setRotation(0);
        gTiles[i].setAcceleration({0, 0});

    }

    // Set moving tile variables
    gTiles[gMovingTileIndex].setPosition({ BOUNDARY_X_MIN, PLATFORM_Y });
    gTiles[gMovingTileIndex].setSpeed((int)PLATFORM_SPEED);
    gTiles[gMovingTileIndex].setMovement({ +1.0f, 0.0f });
    gTiles[gMovingTileIndex].setRotation(0);
    gTiles[gMovingTileIndex].setAcceleration({0,0});

    SetTargetFPS(FPS);
}

void processInput() 
{
    // Reset movement
    gShip->setAcceleration({0.0f, GRAVITY});
    gShip->setRotation(0);
    fuel_used = false;

    if (IsKeyPressed(KEY_P)) {
        startNewGame();
    }

    // Set movement
    if (gGameState == PLAY && fuel > 0) {
        if (IsKeyDown(KEY_UP)) {
            gShip->addForcePolar(SHIP_FORCE);
            fuel_used = true;
        }
  
        if (IsKeyDown(KEY_RIGHT)) {
            gShip->setRotation(100);
            fuel_used = true;
        }

        if (IsKeyDown(KEY_LEFT)) {
            gShip->setRotation(-100);
            fuel_used = true;
        }
  
    }
 
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) gAppStatus = TERMINATED;
} 
 
void update() 
{ 
    float ticks = (float)GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    if (gGameState != PLAY) return;

    if (fuel_used) fuel -= FUEL_LOSS * deltaTime;

    gShip->update(deltaTime, gTiles, NUMBER_OF_TILES + 1);
    gTiles[gMovingTileIndex].update(deltaTime, nullptr, 0);

    const float x = gTiles[gMovingTileIndex].getPosition().x;
    const float rightEdge = x + TILE_DIMENSION;

    // Update moving platform
    if (x <= BOUNDARY_X_MIN) {
        gTiles[gMovingTileIndex].setMovement({ +1.0f, 0.0f });
    } else if (rightEdge >= BOUNDARY_X_MAX) {
        gTiles[gMovingTileIndex].setMovement({ -1.0f, 0.0f });
    }

    if (gShip->isCollidingBottom()) {
        // Check for win condition
        if (gShip->getCollisionStatus() == PAD && 
                gShip->getImpactVelocity().y <= WIN_SPEED && 
                std::fabs(gShip->getAngle()) <= WIN_ANGLE &&
                !gShip->isCollidingTop()) {
            endScreen(SUCCESSFUL_LANDING, LANDING_POINTS + static_cast<int>(fuel / 10));
            }
        else { endScreen(CRASH_LANDING, static_cast<int>(fuel / 10)); }
    }

   // Screen wrap around
    Vector2 shipPosition = gShip->getPosition();
    float  midpoint = gShip->getColliderDimensions().x * 0.5f;
    if (shipPosition.x - midpoint > SCREEN_WIDTH) shipPosition.x = -midpoint;
    else if (shipPosition.x + midpoint < 0) shipPosition.x = SCREEN_WIDTH + midpoint;
    gShip->setPosition(shipPosition);


    // Unused end game message
    // if (fuel <= 0) endScreen(OUT_OF_FUEL, 0);
}

void render() 
{ 
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));
    
    if (gGameState == PLAY) {
        // Render HUD
        DrawText(TextFormat("Fuel: %04i", fuel), ORIGIN.x + 400 , ORIGIN.y - 500, 40, RED);
        DrawText(TextFormat("Speed: %03i", static_cast<int>(gShip->getVelocity().y)), ORIGIN.x + 400, ORIGIN.y - 400, 40, RED);
        
        gShip->render();
        for (int i = 0; i < NUMBER_OF_TILES + 1; i++) {
            gTiles[i].render();
        }

    }

    // Render end game UI
    if (gGameState == GAME_OVER) {
        drawMultilineText(gEndMessage, ORIGIN.x - 300, ORIGIN.y - 40, 40, BLACK);
        DrawText(TextFormat("Points: %03i", gEndPoints), ORIGIN.x - 300, ORIGIN.y + 80, 40, BLACK);
    }
  
    EndDrawing();
}

void shutdown() 
{ 
    CloseWindow();
}
void setTiles(Entity* gTiles, int NUMBER_OF_TILES){
    for (int i = 0; i < NUMBER_OF_TILES; i++) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 4);
        int rng = dist(gen);

        if (rng == 0) {
            gTiles[i].setStatus(PAD);
            gTiles[i].setTexture("assets/game/pad_tile.png");
        }
        else {
            gTiles[i].setStatus(TERRAIN);
            gTiles[i].setTexture("assets/game/terrain_tile.png");
        }
    }

    gTiles[NUMBER_OF_TILES].setStatus(PAD);
    gTiles[NUMBER_OF_TILES].setTexture("assets/game/pad_tile.png");
    gTiles[NUMBER_OF_TILES].setMovement({1.0f, 0.0f});

}

// Reset game state
void startNewGame() {
    gGameState = PLAY;
    gEndMessage.clear();
    gEndPoints = 0;

    fuel = MAX_FUEL;
    fuel_used = false;

    gShip->setStatus(PLAYER);
    gShip->setPosition({ ORIGIN.x - 300.0f, ORIGIN.y - 500.0f });
    gShip->setVelocity({ 0.0f, 0.0f });
    gShip->setAcceleration({ 0.0f, GRAVITY });
    gShip->setAngle(0.0f);
    gShip->setRotation(0);

    setTiles(gTiles, NUMBER_OF_TILES);

    gTiles[gMovingTileIndex].setStatus(PAD);
    gTiles[gMovingTileIndex].setTexture("assets/game/pad_tile.png");
    gTiles[gMovingTileIndex].setPosition({ BOUNDARY_X_MIN, PLATFORM_Y });
    gTiles[gMovingTileIndex].setSpeed((int)PLATFORM_SPEED);
    gTiles[gMovingTileIndex].setMovement({ +1.0f, 0.0f });

}


void endScreen(const std::string& endMessage, int points) {
    gEndMessage = endMessage;
    gEndPoints = points;
    gGameState = GAME_OVER;
}

int main(void)
{
    initialise();

    while (gAppStatus == RUNNING)
    {
        processInput();
        update();
        render();
    }

    shutdown();

    return 0;
}