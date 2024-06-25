#pragma once

#include <vulkan/vulkan.hpp>
#include <iostream>

namespace ref_vk {

    enum LOG_ERRORS {
        NORMAL,
        ERR,
        DEBUG,
    };

    void LOG(LOG_ERRORS type, const char *text) {
        std::cout << "Log ";
        switch (type) {
            case LOG_ERRORS::NORMAL:
                std::cout << "Normal: " << text << "\n";
                break;

            case LOG_ERRORS::ERR:
                std::cout << "Error: " << text << "\n";
                break;

            case LOG_ERRORS::DEBUG:
                std::cout << "Debug: " << text << "\n";
                break;

            default:
                std::cout << "Unknown: " << text << "\n";
                break;
        }
    }

    bool VK_CHECK_RESULT(VkResult res) {
        if (res != VK_SUCCESS) {
            LOG(DEBUG, "Function execute error for vulkan!");
            return false;
        }
        return true;
    }

    bool VK_CHECK_RESULT(VkResult res, const char* text) {
        if (res != VK_SUCCESS) {
            LOG(DEBUG, text);
            return false;
        }
        return true;
    }
}
