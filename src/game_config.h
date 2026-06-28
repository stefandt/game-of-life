#pragma once

enum Rules : int {
    Rules_B2_S23 = 0,  // birth=2, survive=2-3  (hex-friendly)
    Rules_B3_S23 = 1,  // birth=3, survive=2-3  (classic Conway)
    Rules_B2_S34 = 2,  // birth=2, survive=3-4  (hex life)
    Rules_Count  = 3
};

enum SeedSize : int {
    Seed10   = 0,
    Seed25   = 1,
    Seed50   = 2,
    Seed100  = 3,
    Seed200  = 4,
    Seed500  = 5,
    Seed1000 = 6,
    Seed2000 = 7,
    SeedCount = 8
};

// Total cells = 10 × 4^N + 2   (hexagons = 10×(4^N−1),  pentagons = 12)
//   N=2 →    162   ( 150 hex + 12 pent)
//   N=3 →    642   ( 630 hex + 12 pent)
//   N=4 →  2 562   (2550 hex + 12 pent)
//   N=5 → 10 242   (10230 hex + 12 pent)
enum Subdiv : int {
    Subdiv2 = 2,   //    162 cells
    Subdiv3 = 3,   //    642 cells
    Subdiv4 = 4,   //  2 562 cells
    Subdiv5 = 5,   // 10 242 cells
};

// Game settings — the source of truth for all configurable parameters.
struct GameConfig {
    Subdiv   subdiv    = Subdiv4;
    SeedSize seed_size = Seed10;
    Rules    rules     = Rules_B2_S23;
    float    speed     = 1.0f;
    bool     paused    = false;

    static constexpr int SEED_VALS[8] = {10, 25, 50, 100, 200, 500, 1000, 2000};

    int seed_count() const { return SEED_VALS[seed_size]; }
};
