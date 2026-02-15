#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr int kTileSize = 24;
constexpr int kWorldWidth = 300;
constexpr int kWorldHeight = 140;

enum class Tile : std::uint8_t {
    Empty,
    Grass,
    Dirt,
    Stone,
};

struct Player {
    float x = 0.0f;
    float y = 0.0f;
    float w = 18.0f;
    float h = 34.0f;
    float vx = 0.0f;
    float vy = 0.0f;
};

struct World {
    std::vector<Tile> tiles;

    World() : tiles(kWorldWidth * kWorldHeight, Tile::Empty) {}

    [[nodiscard]] bool inBounds(int tx, int ty) const {
        return tx >= 0 && tx < kWorldWidth && ty >= 0 && ty < kWorldHeight;
    }

    [[nodiscard]] Tile get(int tx, int ty) const {
        if (!inBounds(tx, ty)) {
            return Tile::Stone;
        }
        return tiles[ty * kWorldWidth + tx];
    }

    void set(int tx, int ty, Tile tile) {
        if (!inBounds(tx, ty)) {
            return;
        }
        tiles[ty * kWorldWidth + tx] = tile;
    }

    [[nodiscard]] bool solidAt(int tx, int ty) const {
        return get(tx, ty) != Tile::Empty;
    }
};

[[nodiscard]] bool rectIntersectsSolid(const World& world, float x, float y, float w, float h) {
    const int left = static_cast<int>(std::floor(x / kTileSize));
    const int right = static_cast<int>(std::floor((x + w - 0.01f) / kTileSize));
    const int top = static_cast<int>(std::floor(y / kTileSize));
    const int bottom = static_cast<int>(std::floor((y + h - 0.01f) / kTileSize));

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (world.solidAt(tx, ty)) {
                return true;
            }
        }
    }
    return false;
}

void generateTerrain(World& world) {
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> wobble(-1, 1);
    std::uniform_real_distribution<float> caveChance(0.0f, 1.0f);

    int surface = kWorldHeight / 2;
    for (int x = 0; x < kWorldWidth; ++x) {
        surface = std::clamp(surface + wobble(rng), kWorldHeight / 3, (kWorldHeight * 2) / 3);

        for (int y = surface; y < kWorldHeight; ++y) {
            Tile tile = Tile::Dirt;
            if (y == surface) {
                tile = Tile::Grass;
            } else if (y > surface + 14) {
                tile = Tile::Stone;
            }

            if (y > surface + 4 && y < kWorldHeight - 6 && caveChance(rng) < 0.08f) {
                tile = Tile::Empty;
            }

            world.set(x, y, tile);
        }
    }
}

void drawTile(SDL_Renderer* renderer, Tile tile, float x, float y, float size) {
    switch (tile) {
        case Tile::Grass:
            SDL_SetRenderDrawColor(renderer, 70, 180, 90, 255);
            break;
        case Tile::Dirt:
            SDL_SetRenderDrawColor(renderer, 125, 80, 45, 255);
            break;
        case Tile::Stone:
            SDL_SetRenderDrawColor(renderer, 95, 95, 105, 255);
            break;
        case Tile::Empty:
        default:
            return;
    }

    SDL_FRect tileRect{x, y, size, size};
    SDL_RenderFillRect(renderer, &tileRect);
}

}  // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL3 Terraria-ish Sandbox", kWindowWidth, kWindowHeight, 0);
    if (window == nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    World world;
    generateTerrain(world);

    Player player;
    player.x = (kWorldWidth / 2.0f) * kTileSize;
    player.y = (kWorldHeight / 3.0f) * kTileSize;

    bool running = true;
    Uint64 previousTicks = SDL_GetTicks();

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        const Uint64 now = SDL_GetTicks();
        float dt = static_cast<float>(now - previousTicks) / 1000.0f;
        previousTicks = now;
        dt = std::min(dt, 0.033f);

        const bool* keys = SDL_GetKeyboardState(nullptr);
        float moveInput = 0.0f;
        if (keys[SDL_SCANCODE_A]) {
            moveInput -= 1.0f;
        }
        if (keys[SDL_SCANCODE_D]) {
            moveInput += 1.0f;
        }

        player.vx = moveInput * 220.0f;
        player.vy += 900.0f * dt;

        const bool onGround = rectIntersectsSolid(world, player.x, player.y + 1.0f, player.w, player.h);
        if (keys[SDL_SCANCODE_SPACE] && onGround) {
            player.vy = -430.0f;
        }

        float nextX = player.x + player.vx * dt;
        if (!rectIntersectsSolid(world, nextX, player.y, player.w, player.h)) {
            player.x = nextX;
        }

        float nextY = player.y + player.vy * dt;
        if (!rectIntersectsSolid(world, player.x, nextY, player.w, player.h)) {
            player.y = nextY;
        } else {
            if (player.vy > 0) {
                player.y = std::floor((player.y + player.h) / kTileSize) * kTileSize - player.h;
            }
            player.vy = 0.0f;
        }

        float cameraX = player.x + player.w * 0.5f - kWindowWidth * 0.5f;
        float cameraY = player.y + player.h * 0.5f - kWindowHeight * 0.5f;
        cameraX = std::clamp(cameraX, 0.0f, static_cast<float>(kWorldWidth * kTileSize - kWindowWidth));
        cameraY = std::clamp(cameraY, 0.0f, static_cast<float>(kWorldHeight * kTileSize - kWindowHeight));

        float mouseX = 0.0f;
        float mouseY = 0.0f;
        const Uint32 mouse = SDL_GetMouseState(&mouseX, &mouseY);
        const int tileX = static_cast<int>((mouseX + cameraX) / kTileSize);
        const int tileY = static_cast<int>((mouseY + cameraY) / kTileSize);

        if (mouse & SDL_BUTTON_LMASK) {
            world.set(tileX, tileY, Tile::Empty);
        }
        if (mouse & SDL_BUTTON_RMASK) {
            if (world.get(tileX, tileY) == Tile::Empty) {
                world.set(tileX, tileY, Tile::Dirt);
            }
        }

        SDL_SetRenderDrawColor(renderer, 95, 175, 245, 255);
        SDL_RenderClear(renderer);

        const int firstTileX = std::max(0, static_cast<int>(cameraX / kTileSize));
        const int firstTileY = std::max(0, static_cast<int>(cameraY / kTileSize));
        const int lastTileX = std::min(kWorldWidth - 1, firstTileX + (kWindowWidth / kTileSize) + 2);
        const int lastTileY = std::min(kWorldHeight - 1, firstTileY + (kWindowHeight / kTileSize) + 2);

        for (int ty = firstTileY; ty <= lastTileY; ++ty) {
            for (int tx = firstTileX; tx <= lastTileX; ++tx) {
                const float screenX = tx * kTileSize - cameraX;
                const float screenY = ty * kTileSize - cameraY;
                drawTile(renderer, world.get(tx, ty), screenX, screenY, static_cast<float>(kTileSize));
            }
        }

        SDL_SetRenderDrawColor(renderer, 220, 80, 60, 255);
        SDL_FRect playerRect{player.x - cameraX, player.y - cameraY, player.w, player.h};
        SDL_RenderFillRect(renderer, &playerRect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 160);
        SDL_FRect cursorOutline{tileX * kTileSize - cameraX, tileY * kTileSize - cameraY, static_cast<float>(kTileSize), static_cast<float>(kTileSize)};
        SDL_RenderRect(renderer, &cursorOutline);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
