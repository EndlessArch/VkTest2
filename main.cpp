#include <iostream>
#include <optional>
#include <vector>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

const std::vector<const char *> __validationLyrs {
    "VK_LAYER_KHRONOS_validation"
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

    bool isComplete() noexcept {
        return graphicsFamily.has_value();
    }
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

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;

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
        pickPhysicalDevice();
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
#ifndef NDEBUG
        DestroyDebugUtilsMessengerEXT(instance_, dbgMessenger_, nullptr);
#endif

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

    bool isDeviceSuitable(VkPhysicalDevice _device) noexcept {
        QueueFamilyIndices indices = findQueueFamilies(_device);

        return indices.isComplete();
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