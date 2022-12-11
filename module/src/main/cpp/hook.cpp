#include <sstream>
#include <thread>
#include "stdinclude.hpp"
#include "hook.h"
#include "il2cpp_hook.h"
#include "localify/localify.h"
#include "logger/logger.h"
#include "config.hpp"
#include <sys/stat.h>

using namespace std;
using namespace localify;
using namespace logger;

bool g_enable_logger = false;
int g_max_fps = -1;
float g_ui_animation_scale = 1.0f;
bool g_ui_use_system_resolution = false;
float g_resolution_3d_scale = 1.0f;
bool g_replace_to_builtin_font = false;
bool g_replace_to_custom_font = false;
std::string g_font_assetbundle_path;
std::string g_font_asset_name;
std::string g_tmpro_font_asset_name;
bool g_dump_entries = false;
bool g_dump_db_entries = false;
bool g_static_entries_use_hash = false;
bool g_static_entries_use_text_id_name = false;
int g_graphics_quality = -1;
int g_anti_aliasing = -1;
bool g_force_landscape = false;
float g_force_landscape_ui_scale = 0.5f;
bool g_ui_loading_show_orientation_guide = true;
bool g_restore_notification = false;
std::unordered_map<std::string, ReplaceAsset> g_replace_assets;
std::string g_replace_assetbundle_file_path;
std::string text_id_dict;
bool g_enable_carrotjuicer = true;

bool isGame(const char *pkgNm) {
    if (!pkgNm)
        return false;
    if (Game::IsPackageNameEqualsByGameRegion(pkgNm, Game::Region::JAP) ||
        Game::IsPackageNameEqualsByGameRegion(pkgNm, Game::Region::KOR) ||
        Game::IsPackageNameEqualsByGameRegion(pkgNm, Game::Region::TWN)) {
        LOGI("detect package: %s", pkgNm);
        return true;
    }
    return false;
}

bool isSettings(const char *pkgNm) {
    if (!pkgNm)
        return false;
    if ("com.kimjio.umamusumelocalify.settings"s == pkgNm) {
        return true;
    }
    return false;

}

HOOK_DEF(jstring, getModuleVersion,
         JNIEnv *env,
         jclass) {
    return env->NewStringUTF(Module::moduleVersionName);
}

bool isCriWareInit = false;

void dlopen_process(const char *name, void *handle) {
    if (!il2cpp_handle) {
        if (name != nullptr && strstr(name, "libil2cpp.so")) {
            il2cpp_handle = handle;
            LOGI("Got il2cpp handle!");
        }
    }
    if (!native_handle) {
        if (name != nullptr && strstr(name, "libnative.so")) {
            native_handle = handle;
            LOGI("Got native handle!");
        }
    }
    if (name != nullptr && strstr(name, "libcri_ware_unity.so") && !isCriWareInit) {
        isCriWareInit = true;
        il2cpp_load_assetbundle();
    }
    if (!il2cpp_handle && !app_handle) {
        if (name != nullptr && strstr(name, "libapp.so")) {
            app_handle = handle;
            auto getModuleVersion = dlsym(app_handle,
                                          "Java_com_kimjio_umamusumelocalify_settings_ModuleUtils_getModuleVersion");
            DobbyHook(getModuleVersion, (void *) new_getModuleVersion,
                      (void **) &orig_getModuleVersion);
        }
    }
}

HOOK_DEF(void*, do_dlopen, const char *name, int flags) {
    void *handle = orig_do_dlopen(name, flags);
    dlopen_process(name, handle);
    return handle;
}

HOOK_DEF(void*, __loader_dlopen, const char *filename, int flags, const void *caller_addr) {
    void *handle = orig___loader_dlopen(filename, flags, caller_addr);
    dlopen_process(filename, handle);
    return handle;
}

HOOK_DEF(void*, do_dlopen_V24, const char *name, int flags, const void *extinfo [[maybe_unused]],
         void *caller_addr) {
    void *handle = orig_do_dlopen_V24(name, flags, extinfo, caller_addr);
    dlopen_process(name, handle);
    return handle;
}

