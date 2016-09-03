/*

	Copyright 2016 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Tutorial 50 - Vulkan
*/

#include <cfloat>
#include <math.h>
#include <GL/glew.h>
#include <string>
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <sys/types.h>
#include <vector>

#include "ogldev_engine_common.h"
#include "ogldev_app.h"
#include "ogldev_util.h"
#include "ogldev_vulkan.h"

#include "ogldev_xcb_control.h"


#define WINDOW_WIDTH  1024  
#define WINDOW_HEIGHT 1024

PFN_vkGetPhysicalDeviceSurfaceFormatsKHR pfnGetPhysicalDeviceSurfaceFormatsKHR = NULL;
PFN_vkCreateDebugReportCallbackEXT my_vkCreateDebugReportCallbackEXT = NULL; 
//PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT = NULL;
//PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = NULL;

       
VKAPI_ATTR VkBool32 VKAPI_CALL MyDebugReportCallback(
    VkDebugReportFlagsEXT       flags,
    VkDebugReportObjectTypeEXT  objectType,
    uint64_t                    object,
    size_t                      location,
    int32_t                     messageCode,
    const char*                 pLayerPrefix,
    const char*                 pMessage,
    void*                       pUserData)
{
    printf("%s\n", pMessage);
    return VK_FALSE;
}

class OgldevVulkanApp
{
public:

    OgldevVulkanApp(const char* pAppName);
    
    ~OgldevVulkanApp();
    
    bool Init();    
    
    void Run();
    
private:

    void EnumDevices();
    void EnumPhysDeviceProps();
    void EnumPhysDeviceExtProps();
    void CreateInstance();
    void CreateDevice();
    void CreateSurface();
    void CreateCommandBuffer();
    void CreateSemaphore();
    void CreateRenderPass();
    void CreateFramebuffer();
    void CreateShaders();
    void CreatePipeline();
    void Draw();
    
    VkInstance m_inst;
    std::string m_appName;
    std::vector<VkPhysicalDevice> m_physDevices;
    uint m_gfxDevIndex;
    uint m_gfxQueueFamily;
    VkDevice m_device;
    std::vector<std::string> m_devExt;
    VkSurfaceKHR m_surface;
    VkFormat m_surfaceFormat;
    VulkanWindowControl* m_pWindowControl;
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;
    VkSwapchainKHR m_swapChainKHR;
    VkQueue m_queue;
    VkSemaphore m_imageAvailSem;
    VkSemaphore m_rendCompSem;
    std::vector<VkCommandBuffer> m_presentQCmdBuffs;
    VkCommandPool m_presentQCmdPool;
    VkRenderPass m_renderPass;
    VkFramebuffer m_fb;
    VkShaderModule m_vsModule;
    VkShaderModule m_fsModule;
    VkPipeline m_pipeline;
};


OgldevVulkanApp::OgldevVulkanApp(const char* pAppName)
{
    m_appName = std::string(pAppName);
    m_gfxDevIndex = -1;
}


OgldevVulkanApp::~OgldevVulkanApp()
{
    
}

void OgldevVulkanApp::EnumDevices()
{
    uint NumDevices = 0;
    
    VkResult res = vkEnumeratePhysicalDevices(m_inst, &NumDevices, NULL);
    
    if (res != VK_SUCCESS) {
        OGLDEV_ERROR("vkEnumeratePhysicalDevices error");
    }
    
    printf("Num physical devices %d\n", NumDevices);
    
    m_physDevices.resize(NumDevices);
    
    res = vkEnumeratePhysicalDevices(m_inst, &NumDevices, &m_physDevices[0]);
    
    if (res != VK_SUCCESS) {
        OGLDEV_ERROR("vkEnumeratePhysicalDevices");
    }
}

