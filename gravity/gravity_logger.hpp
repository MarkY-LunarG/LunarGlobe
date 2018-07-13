/*
 * Copyright (c) 2018 The Khronos Group Inc.
 * Copyright (c) 2018 Valve Corporation
 * Copyright (c) 2018 LunarG, Inc.
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
 * Author: Mark Young, LunarG <marky@lunarg.com>
 */

#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <cstdarg>
#include <mutex>
#include <unordered_map>

#include "gravity_vulkan_headers.hpp"

enum GravityLogLevel {
    GRAVITY_LOG_DISABLE = 0,
    GRAVITY_LOG_ERROR,
    GRAVITY_LOG_WARN_ERROR,
    GRAVITY_LOG_INFO_WARN_ERROR,
    GRAVITY_LOG_ALL
};

struct InstanceDebugInfo {
    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
    PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;
    VkDebugUtilsMessengerEXT dbg_messenger;
};

class GravityLogger {
   public:
    static GravityLogger &getInstance() {
        static GravityLogger instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    GravityLogger(GravityLogger const &) = delete;
    void operator=(GravityLogger const &) = delete;

    // Extension utilities
    bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions, void **next);
    bool ReleaseCreateInstanceItems(void **next);

    bool CreateInstanceDebugInfo(VkInstance instance);
    bool DestroyInstanceDebugInfo(VkInstance instance);

    bool CheckAndRetrieveDeviceExtensions(const VkPhysicalDevice &physical_device, std::vector<std::string> &extensions);

    // Output targets
    void SetCommandLineOutput(bool enable) { _output_cmdline = enable; }
    void SetFileOutput(std::string output_file);

    void EnableValidation(bool enable) { _enable_validation = enable; }
    bool BreakOnError() { return _enable_break_on_error; }
    void EnableBreakOnError(bool enable) { _enable_break_on_error = enable; }
    GravityLogLevel GetLogLevel() { return _log_level; }
    void SetLogLevel(GravityLogLevel level) { _log_level = level; }
    void EnablePopups(bool enable) { _enable_popups = enable; }
    bool PopupsEnabled() { return _enable_popups; }

    // Log messages
    void LogDebug(std::string message);
    void LogInfo(std::string message);
    void LogWarning(std::string message);
    void LogPerf(std::string message);
    void LogError(std::string message);
    void LogFatalError(std::string message);

   private:
    GravityLogger();
    virtual ~GravityLogger();

    void LogMessage(const std::string &prefix, const std::string &message);

    bool _enable_validation;
    bool _enable_break_on_error;
    bool _output_cmdline;
    bool _output_file;
    bool _enable_popups;
    std::ofstream _file_stream;
    GravityLogLevel _log_level;
    std::unordered_map<VkInstance, InstanceDebugInfo> _instance_debug_info;
    std::mutex _instance_debug_mutex;
};