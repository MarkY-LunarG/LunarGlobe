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

#include "globe_vulkan_headers.hpp"

enum GlobeLogLevel {
    GLOBE_LOG_DISABLE = 0,
    GLOBE_LOG_ERROR,
    GLOBE_LOG_WARN_ERROR,
    GLOBE_LOG_INFO_WARN_ERROR,
    GLOBE_LOG_ALL
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

class GlobeLogger {
   public:
    static GlobeLogger &getInstance() {
        static GlobeLogger logger_instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return logger_instance;
    }

    GlobeLogger(GlobeLogger const &) = delete;
    void operator=(GlobeLogger const &) = delete;

    // Extension utilities
    bool PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                    void **next);
    bool ReleaseCreateInstanceItems(void **next);

    bool CreateInstanceDebugInfo(VkInstance instance);
    bool DestroyInstanceDebugInfo(VkInstance instance);

    // Output targets
    void SetCommandLineOutput(bool enable) { _output_cmdline = enable; }
    void SetFileOutput(std::string output_file);

    void EnableValidation(bool enable) { _enable_validation = enable; }
    void EnableApiDump(bool enable) { _enable_api_dump = enable; }
    bool BreakOnError() { return _enable_break_on_error; }
    void EnableBreakOnError(bool enable) { _enable_break_on_error = enable; }
    GlobeLogLevel GetLogLevel() { return _log_level; }
    void SetLogLevel(GlobeLogLevel level) { _log_level = level; }
    void EnablePopups(bool enable) { _enable_popups = enable; }
    bool PopupsEnabled() { return _enable_popups; }

    bool SetObjectName(VkInstance instance, VkDevice device, uint64_t handle, VkObjectType type,
                       const std::string &name);

    // Log messages
    void LogDebug(std::string message);
    void LogInfo(std::string message);
    void LogWarning(std::string message);
    void LogPerf(std::string message);
    void LogError(std::string message);
    void LogFatalError(std::string message);

   private:
    GlobeLogger();
    virtual ~GlobeLogger();

    void LogMessage(const std::string &prefix, const std::string &message);

    bool _enable_validation;
    bool _enable_api_dump;
    bool _enable_break_on_error;
    bool _output_cmdline;
    bool _output_file;
    bool _enable_popups;
    std::ofstream _file_stream;
    GlobeLogLevel _log_level;
    std::unordered_map<VkInstance, InstanceDebugInfo> _instance_debug_info;
    std::unordered_map<uint64_t, std::string> _object_name_map;
    std::mutex _instance_debug_mutex;
};