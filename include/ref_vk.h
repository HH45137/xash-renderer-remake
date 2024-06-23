#pragma once

#include <common/typedef.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#define EXPORT_DLL __declspec(dllexport)


extern "C" {

SDL_Window *window;

EXPORT_DLL qboolean R_Init(void);

EXPORT_DLL void R_Shutdown(void);

EXPORT_DLL const char *R_GetConfigName(void);

}
