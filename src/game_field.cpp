#include "game_field.h"

#include <algorithm>
#include <numeric>
#include <vector>
#include <cstdlib>

static constexpr Color COLOR_DEAD_HEX   = {5,   15,  50,  255};
static constexpr Color COLOR_DEAD_PENT  = {8,   20,  70,  255};
static constexpr Color COLOR_ALIVE_HEX  = {40,  210, 80,  255};
static constexpr Color COLOR_ALIVE_PENT = {120, 255, 120, 255};

GameField::GameField(const HexSphere& mesh) : sphere(mesh)
{
    cells.resize(sphere.faces.size());
    for (int i = 0; i < (int)sphere.faces.size(); ++i)
        refresh_color(i);
}

void GameField::refresh_color(int idx)
{
    const bool pent  = sphere.faces[idx].pentagon;
    cells[idx].color = cells[idx].alive
        ? (pent ? COLOR_ALIVE_PENT : COLOR_ALIVE_HEX)
        : (pent ? COLOR_DEAD_PENT  : COLOR_DEAD_HEX);
}

void GameField::set(int idx, bool alive)
{
    if (!alive) 
        cells[idx].age = 0;   // organism dies — reset age
    cells[idx].alive = alive;
    refresh_color(idx);
}

void GameField::seed(int count, int rng_seed, int start_face)
{
    for (int i = 0; i < (int)cells.size(); ++i)
        set(i, false);

    std::srand(rng_seed);

    const int start = (start_face >= 0 && start_face < (int)cells.size())
                    ? start_face
                    : std::rand() % (int)cells.size();

    std::vector<bool> visited(cells.size(), false);
    std::vector<int>  frontier;
    frontier.push_back(start);
    visited[start] = true;

    int placed = 0;
    while (placed < count && !frontier.empty()) {
        // Pick a random cell from the current frontier
        int fi   = std::rand() % (int)frontier.size();
        int cell = frontier[fi];
        frontier.erase(frontier.begin() + fi);

        set(cell, true);
        ++placed;

        // Expand frontier to unvisited neighbors
        for (int nb : sphere.faces[cell].neighbors) {
            if (!visited[nb]) {
                visited[nb] = true;
                frontier.push_back(nb);
            }
        }
    }
}

void GameField::step()
{
    const int n = (int)cells.size();
    std::vector<int> live_count(n, 0);

    // Each live cell adds 1 to the counter of each of its neighbors —
    // so live_count[j] ends up = number of live cells adjacent to cell j.
    for (int i = 0; i < n; ++i) {
        if (!cells[i].alive) continue;
        for (int nb : sphere.faces[i].neighbors)
            ++live_count[nb];
    }

    std::vector<bool> next(n);
    for (int i = 0; i < n; ++i) {
        const int c = live_count[i];
        next[i] = cells[i].alive ? (c >= rule_s_lo && c <= rule_s_hi)  // survives
                                 : (c >= rule_b_lo && c <= rule_b_hi); // born
    }
    for (int i = 0; i < n; ++i) {
        if (next[i] != cells[i].alive)
            set(i, next[i]);
        if (cells[i].alive)
            ++cells[i].age;   // organism survived another generation
    }
}
