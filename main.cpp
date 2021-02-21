#include <fstream>
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
    std::vector<VkFramebuffer> swapChainFrameBuffers_;

    VkRenderPass renderPass_;
    VkPipelineLayout pipelineLayout_;
    VkPipeline graphicsPipeline_;

    VkCommandPool commandPool_;
    std::vector<VkCommandBuffer> commandBuffers_;

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
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandPool();
        createCommandBuffers();
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(window_)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyCommandPool(device_, commandPool_, nullptr);

        for(auto frameBuffer : swapChainFrameBuffers_)
            vkDestroyFramebuffer(device_, frameBuffer, nullptr);

        vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        vkDestroyRenderPass(device_, renderPass_, nullptr);

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

#ifndef NDEBUG
    void setupDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if(CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &dbgMessenger_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create debug utils messenger extension");

        return;
    }
#endif

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

    void createRenderPass() {
        VkAttachmentDescription colorAttachment = {
            .format = swapChainImageFormat_,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef
        };

        VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass
        };

        if(vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass");
    }

    void createGraphicsPipeline() {
        // though using vulkan1.2, compiling SPIR-V code as vulkan1.1 has no any issue
        // glslc -w -x glsl --target-env=vulkan1.1 -O (filename).(frag/vert) -o (filename).spv
        auto fragShaderCode = readFile("frag.spv");
        auto vertShaderCode = readFile("vert.spv");

        VkShaderModule fragShaderMod = createShaderModule(fragShaderCode);
        VkShaderModule vertShaderMod = createShaderModule(vertShaderCode);

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderMod,
            .pName = "main"
        }, vertShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderMod,
            .pName = "main"
        };

        VkPipelineShaderStageCreateInfo shaderStages[] = { fragShaderStageInfo, vertShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .vertexAttributeDescriptionCount = 0
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE
        };

        VkViewport viewport = {
            .x = 0.0F,
            .y = 0.0F,
            .width = (float)swapChainExtent_.width,
            .height = (float)swapChainExtent_.height,
            .minDepth = 0.0F,
            .maxDepth = 1.0F
        };

        VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = swapChainExtent_
        };

        VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0F,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE
        };

        VkPipelineMultisampleStateCreateInfo multiSampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE
        };

        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants = { 0.0F, 0.0F, 0.0F, 0.0F }
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0,
            .pushConstantRangeCount = 0
        };

        if(vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout");
        
        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = std::extent_v<decltype(shaderStages)>,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multiSampling,
            .pColorBlendState = &colorBlending,
            .layout = pipelineLayout_,
            .renderPass = renderPass_,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE
        };

        if(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline");

        vkDestroyShaderModule(device_, fragShaderMod, nullptr);
        vkDestroyShaderModule(device_, vertShaderMod, nullptr);
    }

    void createFrameBuffers() {
        swapChainFrameBuffers_.resize(swapChainImageViews_.size());

        for(auto i = 0; i< swapChainImageViews_.size(); ++i) {
            VkImageView attachments[] = {
                swapChainImageViews_[i]
            };

            VkFramebufferCreateInfo frameBufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass_,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = swapChainExtent_.width,
                .height = swapChainExtent_.height,
                .layers = 1
            };

            if(vkCreateFramebuffer(device_, &frameBufferInfo, nullptr, &swapChainFrameBuffers_[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create frame buffer(" + std::to_string(i) + ")");
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queryFamilyIndices = findQueueFamilies(physicalDevice_);

        VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = queryFamilyIndices.graphicsFamily.value()
        };

        if(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool");
    }

    void createCommandBuffers() {
        commandBuffers_.resize(swapChainFrameBuffers_.size());

        VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool_,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = (uint32_t)commandBuffers_.size()
        };

        if(vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers");

        for(auto i = 0; i< commandBuffers_.size(); ++i) {
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            };

            if(vkBeginCommandBuffer(commandBuffers_[i], &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("Failed to begin command buffer(" + std::to_string(i) + ")");
            
            VkRenderPassBeginInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = renderPass_,
                .framebuffer = swapChainFrameBuffers_[i],
                .renderArea.offset = { 0, 0 },
                .renderArea.extent = swapChainExtent_
            };

            VkClearValue clearColor = { 0.0F, 0.0F, 0.0F, 1.0F };

            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(commandBuffers_[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(commandBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

            vkCmdDraw(commandBuffers_[i], 3, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffers_[i]);

            if(vkEndCommandBuffer(commandBuffers_[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to end command buffer");
        }
    }

    VkShaderModule createShaderModule(const std::vector<char> & code) {
        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(), // * sizeof(uint32_t), // idk why not
            .pCode = reinterpret_cast<const uint32_t *>(code.data())
        };

        VkShaderModule shaderModule;
        if(vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module");
        
        return shaderModule;
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

    static std::vector<char> readFile(const std::string & _filename) {
        std::ifstream file(_filename, std::ios::ate | std::ios::binary);

        if(!file.is_open())
            throw std::runtime_error("Failed to open file(\"" + _filename + "\")");
        
        std::size_t fileSz = (std::size_t)file.tellg();
        std::vector<char> buf(fileSz);

        file.seekg(0);
        file.read(buf.data(), fileSz);

        file.close();

        return buf;
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