HOOK_DEF(void*, do_dlopen_V19, const char *name, int flags, const void *extinfo [[maybe_unused]]) {
    void *handle = orig_do_dlopen_V19(name, flags, extinfo);
    dlopen_process(name, handle);
    return handle;
}

HOOK_DEF(void*, NativeBridgeLoadLibrary_V21, const char *filename, int flag) {
    if (string(filename).find(string("libmain.so")) != string::npos) {
        auto *NativeBridgeError = reinterpret_cast<bool (*)()>(DobbySymbolResolver(nullptr,
                                                                                   "_ZN7android17NativeBridgeErrorEv"));

        stringstream path_armV8;
        path_armV8 << "/data/data/" << Game::GetCurrentPackageName().data() << "/arm64-v8a.so";
        stringstream path_armV7;
        path_armV7 << "/data/data/" << Game::GetCurrentPackageName().data() << "/armeabi-v7a.so";

        string path;

        if (access(path_armV8.str().data(), F_OK) != -1) {
            path = path_armV8.str();
        } else if (access(path_armV7.str().data(), F_OK) != -1) {
            path = path_armV7.str();
        }

        if (!path.empty()) {
            std::thread load_thread([path, NativeBridgeError]() {
                void *lib = orig_NativeBridgeLoadLibrary_V21(path.data(), RTLD_NOW);
                LOGI("%s: %p", path.data(), lib);
                if (NativeBridgeError()) {
                    LOGW("LoadLibrary failed");
                }
                DobbyDestroy((void *) new_do_dlopen_V24);
            });
            load_thread.detach();
        }
    }

    return orig_NativeBridgeLoadLibrary_V21(filename, flag);
}

HOOK_DEF(void*, NativeBridgeLoadLibraryExt_V26, const char *filename, int flag,
         struct native_bridge_namespace_t *ns) {
    if (string(filename).find(string("libmain.so")) != string::npos) {
        auto *NativeBridgeError = reinterpret_cast<bool (*)()>(DobbySymbolResolver(nullptr,
                                                                                   "_ZN7android17NativeBridgeErrorEv"));
        auto *NativeBridgeGetError = reinterpret_cast<char *(*)()>(DobbySymbolResolver(nullptr,
                                                                                       "_ZN7android20NativeBridgeGetErrorEv"));

        stringstream path_armV8;
        path_armV8 << "/data/data/" << Game::GetCurrentPackageName().data() << "/arm64-v8a.so";
        stringstream path_armV7;
        path_armV7 << "/data/data/" << Game::GetCurrentPackageName().data() << "/armeabi-v7a.so";

        string path;

        if (access(path_armV8.str().data(), F_OK) != -1) {
            path = path_armV8.str();
        } else if (access(path_armV7.str().data(), F_OK) != -1) {
            path = path_armV7.str();
        }

        if (!path.empty()) {
            std::thread load_thread([path, ns, NativeBridgeError, NativeBridgeGetError]() {
                void *lib = orig_NativeBridgeLoadLibraryExt_V26(path.data(), RTLD_NOW, ns);
                LOGI("%s: %p", path.data(), lib);
                if (NativeBridgeError()) {
                    char *error_bridge;
                    if ((error_bridge = NativeBridgeGetError()) !=
                        nullptr) {
                        LOGW("error_bridge: %s", error_bridge);
                    }
                }
                DobbyDestroy((void *) new_do_dlopen_V24);
            });
            load_thread.detach();
        }
    }

    return orig_NativeBridgeLoadLibraryExt_V26(filename, flag, ns);
}

