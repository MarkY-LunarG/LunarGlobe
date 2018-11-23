/*
 * LunarGlobe - globe_app.cpp
 *
 * Copyright (C) 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Young <marky@lunarg.com>
 */

#include "globe_event.hpp"
#include "globe_submit_manager.hpp"
#include "globe_resource_manager.hpp"
#include "globe_clock.hpp"
#include "globe_app.hpp"
#include "globe_logger.hpp"

#define GLOBE_APP_ENGINE_MAJOR 0
#define GLOBE_APP_ENGINE_MINOR 0
#define GLOBE_APP_ENGINE_PATCH 1

GlobeApp::GlobeApp() {
    _app_version.major = 0;
    _app_version.minor = 0;
    _app_version.patch = 0;
    _engine_version.major = GLOBE_APP_ENGINE_MAJOR;
    _engine_version.minor = GLOBE_APP_ENGINE_MINOR;
    _engine_version.patch = GLOBE_APP_ENGINE_PATCH;
    _globe_resource_mgr = nullptr;
    _globe_submit_mgr = nullptr;
    _globe_clock = nullptr;
    _globe_window = nullptr;
    _width = 100;
    _height = 100;
    _prepared = false;
    _was_minimized = false;
    _focused = true;
    _is_minimized = false;
    _is_paused = false;
    _must_exit = false;
    _google_display_timing_enabled = false;
    _current_frame = 0;
    _current_buffer = 0;
    _exit_on_frame = false;
    _exit_frame = UINT64_MAX;
    _vk_instance = VK_NULL_HANDLE;
    _vk_phys_device = VK_NULL_HANDLE;
    _vk_device = VK_NULL_HANDLE;
    _vk_setup_command_pool = VK_NULL_HANDLE;
    _vk_setup_command_buffer = VK_NULL_HANDLE;
}

GlobeApp::~GlobeApp() {
    if (nullptr != _globe_window) {
        delete _globe_window;
        _globe_window = nullptr;
    }
    if (nullptr != _globe_clock) {
        delete _globe_clock;
        _globe_clock = nullptr;
    }
    if (nullptr != _globe_resource_mgr) {
        _globe_resource_mgr->FreeAllTextures();
        _globe_resource_mgr->FreeAllShaders();
        delete _globe_resource_mgr;
        _globe_resource_mgr = nullptr;
    }
    if (nullptr != _globe_submit_mgr) {
        delete _globe_submit_mgr;
        _globe_submit_mgr = nullptr;
    }
}

bool GlobeApp::Init(GlobeInitStruct &init_struct) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    std::string::size_type argument_size;
    bool print_usage = false;
    bool start_fullscreen = false;

    _name = init_struct.app_name;
    _width = init_struct.width;
    _height = init_struct.height;
    _app_version.major = init_struct.version.major;
    _app_version.minor = init_struct.version.minor;
    _app_version.patch = init_struct.version.patch;
    _resource_directory = "resources";

    // Handle command-line arguments
    size_t max_arg = init_struct.command_line_args.size();
    for (size_t cur_arg = 0; cur_arg < max_arg; ++cur_arg) {
        bool not_last_argument = cur_arg < max_arg - 1;
        if (init_struct.command_line_args[cur_arg] == "--break") {
            logger.EnableBreakOnError(true);
        } else if (init_struct.command_line_args[cur_arg] == "--fullscreen") {
            start_fullscreen = true;
        } else if (init_struct.command_line_args[cur_arg] == "--validate") {
            logger.EnableValidation(true);
        } else if (init_struct.command_line_args[cur_arg] == "--api_dump") {
            logger.EnableApiDump(true);
        } else if (init_struct.command_line_args[cur_arg] == "--c" && !_exit_on_frame && not_last_argument) {
            _exit_frame = std::stoi(init_struct.command_line_args[cur_arg + 1], &argument_size);
            if (_exit_frame > 0) {
                _exit_on_frame = true;
            }
            ++cur_arg;
        } else if (init_struct.command_line_args[cur_arg] == "--resource_dir" && not_last_argument) {
            _resource_directory = init_struct.command_line_args[cur_arg + 1];
            ++cur_arg;
        } else if (init_struct.command_line_args[cur_arg] == "--suppress_popups") {
            logger.EnablePopups(false);
        } else if (init_struct.command_line_args[cur_arg] == "--display_timing") {
            _google_display_timing_enabled = true;
        } else {
            print_usage = true;
            break;
        }
    }

    if (print_usage) {
#if defined(ANDROID)
        GlobeLogger::getInstance().LogFatalError("Usage: globe [--validate]");
#else
        std::string usage_message = "Usage:\n  ";
        usage_message += _name;
        usage_message +=
            "\t[--resource_dir <directory] [--validate] [--break] [--fullscreen]\n"
            "\t[--c <framecount>] [--suppress_popups] [--display_timing]\n\n";
        GlobeLogger::getInstance().LogFatalError(usage_message);
        fflush(stderr);
        return false;
#endif
    }

