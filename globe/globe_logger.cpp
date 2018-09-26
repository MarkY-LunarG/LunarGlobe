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

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include <iostream>
#include <sstream>
#include <cstring>
#include <csignal>
#include <assert.h>

#include "object_type_string_helper.h"
#include "globe_logger.hpp"
#include "globe_event.hpp"

GlobeLogger::GlobeLogger()
    : _enable_validation(false),
      _enable_api_dump(false),
      _enable_break_on_error(false),
      _output_cmdline(false),
      _output_file(false),
      _enable_popups(false),
      _log_level(GLOBE_LOG_WARN_ERROR) {}

GlobeLogger::~GlobeLogger() {
    if (_output_file) {
        _file_stream.close();
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData) {
    std::string message;
    GlobeLogger *logger = reinterpret_cast<GlobeLogger *>(pUserData);

    if (logger->BreakOnError()) {
#ifndef WIN32
        raise(SIGTRAP);
#else
        DebugBreak();
#endif
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        message += "VERBOSE : ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        message += "INFO : ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        message += "WARNING : ";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        message += "ERROR : ";
    }

    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        message += "GENERAL";
    } else {
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
            message += "VALIDATION";
        }
        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
                message += "|";
            }
            message += "PERFORMANCE";
        }
    }

    message += " - Message Id Number: ";
    message += std::to_string(pCallbackData->messageIdNumber);
    if (nullptr != pCallbackData->pMessageIdName) {
        message += " | Message Id Name: ";
        message += pCallbackData->pMessageIdName;
    }
    message += "\n\t";
    message += pCallbackData->pMessage;
    message += "\n";
    if (pCallbackData->objectCount > 0) {
        message += "\t\tObjects - ";
        message += std::to_string(pCallbackData->objectCount);
        message += "\n";
        for (uint32_t object = 0; object < pCallbackData->objectCount; ++object) {
            message += "\t\t\tObject[";
            message += std::to_string(object);
            message += "] - ";
            message += string_VkObjectType(pCallbackData->pObjects[object].objectType);
            message += ", Handle ";
            std::ostringstream oss_handle;
            oss_handle << std::hex << reinterpret_cast<const void *>(pCallbackData->pObjects[object].objectHandle);
            message += oss_handle.str();
            if (NULL != pCallbackData->pObjects[object].pObjectName &&
                strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
                message += ", Name \"";
                message += pCallbackData->pObjects[object].pObjectName;
                message += "\"\n";
            } else {
                message += "\n";
            }
        }
    }
    if (pCallbackData->cmdBufLabelCount > 0) {
        message += "\t\tCommand Buffer Labels - ";
        message += std::to_string(pCallbackData->cmdBufLabelCount);
        for (uint32_t cmd_buf_label = 0; cmd_buf_label < pCallbackData->cmdBufLabelCount; ++cmd_buf_label) {
            message += "\t\t\tLabel[";
            message += std::to_string(cmd_buf_label);
            message += "] - \"";
            message += pCallbackData->pCmdBufLabels[cmd_buf_label].pLabelName;
            message += "\" { ";
            message += std::to_string(pCallbackData->pCmdBufLabels[cmd_buf_label].color[0]);
            message += ", ";
            message += std::to_string(pCallbackData->pCmdBufLabels[cmd_buf_label].color[1]);
            message += ", ";
            message += std::to_string(pCallbackData->pCmdBufLabels[cmd_buf_label].color[2]);
            message += ", ";
            message += std::to_string(pCallbackData->pCmdBufLabels[cmd_buf_label].color[3]);
        }
    }

#ifdef _WIN32

    GlobeEventList::getInstance().InsertEvent(GlobeEvent(GLOBE_EVENT_PLATFORM_PAUSE_START));
    if (logger->PopupsEnabled()) {
        MessageBox(NULL, message.c_str(), "Alert", MB_OK);
    }
    GlobeEventList::getInstance().InsertEvent(GlobeEvent(GLOBE_EVENT_PLATFORM_PAUSE_STOP));

#elif defined(ANDROID)

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        __android_log_print(ANDROID_LOG_INFO, APP_SHORT_NAME, "%s", message.c_str());
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        __android_log_print(ANDROID_LOG_WARN, APP_SHORT_NAME, "%s", message.c_str());
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        __android_log_print(ANDROID_LOG_ERROR, APP_SHORT_NAME, "%s", message.c_str());
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        __android_log_print(ANDROID_LOG_VERBOSE, APP_SHORT_NAME, "%s", message.c_str());
    } else {
        __android_log_print(ANDROID_LOG_INFO, APP_SHORT_NAME, "%s", message.c_str());
    }

