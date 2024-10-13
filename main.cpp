// #include <WinUser.h>
#include "render.h"
#include "battle.h"

// idea: when our ships take damage, spread out fire in a particle system via cellular automata 
// 
// idea: when no ships are selected, holding right click lets you rotate the camera
// 
// idea: clear sight, max sight, and radar site radii. when a ship is in radar site, hit probability is the lowest. when in  max sight it increases, clear sight it increases again.
// you could visualize these with circles, another cool thing to visualize would be a radar circle that grows as the 'pulse' goes on
// idea: spawning new ships costs money, killing ships grants money, also passive income

// todo: experiment with range circles

int main()
{
    g_render = std::make_shared<Render>();

    if (!g_render->init())
        return -1;

    g_render->render();

    return 0;
}