HOOK_DEF(void*, NativeBridgeLoadLibraryExt_V30, const char *filename, int flag,
         struct native_bridge_namespace_t *ns) {
    if (string(filename).find(string("libmain.so")) != string::npos) {
        auto *NativeBridgeError = reinterpret_cast<bool (*)()>(DobbySymbolResolver(nullptr,
                                                                                   "NativeBridgeError"));
        auto *NativeBridgeGetError = reinterpret_cast<char *(*)()>(DobbySymbolResolver(nullptr,
                                                                                       "NativeBridgeGetError"));

        stringstream path_armV8;
        path_armV8 << "/data/data/" << Game::GetCurrentPackageName().data() << "/arm64-v8a.so";
        stringstream path_armV7;
        path_armV7 << "/data/data/" << Game::GetCurrentPackageName().data() << "/armeabi-v7a.so";

        string path;

        if (access(path_armV8.str().data(), F_OK) != -1) {
            path = path_armV8.str();
        } else if (access(path_armV7.str().data(), F_OK) != -1) {
            path = path_armV7.str();
        }

        if (!path.empty()) {
            std::thread load_thread([path, ns, NativeBridgeError, NativeBridgeGetError]() {
                void *lib = orig_NativeBridgeLoadLibraryExt_V30(path.data(), RTLD_NOW, ns);
                LOGI("%s: %p", path.data(), lib);
                if (NativeBridgeError()) {
                    char *error_bridge;
                    if ((error_bridge = NativeBridgeGetError()) !=
                        nullptr) {
                        LOGW("error_bridge: %s", error_bridge);
                    }
                }
                DobbyDestroy((void *) new_do_dlopen_V24);
            });
            load_thread.detach();
        }
    }

    return orig_NativeBridgeLoadLibraryExt_V30(filename, flag, ns);
}

void write_file(const std::string &file_path, const char *buffer, const int len) {
    std::ofstream file{file_path, std::ios::binary};
    file.write(buffer, len);
    file.close();

    if (std::filesystem::exists(file_path)) {
        LOGI("wrote file: [%ju] %s", std::filesystem::file_size(file_path.c_str()), file_path.c_str());
    }
}

std::string current_time() {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );
    return std::to_string(ms.count());
}

HOOK_DEF(int, LZ4_decompress_safe_ext, char *src, char *dst, int compressedSize, int dstCapacity) {
    int ret = orig_LZ4_decompress_safe_ext(src, dst, compressedSize, dstCapacity);
    auto out_path = string("/sdcard/Android/data/").append(Game::GetCurrentPackageName())
            .append("/CarrotJuicer/").append(current_time()).append("R.msgpack");

    LOGI("[R] cmp=%d, dst=%d, ret=%d => %s", compressedSize, dstCapacity, ret, out_path.c_str());
    write_file(out_path, dst, ret);

    return ret;
}

HOOK_DEF(int, LZ4_compress_default_ext, char *src, char *dst, int srcSize, int dstCapacity) {
    int ret = orig_LZ4_compress_default_ext(src, dst, srcSize, dstCapacity);
    auto out_path = string("/sdcard/Android/data/").append(Game::GetCurrentPackageName())
            .append("/CarrotJuicer/").append(current_time()).append("Q.msgpack");

    LOGI("[Q] src=%d, dst=%d, ret=%d => %s", srcSize, dstCapacity, ret, out_path.c_str());
    write_file(out_path, src, srcSize);

    return ret;
}