#if defined(VK_USE_PLATFORM_XLIB_KHR) || defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    _globe_window = new GlobeWindowLinux(this, _name, start_fullscreen);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    _globe_window = new GlobeWindowWindows(this, _name, start_fullscreen);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    _globe_window = new GlobeWindowAndroid(this, _name, true);
#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
    _globe_window = new GlobeWindowApple(this, _name, start_fullscreen);
#else
    _globe_window = new GlobeWindow(this, _name, start_fullscreen);
#endif
    if (!GlobeEventList::getInstance().Alloc(100)) {
        GlobeLogger::getInstance().LogFatalError("Failed allocating space for events");
    }

    std::vector<const char *> enabled_layers;
    std::vector<const char *> enabled_extensions;
    void *next_ptr = nullptr;
    std::vector<std::string> instance_layers;
    std::vector<std::string> current_extensions;
    if (logger.PrepareCreateInstanceItems(instance_layers, current_extensions, &next_ptr) &&
        _globe_window->PrepareCreateInstanceItems(instance_layers, current_extensions, &next_ptr)) {
        enabled_layers.resize(0);
        enabled_extensions.resize(0);
        for (uint32_t i = 0; i < instance_layers.size(); i++) {
            enabled_layers.push_back(instance_layers[i].c_str());
        }
        for (uint32_t i = 0; i < current_extensions.size(); i++) {
            enabled_extensions.push_back(current_extensions[i].c_str());
        }
    }

    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = nullptr;
    app.pApplicationName = init_struct.app_name.c_str();
    app.applicationVersion = 0;
    app.pEngineName = "Globe Engine";
    app.engineVersion = 0;
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = next_ptr;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
    inst_info.ppEnabledLayerNames = enabled_layers.data();
    inst_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    inst_info.ppEnabledExtensionNames = enabled_extensions.data();

    VkResult result = vkCreateInstance(&inst_info, nullptr, &_vk_instance);
    switch (result) {
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            logger.LogFatalError(
                "vkCreateInstance failed: Cannot find a compatible Vulkan installable "
                "client driver (ICD).\n\nPlease look at the Getting Started guide for "
                "additional information.");
            return false;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            logger.LogFatalError(
                "vkCreateInstance failed: Cannot find a specified extension library.\n"
                "Make sure your layers path is set appropriately.");
            return false;
        case VK_SUCCESS:
            break;
        default:
            logger.LogFatalError(
                "vkCreateInstance failed: Do you have a compatible Vulkan installable "
                "client driver (ICD) installed?\nPlease look at the Getting Started "
                "guide for additional information.");
            return false;
    }
    if (!logger.ReleaseCreateInstanceItems(&next_ptr) || !_globe_window->ReleaseCreateInstanceItems(&next_ptr)) {
        logger.LogFatalError("Failed cleaning up after creating instance");
    }
    logger.CreateInstanceDebugInfo(_vk_instance);

    // Pick a physical device
    uint32_t phys_device_count = 0;
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(_vk_instance, &phys_device_count, nullptr) || 0 == phys_device_count) {
        logger.LogFatalError("Failed to find Vulkan capable device!");
        return false;
    }

    std::vector<VkPhysicalDevice> physical_devices;
    physical_devices.resize(phys_device_count);
    if (VK_SUCCESS != vkEnumeratePhysicalDevices(_vk_instance, &phys_device_count, physical_devices.data()) ||
        0 == phys_device_count) {
        logger.LogFatalError("Failed to find Vulkan capable device!");
        return false;
    }

    // For now, just use first one
    _vk_phys_device = physical_devices[0];

    // Query fine-grained feature support for this device.
    //  If app has specific feature requirements it should check supported
    //  features based on this query
    vkGetPhysicalDeviceProperties(_vk_phys_device, &_vk_phys_device_properties);
    vkGetPhysicalDeviceFeatures(_vk_phys_device, &_vk_phys_device_features);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    _globe_window->SetHInstance(init_struct.windows_instance);
