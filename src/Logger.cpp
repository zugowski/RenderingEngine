#include <cstdio>
#include <cstdarg>
#include <Windows.h>

void OutputLog(const char* format, ...)
{
    char msg[2048];
    memset(msg, '\0', sizeof(msg));
    va_list arg;

    va_start(arg, format);
    vsprintf_s(msg, format, arg);
    va_end(arg);

    printf_s("%s", msg);

    OutputDebugStringA(msg);
}