#else

    printf("%s\n", message.c_str());
    fflush(stdout);

#endif

    // Don't bail out, but keep going.
    return false;
}

bool GlobeLogger::PrepareCreateInstanceItems(std::vector<std::string> &layers, std::vector<std::string> &extensions,
                                             void **next) {
    uint32_t extension_count = 0;
    uint32_t layer_count = 0;

    if (_enable_validation || _enable_api_dump) {
        char std_validation_layer_name[] = "VK_LAYER_LUNARG_standard_validation";
        char threading_layer_name[] = "VK_LAYER_GOOGLE_threading";
        char parameter_validation_layer_name[] = "VK_LAYER_LUNARG_parameter_validation";
        char object_tracker_layer_name[] = "VK_LAYER_LUNARG_object_tracker";
        char core_validation_layer_name[] = "VK_LAYER_LUNARG_core_validation";
        char unique_objects_layer_name[] = "VK_LAYER_GOOGLE_unique_objects";
        char api_dump_layer_name[] = "VK_LAYER_LUNARG_api_dump";
        const uint32_t number_of_individual_layers = 5;
        char *instance_validation_layers_list[number_of_individual_layers];
        instance_validation_layers_list[0] = threading_layer_name;
        instance_validation_layers_list[1] = parameter_validation_layer_name;
        instance_validation_layers_list[2] = object_tracker_layer_name;
        instance_validation_layers_list[3] = core_validation_layer_name;
        instance_validation_layers_list[4] = unique_objects_layer_name;

        // Look for validation layers
        bool validation_found = false;
        if (VK_SUCCESS != vkEnumerateInstanceLayerProperties(&layer_count, NULL)) {
            LogFatalError("vkEnumerateInstanceLayerProperties failed on layer count query.\n");
            // NOTE: The above should exit, but just in case...
            return false;
        }

        if (layer_count > 0) {
            std::vector<VkLayerProperties> instance_layers;
            instance_layers.resize(layer_count);
            if (VK_SUCCESS != vkEnumerateInstanceLayerProperties(&layer_count, instance_layers.data())) {
                LogFatalError("vkEnumerateInstanceLayerProperties failed on layer query.\n");
                // NOTE: The above should exit, but just in case...
                return false;
            }

            if (_enable_api_dump) {
                // First, look for API Dump.  We have to because otherwise adding it after validation
                // layers could reflect additional API calls the validation layers make to perform
                // their evaluation.
                for (uint32_t layer = 0; layer < layer_count; ++layer) {
                    if (_enable_api_dump && !strcmp(instance_layers[layer].layerName, api_dump_layer_name)) {
                        layers.push_back(api_dump_layer_name);
                        break;
                    }
                }
            }

            if (_enable_validation) {
                // Look for the global standard validation meta-layer and API-Dump layers first,
                // but only if they are enabled.  If validation is enabled and we can't find the
                // meta-layer, we'll later look for the individual layers.
                for (uint32_t layer = 0; layer < layer_count; ++layer) {
                    if (!strcmp(instance_layers[layer].layerName, std_validation_layer_name)) {
                        validation_found = true;
                        layers.push_back(std_validation_layer_name);
                        break;
                    }
                }

                if (!validation_found) {
                    uint32_t found_mask = 0;

                    // Look for the individual layers second
                    for (uint32_t inst_layer = 0; inst_layer < layer_count; ++inst_layer) {
                        for (uint32_t valid_layer = 0; valid_layer < number_of_individual_layers; ++valid_layer) {
                            if (!strcmp(instance_layers[inst_layer].layerName,
                                        instance_validation_layers_list[valid_layer])) {
                                found_mask |= 1 << valid_layer;
                                break;
                            }
                        }
                    }
                    if (found_mask == (1 << number_of_individual_layers) - 1) {
                        validation_found = true;
                        for (uint32_t valid_layer = 0; valid_layer < number_of_individual_layers; ++valid_layer) {
                            layers.push_back(instance_validation_layers_list[valid_layer]);
                        }
                    }
                }
            }
        }

        if (_enable_validation && !validation_found) {
            LogFatalError(
                "vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
                "Please look at the Getting Started guide for additional information.\n");
            // NOTE: The above should exit, but just in case...
            return false;
        }
    }

    // Determine the number of instance extensions supported
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (VK_SUCCESS != result || 0 >= extension_count) {
        LogFatalError("vkEnumerateInstanceExtensionProperties failed to find any extensions.\n");
        return false;
    }

    // Query the available instance extensions
    std::vector<VkExtensionProperties> extension_properties;
    extension_properties.resize(extension_count);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_properties.data());
    if (VK_SUCCESS != result || 0 >= extension_count) {
        LogFatalError("vkEnumerateInstanceExtensionProperties failed to read any extension information.\n");
        return false;
    }

    for (uint32_t i = 0; i < extension_count; i++) {
        if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extension_properties[i].extensionName)) {
            extensions.push_back(extension_properties[i].extensionName);
        }
    }

    // This is info for a temp callback to use during CreateInstance.
    // After the instance is created, we use the instance-based
    // function to register the final callback.
    VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info = new VkDebugUtilsMessengerCreateInfoEXT;
    // VK_EXT_debug_utils style
    dbg_messenger_create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    dbg_messenger_create_info->pNext = *next;
    dbg_messenger_create_info->flags = 0;
    dbg_messenger_create_info->messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    dbg_messenger_create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    dbg_messenger_create_info->pfnUserCallback = debug_messenger_callback;
    dbg_messenger_create_info->pUserData = this;
    *next = dbg_messenger_create_info;

    return true;
}