#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)
    _globe_window->SetMoltenVkView(init_struct.molten_view);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    _globe_window->SetAndroidNativeWindow(_android_native_window);
#endif
    _globe_window->CreatePlatformWindow(_vk_instance, _vk_phys_device, init_struct.width, init_struct.height);

    _globe_submit_mgr = new GlobeSubmitManager(this, _globe_window, _vk_instance, _vk_phys_device);
    if (nullptr == _globe_submit_mgr) {
        logger.LogFatalError("Failed to create swapchain manager!");
        return false;
    }

    VkDeviceCreateInfo device_create_info = {};
    current_extensions.resize(0);
    next_ptr = nullptr;
    if (_globe_submit_mgr->PrepareCreateDeviceItems(device_create_info, current_extensions, &next_ptr)) {
        enabled_extensions.resize(0);
        for (uint32_t i = 0; i < current_extensions.size(); i++) {
            enabled_extensions.push_back(current_extensions[i].c_str());
        }
    }
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.pEnabledFeatures = nullptr;  // If specific features are required, pass them in here
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
    device_create_info.pNext = next_ptr;
    if (VK_SUCCESS != vkCreateDevice(_vk_phys_device, &device_create_info, nullptr, &_vk_device)) {
        logger.LogFatalError("Failed to create Vulkan device!");
        return false;
    }

    if (!_globe_submit_mgr->ReleaseCreateDeviceItems(device_create_info, &next_ptr)) {
        logger.LogFatalError("Failed cleaning up after creating device");
        return false;
    }

    _globe_resource_mgr = new GlobeResourceManager(this, _resource_directory);
    if (nullptr == _globe_resource_mgr) {
        logger.LogFatalError("Failed to create resource manager!");
        return false;
    }

    if (!_globe_submit_mgr->PrepareForSwapchain(_vk_device, init_struct.num_swapchain_buffers, init_struct.present_mode,
                                                init_struct.ideal_swapchain_format,
                                                init_struct.secondary_swapchain_format)) {
        logger.LogFatalError("Failed to prepare swapchain");
        return false;
    }

    _globe_clock = GlobeClock::CreateClock();

    if (!Setup()) {
        return false;
    }

    return true;
}

bool GlobeApp::PreSetup(VkCommandPool &vk_setup_command_pool, VkCommandBuffer &vk_setup_command_buffer) {
    GlobeLogger &logger = GlobeLogger::getInstance();

    _globe_submit_mgr->CreateSwapchain();
    _swapchain_count = _globe_submit_mgr->NumSwapchainImages();
    _vk_swapchain_format = _globe_submit_mgr->GetSwapchainVkFormat();

    if (_vk_setup_command_pool == VK_NULL_HANDLE) {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = nullptr;
        cmd_pool_info.queueFamilyIndex = _globe_submit_mgr->GetGraphicsQueueIndex();
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (VK_SUCCESS != vkCreateCommandPool(_vk_device, &cmd_pool_info, nullptr, &_vk_setup_command_pool)) {
            logger.LogFatalError("Failed creating device command pool");
            return false;
        }
    }

    VkCommandBufferAllocateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd.pNext = nullptr;
    cmd.commandPool = _vk_setup_command_pool;
    cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd.commandBufferCount = 1;
    if (VK_SUCCESS != vkAllocateCommandBuffers(_vk_device, &cmd, &_vk_setup_command_buffer)) {
        logger.LogFatalError("Failed creating primary device command buffer");
        return false;
    }
    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = nullptr;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = nullptr;
    if (VK_SUCCESS != vkBeginCommandBuffer(_vk_setup_command_buffer, &cmd_buf_info)) {
        logger.LogFatalError("Failed beginning primary device command buffer");
        return false;
    }

    if (_is_minimized) {
        _prepared = false;
    } else {
        const VkFormat depth_format = VK_FORMAT_D16_UNORM;
        VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = nullptr;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = depth_format;
        image_create_info.extent = {_width, _height, 1};
        image_create_info.mipLevels = 1;
        image_create_info.arrayLayers = 1;
        image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_create_info.flags = 0;

        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = nullptr;
        image_view_create_info.image = VK_NULL_HANDLE;
        image_view_create_info.format = depth_format;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.flags = 0;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

        _depth_buffer = {};
        _depth_buffer.vk_format = depth_format;

        if (VK_SUCCESS != vkCreateImage(_vk_device, &image_create_info, nullptr, &_depth_buffer.vk_image)) {
            logger.LogFatalError("Failed creating depth buffer image");
            return false;
        }

        if (!_globe_resource_mgr->AllocateDeviceImageMemory(_depth_buffer.vk_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                            _depth_buffer.vk_device_memory,
                                                            _depth_buffer.vk_allocated_size)) {
            logger.LogFatalError("Failed allocating depth buffer image to memory");
            return false;
        }

        if (VK_SUCCESS != vkBindImageMemory(_vk_device, _depth_buffer.vk_image, _depth_buffer.vk_device_memory, 0)) {
            logger.LogFatalError("Failed binding depth buffer image to memory");
            return false;
        }

        image_view_create_info.image = _depth_buffer.vk_image;
        if (VK_SUCCESS !=
            vkCreateImageView(_vk_device, &image_view_create_info, nullptr, &_depth_buffer.vk_image_view)) {
            logger.LogFatalError("Failed creating image view to depth buffer image");
            return false;
        }
    }
    vk_setup_command_pool = _vk_setup_command_pool;
    vk_setup_command_buffer = _vk_setup_command_buffer;
    return true;
}