std::optional<std::vector<std::string>> read_config() {
    std::ifstream config_stream{
            string("/sdcard/Android/data/").append(Game::GetCurrentPackageName()).append(
                    "/config.json")};
    std::vector<std::string> dicts{};

    if (!config_stream.is_open()) {
        LOGW("config.json not loaded.");
        return nullopt;
    }

    LOGI("config.json loaded.");

    rapidjson::IStreamWrapper wrapper{config_stream};
    rapidjson::Document document;

    document.ParseStream(wrapper);

    if (!document.HasParseError()) {
        if (document.HasMember("enableLogger")) {
            g_enable_logger = document["enableLogger"].GetBool();
        }
        if (document.HasMember("dumpStaticEntries")) {
            g_dump_entries = document["dumpStaticEntries"].GetBool();
        }
        if (document.HasMember("dumpDbEntries")) {
            g_dump_db_entries = document["dumpDbEntries"].GetBool();
        }
        if (document.HasMember("staticEntriesUseHash")) {
            g_static_entries_use_hash = document["staticEntriesUseHash"].GetBool();
        }
        if (document.HasMember("staticEntriesUseTextIdName")) {
            g_static_entries_use_text_id_name = document["staticEntriesUseTextIdName"].GetBool();
        }
        if (document.HasMember("maxFps")) {
            g_max_fps = document["maxFps"].GetInt();
        }
        if (document.HasMember("uiAnimationScale")) {
            g_ui_animation_scale = document["uiAnimationScale"].GetFloat();
        }
        if (document.HasMember("uiUseSystemResolution")) {
            g_ui_use_system_resolution = document["uiUseSystemResolution"].GetBool();
        }
        if (document.HasMember("resolution3dScale")) {
            g_resolution_3d_scale = document["resolution3dScale"].GetFloat();
        }
        if (document.HasMember("replaceFont")) {
            g_replace_to_builtin_font = document["replaceFont"].GetBool();
        }
        if (!document.HasMember("replaceFont") && document.HasMember("replaceToBuiltinFont")) {
            g_replace_to_builtin_font = document["replaceToBuiltinFont"].GetBool();
        }
        if (document.HasMember("replaceToCustomFont")) {
            g_replace_to_custom_font = document["replaceToCustomFont"].GetBool();
        }
        if (document.HasMember("fontAssetBundlePath")) {
            g_font_assetbundle_path = std::string(document["fontAssetBundlePath"].GetString());
        }
        if (document.HasMember("fontAssetName")) {
            g_font_asset_name = std::string(document["fontAssetName"].GetString());
        }
        if (document.HasMember("tmproFontAssetName")) {
            g_tmpro_font_asset_name = std::string(document["tmproFontAssetName"].GetString());
        }
        if (document.HasMember("graphicsQuality")) {
            g_graphics_quality = document["graphicsQuality"].GetInt();
            if (g_graphics_quality < -1) {
                g_graphics_quality = -1;
            }
            if (g_graphics_quality > 4) {
                g_graphics_quality = 3;
            }
        }
        if (document.HasMember("antiAliasing")) {
            g_anti_aliasing = document["antiAliasing"].GetInt();
            vector<int> options = {0, 2, 4, 8, -1};
            g_anti_aliasing =
                    find(options.begin(), options.end(), g_anti_aliasing) - options.begin();
        }
        if (document.HasMember("forceLandscape")) {
            g_force_landscape = document["forceLandscape"].GetBool();
        }
        if (document.HasMember("forceLandscapeUiScale")) {
            g_force_landscape_ui_scale = document["forceLandscapeUiScale"].GetFloat();
            if (g_force_landscape_ui_scale <= 0) {
                g_force_landscape_ui_scale = 1;
            }
        }
        if (document.HasMember("uiLoadingShowOrientationGuide")) {
            g_ui_loading_show_orientation_guide = document["uiLoadingShowOrientationGuide"].GetBool();
        }
        /*if (document.HasMember("restoreNotification")) {
            g_restore_notification = document["restoreNotification"].GetBool();
        }*/
        if (document.HasMember("replaceAssetsPath")) {
            auto replaceAssetsPath = localify::u8_u16(document["replaceAssetsPath"].GetString());
            if (!replaceAssetsPath.starts_with(u"/")) {
                replaceAssetsPath.insert(0, u16string(u"/sdcard/Android/data/").append(
                        localify::u8_u16(Game::GetCurrentPackageName())).append(u"/"));
            }
            if (filesystem::exists(replaceAssetsPath) &&
                filesystem::is_directory(replaceAssetsPath)) {
                for (auto &file: filesystem::directory_iterator(replaceAssetsPath)) {
                    if (file.is_regular_file()) {
                        g_replace_assets.emplace(file.path().filename().string(),
                                                 ReplaceAsset{file.path().string(), nullptr});
                    }
                }
            }
        }

        if (document.HasMember("replaceAssetBundleFilePath")) {
            auto replaceAssetBundleFilePath = localify::u8_u16(
                    document["replaceAssetBundleFilePath"].GetString());
            if (!replaceAssetBundleFilePath.starts_with(u"/")) {
                replaceAssetBundleFilePath.insert(0, u16string(u"/sdcard/Android/data/").append(
                        localify::u8_u16(Game::GetCurrentPackageName())).append(u"/"));
            }
            if (filesystem::exists(replaceAssetBundleFilePath) &&
                filesystem::is_regular_file(replaceAssetBundleFilePath)) {
                g_replace_assetbundle_file_path = localify::u16_u8(replaceAssetBundleFilePath);
            }
        }

        if (document.HasMember("textIdDict")) {
            text_id_dict = document["textIdDict"].GetString();
        }

        if (document.HasMember("dicts")) {
            auto &dicts_arr = document["dicts"];
            auto len = dicts_arr.Size();

            for (size_t i = 0; i < len; ++i) {
                auto dict = dicts_arr[i].GetString();

                dicts.emplace_back(dict);
            }
        }

        if (document.HasMember("enableCarrotJuicer")) {
            auto dir = string("/sdcard/Android/data/").append(Game::GetCurrentPackageName()).append("/CarrotJuicer/");
            if (!filesystem::exists(dir)) {
                if (mkdir(dir.c_str(), 2770)) {
                    LOGI("%s created.", dir.c_str());
                } else {
                    LOGE("%s", strerror(errno));
                }
            }
            g_enable_carrotjuicer = document["enableCarrotJuicer"].GetBool();
        }
    }

    config_stream.close();
    return dicts;
}

