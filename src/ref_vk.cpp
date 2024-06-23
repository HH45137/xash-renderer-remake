#include <ref_vk.h>
#include <iostream>


qboolean R_Init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window = SDL_CreateWindow(
            "Test window",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            1280, 720,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    return false;
}

void R_Shutdown(void) {
    SDL_DestroyWindow(window);
    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

const char *R_GetConfigName(void) {
    return "vulkan";
}
