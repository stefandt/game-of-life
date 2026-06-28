#pragma once
#include "raylib.h"

// Two 64×64 circular textures generated at startup.
// Both use radial UV mapping: center → (0.5, 0.5), rim → unit circle.
struct CellTextures {
    Texture2D alive;   // bright green bacteria cell
    Texture2D dead;    // dark navy flat circle

    void load();
    void unload();
};
