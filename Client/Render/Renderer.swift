import Vulkan

#if os(macOS)
    import Metal
    import QuartzCore
#endif

enum VulkanError: Error {
    case instanceCreationFailed(VkResult)
    case surfaceCreationFailed(VkResult)
    case physicalDeviceSelectionFailed
    case deviceCreationFailed(VkResult)
}

public class Renderer {
    private var instance: VkInstance? = nil
    private var surface: VkSurfaceKHR? = nil
    private var physicalDevice: VkPhysicalDevice? = nil
    private var device: VkDevice? = nil

    #if os(macOS)
        private var metalLayer: CAMetalLayer

        public init(metalLayer: CAMetalLayer) throws {
            self.metalLayer = metalLayer
            var appInfo = VkApplicationInfo()
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO
            appInfo.apiVersion = 0x0040_2000  // Vulkan 1.2

            try "Minecraft".withCString { pAppName in
                appInfo.pApplicationName = pAppName

                try createInstance(appInfo: &appInfo)
                try createSurface()
            }
        }

        deinit {
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

    public func render() {
        #if os(macOS)
            if let drawable = metalLayer.nextDrawable() {
                let renderPassDescriptor = MTLRenderPassDescriptor()
                renderPassDescriptor.colorAttachments[0].texture = drawable.texture
                renderPassDescriptor.colorAttachments[0].loadAction = .clear
                renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColor(
                    red: 0.5, green: 0.7, blue: 1.0, alpha: 1.0)

                let commandBuffer = metalLayer.device?.makeCommandQueue()?.makeCommandBuffer()
                let renderEncoder = commandBuffer?.makeRenderCommandEncoder(
                    descriptor: renderPassDescriptor)
                renderEncoder?.endEncoding()

                commandBuffer?.present(drawable)
                commandBuffer?.commit()
            }
        #endif
    }
}
