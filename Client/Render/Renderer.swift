import Foundation
import Vulkan

#if os(macOS)
    import Metal
    import QuartzCore
#endif

func makeVulkanVersion(_ major: UInt32, _ minor: UInt32, _ patch: UInt32) -> UInt32 {
    return (major << 22) | (minor << 12) | patch
}

func withCPointers(
    _ strings: [String], _ pointers: [UnsafePointer<CChar>?] = [],
    _ body: ([UnsafePointer<CChar>?]) throws -> Void
) throws {
    if pointers.count == strings.count {
        try body(pointers)
    } else {
        try strings[pointers.count].withCString { pStr in
            try withCPointers(strings, pointers + [pStr], body)
        }
    }
}

enum VulkanError: Error {
    case instanceCreationFailed(VkResult)
    case surfaceCreationFailed(VkResult)
    case physicalDeviceSelectionFailed
    case deviceCreationFailed(VkResult)
    case swapchainCreationFailed(VkResult)
    case commandPoolCreationFailed(VkResult)
    case pipelineCreationFailed(VkResult)
}

public class Renderer {
    private var instance: VkInstance? = nil
    private var surface: VkSurfaceKHR? = nil
    private var physicalDevice: VkPhysicalDevice? = nil
    private var queueFamilyIndex: UInt32? = nil
    private var device: VkDevice? = nil
    private var graphicsQueue: VkQueue? = nil

    private var swapchain: VkSwapchainKHR? = nil
    private var swapchainImages: [VkImage] = []
    private var swapchainImageViews: [VkImageView] = []
    private var swapchainFormat: VkFormat = VK_FORMAT_B8G8R8A8_UNORM
    private var swapchainExtent: VkExtent2D = VkExtent2D()

    private var commandPool: VkCommandPool? = nil
    private var commandBuffers: [VkCommandBuffer?] = []

    private var pipelineLayout: VkPipelineLayout? = nil
    private var graphicsPipeline: VkPipeline? = nil

    private let MAX_FRAMES_IN_FLIGHT = 2
    private var currentFrame = 0

    private var imageAvailableSemaphores: [VkSemaphore?] = []
    private var renderFinishedSemaphores: [VkSemaphore?] = []
    private var inFlightFences: [VkFence?] = []

    #if os(macOS)
        private var metalLayer: CAMetalLayer

        public init(metalLayer: CAMetalLayer) throws {
            self.metalLayer = metalLayer
            var appInfo = VkApplicationInfo()
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO
            appInfo.apiVersion = makeVulkanVersion(1, 4, 0)

            try "Minecraft".withCString { pAppName in
                appInfo.pApplicationName = pAppName
                appInfo.applicationVersion = makeVulkanVersion(1, 0, 0)

                try createInstance(appInfo: &appInfo)
                try createSurface()
                try pickPhysicalDevice()
                try createDevice()
                try createSwapchain()
                try createCommandPool()
                try createPipeline()
                try createSyncObjects()
            }
        }

        deinit {
            vkDeviceWaitIdle(device)

            for i in 0..<MAX_FRAMES_IN_FLIGHT {
                vkDestroyFence(device, inFlightFences[i], nil)
            }

            for semaphore in imageAvailableSemaphores {
                vkDestroySemaphore(device, semaphore, nil)
            }
            for semaphore in renderFinishedSemaphores {
                vkDestroySemaphore(device, semaphore, nil)
            }

            vkDestroyPipeline(device, graphicsPipeline, nil)
            vkDestroyPipelineLayout(device, pipelineLayout, nil)

            vkDestroyCommandPool(device, commandPool, nil)

            for imageView in swapchainImageViews {
                vkDestroyImageView(device, imageView, nil)
            }
            vkDestroySwapchainKHR(device, swapchain, nil)

            vkDestroyDevice(device, nil)
            vkDestroySurfaceKHR(instance, surface, nil)
            vkDestroyInstance(instance, nil)
        }
    #endif