bool GlobeApp::PostSetup(VkCommandPool &vk_setup_command_pool, VkCommandBuffer &vk_setup_command_buffer) {
    GlobeLogger &logger = GlobeLogger::getInstance();
    vk_setup_command_pool = VK_NULL_HANDLE;
    vk_setup_command_buffer = VK_NULL_HANDLE;

    if (!_is_minimized) {
        // Flush all the initialization command buffer items
        if (VK_SUCCESS != vkEndCommandBuffer(_vk_setup_command_buffer)) {
            logger.LogFatalError("Failed ending primary device command buffer");
            return false;
        }

        VkFence vk_fence;
        VkFenceCreateInfo fence_create_info = {};
        fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_create_info.pNext = NULL;
        fence_create_info.flags = 0;
        if (VK_SUCCESS != vkCreateFence(_vk_device, &fence_create_info, NULL, &vk_fence)) {
            logger.LogFatalError("Failed creating fence for initial setup command buffer submit");
            return false;
        }

        _globe_submit_mgr->Submit(_vk_setup_command_buffer, VK_NULL_HANDLE, VK_NULL_HANDLE, vk_fence, true);
        vkFreeCommandBuffers(_vk_device, _vk_setup_command_pool, 1, &_vk_setup_command_buffer);
        vkDestroyFence(_vk_device, vk_fence, nullptr);
        _vk_setup_command_buffer = VK_NULL_HANDLE;
        vkDestroyCommandPool(_vk_device, _vk_setup_command_pool, nullptr);
        _vk_setup_command_pool = VK_NULL_HANDLE;

        _current_buffer = 0;
        _prepared = true;
    }
    return true;
}

void GlobeApp::Resize() {
    if (_must_exit) {
        return;
    }

    if (!_was_minimized) {
        vkDeviceWaitIdle(_vk_device);
        // In order to properly resize the window, we must re-create the swapchain
        // AND redo the command buffers, etc.
        CleanupCommandObjects(true);
        _width = _globe_submit_mgr->CurrentWidth();
        _height = _globe_submit_mgr->CurrentHeight();
    }
    Setup();
}

void GlobeApp::PreCleanup() { CleanupCommandObjects(false); }

void GlobeApp::PostCleanup() {
    if (nullptr != _globe_resource_mgr) {
        delete _globe_resource_mgr;
        _globe_resource_mgr = nullptr;
    }
}

void GlobeApp::Cleanup() {
    PreCleanup();
    PostCleanup();
}

void GlobeApp::CleanupCommandObjects(bool is_resize) {
    _prepared = false;
    if (!_is_minimized) {
        vkDestroyImageView(_vk_device, _depth_buffer.vk_image_view, nullptr);
        vkDestroyImage(_vk_device, _depth_buffer.vk_image, nullptr);
        _globe_resource_mgr->FreeDeviceMemory(_depth_buffer.vk_device_memory);

        if (is_resize) {
            _globe_submit_mgr->Resize();
        } else {
            _globe_submit_mgr->DestroySwapchain();
        }

        if (!is_resize) {
            vkDeviceWaitIdle(_vk_device);
        }
    }
}

