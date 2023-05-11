#pragma once
#include <iostream>
#include <vulkan/vk_enum_string_helper.h>

constexpr int width = 1920;
constexpr int height = 1080;
constexpr unsigned numPreferredImages = 3;

#define ARR_CNT(arr) (sizeof(arr) / sizeof(arr[0]))
#define STR(val) #val
#define CONCAT(a, b) a b

#ifndef SHADER_PATH
#define SHADER_PATH ""
static_assert(false, "Must define a shader path for executable to look in");
#endif

#ifndef ASSET_PATH
#define ASSET_PATH ""
static_assert(false, "Must define an asset path for executable to look in");
#endif

#ifndef NDEBUG
#define VK_LOG_ERR(f_)                                                                                                                                         \
    {                                                                                                                                                          \
        VkResult retCode = f_;                                                                                                                                 \
        if (retCode != VK_SUCCESS)                                                                                                                             \
        {                                                                                                                                                      \
            std::cerr << string_VkResult(retCode) << " Returned from " << __FUNCTION__ << " in " << __FILE__ << ":" << __LINE__ << std::endl;                  \
        }                                                                                                                                                      \
    }
#else
#define VK_LOG_ERR(f_) f_
#endif

#define VK_LOG_ERR_FATAL(f_)                                                                                                                                   \
    {                                                                                                                                                          \
        VkResult retCode = f_;                                                                                                                                 \
        if (retCode != VK_SUCCESS)                                                                                                                             \
        {                                                                                                                                                      \
            throw std::invalid_argument(string_VkResult(retCode) + " Returned from " + __FUNCTION__ + " in " + __FILE__ + ":" + __LINE__);                     \
        }                                                                                                                                                      \
    }

#define LOG_ERR_FATAL(cond)                                                                                                                                    \
    {                                                                                                                                                          \
        if (!cond)                                                                                                                                             \
        {                                                                                                                                                      \
            throw std::invalid_argument(std::string(#cond) + " Returned from " + __FUNCTION__ + " in " + __FILE__ + ":" + __LINE__);                           \
        }                                                                                                                                                      \
    }