void hack_thread(void *arg [[maybe_unused]]) {
    LOGI("%s hack thread: %d", ABI, gettid());
    int api_level = GetAndroidApiLevel();
    LOGI("%s api level: %d", ABI, api_level);
    if (api_level >= 30) {
        if (!IsABIRequiredNativeBridge()) {
            void *addr;
            if (IsRunningOnNativeBridge()) {
                addr = (void *) dlopen;
            } else {
                addr = DobbySymbolResolver(nullptr,
                                           "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv");
            }
            if (addr) {
                LOGI("%s do_dlopen at: %p", ABI, addr);
                if (IsRunningOnNativeBridge()) {
                    DobbyHook(addr, (void *) new_do_dlopen,
                              (void **) &orig_do_dlopen);
                } else {
                    DobbyHook(addr, (void *) new_do_dlopen_V24,
                              (void **) &orig_do_dlopen_V24);
                }
            }
        } else {
            void *addr = DobbySymbolResolver(nullptr,
                                             "NativeBridgeLoadLibraryExt");
            if (addr) {
                LOGI("NativeBridgeLoadLibraryExt at: %p", addr);
                DobbyHook(addr, (void *) new_NativeBridgeLoadLibraryExt_V30,
                          (void **) &orig_NativeBridgeLoadLibraryExt_V30);
            }
        }
    } else if (api_level >= 26) {
        if (!IsABIRequiredNativeBridge()) {
            void *addr;
            if (IsRunningOnNativeBridge()) {
                addr = (void *) dlopen;
            } else {
                void *libdl_handle = dlopen("libdl.so", RTLD_LAZY);
                addr = dlsym(libdl_handle, "__loader_dlopen");
            }
            if (addr) {
                if (IsRunningOnNativeBridge()) {
                    LOGI("do_dlopen at: %p", addr);
                    DobbyHook(addr, (void *) new_do_dlopen,
                              (void **) &orig_do_dlopen);
                } else {
                    LOGI("__loader_dlopen at: %p", addr);
                    DobbyHook(addr, (void *) new___loader_dlopen,
                              (void **) &orig___loader_dlopen);
                }
            }
        } else {
            void *addr = DobbySymbolResolver(nullptr,
                                             "_ZN7android26NativeBridgeLoadLibraryExtEPKciPNS_25native_bridge_namespace_tE");
            if (addr) {
                LOGI("NativeBridgeLoadLibraryExt at: %p", addr);
                DobbyHook(addr, (void *) new_NativeBridgeLoadLibraryExt_V26,
                          (void **) &orig_NativeBridgeLoadLibraryExt_V26);
            }
        }
    } else if (api_level >= 24) {
        if (!IsABIRequiredNativeBridge()) {
            void *addr;
            if (IsRunningOnNativeBridge()) {
                addr = (void *) dlopen;
            } else {
                addr = DobbySymbolResolver(nullptr,
                                           "__dl__Z9do_dlopenPKciPK17android_dlextinfoPv");
            }
            if (addr) {
                LOGI("do_dlopen at: %p", addr);
                DobbyHook(addr, (void *) new_do_dlopen_V24,
                          (void **) &orig_do_dlopen_V24);
            }
        } else {
            void *addr = DobbySymbolResolver(nullptr,
                                             "_ZN7android23NativeBridgeLoadLibraryEPKci");
            if (addr) {
                LOGI("NativeBridgeLoadLibrary at: %p", addr);
                DobbyHook(addr, (void *) new_NativeBridgeLoadLibrary_V21,
                          (void **) &orig_NativeBridgeLoadLibrary_V21);
            }
        }
    } else {
        if (!IsABIRequiredNativeBridge()) {
            void *addr = DobbySymbolResolver(nullptr,
                                             "__dl__Z9do_dlopenPKciPK17android_dlextinfo");
            if (addr) {
                LOGI("do_dlopen at: %p", addr);
                DobbyHook(addr, (void *) new_do_dlopen_V19,
                          (void **) &orig_do_dlopen_V19);
            }
        } else {
            void *addr = DobbySymbolResolver(nullptr,
                                             "_ZN7android23NativeBridgeLoadLibraryEPKci");
            if (addr) {
                LOGI("NativeBridgeLoadLibrary at: %p", addr);
                DobbyHook(addr, (void *) new_NativeBridgeLoadLibrary_V21,
                          (void **) &orig_NativeBridgeLoadLibrary_V21);
            }
        }
    }
    if (IsABIRequiredNativeBridge()) {
        return;
    }
    while (!il2cpp_handle || !native_handle) {
        sleep(1);
    }
    // prevent crash
    sleep(1);

    auto dict = read_config();

    // std::thread init_thread([dict]() {
    logger::init_logger();
    il2cpp_hook_init(il2cpp_handle);
    if (dict.has_value()) {
        localify::load_textdb(get_application_version(), &dict.value());
    }
    if (!text_id_dict.empty()) {
        localify::load_textId_textdb(text_id_dict);
    }
    il2cpp_hook();
    // });
    // init_thread.detach();

    if (g_enable_carrotjuicer) {
        ENABLE_HOOK(native_handle, LZ4_decompress_safe_ext)
        ENABLE_HOOK(native_handle, LZ4_compress_default_ext)
    }
}