bool GlobeApp::Run() {
    _globe_clock->Start();
    _globe_clock->StartGameTime();

    while (!_must_exit) {
        float comp_diff = 0;
        float game_diff = 0;
        _globe_clock->GetTimeDiffMS(comp_diff, game_diff);
#if defined(VK_USE_PLATFORM_XCB_KHR)
        if (_is_paused) {
            _globe_window->HandlePausedXcbEvent();
        }
        _globe_window->HandleAllXcbEvents();
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        if (_is_paused) {
            _globe_window->HandleXlibEvent();
        }
        _globe_window->HandleAllXlibEvents();
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
        if (_is_paused) {
            _globe_window->HandlePausedWaylandEvent();
        } else {
            _globe_window->HandleActiveWaylandEvents();
        }
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        MSG msg = {0};
        PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
        if (msg.message == WM_QUIT)  // check for a quit message
        {
            GlobeEvent quit_event(GLOBE_EVENT_QUIT);
            GlobeEventList::getInstance().InsertEvent(quit_event);
        } else {
            /* Translate and dispatch to event queue*/
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
        int events;
        struct android_poll_source *source;
        while (ALooper_pollAll(active ? 0 : -1, NULL, &events, (void **)&source) >= 0) {
            if (source) {
                source->process(app, source);
            }

            if (app->destroyRequested != 0) {
                g_app->Exit();
                return;
            }
        }
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        if (_focused) {
            if (!ProcessEvents()) {
                return false;
            }
            Update(game_diff);
            Draw();
        }
#else
        if (!ProcessEvents()) {
            return false;
        }
        if (_focused) {
            Update(game_diff);
            Draw();
        }
#endif
    }
    return true;
}

bool GlobeApp::Draw() {
    _current_frame++;
    if (_exit_on_frame && _current_frame == _exit_frame) {
        GlobeEvent quit_event(GLOBE_EVENT_QUIT);
        GlobeEventList::getInstance().InsertEvent(quit_event);
    }
    return true;
}

void GlobeApp::Exit() {
    if (!_is_minimized) {
        vkDeviceWaitIdle(_vk_device);
    }
    Cleanup();
    vkDeviceWaitIdle(_vk_device);
    if (_globe_submit_mgr) {
        delete _globe_submit_mgr;
        _globe_submit_mgr = nullptr;
    }
    vkDestroyDevice(_vk_device, nullptr);
    GlobeLogger::getInstance().DestroyInstanceDebugInfo(_vk_instance);
    delete _globe_window;
    _globe_window = nullptr;

    vkDestroyInstance(_vk_instance, nullptr);
}

bool GlobeApp::ProcessEvents() {
    std::vector<GlobeEvent> current_events;
    if (GlobeEventList::getInstance().GetEvents(current_events)) {
        for (auto &cur_event : current_events) {
            HandleEvent(cur_event);
        }
    }
    return true;
}

void GlobeApp::HandleEvent(GlobeEvent &event) {
    switch (event.Type()) {
        case GLOBE_EVENT_KEY_RELEASE:
            switch (event._data.key) {
                case GLOBE_KEYNAME_ESCAPE:
                    _must_exit = true;
                    break;
                case GLOBE_KEYNAME_SPACE:
                    _is_paused = !_is_paused;
                    break;
                default:
                    break;
            }
            break;
        case GLOBE_EVENT_WINDOW_DRAW:
            if (_focused) {
                Update(0.0f);
                Draw();
            }
            break;
        case GLOBE_EVENT_WINDOW_RESIZE:
            // Only resize if the data is different
            if (_width != event._data.resize.width || _height != event._data.resize.height) {
                _was_minimized = _width == 0 || _height == 0;
                _is_minimized = event._data.resize.width == 0 || event._data.resize.height == 0;
                _focused = !_is_minimized;
                _width = event._data.resize.width;
                _height = event._data.resize.height;
                Resize();
            } else {
                GlobeLogger::getInstance().LogInfo("Redundant resize call");
            }
            break;
        case GLOBE_EVENT_QUIT:
            _must_exit = true;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            PostQuitMessage(0);
#endif
            break;
        default:
            break;
    }
}