    private func createInstance(appInfo: inout VkApplicationInfo) throws {
        let extensionNames = [
            "VK_KHR_portability_enumeration",
            "VK_KHR_surface",
            "VK_MVK_macos_surface",
        ]

        try withCPointers(extensionNames) { pExtensions in
            try withUnsafePointer(to: &appInfo) { pAppInfo in
                var createInfo = VkInstanceCreateInfo()
                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
                createInfo.pApplicationInfo = pAppInfo
                createInfo.enabledExtensionCount = UInt32(pExtensions.count)
                createInfo.ppEnabledExtensionNames = pExtensions.withUnsafeBufferPointer {
                    $0.baseAddress
                }
                createInfo.flags = 0x0000_0001  // VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR

                let result = vkCreateInstance(&createInfo, nil, &instance)
                if result != VK_SUCCESS {
                    throw VulkanError.instanceCreationFailed(result)
                }
            }
        }
    }

    private func createSurface() throws {
        var surface: VkSurfaceKHR?
        var surfaceCreateInfo = VkMacOSSurfaceCreateInfoMVK()
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK

        let mutablePointer = Unmanaged.passUnretained(metalLayer).toOpaque()

        surfaceCreateInfo.pView = UnsafeRawPointer(mutablePointer)

        let result = vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, nil, &surface)
        if result != VK_SUCCESS {
            throw VulkanError.surfaceCreationFailed(result)
        }

