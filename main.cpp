#include <iostream>
#include <optional>
#include <set>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

const std::vector<const char *> __validationLyrs {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char *> __deviceExts {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance _instance,
    const VkDebugUtilsMessengerCreateInfoEXT * _createInfo,
    const VkAllocationCallbacks * _allocator,
    VkDebugUtilsMessengerEXT * _msgr_ext) {
    ;
    // Pointer FunctioN
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr)
        return func(_instance, _createInfo, _allocator, _msgr_ext);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance _instance,
    VkDebugUtilsMessengerEXT _dbgMessenger,
    const VkAllocationCallbacks * _allocator) {
    ;
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != nullptr)
        return func(_instance, _dbgMessenger, _allocator);

    return;
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // gpu family
    std::optional<uint32_t> presentFamily;

    bool isComplete() noexcept {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VkProgram {
public:

    VkProgram(int _width, int _height, const char * _title)
        : width_(_width), height_(_height), title_(_title) {}
    void run() {
        initWindow();
        initVulkan();
        
        mainLoop();

        cleanup();
    }

private:
    GLFWwindow * window_;
    int width_, height_;
    std::string title_;

    VkInstance instance_;

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT dbgMessenger_;
#endif

    VkSurfaceKHR surface_;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_;

    VkQueue graphicsQueue_;
    VkQueue presentQueue_;

    VkSwapchainKHR swapChain_;
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;
    std::vector<VkImageView> swapChainImageViews_;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    }

    void initVulkan() {
        createVkInstance();
#ifndef NDEBUG
        setupDebugMessenger();
#endif
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        for(auto imgView : swapChainImageViews_)
            vkDestroyImageView(device_, imgView, nullptr);

        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
        vkDestroyDevice(device_, nullptr);

#ifndef NDEBUG
        DestroyDebugUtilsMessengerEXT(instance_, dbgMessenger_, nullptr);
#endif

        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkDestroyInstance(instance_, nullptr);

        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void createVkInstance() {
#ifndef NDEBUG
        if(!checkValidationLayerSupport())
            throw std::runtime_error("Vulkan validation layer required, but isn't available");
#endif

        VkApplicationInfo appInfo {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = title_.c_str(),
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(0, 0, 0),
            .apiVersion = VK_API_VERSION_1_2
        };

        VkInstanceCreateInfo instcInfo {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &appInfo
        };

        uint glfwExtCnt = 0;
        const char ** glfwExts;
        glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCnt);

        auto exts = getRequiredExtensions();
        instcInfo.enabledExtensionCount = exts.size();
        instcInfo.ppEnabledExtensionNames = exts.data();
        
#ifndef NDEBUG
        VkDebugUtilsMessengerCreateInfoEXT dbgCreateInfo;

        instcInfo.enabledLayerCount = __validationLyrs.size();
        instcInfo.ppEnabledLayerNames = __validationLyrs.data();

        populateDebugMessengerCreateInfo(dbgCreateInfo);
        instcInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&dbgCreateInfo;
#else
        instcInfo.enabledLayerCount = 0;
        instcInfo.pNext = nullptr;
#endif

        if(vkCreateInstance(&instcInfo, nullptr, &instance_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create vulkan instance");
        
        std::cout << "Successfully created vulkan instance\n";
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT & createInfo) noexcept {
        createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            ,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            ,
            .pfnUserCallback = _debugCallBack
        };
    }

    void setupDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if(CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &dbgMessenger_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create debug utils messenger extension");

        return;
    }

    void createSurface() {
        if(glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface");
    }

    void pickPhysicalDevice() {
        uint32_t deviceCnt = 0;
        vkEnumeratePhysicalDevices(instance_, &deviceCnt, nullptr);

        if(deviceCnt == 0)
            throw std::runtime_error("Failed to find GPU with Vulkan support");
        
        ;
        std::vector<VkPhysicalDevice> devices(deviceCnt);
        vkEnumeratePhysicalDevices(instance_, &deviceCnt, devices.data());

        for(const auto & device : devices) {
            if(isDeviceSuitable(device)) {
                physicalDevice_ = device;
                break;
            }
        }

        if(physicalDevice_ == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find a suitable GPU device");

        return;
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0F;
        for(uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queueFamily,
                .queueCount = 1,
                .pQueuePriorities = &queuePriority
            };
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures {};

        VkDeviceCreateInfo deviceCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,

            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos = queueCreateInfos.data(),

            .pEnabledFeatures = &deviceFeatures,

            .enabledExtensionCount = static_cast<uint32_t>(__deviceExts.size()),
            .ppEnabledExtensionNames = __deviceExts.data()
        };

#ifndef NDEBUG
        deviceCreateInfo.enabledLayerCount = __validationLyrs.size();
        deviceCreateInfo.ppEnabledLayerNames = __validationLyrs.data();
#else
        deviceCreateInfo.enabledLayerCount = 0;
#endif

        if(vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create logical device");
        
        vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice_);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCnt = swapChainSupport.capabilities.minImageCount + 1;
        // imageCnt > ..maxImageCount > 0
        if(swapChainSupport.capabilities.maxImageCount > 0
            && imageCnt > swapChainSupport.capabilities.maxImageCount)
            imageCnt = swapChainSupport.capabilities.maxImageCount;
        
        VkSwapchainCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface_,

            .minImageCount = imageCnt,
            .imageFormat = surfaceFormat.format,
            .imageColorSpace = surfaceFormat.colorSpace,
            .imageExtent = extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT // color buffer?
        };

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if(indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else // graphicsFamily == presentFamily
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain");
        
        vkGetSwapchainImagesKHR(device_, swapChain_, &imageCnt, nullptr);
        swapChainImages_.resize(imageCnt);
        vkGetSwapchainImagesKHR(device_, swapChain_, &imageCnt, swapChainImages_.data());

        swapChainImageFormat_ = surfaceFormat.format;
        swapChainExtent_ = extent;
    }

    void createImageViews() {
        swapChainImageViews_.resize(swapChainImages_.size());

        for(auto i = 0; i< swapChainImages_.size(); ++i) {
            VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = swapChainImages_[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = swapChainImageFormat_,
                .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel = 0,
                .subresourceRange.levelCount = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount = 1
            };

            if(vkCreateImageView(device_, &createInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image view" + std::to_string(i));
        }
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> & availableFormats) noexcept {
        for(const auto & format : availableFormats) {
            if(format.format == VK_FORMAT_B8G8R8A8_SRGB
                && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        }

        return availableFormats.front();
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> & availablePresentModes) noexcept {
        for(const auto & presentMode : availablePresentModes) {
            if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return presentMode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities) noexcept {
        if(capabilities.currentExtent.width != UINT32_MAX)
            return capabilities.currentExtent;
        
        int width, height;
        glfwGetFramebufferSize(window_, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width), static_cast<uint32_t>(height)
        };

        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice _device) noexcept {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, surface_, &details.capabilities);

        uint32_t formatCnt;
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface_, &formatCnt, nullptr);

        if(formatCnt != 0) {
            details.formats.resize(formatCnt);
            vkGetPhysicalDeviceSurfaceFormatsKHR(_device, surface_, &formatCnt, details.formats.data());
        }

        uint32_t presentModeCnt;
        vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface_, &presentModeCnt, nullptr);

        if(presentModeCnt != 0) {
            details.presentModes.resize(presentModeCnt);
            vkGetPhysicalDeviceSurfacePresentModesKHR(_device, surface_, &presentModeCnt, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice _device) noexcept {
        QueueFamilyIndices indices = findQueueFamilies(_device);

        bool extensionsSupported = checkDeviceExtensionSupport(_device);

        bool swapChainAdequate = false;
        if(extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice _device) noexcept {
        uint32_t extensionCnt;
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCnt, nullptr);

        std::vector<VkExtensionProperties> availableExts(extensionCnt);
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCnt, availableExts.data());

        std::set<std::string> requiredExtensions(__deviceExts.begin(), __deviceExts.end());

        for(const auto & ext : availableExts)
            requiredExtensions.erase(ext.extensionName);
            
        return requiredExtensions.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice _device) noexcept {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCnt, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCnt);
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCnt, queueFamilies.data());

        auto i = 0;
        for(const auto & queueFamily : queueFamilies) {
            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;
            
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, surface_, &presentSupport);

            if(presentSupport)
                indices.presentFamily = i;
            
            if(indices.isComplete()) break;

            ++i;
        }

        return indices;
    }

    std::vector<const char *> getRequiredExtensions() noexcept {
        uint glfwExtensionCnt = 0;
        const char ** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCnt);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCnt);

#ifndef NDEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
        return extensions;
    }

    bool checkValidationLayerSupport() noexcept {
        uint layerCnt = 0;
        vkEnumerateInstanceLayerProperties(&layerCnt, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCnt);
        vkEnumerateInstanceLayerProperties(&layerCnt, availableLayers.data());

        for(auto lyrName : __validationLyrs) {
            bool lyrFound = false;

            for(const auto & lyrProps : availableLayers) {
                // is same
                if(strcmp(lyrName, lyrProps.layerName) == 0) {
                    lyrFound = true;
                    break;
                }
            }

            if(!lyrFound)
                return false;
        }
        
        return true;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL _debugCallBack(VkDebugUtilsMessageSeverityFlagBitsEXT _msgSeverity,
        VkDebugUtilsMessageTypeFlagsEXT _msgType,
        const VkDebugUtilsMessengerCallbackDataEXT * _cb_data,
        void * _usr_data) noexcept {
        ;
        std::cerr << "Validation layer: " << _cb_data->pMessage << std::endl;

        return VK_FALSE;
    }
};

auto main(int argc, char * argv[]) -> int32_t {
    
    VkProgram program(800, 600, "Simple Vulkan program");

    try {
        program.run();
    } catch(const std::exception & e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}