void hack_settings_thread(void *arg [[maybe_unused]]) {
    int api_level = GetAndroidApiLevel();
    LOGI("%s api level: %d", ABI, api_level);
    if (api_level >= 30) {
        void *addr = DobbySymbolResolver(nullptr,
                                         "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv");
        if (addr) {
            LOGI("%s do_dlopen at: %p", ABI, addr);
            DobbyHook(addr, (void *) new_do_dlopen_V24,
                      (void **) &orig_do_dlopen_V24);

        }
    } else if (api_level >= 26) {
        void *libdl_handle = dlopen("libdl.so", RTLD_LAZY);
        void *addr = dlsym(libdl_handle, "__loader_dlopen");

        if (addr) {
            LOGI("__loader_dlopen at: %p", addr);
            DobbyHook(addr, (void *) new___loader_dlopen,
                      (void **) &orig___loader_dlopen);

        }
    } else if (api_level >= 24) {
        void *addr = DobbySymbolResolver(nullptr,
                                         "__dl__Z9do_dlopenPKciPK17android_dlextinfoPv");
        if (addr) {
            LOGI("do_dlopen at: %p", addr);
            DobbyHook(addr, (void *) new_do_dlopen_V24,
                      (void **) &orig_do_dlopen_V24);
        }
    } else {
        void *addr = DobbySymbolResolver(nullptr,
                                         "__dl__Z9do_dlopenPKciPK17android_dlextinfo");
        if (addr) {
            LOGI("do_dlopen at: %p", addr);
            DobbyHook(addr, (void *) new_do_dlopen_V19,
                      (void **) &orig_do_dlopen_V19);
        }
    }
}
