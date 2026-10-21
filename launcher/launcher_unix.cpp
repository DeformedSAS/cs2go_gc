#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(DEDICATED)
#define LAUNCHER_LIB "dedicated"
#define SYMBOL_NAME "DedicatedMain"
#else
#define LAUNCHER_LIB "launcher"
#define SYMBOL_NAME "LauncherMain"
#endif

typedef int (*LauncherMain_t)(int argc, char **argv);

// csgo_gc/main.cpp
void InstallGC(bool dedicated);

static void ErrorMessageBox(const char *format, ...)
{
    va_list ap;
    char message[4096];

    va_start(ap, format);
    vsnprintf(message, sizeof(message), format, ap);
    va_end(ap);

    fprintf(stderr, "csgo_gc: %s\n", message);

#if defined(__APPLE__)
    CFStringRef messageString = CFStringCreateWithCString(kCFAllocatorDefault, message, kCFStringEncodingUTF8);
    CFOptionFlags responseFlags;

    CFUserNotificationDisplayAlert(0,
        kCFUserNotificationStopAlertLevel,
        nullptr, nullptr, nullptr,
        CFSTR("csgo_gc"),
        messageString,
        nullptr, nullptr, nullptr,
        &responseFlags);

    CFRelease(messageString);
#endif
}

static void *LoadModuleAndFindSymbol(const char *modulePath, const char *symbol)
{
    void *module = dlopen(modulePath, RTLD_NOW);
    if (!module)
    {
        ErrorMessageBox("%s", dlerror());
        return nullptr;
    }

    void *function = dlsym(module, symbol);
    if (!function)
    {
        ErrorMessageBox("%s", dlerror());
        return nullptr;
    }

    return function;
}

int main(int argc, char **argv)
{
    // Define the custom arguments you want to add
    const char *extraArgs[] = {
        "-game", "cs2go/csgo",
        "-language", "cs2go",
        "-steam"
    };
    int extraCount = sizeof(extraArgs) / sizeof(extraArgs[0]);

    // Allocate a new argv array: original argc + extra arguments + 1 for nullptr terminator
    int newArgc = argc + extraCount;
    char **newArgv = (char **)malloc((newArgc + 1) * sizeof(char *));
    if (!newArgv)
    {
        ErrorMessageBox("Failed to allocate memory for command-line arguments.");
        return 1;
    }

    // Copy original arguments
    for (int i = 0; i < argc; i++)
    {
        newArgv[i] = argv[i];
    }

    // Append your custom mod arguments
    for (int i = 0; i < extraCount; i++)
    {
        newArgv[argc + i] = (char *)extraArgs[i];
    }
    newArgv[newArgc] = nullptr;

    const char *modulePath = "bin/" GC_LIB_DIR "/" LAUNCHER_LIB GC_LIB_SUFFIX GC_LIB_EXTENSION;
    LauncherMain_t LauncherMain = (LauncherMain_t)LoadModuleAndFindSymbol(modulePath, SYMBOL_NAME);
    if (!LauncherMain)
    {
        free(newArgv);
        return 1;
    }

#if defined(DEDICATED)
    InstallGC(true);
#else
    InstallGC(false);
#endif

    int result = LauncherMain(newArgc, newArgv);

    free(newArgv);
    return result;
}