        self.surface = surface
    }

    private func pickPhysicalDevice() throws {
        var physicalDeviceCount: UInt32 = 0
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nil)

        guard physicalDeviceCount > 0 else {
            throw VulkanError.physicalDeviceSelectionFailed
        }

        var physicalDevices = [VkPhysicalDevice?](repeating: nil, count: Int(physicalDeviceCount))
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, &physicalDevices)

        guard let physicalDevice = physicalDevices.first else {
            throw VulkanError.physicalDeviceSelectionFailed
        }

        self.physicalDevice = physicalDevice

        var queueFamilyProperties: [VkQueueFamilyProperties] = []
        var count: UInt32 = 0
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nil)
        queueFamilyProperties = [VkQueueFamilyProperties](
            repeating: VkQueueFamilyProperties(), count: Int(count))
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, &queueFamilyProperties)

        var foundIndex: UInt32? = nil
        for (i, queueFamily) in queueFamilyProperties.enumerated() {
            if queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT.rawValue != 0 {
                foundIndex = UInt32(i)
                break
            }
        }

        guard let index = foundIndex else {
            throw VulkanError.physicalDeviceSelectionFailed
        }

        self.queueFamilyIndex = index
    }

    private func createDevice() throws {
        guard let queueFamilyIndex = queueFamilyIndex else {
            throw VulkanError.physicalDeviceSelectionFailed
        }

        var queuePriority: Float = 1.0
        var queueCreateInfo = VkDeviceQueueCreateInfo()
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex
        queueCreateInfo.queueCount = 1
        queueCreateInfo.pQueuePriorities = withUnsafePointer(to: &queuePriority) { $0 }

        let deviceExtensions = [
            "VK_KHR_portability_subset",
            "VK_KHR_swapchain",
        ]

        try withCPointers(deviceExtensions) { pExtensions in
            var features13 = VkPhysicalDeviceVulkan13Features()
            features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
            features13.dynamicRendering = VK_TRUE

            withUnsafePointer(to: &features13) { pFeatures13 in
                var deviceCreateInfo = VkDeviceCreateInfo()
                deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
                deviceCreateInfo.queueCreateInfoCount = 1
                deviceCreateInfo.pQueueCreateInfos = withUnsafePointer(to: &queueCreateInfo) { $0 }
                deviceCreateInfo.enabledExtensionCount = UInt32(pExtensions.count)
                deviceCreateInfo.ppEnabledExtensionNames = pExtensions.withUnsafeBufferPointer {
                    $0.baseAddress
                }
                deviceCreateInfo.pNext = UnsafeRawPointer(pFeatures13)

                let result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nil, &device)
                if result != VK_SUCCESS {
                    print("Failed to create device: \(result)")
                }
            }

            vkGetDeviceQueue(device, queueFamilyIndex, 0, &graphicsQueue)
        }
    }

    private func createSwapchain() throws {
        var capabilities = VkSurfaceCapabilitiesKHR()
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities)

        swapchainExtent = capabilities.currentExtent
        if swapchainExtent.width == 0xFFFF_FFFF {
            swapchainExtent = VkExtent2D(width: 800, height: 600)
        }

        var createInfo = VkSwapchainCreateInfoKHR()
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
        createInfo.surface = surface
        createInfo.minImageCount = capabilities.minImageCount + 1
        if capabilities.maxImageCount > 0 && createInfo.minImageCount > capabilities.maxImageCount {
            createInfo.minImageCount = capabilities.maxImageCount
        }
        createInfo.imageFormat = swapchainFormat
        createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        createInfo.imageExtent = swapchainExtent
        createInfo.imageArrayLayers = 1
        createInfo.imageUsage = UInt32(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT.rawValue)
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE
        createInfo.preTransform = capabilities.currentTransform
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR
        createInfo.clipped = VK_TRUE

        let result = vkCreateSwapchainKHR(device, &createInfo, nil, &swapchain)
        if result != VK_SUCCESS {
            throw VulkanError.swapchainCreationFailed(result)
        }

        var count: UInt32 = 0
        vkGetSwapchainImagesKHR(device, swapchain, &count, nil)
        swapchainImages = [VkImage?](repeating: nil, count: Int(count)).compactMap { $0 }  // Dummy to size it
        // Actually we need to get the images
        var images = [VkImage?](repeating: nil, count: Int(count))
        vkGetSwapchainImagesKHR(device, swapchain, &count, &images)
        swapchainImages = images.compactMap { $0 }

        for image in swapchainImages {
            var viewInfo = VkImageViewCreateInfo()
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
            viewInfo.image = image
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D
            viewInfo.format = swapchainFormat
            viewInfo.components = VkComponentMapping(
                r: VK_COMPONENT_SWIZZLE_IDENTITY, g: VK_COMPONENT_SWIZZLE_IDENTITY,
                b: VK_COMPONENT_SWIZZLE_IDENTITY, a: VK_COMPONENT_SWIZZLE_IDENTITY)
            viewInfo.subresourceRange = VkImageSubresourceRange(
                aspectMask: UInt32(VK_IMAGE_ASPECT_COLOR_BIT.rawValue), baseMipLevel: 0,
                levelCount: 1, baseArrayLayer: 0, layerCount: 1)

            var imageView: VkImageView?
            let viewResult = vkCreateImageView(device, &viewInfo, nil, &imageView)
            if viewResult != VK_SUCCESS {
                throw VulkanError.swapchainCreationFailed(viewResult)
            }
            swapchainImageViews.append(imageView!)
        }
    }

    private func createCommandPool() throws {
        var poolInfo = VkCommandPoolCreateInfo()
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
        poolInfo.flags = UInt32(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.rawValue)
        poolInfo.queueFamilyIndex = queueFamilyIndex!

        let result = vkCreateCommandPool(device, &poolInfo, nil, &commandPool)
        if result != VK_SUCCESS {
            throw VulkanError.commandPoolCreationFailed(result)
        }

        var allocInfo = VkCommandBufferAllocateInfo()
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
        allocInfo.commandPool = commandPool
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
        allocInfo.commandBufferCount = UInt32(MAX_FRAMES_IN_FLIGHT)

        commandBuffers = [VkCommandBuffer?](repeating: nil, count: MAX_FRAMES_IN_FLIGHT)
        let bufResult = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffers)
        if bufResult != VK_SUCCESS {
            throw VulkanError.commandPoolCreationFailed(bufResult)
        }
    }

    private func createSyncObjects() throws {
        inFlightFences = [VkFence?](repeating: nil, count: MAX_FRAMES_IN_FLIGHT)

        var semaphoreInfo = VkSemaphoreCreateInfo()
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO

        var fenceInfo = VkFenceCreateInfo()
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        fenceInfo.flags = UInt32(VK_FENCE_CREATE_SIGNALED_BIT.rawValue)

        for i in 0..<MAX_FRAMES_IN_FLIGHT {
            if vkCreateFence(device, &fenceInfo, nil, &inFlightFences[i]) != VK_SUCCESS {
                throw VulkanError.instanceCreationFailed(VK_ERROR_INITIALIZATION_FAILED)
            }
        }

        imageAvailableSemaphores = [VkSemaphore?](repeating: nil, count: swapchainImages.count)
        renderFinishedSemaphores = [VkSemaphore?](repeating: nil, count: swapchainImages.count)

        for i in 0..<swapchainImages.count {
            if vkCreateSemaphore(device, &semaphoreInfo, nil, &imageAvailableSemaphores[i]) != VK_SUCCESS
                || vkCreateSemaphore(device, &semaphoreInfo, nil, &renderFinishedSemaphores[i])
                    != VK_SUCCESS
            {
                throw VulkanError.instanceCreationFailed(VK_ERROR_INITIALIZATION_FAILED)
            }
        }
    }

    private func createShaderModule(path: String) throws -> VkShaderModule {
        let url = URL(fileURLWithPath: path)
        let data = try Data(contentsOf: url)

        return try data.withUnsafeBytes { buffer in
            var createInfo = VkShaderModuleCreateInfo()
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
            createInfo.codeSize = data.count
            createInfo.pCode = buffer.bindMemory(to: UInt32.self).baseAddress

            var module: VkShaderModule?
            let result = vkCreateShaderModule(device, &createInfo, nil, &module)
            if result != VK_SUCCESS {
                throw VulkanError.pipelineCreationFailed(result)
            }
            return module!
        }
    }

    private func createPipeline() throws {
        let vertShader = try createShaderModule(path: "Client/Render/Shaders/triangle.vert.spv")
        let fragShader = try createShaderModule(path: "Client/Render/Shaders/triangle.frag.spv")

        defer {
            vkDestroyShaderModule(device, vertShader, nil)
            vkDestroyShaderModule(device, fragShader, nil)
        }

        try "main".withCString { pVertName in
            try "main".withCString { pFragName in
                var vertStageInfo = VkPipelineShaderStageCreateInfo()
                vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
                vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT
                vertStageInfo.module = vertShader
                vertStageInfo.pName = pVertName

                var fragStageInfo = VkPipelineShaderStageCreateInfo()
                fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
                fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT
                fragStageInfo.module = fragShader
                fragStageInfo.pName = pFragName

                let shaderStages = [vertStageInfo, fragStageInfo]

                var vertexInputInfo = VkPipelineVertexInputStateCreateInfo()
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO

                var inputAssembly = VkPipelineInputAssemblyStateCreateInfo()
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
                inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST

                var viewportState = VkPipelineViewportStateCreateInfo()
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
                viewportState.viewportCount = 1
                viewportState.scissorCount = 1

                var rasterizer = VkPipelineRasterizationStateCreateInfo()
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
                rasterizer.lineWidth = 1.0
                rasterizer.cullMode = UInt32(VK_CULL_MODE_BACK_BIT.rawValue)
                rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE

                var multisampling = VkPipelineMultisampleStateCreateInfo()
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT

                var colorBlendAttachment = VkPipelineColorBlendAttachmentState()
                colorBlendAttachment.colorWriteMask = UInt32(
                    VK_COLOR_COMPONENT_R_BIT.rawValue | VK_COLOR_COMPONENT_G_BIT.rawValue
                        | VK_COLOR_COMPONENT_B_BIT.rawValue | VK_COLOR_COMPONENT_A_BIT.rawValue)

                var colorBlending = VkPipelineColorBlendStateCreateInfo()
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
                colorBlending.attachmentCount = 1
                colorBlending.pAttachments = withUnsafePointer(to: &colorBlendAttachment) { $0 }

                let dynamicStates = [VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR]
                var dynamicState = VkPipelineDynamicStateCreateInfo()
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
                dynamicState.dynamicStateCount = UInt32(dynamicStates.count)
                dynamicState.pDynamicStates = dynamicStates.withUnsafeBufferPointer {
                    $0.baseAddress
                }

                var layoutInfo = VkPipelineLayoutCreateInfo()
                layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
                if vkCreatePipelineLayout(device, &layoutInfo, nil, &pipelineLayout) != VK_SUCCESS {
                    throw VulkanError.pipelineCreationFailed(VK_ERROR_INITIALIZATION_FAILED)
                }

                var pipelineInfo = VkGraphicsPipelineCreateInfo()
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
                pipelineInfo.stageCount = 2
                pipelineInfo.pStages = shaderStages.withUnsafeBufferPointer { $0.baseAddress }
                pipelineInfo.pVertexInputState = withUnsafePointer(to: &vertexInputInfo) { $0 }
                pipelineInfo.pInputAssemblyState = withUnsafePointer(to: &inputAssembly) { $0 }
                pipelineInfo.pViewportState = withUnsafePointer(to: &viewportState) { $0 }
                pipelineInfo.pRasterizationState = withUnsafePointer(to: &rasterizer) { $0 }
                pipelineInfo.pMultisampleState = withUnsafePointer(to: &multisampling) { $0 }
                pipelineInfo.pColorBlendState = withUnsafePointer(to: &colorBlending) { $0 }
                pipelineInfo.pDynamicState = withUnsafePointer(to: &dynamicState) { $0 }
                pipelineInfo.layout = pipelineLayout

                var renderingInfo = VkPipelineRenderingCreateInfo()
                renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO
                renderingInfo.colorAttachmentCount = 1
                var format = swapchainFormat
                renderingInfo.pColorAttachmentFormats = withUnsafePointer(to: &format) { $0 }

                pipelineInfo.pNext = UnsafeRawPointer(withUnsafePointer(to: &renderingInfo) { $0 })

                let result = vkCreateGraphicsPipelines(
                    device, nil, 1, &pipelineInfo, nil, &graphicsPipeline)
                if result != VK_SUCCESS {
                    throw VulkanError.pipelineCreationFailed(result)
                }
            }
        }
    }

    public func render() {
        withUnsafePointer(to: &inFlightFences[currentFrame]) { pFence in
            vkWaitForFences(device, 1, pFence, VK_TRUE, UInt64.max)
            vkResetFences(device, 1, pFence)
        }

        var imageIndex: UInt32 = 0
        // We use a temporary semaphore for acquisition, but actually we need to know WHICH image we will get.
        // Wait, the acquire semaphore is passed IN. We don't know the image index yet.
        // So we should have a pool of semaphores for acquisition.
        
        // Actually, the common pattern IS to have MAX_FRAMES_IN_FLIGHT semaphores.
        // The error suggests that the semaphore is still in use by the presentation of ONE image when we try to signal it for another.
        
        // Let's try to stick to MAX_FRAMES_IN_FLIGHT but make sure we don't reuse the "finished" semaphore too early.
        // If we use imageIndex for the "finished" semaphore, it should be safe because that image won't be re-acquired until presentation is fully over.
        
        // Wait, let's use currentFrame for imageAvailable (since we haven't acquired yet) 
        // and imageIndex for renderFinished.
        
        let acquireSemaphore = imageAvailableSemaphores[currentFrame] // This should be indexed by currentFrame
        vkAcquireNextImageKHR(device, swapchain, UInt64.max, acquireSemaphore, nil, &imageIndex)

        let commandBuffer = commandBuffers[currentFrame]!
        vkResetCommandBuffer(commandBuffer, 0)

        var beginInfo = VkCommandBufferBeginInfo()
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        vkBeginCommandBuffer(commandBuffer, &beginInfo)

        // Image layout transition to attachment
        var barrier = VkImageMemoryBarrier()
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED
        barrier.image = swapchainImages[Int(imageIndex)]
        barrier.subresourceRange = VkImageSubresourceRange(
            aspectMask: UInt32(VK_IMAGE_ASPECT_COLOR_BIT.rawValue), baseMipLevel: 0, levelCount: 1,
            baseArrayLayer: 0, layerCount: 1)
        barrier.srcAccessMask = 0
        barrier.dstAccessMask = UInt32(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT.rawValue)

        vkCmdPipelineBarrier(
            commandBuffer, UInt32(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT.rawValue),
            UInt32(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT.rawValue), 0, 0, nil, 0, nil, 1,
            &barrier)

        var colorAttachment = VkRenderingAttachmentInfo()
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO
        colorAttachment.imageView = swapchainImageViews[Int(imageIndex)]
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE

        var clearValue = VkClearValue()
        clearValue.color = VkClearColorValue(float32: (0.5, 0.7, 1.0, 1.0))
        colorAttachment.clearValue = clearValue

        var renderingInfo = VkRenderingInfo()
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO
        renderingInfo.renderArea = VkRect2D(offset: VkOffset2D(x: 0, y: 0), extent: swapchainExtent)
        renderingInfo.layerCount = 1
        renderingInfo.colorAttachmentCount = 1

        withUnsafePointer(to: &colorAttachment) { pColorAttachment in
            renderingInfo.pColorAttachments = pColorAttachment
            vkCmdBeginRendering(commandBuffer, &renderingInfo)
        }

        var viewport = VkViewport(
            x: 0, y: 0, width: Float(swapchainExtent.width), height: Float(swapchainExtent.height),
            minDepth: 0.0, maxDepth: 1.0)
        withUnsafePointer(to: &viewport) { pViewport in
            vkCmdSetViewport(commandBuffer, 0, 1, pViewport)
        }

        var scissor = VkRect2D(offset: VkOffset2D(x: 0, y: 0), extent: swapchainExtent)
        withUnsafePointer(to: &scissor) { pScissor in
            vkCmdSetScissor(commandBuffer, 0, 1, pScissor)
        }

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline)
        vkCmdDraw(commandBuffer, 3, 1, 0, 0)

        vkCmdEndRendering(commandBuffer)

        // Transition to present
        barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        barrier.srcAccessMask = UInt32(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT.rawValue)
        barrier.dstAccessMask = 0
        vkCmdPipelineBarrier(
            commandBuffer, UInt32(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT.rawValue),
            UInt32(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.rawValue), 0, 0, nil, 0, nil, 1, &barrier)

        vkEndCommandBuffer(commandBuffer)

        var submitInfo = VkSubmitInfo()
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO
        submitInfo.waitSemaphoreCount = 1

        var imageAvailableSemaphore = imageAvailableSemaphores[currentFrame]
        var renderFinishedSemaphore = renderFinishedSemaphores[Int(imageIndex)]
        var currentCommandBuffer = commandBuffers[currentFrame]

        withUnsafePointer(to: &imageAvailableSemaphore) { pWaitSemaphore in
            submitInfo.pWaitSemaphores = pWaitSemaphore
            let waitStages: [UInt32] = [UInt32(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT.rawValue)]
            
            waitStages.withUnsafeBufferPointer { pWaitStages in
                submitInfo.pWaitDstStageMask = pWaitStages.baseAddress
                submitInfo.commandBufferCount = 1
                
                withUnsafePointer(to: &currentCommandBuffer) { pCommandBuffer in
                    submitInfo.pCommandBuffers = pCommandBuffer
                    submitInfo.signalSemaphoreCount = 1
                    
                    withUnsafePointer(to: &renderFinishedSemaphore) { pSignalSemaphore in
                        submitInfo.pSignalSemaphores = pSignalSemaphore
                        
                        if let fence = inFlightFences[currentFrame] {
                            vkQueueSubmit(graphicsQueue, 1, &submitInfo, fence)
                        }
                    }
                }
            }
        }

        var presentInfo = VkPresentInfoKHR()
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
        presentInfo.waitSemaphoreCount = 1
        
        withUnsafePointer(to: &renderFinishedSemaphore) { pWaitSemaphore in
            presentInfo.pWaitSemaphores = pWaitSemaphore
            presentInfo.swapchainCount = 1
            
            withUnsafePointer(to: &swapchain) { pSwapchain in
                presentInfo.pSwapchains = pSwapchain
                
                withUnsafePointer(to: &imageIndex) { pImageIndex in
                    presentInfo.pImageIndices = pImageIndex
                    vkQueuePresentKHR(graphicsQueue, &presentInfo)
                }
            }
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT
    }
}
