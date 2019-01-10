//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_main.hpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#pragma once

#if defined(VK_USE_PLATFORM_WIN32_KHR)

// Include header required for parsing the command line options.
#include <shellapi.h>

#define GLOBE_APP_MAIN() int32_t WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int32_t nCmdShow)

#define GLOBE_APP_MAIN_BEGIN(init_struct)                                                                \
    int32_t argc;                                                                                        \
    LPWSTR *commandLineArgs = CommandLineToArgvW(GetCommandLineW(), &argc);                              \
    if (NULL == commandLineArgs) {                                                                       \
        argc = 0;                                                                                        \
    }                                                                                                    \
    if (argc > 1) {                                                                                      \
        init_struct.command_line_args.resize(argc - 1);                                                  \
        for (int32_t iii = 1; iii < argc; iii++) {                                                       \
            size_t wideCharLen = wcslen(commandLineArgs[iii]);                                           \
            size_t numConverted = 0;                                                                     \
            char *argv = (char *)malloc(sizeof(char) * (wideCharLen + 1));                               \
            if (argv != NULL) {                                                                          \
                wcstombs_s(&numConverted, argv, wideCharLen + 1, commandLineArgs[iii], wideCharLen + 1); \
                init_struct.command_line_args[iii - 1] = argv;                                           \
            }                                                                                            \
            free(argv);                                                                                  \
        }                                                                                                \
    }                                                                                                    \
    init_struct.windows_instance = hInstance;

#define GLOBE_APP_MAIN_END(return_val) return return_val;

#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)

#error("Not supported")

static void demo_main(struct Demo *demo, void *view) {
    GlobeInitStruct init_struct = {};
    init_struct.app_name = "Globe App 1 - Cube";
    init_struct.version.major = 0;
    init_struct.version.minor = 1;
    init_struct.version.patch = 0;
    init_struct.width = 500;
    init_struct.height = 500;
    init_struct.present_mode = VK_PRESENT_MODE_FIFO_KHR;
    init_struct.num_swapchain_buffers = 3;
    init_struct.ideal_swapchain_format = VK_FORMAT_B8G8R8A8_SRGB;
    init_struct.secondary_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    g_app = new CubeApp();
    g_app->Init(init_struct);
}

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)

#define GLOBE_APP_MAIN()                                                                            \
#include <android/log.h> \
    #include <android_native_app_glue.h> \
    #include "android_util.h" \
    static int32_t processInput(struct android_app *app, AInputEvent *event) {                                                                    \
        return 0;                                                                                   \
    }                                                                                               \
    static void processCommand(struct android_app *app, int32_t cmd) {                              \
        switch (cmd) {                                                                              \
            case APP_CMD_INIT_WINDOW: {                                                             \
                if (app->window) {                                                                  \
                    if (g_app->Prepared()) {                                                        \
                        g_app->CleanupCommandObjects(false);                                        \
                    }                                                                               \
                    const char key[] = "args";                                                      \
                    char *appTag = (char *)"Globe App";                                             \
                    int32_t argc = 0;                                                               \
                    char **argv = get_args(app, key, appTag, &argc);                                \
                    __android_log_print(ANDROID_LOG_INFO, appTag, "argc = %i", argc);               \
                    for (int32_t i = 0; i < argc; i++)                                              \
                        __android_log_print(ANDROID_LOG_INFO, appTag, "argv[%i] = %s", i, argv[i]); \
                    g_app->SetAndroidNativeWindow(app->window);                                     \
                    g_app->Init();                                                                  \
                    for (int32_t i = 0; i < argc; i++) free(argv[i]);                               \
                }                                                                                   \
                break;                                                                              \
            }                                                                                       \
            case APP_CMD_GAINED_FOCUS: {                                                            \
                g_app->SetFocus(true);                                                              \
                break;                                                                              \
            }                                                                                       \
            case APP_CMD_LOST_FOCUS: {                                                              \
                g_app->SetFocus(false);                                                             \
                break;                                                                              \
            }                                                                                       \
        }                                                                                           \
    }                                                                                               \
    void android_main(struct android_app *app)

#define GLOBE_APP_MAIN_BEGIN(init_struct) \
    int32_t vulkanSupport = InitVulkan(); \
    if (vulkanSupport == 0) {             \
        return;                           \
    }                                     \
    app->onAppCmd = processCommand;       \
    app->onInputEvent = processInput;

#define GLOBE_APP_MAIN_END(return_val)

#else

#define GLOBE_APP_MAIN() int32_t main(int32_t argc, char **argv)

#define GLOBE_APP_MAIN_BEGIN(init_struct)                   \
    init_struct.command_line_args.resize(argc - 1);         \
    for (int32_t iii = 1; iii < argc; iii++) {              \
        init_struct.command_line_args[iii - 1] = argv[iii]; \
    }

#define GLOBE_APP_MAIN_END(return_val) return return_val;

#endif