bool GlobeLogger::ReleaseCreateInstanceItems(void **next) {
    try {
        GlobeBasicVulkanStruct *prev_next = nullptr;
        GlobeBasicVulkanStruct *next_ptr = reinterpret_cast<GlobeBasicVulkanStruct *>(*next);
        while (next_ptr != nullptr) {
            if (next_ptr->sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT) {
                VkDebugUtilsMessengerCreateInfoEXT *dbg_messenger_create_info =
                    reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(next_ptr);
                if (*next == next_ptr) {
                    *next = next_ptr->pNext;
                    next_ptr = reinterpret_cast<GlobeBasicVulkanStruct *>(*next);
                } else if (nullptr != prev_next) {
                    prev_next->pNext = next_ptr->pNext;
                }
                delete dbg_messenger_create_info;
            }
            prev_next = next_ptr;
            if (nullptr != next_ptr) {
                next_ptr = reinterpret_cast<GlobeBasicVulkanStruct *>(next_ptr->pNext);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool GlobeLogger::CreateInstanceDebugInfo(VkInstance instance) {
    bool locked = false;
    try {
        InstanceDebugInfo instance_debug_info = {};

        // Setup VK_EXT_debug_utils function pointers always (we use them for
        // debug labels and names).
        instance_debug_info.CreateDebugUtilsMessengerEXT =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        instance_debug_info.DestroyDebugUtilsMessengerEXT =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        instance_debug_info.SubmitDebugUtilsMessageEXT =
            (PFN_vkSubmitDebugUtilsMessageEXT)vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");
        instance_debug_info.CmdBeginDebugUtilsLabelEXT =
            (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT");
        instance_debug_info.CmdEndDebugUtilsLabelEXT =
            (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT");
        instance_debug_info.CmdInsertDebugUtilsLabelEXT =
            (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT");
        instance_debug_info.SetDebugUtilsObjectNameEXT =
            (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
        if (NULL == instance_debug_info.CreateDebugUtilsMessengerEXT ||
            NULL == instance_debug_info.DestroyDebugUtilsMessengerEXT ||
            NULL == instance_debug_info.SubmitDebugUtilsMessageEXT ||
            NULL == instance_debug_info.CmdBeginDebugUtilsLabelEXT ||
            NULL == instance_debug_info.CmdEndDebugUtilsLabelEXT ||
            NULL == instance_debug_info.CmdInsertDebugUtilsLabelEXT ||
            NULL == instance_debug_info.SetDebugUtilsObjectNameEXT) {
            LogFatalError("GetProcAddr: Failed to init VK_EXT_debug_utils\n");
            return false;
        }

        // Create a debug messenger
        VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info = {};
        dbg_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbg_messenger_create_info.pNext = nullptr;
        dbg_messenger_create_info.flags = 0;
        dbg_messenger_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbg_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbg_messenger_create_info.pfnUserCallback = debug_messenger_callback;
        dbg_messenger_create_info.pUserData = this;
        if (VK_SUCCESS != instance_debug_info.CreateDebugUtilsMessengerEXT(instance, &dbg_messenger_create_info, NULL,
                                                                           &instance_debug_info.dbg_messenger)) {
            LogFatalError("vkCreateDebugUtilsMessengerEXT: Failed to create messenger\n");
            return false;
        }

        _instance_debug_mutex.lock();
        locked = true;
        _instance_debug_info[instance] = instance_debug_info;
        _instance_debug_mutex.unlock();
        locked = false;
        return true;
    } catch (...) {
        if (locked) {
            _instance_debug_mutex.unlock();
        }
        return false;
    }
}

bool GlobeLogger::DestroyInstanceDebugInfo(VkInstance instance) {
    bool locked = false;
    try {
        _instance_debug_mutex.lock();
        locked = true;
        for (auto it = _instance_debug_info.begin(); it != _instance_debug_info.end();) {
            if (it->first == instance) {
                InstanceDebugInfo instance_debug_info = it->second;
                instance_debug_info.DestroyDebugUtilsMessengerEXT(instance, instance_debug_info.dbg_messenger, nullptr);
                _instance_debug_info.erase(it++);
            } else {
                ++it;
            }
        }
        _instance_debug_mutex.unlock();
        locked = false;
        return true;
    } catch (...) {
        if (locked) {
            _instance_debug_mutex.unlock();
        }
        return false;
    }
}

void GlobeLogger::SetFileOutput(std::string output_file) {
    if (output_file.size() > 0) {
        _file_stream.open(output_file);
        if (_file_stream.fail()) {
            std::cerr << "Error failed opening output file stream for " << output_file << std::endl;
            std::cerr << std::flush;
            _output_file = false;
        } else {
            _output_file = true;
        }
    }
}

void GlobeLogger::LogMessage(const std::string &prefix, const std::string &message) {
    if (_output_cmdline) {
        std::cout << prefix << message << std::endl;
        std::cout << std::flush;
    }
    if (_output_file) {
        _file_stream << prefix << message << std::endl;
        _file_stream << std::flush;
    }
}

void GlobeLogger::LogDebug(std::string message) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_DEBUG, "LunarGlobe", "%s", message.c_str());
#else
    std::string prefix = "LunarGlobe DEBUG: ";
    if (_log_level >= GLOBE_LOG_ALL) {
        LogMessage(prefix, message);
    }
#endif
}

void GlobeLogger::LogInfo(std::string message) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "LunarGlobe", "%s", message.c_str());
#else
    std::string prefix = "LunarGlobe INFO: ";
    if (_log_level >= GLOBE_LOG_INFO_WARN_ERROR) {
        LogMessage(prefix, message);
    }
#endif
}

void GlobeLogger::LogWarning(std::string message) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_WARN, "LunarGlobe", "%s", message.c_str());
#else
    std::string prefix = "LunarGlobe WARNING: ";
    if (_log_level >= GLOBE_LOG_WARN_ERROR) {
        LogMessage(prefix, message);
#ifdef _WIN32
        if (_enable_popups) {
            MessageBox(NULL, message.c_str(), prefix.c_str(), MB_OK);
        }
#endif
    }
#endif
}

void GlobeLogger::LogPerf(std::string message) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_WARN, "LunarGlobe", "%s", message.c_str());
#else
    std::string prefix = "LunarGlobe PERF: ";
    if (_log_level >= GLOBE_LOG_WARN_ERROR) {
        LogMessage(prefix, message);
    }
#endif
}

