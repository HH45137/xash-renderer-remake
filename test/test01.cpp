#include <ref_vk.h>
#include <iostream>
#include <windows.h>

#include <SDL2/SDL.h>

void init() {
    R_Init();
}

void update() {

}

void destroy() {
    R_Shutdown();
}

int main(int argc, char *argv[]) {

    init();

    REF_VK::qboolean running = true;
    while (running) {
        update();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
        }
    }

    destroy();

    return 0;
}
