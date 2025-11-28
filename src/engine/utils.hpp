#pragma once

#include <vulkan/vulkan.h>

#include <set>
#include <vector>

// TODO: use Smol/SMOL_ or something similar to start the defines and the functions

#define RECREATE_SWAPCHAIN ~(0u)

// Message formating
#define MAX_MSG_SIZE 256u
#define MSG_RED "\033[31m"
#define MSG_RED_B "\033[31;1m"      // bold red
#define MSG_GREEN "\033[32m"
#define MSG_GREEN_B "\033[32;1m"    // bold green
#define MSG_YELLOW "\033[33m"
#define MSG_YELLOW_B "\033[33;1m"   // bold yellow
#define MSG_MAGENTA "\033[35m"
#define MSG_MAGENTA_B "\033[35;1m"  // bold magenta
#define MSG_CYAN "\033[36m"
#define MSG_CYAN_B "\033[36;1m"     // bold cyan
#define MSG_RESET "\033[0m"

inline void vkCheck(VkResult result, const char* msg) {
    if (result != VK_SUCCESS) {
        char buf[MAX_MSG_SIZE];
        snprintf(buf, sizeof(buf), "%s (error code %s%i%s)", msg, MSG_RED, (int)result, MSG_RESET);
        throw std::runtime_error(buf);
    }
}
