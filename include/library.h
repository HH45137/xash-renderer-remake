#pragma once

#define EXPORT_DLL __declspec(dllexport)

namespace TEST {
    extern "C" EXPORT_DLL void hello();
}