void OgldevVulkanApp::EnumPhysDeviceProps()
{
    VkPhysicalDeviceProperties DeviceProperties;
    
    printf("%d\n", m_physDevices.size());
    for (uint i = 0 ; i < m_physDevices.size() ; i++) {
        memset(&DeviceProperties, 0, sizeof(DeviceProperties));
        vkGetPhysicalDeviceProperties(m_physDevices[i], &DeviceProperties);
        
        printf("Device name: %s\n", DeviceProperties.deviceName);
        uint32_t apiVer = DeviceProperties.apiVersion;
        printf("API version: %d.%d.%d\n", VK_VERSION_MAJOR(apiVer),
                                          VK_VERSION_MINOR(apiVer),
                                          VK_VERSION_PATCH(apiVer));
        uint NumQFamily = 0;         
        vkGetPhysicalDeviceQueueFamilyProperties(m_physDevices[i], &NumQFamily, NULL);
        printf("Num of family queues: %d\n", NumQFamily);
        
        std::vector<VkQueueFamilyProperties> QFamilyProp(NumQFamily);
        vkGetPhysicalDeviceQueueFamilyProperties(m_physDevices[i], &NumQFamily, &QFamilyProp[0]);
        
        for (uint j = 0 ; j < NumQFamily ; j++) {
            printf("Family %d Num queues: %d\n", j, QFamilyProp[j].queueCount);
            VkQueueFlags flags = QFamilyProp[j].queueFlags;
            printf("    GFX %s, Compute %s, Transfer %s, Sparse binding %s\n",
                    (flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
                    (flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
                    (flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
                    (flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No");
            
            if ((flags & VK_QUEUE_GRAPHICS_BIT) && (m_gfxDevIndex == -1)) {
                m_gfxDevIndex = i;
                m_gfxQueueFamily = j;
                printf("Using GFX device %d and queue family %d\n", m_gfxDevIndex, m_gfxQueueFamily);
            }
        }
    }
    
    if (m_gfxDevIndex == -1) {
        printf("No GFX device found!\n");
        assert(0);
    }    
}
    
void OgldevVulkanApp::EnumPhysDeviceExtProps()
{
    uint NumExt = 0;
    
    VkPhysicalDevice& gfxPhysDev = m_physDevices[m_gfxDevIndex];
    
    VkResult res = vkEnumerateDeviceExtensionProperties(gfxPhysDev, NULL, &NumExt, NULL);
    
    if (res != VK_SUCCESS) {
        printf("Error enumerating device extensions %x\n", res);
        assert(0);
    }
    
    std::vector<VkExtensionProperties> ExtProps(NumExt);

    res = vkEnumerateDeviceExtensionProperties(gfxPhysDev, NULL, &NumExt, &ExtProps[0]);
    
    if (res != VK_SUCCESS) {
        printf("Error enumerating extensions");
        assert(0);
    }
        
    for (uint i = 0 ; i < NumExt ; i++) {
        printf("Device extension %d - %s\n", i, ExtProps[i].extensionName);
        m_devExt.push_back(std::string(ExtProps[i].extensionName));
    }            
}


void OgldevVulkanApp::CreateInstance()
{
    VkApplicationInfo appInfo = {};       
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_appName.c_str();
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* pInstExt[] = {
        "VK_EXT_debug_report",
        VK_KHR_SURFACE_EXTENSION_NAME,
    #ifdef _WIN32    
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    #else    
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    #endif            
    };
    
    const char* pInstLayers[] = {
        "VK_LAYER_LUNARG_standard_validation"
    };
    //std::vector enabledInstanceExtensions;
    
    VkInstanceCreateInfo instInfo = {};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = ARRAY_SIZE_IN_ELEMENTS(pInstLayers);
    instInfo.ppEnabledLayerNames = pInstLayers;
    instInfo.enabledExtensionCount = ARRAY_SIZE_IN_ELEMENTS(pInstExt);
    instInfo.ppEnabledExtensionNames = pInstExt;         

    VkResult res = vkCreateInstance(&instInfo, NULL, &m_inst);
    
    if (res != VK_SUCCESS) {
        OGLDEV_ERROR("Failed to create instance");
        assert(0);
    }    
    
    my_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_inst, "vkCreateDebugReportCallbackEXT"));
    
    /* Setup callback creation information */
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext       = NULL;
    callbackCreateInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                     VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                     VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = &MyDebugReportCallback;
    callbackCreateInfo.pUserData   = NULL;

    /* Register the callback */
    VkDebugReportCallbackEXT callback;
    res = my_vkCreateDebugReportCallbackEXT(m_inst, &callbackCreateInfo, NULL, &callback);
    CheckVulkanError("my_vkCreateDebugReportCallbackEXT failed");
  //  vkDebugReportMessageEXT = reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(m_inst, "vkDebugReportMessageEXT"));
   // vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(m_inst, "vkDestroyDebugReportCallbackEXT"));
}


void OgldevVulkanApp::CreateDevice()
{
    VkDeviceQueueCreateInfo qInfo = {};
    qInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    
    float qPriorities = 1.0f;
    qInfo.queueCount = 1;
    qInfo.pQueuePriorities = &qPriorities;
    qInfo.queueFamilyIndex = m_gfxQueueFamily;

    const char* pDevExt[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    const char* pDevLayers[] = {
        "VK_LAYER_LUNARG_standard_validation"
    };
    
    VkDeviceCreateInfo devInfo = {};
    devInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    devInfo.enabledLayerCount = ARRAY_SIZE_IN_ELEMENTS(pDevLayers);
    devInfo.ppEnabledLayerNames = pDevLayers;
    devInfo.enabledExtensionCount = ARRAY_SIZE_IN_ELEMENTS(pDevExt);
    devInfo.ppEnabledExtensionNames = pDevExt;
    devInfo.queueCreateInfoCount = 1;
    devInfo.pQueueCreateInfos = &qInfo;
    
    printf("%d\n",devInfo.enabledExtensionCount );
    
    VkResult res = vkCreateDevice(m_physDevices[m_gfxDevIndex], &devInfo, NULL, &m_device);
    
    if (res != VK_SUCCESS) {
        printf("Error creating device\n");
        assert(0);
    }
   
    printf("Device created\n");
    
    vkGetDeviceQueue(m_device, m_gfxQueueFamily, 0, &m_queue);
}


void OgldevVulkanApp::CreateSurface()
{
#ifdef WIN32
    assert(0);
#else
    m_surface = m_pWindowControl->CreateSurface(m_inst);
    assert(m_surface);          
#endif
    printf("Surface created\n");
    
    VkPhysicalDevice& gfxPhysDev = m_physDevices[m_gfxDevIndex];
    
    uint NumFormats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gfxPhysDev, m_surface, &NumFormats, NULL);
    std::vector<VkSurfaceFormatKHR> SurfaceFormats(NumFormats);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gfxPhysDev, m_surface, &NumFormats, &(SurfaceFormats[0]));
    
    for (uint i = 0 ; i < NumFormats ; i++) {
        printf("Format %d color space %d\n", SurfaceFormats[i].format, SurfaceFormats[i].colorSpace);
    }
    
    assert(NumFormats > 0);
    
    m_surfaceFormat = SurfaceFormats[0].format;
    assert(m_surfaceFormat != VK_FORMAT_UNDEFINED);
    
    VkBool32 SupportsPresent = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(gfxPhysDev, m_gfxQueueFamily, m_surface, &SupportsPresent);
    
    if (!SupportsPresent) {
        printf("Present is not supported\n");
        exit(0);
    }
    
    VkSurfaceCapabilitiesKHR SurfaceCaps;
    
    VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfxPhysDev, m_surface, &SurfaceCaps);
    
    if (res != VK_SUCCESS) {
        printf("Error getting surface caps\n");
        assert(0);
    }
    
    VulkanPrintImageUsageFlags(SurfaceCaps.supportedUsageFlags);

    uint NumPresentModes = 0;
    
    res = vkGetPhysicalDeviceSurfacePresentModesKHR(gfxPhysDev, m_surface, &NumPresentModes, NULL);
    
    assert(NumPresentModes != 0);
            
    printf("Number of presentation modes %d\n", NumPresentModes);
    
    VkExtent2D SwapChainExtent = SurfaceCaps.currentExtent;
    
    if (SurfaceCaps.currentExtent.width == -1) {
        SwapChainExtent.width = WINDOW_WIDTH;
        SwapChainExtent.height = WINDOW_HEIGHT;
    }
    else {
        SwapChainExtent = SurfaceCaps.currentExtent;
    }
    
    printf("Swap chain extent: width %d height %d\n", SwapChainExtent.width, SwapChainExtent.height);
    
    uint NumImages = SurfaceCaps.minImageCount + 1;
    
    if ((SurfaceCaps.maxImageCount > 0) &&
        (NumImages > SurfaceCaps.maxImageCount)) {
        NumImages =  SurfaceCaps.maxImageCount;
    }
    
    printf("Num images: %d\n", NumImages);
    
    VkSurfaceTransformFlagBitsKHR preTransform;
    
    if (SurfaceCaps.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else {
        preTransform = SurfaceCaps.currentTransform;
    }
    
    VkSwapchainCreateInfoKHR SwapChainCreateInfo = {};
    
    SwapChainCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapChainCreateInfo.surface          = m_surface;
    SwapChainCreateInfo.minImageCount    = NumImages;
    SwapChainCreateInfo.imageFormat      = m_surfaceFormat;
    SwapChainCreateInfo.imageColorSpace  = SurfaceFormats[0].colorSpace;
    SwapChainCreateInfo.imageExtent      = SwapChainExtent;
    SwapChainCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapChainCreateInfo.preTransform     = preTransform;
    SwapChainCreateInfo.imageArrayLayers = 1;
    SwapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapChainCreateInfo.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    SwapChainCreateInfo.clipped          = true;
    SwapChainCreateInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    
    res = vkCreateSwapchainKHR(m_device, &SwapChainCreateInfo, NULL, &m_swapChainKHR);
    
    if (res != VK_SUCCESS) {
        printf("Error creating swap chain\n");
        assert(0);
    }
    
    printf("Swap chain created\n");
    
    uint NumSwapChainImages = 0;
    res = vkGetSwapchainImagesKHR(m_device, m_swapChainKHR, &NumSwapChainImages, NULL);

    if (res != VK_SUCCESS) {
        printf("Error getting number of images\n");
        assert(0);
    }
    
    printf("Number of images %d\n", NumSwapChainImages);

    m_images.resize(NumSwapChainImages);
    m_views.resize(NumSwapChainImages);
    m_presentQCmdBuffs.resize(NumSwapChainImages);
    
    res = vkGetSwapchainImagesKHR(m_device, m_swapChainKHR, &NumSwapChainImages, &(m_images[0]));

    if (res != VK_SUCCESS) {
        printf("Error getting images\n");
        assert(0);
    }    
}


void OgldevVulkanApp::CreateCommandBuffer()
{
    VkCommandPoolCreateInfo cmdPoolCreateInfo = {};
    cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolCreateInfo.queueFamilyIndex = m_gfxQueueFamily;
    
    VkResult res = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, NULL, &m_presentQCmdPool);
    
    CheckVulkanError("vkCreateCommandPool");
    
    printf("Command buffer pool created\n");
    
    VkCommandBufferAllocateInfo cmdBufAllocInfo = {};
    cmdBufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufAllocInfo.commandPool = m_presentQCmdPool;
    cmdBufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufAllocInfo.commandBufferCount = m_images.size();
    
    res = vkAllocateCommandBuffers(m_device, &cmdBufAllocInfo, &m_presentQCmdBuffs[0]);
            
    CheckVulkanError("vkAllocateCommandBuffers failed");
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    
    VkClearColorValue clearColor = { 1.0f, 0.0f, 0.0f, 0.0f };
    
    VkImageSubresourceRange imageRange = {};
    imageRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageRange.levelCount = 1;
    imageRange.layerCount = 1;
    
    for (uint i = 0 ; i < m_presentQCmdBuffs.size() ; i++) {
        VkImageMemoryBarrier memBarrier_PresentToClear = {};
        memBarrier_PresentToClear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memBarrier_PresentToClear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        memBarrier_PresentToClear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarrier_PresentToClear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        memBarrier_PresentToClear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memBarrier_PresentToClear.srcQueueFamilyIndex = m_gfxQueueFamily;
        memBarrier_PresentToClear.dstQueueFamilyIndex = m_gfxQueueFamily;
        memBarrier_PresentToClear.image = m_images[i];
        memBarrier_PresentToClear.subresourceRange = imageRange;        
        
        VkImageMemoryBarrier memBarrier_ClearToPresent = {};
        memBarrier_PresentToClear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memBarrier_PresentToClear.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        memBarrier_PresentToClear.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;               
        memBarrier_PresentToClear.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        memBarrier_PresentToClear.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                
        memBarrier_PresentToClear.srcQueueFamilyIndex = m_gfxQueueFamily;
        memBarrier_PresentToClear.dstQueueFamilyIndex = m_gfxQueueFamily;
        memBarrier_PresentToClear.image = m_images[i];
        memBarrier_PresentToClear.subresourceRange = imageRange;        
        
        res = vkBeginCommandBuffer(m_presentQCmdBuffs[i], &beginInfo);
        CheckVulkanError("vkBeginCommandBuffer failed\n");
        
        vkCmdPipelineBarrier(m_presentQCmdBuffs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, NULL, 0, NULL, 1, &memBarrier_PresentToClear);
        
        vkCmdClearColorImage(m_presentQCmdBuffs[i], m_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor,
                             1, &imageRange);
        
        vkCmdPipelineBarrier(m_presentQCmdBuffs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             0, 0, NULL, 0, NULL, 1, &memBarrier_ClearToPresent);

        res = vkEndCommandBuffer(m_presentQCmdBuffs[i]);
        CheckVulkanError("vkEndCommandBuffer failed\n");        
    }
    
    printf("Command buffers recorded\n");
}

void OgldevVulkanApp::CreateSemaphore()
{
    VkSemaphoreCreateInfo semCreateInfo = {};
    semCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkResult res = vkCreateSemaphore(m_device, &semCreateInfo, NULL, &m_imageAvailSem);
    
    if (res != VK_SUCCESS) {
        printf("Error creating semaphore\n");
        assert(0);
    }

    res = vkCreateSemaphore(m_device, &semCreateInfo, NULL, &m_rendCompSem);
    
    if (res != VK_SUCCESS) {
        printf("Error creating semaphore\n");
        assert(0);
    }
    
    printf("Semaphores created\n");    
}


void OgldevVulkanApp::CreateRenderPass()
{
    VkAttachmentDescription attachDesc = {};
    attachDesc.format = m_surfaceFormat;
    attachDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachDesc.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachRef = {};
    attachRef.attachment = 0;
    attachRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &attachRef;

    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &attachDesc;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDesc;

    VkResult res = vkCreateRenderPass(m_device, &renderPassCreateInfo, NULL, &m_renderPass);
    CheckVulkanError("vkCreateRenderPass failed");

    printf("Created a render pass\n");
}


void OgldevVulkanApp::CreateFramebuffer()
{
    VkResult res;
            
    for (uint i = 0 ; i < m_images.size() ; i++) {
        VkImageViewCreateInfo ViewCreateInfo = {};
        ViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ViewCreateInfo.image = m_images[i];
        ViewCreateInfo.format = m_surfaceFormat;
        ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        ViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewCreateInfo.subresourceRange.baseMipLevel = 0;
        ViewCreateInfo.subresourceRange.levelCount = 1;
        ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        ViewCreateInfo.subresourceRange.layerCount = 1;    

        res = vkCreateImageView(m_device, &ViewCreateInfo, NULL, &m_views[i]);
        CheckVulkanError("vkCreateImageView failed\n");
    }

    VkFramebufferCreateInfo fbCreateInfo = {};
    fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbCreateInfo.renderPass = m_renderPass;
    fbCreateInfo.attachmentCount = m_views.size();
    fbCreateInfo.pAttachments = &m_views[0];
    fbCreateInfo.width = WINDOW_WIDTH;
    fbCreateInfo.height = WINDOW_HEIGHT;
    fbCreateInfo.layers = 1;
    
    res = vkCreateFramebuffer(m_device, &fbCreateInfo, NULL, &m_fb);
    CheckVulkanError("vkCreateFramebuffer failed\n");
    
    printf("Framebuffer created\n");
}

void OgldevVulkanApp::Draw()
{
    uint ImageIndex = 0;
    
    VkResult res = vkAcquireNextImageKHR(m_device, m_swapChainKHR, UINT64_MAX, m_imageAvailSem, NULL, &ImageIndex);
    
    switch (res) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            assert(0);
        default:
            assert(0);
    }
    
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &m_imageAvailSem;
    submitInfo.pWaitDstStageMask    = &stageFlags;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &m_presentQCmdBuffs[ImageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_rendCompSem;
    
    res = vkQueueSubmit(m_queue, 1, &submitInfo, NULL);
    
    CheckVulkanError("vkQueueSubmit failed");
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &m_rendCompSem;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &m_swapChainKHR;
    presentInfo.pImageIndices      = &ImageIndex;
    
    res = vkQueuePresentKHR(m_queue, &presentInfo);
    
    CheckVulkanError("vkQueuePresentKHR failed");
}



void OgldevVulkanApp::CreateShaders()
{
    m_vsModule = VulkanCreateShaderModule(m_device, "Shaders/vs.spv");
    assert(m_vsModule);

    m_fsModule = VulkanCreateShaderModule(m_device, "Shaders/fs.spv");
    assert(m_fsModule);
    }


void OgldevVulkanApp::CreatePipeline()
{
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};
    
    shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStageCreateInfo[0].module = m_vsModule;
    shaderStageCreateInfo[0].pName = "main";
    shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStageCreateInfo[1].module = m_fsModule;
    shaderStageCreateInfo[1].pName = "main";       

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkPipelineInputAssemblyStateCreateInfo pipelineIACreateInfo = {};
    pipelineIACreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineIACreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkViewport vp = {};
    vp.x = 0.0f;
    vp.y = 0.0f;
    vp.width  = (float)WINDOW_WIDTH;
    vp.height = (float)WINDOW_HEIGHT;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width  = WINDOW_WIDTH;
    scissor.extent.height = WINDOW_HEIGHT;
    
    VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE] = {};
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables;

    VkPipelineViewportStateCreateInfo vpCreateInfo = {};
    vpCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpCreateInfo.viewportCount = 1;
    vpCreateInfo.pViewports = &vp;
    vpCreateInfo.scissorCount = 1;
    vpCreateInfo.pScissors = &scissor;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    
    VkPipelineDepthStencilStateCreateInfo dsInfo = {};
    dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsInfo.depthTestEnable = VK_TRUE;
    dsInfo.depthWriteEnable = VK_TRUE;
    dsInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    dsInfo.back.failOp = VK_STENCIL_OP_KEEP;
    dsInfo.back.passOp = VK_STENCIL_OP_KEEP;
    dsInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
    dsInfo.front = dsInfo.back;
    
    VkPipelineRasterizationStateCreateInfo rastCreateInfo = {};
    rastCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rastCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rastCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rastCreateInfo.lineWidth = 1.0f;
    
    VkPipelineMultisampleStateCreateInfo pipelineMSCreateInfo = {};
    pipelineMSCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMSCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    //pipelineMSCreateInfo.minSampleShading = 1.0f;
    
    VkPipelineColorBlendAttachmentState blendAttachState = {};
    /*blendAttachState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachState.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                      VK_COLOR_COMPONENT_G_BIT | 
                                      VK_COLOR_COMPONENT_B_BIT | 
                                      VK_COLOR_COMPONENT_A_BIT;*/
    blendAttachState.colorWriteMask = 0xf;
    
    VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
    blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    blendCreateInfo.attachmentCount = 1;
    blendCreateInfo.pAttachments = &blendAttachState;
    
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    
    VkPipelineLayout pipelineLayout;
    VkResult res = vkCreatePipelineLayout(m_device, &layoutInfo, NULL, &pipelineLayout);
    CheckVulkanError("vkCreatePipelineLayout failed");
        
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = ARRAY_SIZE_IN_ELEMENTS(shaderStageCreateInfo);
    pipelineInfo.pStages = &shaderStageCreateInfo[0];
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &pipelineIACreateInfo;
    pipelineInfo.pViewportState = &vpCreateInfo;
    pipelineInfo.pDepthStencilState = &dsInfo;
    pipelineInfo.pRasterizationState = &rastCreateInfo;
    pipelineInfo.pMultisampleState = &pipelineMSCreateInfo;
    pipelineInfo.pColorBlendState = &blendCreateInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.basePipelineIndex = -1;
    
    res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &m_pipeline);
    CheckVulkanError("vkCreateGraphicsPipelines failed");
    
    printf("Graphics pipeline created\n");
}


