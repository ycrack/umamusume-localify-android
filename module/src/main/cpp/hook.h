#ifndef UMAMUSUMELOCALIFYANDROID_HOOK_H
#define UMAMUSUMELOCALIFYANDROID_HOOK_H

#include "log.h"

static bool enable_hack;
static bool enable_settings_hack;
static void *il2cpp_handle = nullptr;
static void *app_handle = nullptr;
static void *native_handle = nullptr;

bool isGame(const char *appDataDir);
bool isSettings(const char *appDataDir);

void hack_thread(void *arg);
void hack_settings_thread(void *arg);

#define HOOK_DEF(ret, func, ...) \
  ret (*orig_##func)(__VA_ARGS__); \
  ret new_##func(__VA_ARGS__)

#define ENABLE_HOOK(handle, func) \
    void *func##_addr = dlsym(handle, #func); \
    if (func##_addr) { \
        LOGI(#func " at: %p", func##_addr); \
        DobbyHook(func##_addr, (void *) new_##func, (void **) &orig_##func); \
    }

#endif //UMAMUSUMELOCALIFYANDROID_HOOK_H
