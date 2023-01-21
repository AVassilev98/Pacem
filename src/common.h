#pragma once
#include <iostream>
#include <vulkan/vk_enum_string_helper.h>

constexpr int width = 1920;
constexpr int height = 1080;
constexpr unsigned numPreferredImages = 3;

#define ARR_CNT(arr) (sizeof(arr) / sizeof(arr[0]))

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
