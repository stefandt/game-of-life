#include "cell_textures.h"
#include <cmath>

static Image gen_alive_image(int sz)
{
    Image img = GenImageColor(sz, sz, {0, 0, 0, 0});
    const Vector2 c = {(float)sz * 0.5f, (float)sz * 0.5f};
    const int r = sz / 2 - 1;

    // Membrane — dark green outer ring
    ImageDrawCircleV(&img, c, r,      {18,  90,  40, 255});
    // Cell body — medium green
    ImageDrawCircleV(&img, c, r - 4,  {35, 160,  65, 255});
    // Inner glow
    ImageDrawCircleV(&img, c, r - 10, {55, 200,  85, 255});
    // Nucleus
    ImageDrawCircleV(&img, c, r / 3,  {110, 250, 130, 255});
    // Highlight (off-center bright spot — gives 3D feel)
    Vector2 hi = {c.x - r * 0.25f, c.y - r * 0.25f};
    ImageDrawCircleV(&img, hi, r / 6, {200, 255, 210, 160});
    // Membrane outline
    ImageDrawCircleLinesV(&img, c, r, {10, 60, 25, 255});

    return img;
}

static Image gen_dead_image(int sz)
{
    Image img = GenImageColor(sz, sz, {0, 0, 0, 0});
    const Vector2 c = {(float)sz * 0.5f, (float)sz * 0.5f};
    const int r = sz / 2 - 1;

    // Flat dark navy fill
    ImageDrawCircleV(&img, c, r,     {5,  12, 48, 255});
    // Subtle lighter edge so cells don't merge visually
    ImageDrawCircleLinesV(&img, c, r, {10, 22, 70, 255});

    return img;
}

void CellTextures::load()
{
    // ── Load from texture atlas ───────────────────────────────────────────
    // Atlas: atlas_hex_pent_2048x256.png, 8 tiles of 256×256.
    // Tile 0 = Hex Empty  (U 0.000–0.125)  → pixel x 0–256
    // Tile 1 = Hex Life L1 (U 0.125–0.250) → pixel x 256–512
    Image atlas = LoadImage("res/atlas_hex_pent_2048x256.png");

    Image id = ImageFromImage(atlas, {  0, 0, 256, 256 });  // Hex Empty
    Image ia = ImageFromImage(atlas, {256, 0, 256, 256 });  // Hex Life L1

    dead  = LoadTextureFromImage(id);
    alive = LoadTextureFromImage(ia);
    SetTextureFilter(dead,  TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(alive, TEXTURE_FILTER_BILINEAR);

    UnloadImage(id);
    UnloadImage(ia);
    UnloadImage(atlas);

    // ── Procedural fallback (kept for reference) ──────────────────────────
    // constexpr int SZ = 64;
    //
    // Image ia = gen_alive_image(SZ);
    // ExportImage(ia, "debug_cell_alive.png");
    // alive = LoadTextureFromImage(ia);
    // SetTextureFilter(alive, TEXTURE_FILTER_BILINEAR);
    // UnloadImage(ia);
    //
    // Image id = gen_dead_image(SZ);
    // ExportImage(id, "debug_cell_dead.png");
    // dead = LoadTextureFromImage(id);
    // SetTextureFilter(dead, TEXTURE_FILTER_BILINEAR);
    // UnloadImage(id);
}

void CellTextures::unload()
{
    UnloadTexture(alive);
    UnloadTexture(dead);
}
