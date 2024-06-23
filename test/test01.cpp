#include <library.h>
#include <windows.h>

int main() {
    HINSTANCE handle = LoadLibrary("ref_vk.dll");
    auto f = GetProcAddress(handle, "hello");

    f();

    FreeLibrary(handle);

    return 0;
}
