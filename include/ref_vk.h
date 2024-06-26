#pragma once

#include <common/Typedef.h>

#define EXPORT_DLL __declspec(dllexport)


extern "C" {

EXPORT_DLL REF_VK::qboolean R_Init(void);

EXPORT_DLL void R_Shutdown(void);

EXPORT_DLL const char *R_GetConfigName(void);

}
