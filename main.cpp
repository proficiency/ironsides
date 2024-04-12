// #include <WinUser.h>
#include "render.h"
#include "battle.h"

int main()
{
    std::shared_ptr<Render> renderer = std::make_shared<Render>();

    if (!renderer->init())
        return -1;

    renderer->render();

    return 0;
}