void GlobeLogger::LogError(std::string message) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_ERROR, "LunarGlobe", "%s", message.c_str());
#else
    std::string prefix = "LunarGlobe ERROR: ";
    if (_log_level >= GLOBE_LOG_ERROR) {
        LogMessage(prefix, message);
#ifdef _WIN32
        if (_enable_popups) {
            MessageBox(NULL, message.c_str(), prefix.c_str(), MB_OK);
        }
#endif
    }
#endif
    if (_enable_break_on_error) {
        assert(false);
    }
}

void GlobeLogger::LogFatalError(std::string message) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_FATAL, "LunarGlobe", "%s", message.c_str());
#else
    std::string prefix = "LunarGlobe FATAL_ERROR: ";
    if (_log_level >= GLOBE_LOG_ERROR) {
        if (_output_cmdline) {
            std::cerr << prefix << message << std::endl;
            std::cerr << std::flush;
        }
        if (_output_file) {
            _file_stream << prefix << message << std::endl;
            _file_stream << std::flush;
        }
#ifdef _WIN32
        if (_enable_popups) {
            MessageBox(NULL, message.c_str(), prefix.c_str(), MB_OK);
        }
#endif
    }
#endif
    // Assert before we exit to allow debugging.
    assert(false);
    exit(-1);
}