bool OgldevVulkanApp::Init()
{
    std::vector<VkExtensionProperties> ExtProps;
    VulkanEnumExtProps(ExtProps);
#ifdef WIN32
    
#else            
    m_pWindowControl = new XCBControl();
#endif    
    bool ret = m_pWindowControl->Init(WINDOW_WIDTH, WINDOW_HEIGHT);
    assert(ret);
    CreateInstance();
    EnumDevices();
    EnumPhysDeviceProps();
    EnumPhysDeviceExtProps();
    CreateDevice();
    CreateSurface();
    CreateSemaphore();
    CreateCommandBuffer();
    CreateRenderPass();
    CreateFramebuffer();
    CreateShaders();
    CreatePipeline();

    return true;
}


void OgldevVulkanApp::Run()
{
    m_pWindowControl->PreRun();

    while (true) {
        m_pWindowControl->PollEvent();
      //  if (event) {
       //     demo_handle_event(demo, event);
      //      free(event);
      //  }

        Draw();

        // Wait for work to finish before updating MVP.
        vkDeviceWaitIdle(m_device);
       // demo->curFrame++;
        //if (demo->frameCount != INT32_MAX && demo->curFrame == demo->frameCount)
        //    demo->quit = true;
    }
}

class Tutorial50 : public OgldevVulkanApp
{
public:
    Tutorial50(const char* pAppName);
    
    ~Tutorial50();
};


Tutorial50::Tutorial50(const char* pAppName) : OgldevVulkanApp(pAppName)
{
    
}

Tutorial50::~Tutorial50()
{
    
}

int main(int argc, char** argv)
{
    Tutorial50 app("Tutorial 50");
    
    if (!app.Init()) {
        return 1;
    }      
    
    app.Run();
    
    return 0;
}

