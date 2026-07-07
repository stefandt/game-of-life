#pragma once

#include "game_config.h"

// Everything a UI can write to control the running app — the raygui panel
// today, a JS panel later. GameConfig stays the simulation's own rule
// parameters (nested here as the config sub-surface); GameWorld stays the
// read-only derived state (generation, alive_count, ...) and is never
// duplicated into this struct — readers pull it directly from GameWorld.
struct SimControls {
    GameConfig cfg;

    bool  is_orbital   = false;
    bool  use_textures = true;
    float cam_yaw      = 0.0f;
    float cam_pitch    = 20.0f;
    float cam_distance = 9.0f;

    // Set by a UI, cleared by the engine once handled.
    bool restart_requested = false;
    bool rebuild_requested = false;
};
