#include "cell_atlas.h"

// Tile width in atlas UV space: 1/8 per tile
static constexpr float TILE_W = 1.0f / 8.0f;

// Tile index → U start
// Layout: 0=HexEmpty, 1=HexL1, 2=HexL2, 3=HexL3,
//         4=PentEmpty, 5=PentL1, 6=PentL2, 7=PentL3
static float tile_u0(int idx)
{
    return idx * TILE_W;
}

bool CellAtlas::load(const char* path)
{
    texture = LoadTexture(path);
    if (texture.id == 0) return false;
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return true;
}

void CellAtlas::unload()
{
    UnloadTexture(texture);
    texture = {};
}

AtlasRect CellAtlas::rect(bool pentagon, bool alive, LifeLevel level) const
{
    // Base offset: hexagons start at tile 0, pentagons at tile 4
    int idx = pentagon ? 4 : 0;

    if (alive)
        idx += 1 + (int)level;   // L1→+1, L2→+2, L3→+3
    // else idx stays at 0 (hex empty) or 4 (pent empty)

    const float u0 = tile_u0(idx);
    return {u0, u0 + TILE_W};
}
