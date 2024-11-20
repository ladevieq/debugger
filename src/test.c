#include <Windows.h>
int mainCRTStartup() {
    unsigned long long int i = 0;

    while (1) {
        __debugbreak();
        OutputDebugStringA("Hello World !");
        i++;
    }

    return 0;
}
