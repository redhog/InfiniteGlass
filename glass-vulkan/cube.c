#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <X11/Xutil.h>


#include <vulkan/vk_sdk_platform.h>
#include "linmath.h"
#include "object_type_string_helper.h"

#include "gettime.h"
#include "inttypes.h"
#define MILLION 1000000L
#define BILLION 1000000000L

#define DEMO_TEXTURE_COUNT 1
#define APP_SHORT_NAME "vkcube"
#define APP_LONG_NAME "Vulkan Cube"

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#define ERR_EXIT(err_msg, err_class) \
	do { \
		printf("%s\n", err_msg); \
		fflush(stdout); \
		exit(1); \
	} while (0)
void DbgMsg(char *fmt, ...) {
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
	fflush(stdout);
}

#define GET_INSTANCE_PROC_ADDR(inst, entrypoint) { \
	wm->fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr( \
			inst, "vk" #entrypoint); \
	if(wm->fp##entrypoint == NULL) { \
		ERR_EXIT("vkGetInstanceProcAddr failed to find vk" \
			#entrypoint, "vkGetInstanceProcAddr Failure"); \
	} \
}

static PFN_vkGetDeviceProcAddr g_gdpa = NULL;

#define GET_DEVICE_PROC_ADDR(dev, entrypoint) { \
	if(!g_gdpa) g_gdpa = (PFN_vkGetDeviceProcAddr)vkGetInstanceProcAddr( \
			wm->inst, "vkGetDeviceProcAddr"); \
	wm->fp##entrypoint = (PFN_vk##entrypoint)g_gdpa( \
			dev, "vk" #entrypoint); \
	if(wm->fp##entrypoint == NULL) { \
		ERR_EXIT("vkGetDeviceProcAddr failed to find vk" \
			#entrypoint, "vkGetDeviceProcAddr Failure"); \
	} \
}

/*
 * structure to track all objects related to a texture.
 */
struct texture_object {
	VkSampler sampler;

	VkImage image;
	VkBuffer buffer;
	VkImageLayout imageLayout;

	VkMemoryAllocateInfo mem_alloc;
	VkDeviceMemory mem;
	VkImageView view;
	int32_t tex_width, tex_height;
};

static char *tex_files[] = {"lunarg.ppm"};

static int validation_error = 0;

struct vktexcube_vs_uniform {
	// Must start with MVP
	float mvp[4][4];
	float position[12 * 3][4];
	float attr[12 * 3][4];
};

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
// clang-format off
static const float g_vertex_buffer_data[] = {
    -1.0f,-1.0f,-1.0f,  // -X side
    -1.0f,-1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Z side
     1.0f, 1.0f,-1.0f,
     1.0f,-1.0f,-1.0f,
    -1.0f,-1.0f,-1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f,-1.0f,-1.0f,  // -Y side
     1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f,-1.0f,
     1.0f,-1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,

    -1.0f, 1.0f,-1.0f,  // +Y side
    -1.0f, 1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f, 1.0f,-1.0f,
     1.0f, 1.0f, 1.0f,
     1.0f, 1.0f,-1.0f,

     1.0f, 1.0f,-1.0f,  // +X side
     1.0f, 1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f,-1.0f,-1.0f,
     1.0f, 1.0f,-1.0f,

    -1.0f, 1.0f, 1.0f,  // +Z side
    -1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
    -1.0f,-1.0f, 1.0f,
     1.0f,-1.0f, 1.0f,
     1.0f, 1.0f, 1.0f,
};

static const float g_uv_buffer_data[] = {
    0.0f, 1.0f,  // -X side
    1.0f, 1.0f,
    1.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,
    0.0f, 1.0f,

    1.0f, 1.0f,  // -Z side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // -Y side
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,

    1.0f, 0.0f,  // +Y side
    0.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,

    1.0f, 0.0f,  // +X side
    0.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,

    0.0f, 0.0f,  // +Z side
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, 0.0f,
};
// clang-format on

void dumpMatrix(const char *note, mat4x4 MVP) {
    int i;

    printf("%s: \n", note);
    for (i = 0; i < 4; i++) {
        printf("%f, %f, %f, %f\n", MVP[i][0], MVP[i][1], MVP[i][2], MVP[i][3]);
    }
    printf("\n");
    fflush(stdout);
}

void dumpVec4(const char *note, vec4 vector) {
    printf("%s: \n", note);
    printf("%f, %f, %f, %f\n", vector[0], vector[1], vector[2], vector[3]);
    printf("\n");
    fflush(stdout);
}

typedef struct {
    VkImage image;
    VkCommandBuffer cmd;
    VkCommandBuffer graphics_to_present_cmd;
    VkImageView view;
    VkBuffer uniform_buffer;
    VkDeviceMemory uniform_memory;
    void *uniform_memory_ptr;
    VkFramebuffer framebuffer;
    VkDescriptorSet descriptor_set;
} SwapchainImageResources;

struct Wm {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
#define APP_NAME_STR_LEN 80
    HINSTANCE connection;         // hInstance - Windows Instance
    char name[APP_NAME_STR_LEN];  // Name to put on the window/icon
    HWND window;                  // hWnd - window handle
    POINT minsize;                // minimum window size
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    Display *display;
    Window xlib_window;
    Atom xlib_wm_delete_window;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    Display *display;
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t xcb_window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_surface *window;
    struct wl_shell *shell;
    struct wl_shell_surface *shell_surface;
    struct wl_seat *seat;
    struct wl_pointer *pointer;
    struct wl_keyboard *keyboard;
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    struct ANativeWindow *window;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    void *caMetalLayer;
#endif
    VkSurfaceKHR surface;
    bool prepared;
    bool use_staging_buffer;
    bool separate_present_queue;
    bool is_minimized;

    bool VK_KHR_incremental_present_enabled;

    bool VK_GOOGLE_display_timing_enabled;
    bool syncd_with_actual_presents;
    uint64_t refresh_duration;
    uint64_t refresh_duration_multiplier;
    uint64_t target_IPD;  // image present duration (inverse of frame rate)
    uint64_t prev_desired_present_time;
    uint32_t next_present_id;
    uint32_t last_early_id;  // 0 if no early images
    uint32_t last_late_id;   // 0 if no late images

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue graphics_queue;
    VkQueue present_queue;
    uint32_t graphics_queue_family_index;
    uint32_t present_queue_family_index;
    VkSemaphore image_acquired_semaphores[FRAME_LAG];
    VkSemaphore draw_complete_semaphores[FRAME_LAG];
    VkSemaphore image_ownership_semaphores[FRAME_LAG];
    VkPhysicalDeviceProperties gpu_props;
    VkQueueFamilyProperties *queue_props;
    VkPhysicalDeviceMemoryProperties memory_properties;

    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    char *extension_names[64];
    char *enabled_layers[64];

    int width, height;
    VkFormat format;
    VkColorSpaceKHR color_space;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR fpQueuePresentKHR;
    PFN_vkGetRefreshCycleDurationGOOGLE fpGetRefreshCycleDurationGOOGLE;
    PFN_vkGetPastPresentationTimingGOOGLE fpGetPastPresentationTimingGOOGLE;
    uint32_t swapchainImageCount;
    VkSwapchainKHR swapchain;
    SwapchainImageResources *swapchain_image_resources;
    VkPresentModeKHR presentMode;
    VkFence fences[FRAME_LAG];
    int frame_index;

    VkCommandPool cmd_pool;
    VkCommandPool present_cmd_pool;

    struct {
        VkFormat format;

        VkImage image;
        VkMemoryAllocateInfo mem_alloc;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];
    struct texture_object staging_texture;

    VkCommandBuffer cmd;  // Buffer for initialization commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    mat4x4 projection_matrix;
    mat4x4 view_matrix;
    mat4x4 model_matrix;

    float spin_angle;
    float spin_increment;
    bool pause;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;

    bool quit;
    int32_t curFrame;
    int32_t frameCount;
    bool validate;
    bool validate_checks_disabled;
    bool use_break;
    bool suppress_popups;

    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
    PFN_vkSubmitDebugUtilsMessageEXT SubmitDebugUtilsMessageEXT;
    PFN_vkCmdBeginDebugUtilsLabelEXT CmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT CmdEndDebugUtilsLabelEXT;
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabelEXT;
    PFN_vkSetDebugUtilsObjectNameEXT SetDebugUtilsObjectNameEXT;
    VkDebugUtilsMessengerEXT dbg_messenger;

    uint32_t current_buffer;
    uint32_t queue_family_count;
};

VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData) {
	char prefix[64] = "";
	char *message = (char *)malloc(strlen(pCallbackData->pMessage) + 5000);
	assert(message);
	struct Wm *wm = (struct Wm *)pUserData;

	if(wm->use_break) {
		raise(SIGTRAP);
	}

	if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		strcat(prefix, "VERBOSE : ");
	} else if(messageSeverity &
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
		strcat(prefix, "INFO : ");
	} else if (messageSeverity &
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		strcat(prefix, "WARNING : ");
	} else if (messageSeverity &
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		strcat(prefix, "ERROR : ");
	}

	if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		strcat(prefix, "GENERAL");
	} else {
		if(messageType &
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
			strcat(prefix, "VALIDATION");
			validation_error = 1;
		}
		if(messageType &
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
			if(messageType &
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
				strcat(prefix, "|");
			}
			strcat(prefix, "PERFORMANCE");
		}
	}

	sprintf(message,
		"%s - Message Id Number: %d | Message Id Name: %s\n\t%s\n",
			prefix, pCallbackData->messageIdNumber,
			pCallbackData->pMessageIdName, pCallbackData->pMessage);
	if(pCallbackData->objectCount > 0) {
		char tmp_message[500];
		sprintf(tmp_message, "\n\tObjects - %d\n",
				pCallbackData->objectCount);
		strcat(message, tmp_message);
		for(uint32_t object = 0; object < pCallbackData->objectCount;
				++object) {
			if(NULL != pCallbackData->pObjects[object].pObjectName
		&& strlen(pCallbackData->pObjects[object].pObjectName) > 0) {
				sprintf(tmp_message,
			"\t\tObject[%d] - %s, Handle %p, Name \"%s\"\n", object,
				string_VkObjectType(pCallbackData->pObjects[
							object].objectType),
				(void *)(pCallbackData->pObjects[
					object].objectHandle),
				pCallbackData->pObjects[object].pObjectName);
			} else {
				sprintf(tmp_message,
				"\t\tObject[%d] - %s, Handle %p\n", object,
				string_VkObjectType(pCallbackData->pObjects[
					object].objectType),
				(void *)(pCallbackData->pObjects[
						object].objectHandle));
			}
			strcat(message, tmp_message);
		}
	}
	if(pCallbackData->cmdBufLabelCount > 0) {
		char tmp_message[500];
		sprintf(tmp_message, "\n\tCommand Buffer Labels - %d\n",
				pCallbackData->cmdBufLabelCount);
		strcat(message, tmp_message);
		for(uint32_t cmd_buf_label = 0; cmd_buf_label <
			pCallbackData->cmdBufLabelCount; ++cmd_buf_label) {
			sprintf(tmp_message,
				"\t\tLabel[%d] - %s { %f, %f, %f, %f}\n",
				cmd_buf_label,
			pCallbackData->pCmdBufLabels[cmd_buf_label].pLabelName,
			pCallbackData->pCmdBufLabels[cmd_buf_label].color[0],
			pCallbackData->pCmdBufLabels[cmd_buf_label].color[1],
			pCallbackData->pCmdBufLabels[cmd_buf_label].color[2],
			pCallbackData->pCmdBufLabels[cmd_buf_label].color[3]);
			strcat(message, tmp_message);
		}
	}

	printf("%s\n", message);
	fflush(stdout);

	free(message);

	// Don't bail out, but keep going.
	return false;
}

bool ActualTimeLate(uint64_t desired, uint64_t actual, uint64_t rdur) {
    // The desired time was the earliest time that the present should have
    // occured.  In almost every case, the actual time should be later than the
    // desired time.  We should only consider the actual time "late" if it is
    // after "desired + rdur".
	if(actual <= desired) {
	// The actual time was before or equal to the desired time.  This will
	// probably never happen, but in case it does, return false since the
	// present was obviously NOT late.
		return false;
	}
	uint64_t deadline = desired + rdur;
	if(actual > deadline) {
		return true;
	} else {
		return false;
	}
}

bool CanPresentEarlier(uint64_t earliest, uint64_t actual, uint64_t margin,
		uint64_t rdur) {
	if(earliest < actual) {
        // Consider whether this present could have occured earlier.  Make sure
        // that earliest time was at least 2msec earlier than actual time, and
        // that the margin was at least 2msec:
		uint64_t diff = actual - earliest;
		if((diff >= (2 * MILLION)) && (margin >= (2 * MILLION))) {
            // This present could have occured earlier because both: 1) the
            // earliest time was at least 2 msec before actual time, and 2) the
            // margin was at least 2msec.
			return true;
		}
	}
	return false;
}

// Forward declarations:
static void wm_resize(struct Wm *wm);
static void wm_create_surface(struct Wm *wm);

static bool memory_type_from_properties(struct Wm *wm, uint32_t typeBits,
		VkFlags requirements_mask, uint32_t *typeIndex) {
	// Search memtypes to find first index with those properties
	for(uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
		if((typeBits & 1) == 1) {
			// Type is available, does it match user properties?
			if((wm->memory_properties.memoryTypes[i]
					.propertyFlags & requirements_mask) ==
					requirements_mask) {
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	// No memory types matched, return failure
	return false;
}

static void wm_flush_init_cmd(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;

    // This function could get called twice if the texture uses a staging buffer
    // In that case the second call should be ignored
	if(wm->cmd == VK_NULL_HANDLE) return;

	err = vkEndCommandBuffer(wm->cmd);
	assert(!err);

	VkFence fence;
	VkFenceCreateInfo fence_ci = {.sType =
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = NULL, .flags = 0};
	err = vkCreateFence(wm->device, &fence_ci, NULL, &fence);
	assert(!err);

	const VkCommandBuffer cmd_bufs[] = {wm->cmd};
	VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = NULL,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = NULL,
			.pWaitDstStageMask = NULL,
			.commandBufferCount = 1,
			.pCommandBuffers = cmd_bufs,
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = NULL};

	err = vkQueueSubmit(wm->graphics_queue, 1, &submit_info, fence);
	assert(!err);

	err = vkWaitForFences(wm->device, 1, &fence, VK_TRUE, UINT64_MAX);
	assert(!err);

	vkFreeCommandBuffers(wm->device, wm->cmd_pool, 1, cmd_bufs);
	vkDestroyFence(wm->device, fence, NULL);
	wm->cmd = VK_NULL_HANDLE;
}

static void wm_set_image_layout(struct Wm *wm, VkImage image,
		VkImageAspectFlags aspectMask, VkImageLayout old_image_layout,
		VkImageLayout new_image_layout, VkAccessFlagBits srcAccessMask,
		VkPipelineStageFlags src_stages,
		VkPipelineStageFlags dest_stages) {
	assert(wm->cmd);

	VkImageMemoryBarrier image_memory_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = srcAccessMask,
			.dstAccessMask = 0,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.oldLayout = old_image_layout,
			.newLayout = new_image_layout,
			.image = image,
			.subresourceRange = {aspectMask, 0, 1, 0, 1}};

	switch(new_image_layout) {
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
	// Make sure anything that was copying from this image has completed
		image_memory_barrier.dstAccessMask =
				VK_ACCESS_TRANSFER_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask =
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		image_memory_barrier.dstAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		image_memory_barrier.dstAccessMask =
				VK_ACCESS_SHADER_READ_BIT |
				VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		image_memory_barrier.dstAccessMask =
				VK_ACCESS_TRANSFER_READ_BIT;
		break;

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		image_memory_barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		break;

	default:
		image_memory_barrier.dstAccessMask = 0;
		break;
	}

	VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

	vkCmdPipelineBarrier(wm->cmd, src_stages, dest_stages,
			0, 0, NULL, 0, NULL, 1, pmemory_barrier);
}

static void wm_draw_build_cmd(struct Wm *wm, VkCommandBuffer cmd_buf) {
	VkDebugUtilsLabelEXT label;
	memset(&label, 0, sizeof(label));
	const VkCommandBufferBeginInfo cmd_buf_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = NULL,
	};
	const VkClearValue clear_values[2] = {
		[0] = {.color.float32 = {0.2f, 0.2f, 0.2f, 0.2f}},
		[1] = {.depthStencil = {1.0f, 0}},
	};
	const VkRenderPassBeginInfo rp_begin = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = wm->render_pass,
		.framebuffer = wm->swapchain_image_resources[
				wm->current_buffer].framebuffer,
		.renderArea.offset.x = 0,
		.renderArea.offset.y = 0,
		.renderArea.extent.width = wm->width,
		.renderArea.extent.height = wm->height,
		.clearValueCount = 2,
		.pClearValues = clear_values,
	};
	VkResult U_ASSERT_ONLY err;

	err = vkBeginCommandBuffer(cmd_buf, &cmd_buf_info);

	if(wm->validate) {
		// Set a name for the command buffer
		VkDebugUtilsObjectNameInfoEXT cmd_buf_name = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.pNext = NULL,
			.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
			.objectHandle = (uint64_t)cmd_buf,
			.pObjectName = "CubeDrawCommandBuf",
		};
		wm->SetDebugUtilsObjectNameEXT(wm->device, &cmd_buf_name);

		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pNext = NULL;
		label.pLabelName = "DrawBegin";
		label.color[0] = 0.4f;
		label.color[1] = 0.3f;
		label.color[2] = 0.2f;
		label.color[3] = 0.1f;
		wm->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
	}

	assert(!err);
	vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

	if(wm->validate) {
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pNext = NULL;
		label.pLabelName = "InsideRenderPass";
		label.color[0] = 8.4f;
		label.color[1] = 7.3f;
		label.color[2] = 6.2f;
		label.color[3] = 7.1f;
		wm->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
	}

	vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			wm->pipeline);
	vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
			wm->pipeline_layout, 0, 1,
			&wm->swapchain_image_resources[
			wm->current_buffer].descriptor_set, 0, NULL);
	VkViewport viewport;
	memset(&viewport, 0, sizeof(viewport));
	float viewport_dimension;
	if(wm->width < wm->height) {
		viewport_dimension = (float)wm->width;
		viewport.y = (wm->height - wm->width) / 2.0f;
	} else {
		viewport_dimension = (float)wm->height;
		viewport.x = (wm->width - wm->height) / 2.0f;
	}
	viewport.height = viewport_dimension;
	viewport.width = viewport_dimension;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

	VkRect2D scissor;
	memset(&scissor, 0, sizeof(scissor));
	scissor.extent.width = wm->width;
	scissor.extent.height = wm->height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

	if(wm->validate) {
		label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		label.pNext = NULL;
		label.pLabelName = "ActualDraw";
		label.color[0] = -0.4f;
		label.color[1] = -0.3f;
		label.color[2] = -0.2f;
		label.color[3] = -0.1f;
		wm->CmdBeginDebugUtilsLabelEXT(cmd_buf, &label);
	}

	vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
	if(wm->validate) {
		wm->CmdEndDebugUtilsLabelEXT(cmd_buf);
	}

	// Note that ending the renderpass changes the image's layout from
	// COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
	vkCmdEndRenderPass(cmd_buf);
	if(wm->validate) {
		wm->CmdEndDebugUtilsLabelEXT(cmd_buf);
	}

	if(wm->separate_present_queue) {
       // We have to transfer ownership from the graphics queue family to the
       // present queue family to be able to present.  Note that we don't have
       // to transfer from present queue family back to graphics queue family at
       // the start of the next frame because we don't care about the image's
       // contents at that point.
		VkImageMemoryBarrier image_ownership_barrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = NULL,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = wm->graphics_queue_family_index,
			.dstQueueFamilyIndex = wm->present_queue_family_index,
			.image = wm->swapchain_image_resources[
				wm->current_buffer].image,
			.subresourceRange = {
				VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

		vkCmdPipelineBarrier(cmd_buf,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0, 0, NULL, 0, NULL, 1,
				&image_ownership_barrier);
	}
	if(wm->validate) {
		wm->CmdEndDebugUtilsLabelEXT(cmd_buf);
	}
	err = vkEndCommandBuffer(cmd_buf);
	assert(!err);
}

void wm_build_image_ownership_cmd(struct Wm *wm, int i) {
	VkResult U_ASSERT_ONLY err;

	const VkCommandBufferBeginInfo cmd_buf_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = NULL,
	};
	err = vkBeginCommandBuffer(
			wm->swapchain_image_resources[i]
			.graphics_to_present_cmd, &cmd_buf_info);
	assert(!err);

	VkImageMemoryBarrier image_ownership_barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = wm->graphics_queue_family_index,
		.dstQueueFamilyIndex = wm->present_queue_family_index,
		.image = wm->swapchain_image_resources[i].image,
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

	vkCmdPipelineBarrier(wm->swapchain_image_resources[i]
		.graphics_to_present_cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1,
		&image_ownership_barrier);
	err = vkEndCommandBuffer(wm->swapchain_image_resources[i]
			.graphics_to_present_cmd);
	assert(!err);
}

void wm_update_data_buffer(struct Wm *wm) {
	mat4x4 MVP, Model, VP;
	int matrixSize = sizeof(MVP);

	mat4x4_mul(VP, wm->projection_matrix, wm->view_matrix);

	// Rotate around the Y axis
	mat4x4_dup(Model, wm->model_matrix);
	mat4x4_rotate(wm->model_matrix, Model, 0.0f, 1.0f, 0.0f,
		(float)degreesToRadians(wm->spin_angle));
	mat4x4_mul(MVP, VP, wm->model_matrix);

	memcpy(wm->swapchain_image_resources[wm->current_buffer]
		.uniform_memory_ptr, (const void *)&MVP[0][0], matrixSize);
}

void DemoUpdateTargetIPD(struct Wm *wm) {
	// Look at what happened to previous presents, and make appropriate
	// adjustments in timing:
	VkResult U_ASSERT_ONLY err;
	VkPastPresentationTimingGOOGLE *past = NULL;
	uint32_t count = 0;

	err = wm->fpGetPastPresentationTimingGOOGLE(wm->device, wm->swapchain,
		&count, NULL);
	assert(!err);
	if(count) {
		past = (VkPastPresentationTimingGOOGLE *)malloc(
				sizeof(VkPastPresentationTimingGOOGLE) * count);
		assert(past);
		err = wm->fpGetPastPresentationTimingGOOGLE(
				wm->device, wm->swapchain, &count, past);
		assert(!err);

		bool early = false;
		bool late = false;
		bool calibrate_next = false;
		for(uint32_t i = 0; i < count; i++) {
			if(!wm->syncd_with_actual_presents) {
                // This is the first time that we've received an
                // actualPresentTime for this swapchain.  In order to not
                // perceive these early frames as "late", we need to sync-up
                // our future desiredPresentTime's with the
                // actualPresentTime(s) that we're receiving now.
				calibrate_next = true;

                // So that we don't suspect any pending presents as late,
                // record them all as suspected-late presents:
				wm->last_late_id = wm->next_present_id - 1;
				wm->last_early_id = 0;
				wm->syncd_with_actual_presents = true;
				break;
			} else if(CanPresentEarlier(past[i].earliestPresentTime,
					past[i].actualPresentTime,
					past[i].presentMargin,
					wm->refresh_duration)) {
                // This image could have been presented earlier.  We don't want
                // to decrease the target_IPD until we've seen early presents
                // for at least two seconds.
				if(wm->last_early_id == past[i].presentID) {
                    // We've now seen two seconds worth of early presents.
                    // Flag it as such, and reset the counter:
					early = true;
					wm->last_early_id = 0;
				} else if(wm->last_early_id == 0) {
                    // This is the first early present we've seen.
                    // Calculate the presentID for two seconds from now.
					uint64_t lastEarlyTime =
						past[i].actualPresentTime +
						(2 * BILLION);
						uint32_t howManyPresents =
						(uint32_t)((lastEarlyTime -
						past[i].actualPresentTime) /
						wm->target_IPD);
					wm->last_early_id = past[i].presentID +
						howManyPresents;
				} else {
                    // We are in the midst of a set of early images,
                    // and so we won't do anything.
				}
				late = false;
				wm->last_late_id = 0;
			} else if(ActualTimeLate(past[i].desiredPresentTime,
				past[i].actualPresentTime,
				wm->refresh_duration)) {
                // This image was presented after its desired time.  Since
                // there's a delay between calling vkQueuePresentKHR and when
                // we get the timing data, several presents may have been late.
                // Thus, we need to threat all of the outstanding presents as
                // being likely late, so that we only increase the target_IPD
                // once for all of those presents.
				if((wm->last_late_id == 0) ||
							(wm->last_late_id <
							past[i].presentID)) {
					late = true;
				// Record the last suspected-late present:
					wm->last_late_id =
						wm->next_present_id - 1;
				} else {
                    // We are in the midst of a set of likely-late images,
                    // and so we won't do anything.
				}
				early = false;
				wm->last_early_id = 0;
			} else {
                // Since this image was not presented early or late, reset
                // any sets of early or late presentIDs:
				early = false;
				late = false;
				calibrate_next = true;
				wm->last_early_id = 0;
				wm->last_late_id = 0;
			}
		}

		if(early) {
            // Since we've seen at least two-seconds worth of presnts that
            // could have occured earlier than desired, let's decrease the
            // target_IPD (i.e. increase the frame rate):
            //
            // TODO(ianelliott): Try to calculate a better target_IPD based
            // on the most recently-seen present (this is overly-simplistic).
			wm->refresh_duration_multiplier--;
			if(wm->refresh_duration_multiplier == 0) {
                // This should never happen, but in case it does, don't
                // try to go faster.
				wm->refresh_duration_multiplier = 1;
			}
			wm->target_IPD = wm->refresh_duration *
					wm->refresh_duration_multiplier;
		}
		if(late) {
            // Since we found a new instance of a late present, we want to
            // increase the target_IPD (i.e. decrease the frame rate):
            //
            // TODO(ianelliott): Try to calculate a better target_IPD based
            // on the most recently-seen present (this is overly-simplistic).
			wm->refresh_duration_multiplier++;
			wm->target_IPD = wm->refresh_duration *
					wm->refresh_duration_multiplier;
		}

		if(calibrate_next) {
			int64_t multiple = wm->next_present_id -
						past[count - 1].presentID;
			wm->prev_desired_present_time =
				(past[count - 1].actualPresentTime +
				(multiple * wm->target_IPD));
		}
		free(past);
	}
}

static void wm_draw(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;

	// Ensure no more than FRAME_LAG renderings are outstanding
	vkWaitForFences(wm->device, 1, &wm->fences[wm->frame_index], VK_TRUE,
								UINT64_MAX);
	vkResetFences(wm->device, 1, &wm->fences[wm->frame_index]);

	do {
		// Get the index of the next available swapchain image:
		err = wm->fpAcquireNextImageKHR(wm->device, wm->swapchain,
								UINT64_MAX,
			wm->image_acquired_semaphores[wm->frame_index],
			VK_NULL_HANDLE, &wm->current_buffer);

		if(err == VK_ERROR_OUT_OF_DATE_KHR) {
            // wm->swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
			wm_resize(wm);
		} else if (err == VK_SUBOPTIMAL_KHR) {
           // wm->swapchain is not as optimal as it could be, but the platform's
           // presentation engine will still present the image correctly.
			break;
		} else if (err == VK_ERROR_SURFACE_LOST_KHR) {
			vkDestroySurfaceKHR(wm->inst, wm->surface, NULL);
			wm_create_surface(wm);
			wm_resize(wm);
		} else {
			assert(!err);
		}
	} while(err != VK_SUCCESS);

	wm_update_data_buffer(wm);

	if(wm->VK_GOOGLE_display_timing_enabled) {
        // Look at what happened to previous presents, and make appropriate
        // adjustments in timing:
		DemoUpdateTargetIPD(wm);

        // Note: a real application would position its geometry to that it's in
        // the correct locatoin for when the next image is presented.  It might
        // also wait, so that there's less latency between any input and when
        // the next image is rendered/presented.  This demo program is so
        // simple that it doesn't do either of those.
	}

    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
	VkPipelineStageFlags pipe_stage_flags;
	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;
	submit_info.pWaitDstStageMask = &pipe_stage_flags;
	pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores =
			&wm->image_acquired_semaphores[wm->frame_index];
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers =
			&wm->swapchain_image_resources[wm->current_buffer].cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores =
			&wm->draw_complete_semaphores[wm->frame_index];
	err = vkQueueSubmit(wm->graphics_queue, 1, &submit_info,
			wm->fences[wm->frame_index]);
	assert(!err);

	if(wm->separate_present_queue) {
     // If we are using separate queues, change image ownership to the
     // present queue before presenting, waiting for the draw complete
     // semaphore and signalling the ownership released semaphore when finished
		VkFence nullFence = VK_NULL_HANDLE;
		pipe_stage_flags =VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores =
			&wm->draw_complete_semaphores[wm->frame_index];
			submit_info.commandBufferCount = 1;
			submit_info.pCommandBuffers =
				&wm->swapchain_image_resources[
				wm->current_buffer].graphics_to_present_cmd;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores =
			&wm->image_ownership_semaphores[wm->frame_index];
		err = vkQueueSubmit(wm->present_queue, 1, &submit_info,
			nullFence);
		assert(!err);
	}

	// If we are using separate queues we have to wait for image ownership,
	// otherwise wait for draw complete
	VkPresentInfoKHR present = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = (wm->separate_present_queue)?
			&wm->image_ownership_semaphores[wm->frame_index]:
			&wm->draw_complete_semaphores[wm->frame_index],
		.swapchainCount = 1,
		.pSwapchains = &wm->swapchain,
		.pImageIndices = &wm->current_buffer,
	};

	VkRectLayerKHR rect;
	VkPresentRegionKHR region;
	VkPresentRegionsKHR regions;
	if(wm->VK_KHR_incremental_present_enabled) {
        // If using VK_KHR_incremental_present, we provide a hint of the region
        // that contains changed content relative to the previously-presented
        // image.  The implementation can use this hint in order to save
        // work/power (by only copying the region in the hint).  The
        // implementation is free to ignore the hint though, and so we must
        // ensure that the entire image has the correctly-drawn content.
		uint32_t eighthOfWidth = wm->width / 8;
		uint32_t eighthOfHeight = wm->height / 8;

		rect.offset.x = eighthOfWidth;
		rect.offset.y = eighthOfHeight;
		rect.extent.width = eighthOfWidth * 6;
		rect.extent.height = eighthOfHeight * 6;
		rect.layer = 0;

		region.rectangleCount = 1;
		region.pRectangles = &rect;

		regions.sType = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
		regions.pNext = present.pNext;
		regions.swapchainCount = present.swapchainCount;
		regions.pRegions = &region;
		present.pNext = &regions;
	}

	if(wm->VK_GOOGLE_display_timing_enabled) {
		VkPresentTimeGOOGLE ptime;
		if(wm->prev_desired_present_time == 0) {
            // This must be the first present for this swapchain.
            //
            // We don't know where we are relative to the presentation engine's
            // display's refresh cycle.  We also don't know how long rendering
            // takes.  Let's make a grossly-simplified assumption that the
            // desiredPresentTime should be half way between now and
            // now+target_IPD.  We will adjust over time.
			uint64_t curtime = getTimeInNanoseconds();
			if(curtime == 0) {
		// Since we didn't find out the current time, don't give a
		// desiredPresentTime:
				ptime.desiredPresentTime = 0;
			} else {
				ptime.desiredPresentTime = curtime +
						(wm->target_IPD >> 1);
			}
		} else {
			ptime.desiredPresentTime =
			(wm->prev_desired_present_time + wm->target_IPD);
		}
		ptime.presentID = wm->next_present_id++;
		wm->prev_desired_present_time = ptime.desiredPresentTime;

		VkPresentTimesInfoGOOGLE present_time = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE,
			.pNext = present.pNext,
			.swapchainCount = present.swapchainCount,
			.pTimes = &ptime,
		};
		if(wm->VK_GOOGLE_display_timing_enabled) {
			present.pNext = &present_time;
		}
	}

	err = wm->fpQueuePresentKHR(wm->present_queue, &present);
	wm->frame_index += 1;
	wm->frame_index %= FRAME_LAG;

	if(err == VK_ERROR_OUT_OF_DATE_KHR) {
        // wm->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
		wm_resize(wm);
	} else if(err == VK_SUBOPTIMAL_KHR) {
        // wm->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
	} else if(err == VK_ERROR_SURFACE_LOST_KHR) {
		vkDestroySurfaceKHR(wm->inst, wm->surface, NULL);
		wm_create_surface(wm);
		wm_resize(wm);
	} else {
		assert(!err);
	}
}

static void wm_prepare_buffers(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;
	VkSwapchainKHR oldSwapchain = wm->swapchain;

	// Check the surface capabilities and formats
	VkSurfaceCapabilitiesKHR surfCapabilities;
	err = wm->fpGetPhysicalDeviceSurfaceCapabilitiesKHR(
			wm->gpu, wm->surface, &surfCapabilities);
	assert(!err);

	uint32_t presentModeCount;
	err = wm->fpGetPhysicalDeviceSurfacePresentModesKHR(
			wm->gpu, wm->surface, &presentModeCount, NULL);
	assert(!err);
	VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(
			presentModeCount * sizeof(VkPresentModeKHR));
	assert(presentModes);
	err = wm->fpGetPhysicalDeviceSurfacePresentModesKHR(
			wm->gpu, wm->surface, &presentModeCount, presentModes);
	assert(!err);

	VkExtent2D swapchainExtent;
	// width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
	if(surfCapabilities.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.
		swapchainExtent.width = wm->width;
		swapchainExtent.height = wm->height;

		if(swapchainExtent.width <
				surfCapabilities.minImageExtent.width) {
		swapchainExtent.width = surfCapabilities.minImageExtent.width;
		} else if (swapchainExtent.width >
					surfCapabilities.maxImageExtent.width) {
			swapchainExtent.width =
				surfCapabilities.maxImageExtent.width;
		}

		if(swapchainExtent.height <
				surfCapabilities.minImageExtent.height) {
			swapchainExtent.height =
				surfCapabilities.minImageExtent.height;
		} else if(swapchainExtent.height >
				surfCapabilities.maxImageExtent.height) {
			swapchainExtent.height =
				surfCapabilities.maxImageExtent.height;
		}
	} else {
	     // If the surface size is defined, the swap chain size must match
		swapchainExtent = surfCapabilities.currentExtent;
		wm->width = surfCapabilities.currentExtent.width;
		wm->height = surfCapabilities.currentExtent.height;
	}

	if(wm->width == 0 || wm->height == 0) {
		wm->is_minimized = true;
		return;
	} else {
		wm->is_minimized = false;
	}

	// The FIFO present mode is guaranteed by the spec to be supported
	// and to have no tearing.  It's a great default present mode to use.
//	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    //  There are times when you may wish to use another present mode.  The
    //  following code shows how to select them, and the comments provide some
    //  reasons you may wish to use them.
    //
    // It should be noted that Vulkan 1.0 doesn't provide a method for
    // synchronizing rendering with the presentation engine's display.  There
    // is a method provided for throttling rendering with the display, but
    // there are some presentation engines for which this method will not work.
    // If an application doesn't throttle its rendering, and if it renders much
    // faster than the refresh rate of the display, this can waste power on
    // mobile devices.  That is because power is being spent rendering images
    // that may never be seen.

    // VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care about
    // tearing, or have some way of synchronizing their rendering with the
    // display.
    // VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
    // generally render a new presentable image every refresh cycle, but are
    // occasionally early.  In this case, the application wants the new image
    // to be displayed instead of the previously-queued-for-presentation image
    // that has not yet been displayed.
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
    // render a new presentable image every refresh cycle, but are occasionally
    // late.  In this case (perhaps because of stuttering/latency concerns),
    // the application wants the late image to be immediately displayed, even
    // though that may mean some tearing.

	if(wm->presentMode != swapchainPresentMode) {
		for(size_t i = 0; i < presentModeCount; ++i) {
			if(presentModes[i] == wm->presentMode) {
				swapchainPresentMode = wm->presentMode;
				break;
			}
		}
	}
	if(swapchainPresentMode != wm->presentMode) {
		ERR_EXIT("Present mode specified is not supported\n",
			"Present mode unsupported");
	}

    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple
    // buffering
	uint32_t desiredNumOfSwapchainImages = 3;
	if(desiredNumOfSwapchainImages < surfCapabilities.minImageCount) {
		desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
	}
    // If maxImageCount is 0, we can ask for as many images as we want;
    // otherwise we're limited to maxImageCount
	if((surfCapabilities.maxImageCount > 0) &&
			(desiredNumOfSwapchainImages >
			surfCapabilities.maxImageCount)) {
		// Application must settle for fewer images than desired:
		desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
	}

	VkSurfaceTransformFlagsKHR preTransform;
	if(surfCapabilities.supportedTransforms &
				VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else {
		preTransform = surfCapabilities.currentTransform;
	}

// Find a supported composite alpha mode - one of these is guaranteed to be set
	VkCompositeAlphaFlagBitsKHR compositeAlpha =
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for(uint32_t i = 0; i < ARRAY_SIZE(compositeAlphaFlags); i++) {
		if(surfCapabilities.supportedCompositeAlpha &
						compositeAlphaFlags[i]) {
			compositeAlpha = compositeAlphaFlags[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR swapchain_ci = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.surface = wm->surface,
		.minImageCount = desiredNumOfSwapchainImages,
		.imageFormat = wm->format,
		.imageColorSpace = wm->color_space,
		.imageExtent = {
			.width = swapchainExtent.width,
			.height = swapchainExtent.height,
		},
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = preTransform,
		.compositeAlpha = compositeAlpha,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.presentMode = swapchainPresentMode,
		.oldSwapchain = oldSwapchain,
		.clipped = true,
	};
	uint32_t i;
	err = wm->fpCreateSwapchainKHR(wm->device, &swapchain_ci, NULL,
			&wm->swapchain);
	assert(!err);

    // If we just re-created an existing swapchain, we should destroy the old
    // swapchain at this point.
    // Note: destroying the swapchain also cleans up all its associated
    // presentable images once the platform is done with them.
	if(oldSwapchain != VK_NULL_HANDLE) {
		wm->fpDestroySwapchainKHR(wm->device, oldSwapchain, NULL);
	}

	err = wm->fpGetSwapchainImagesKHR(wm->device, wm->swapchain,
			&wm->swapchainImageCount, NULL);
	assert(!err);

	VkImage *swapchainImages = (VkImage *)malloc(
				wm->swapchainImageCount * sizeof(VkImage));
	assert(swapchainImages);
	err = wm->fpGetSwapchainImagesKHR(wm->device, wm->swapchain,
		&wm->swapchainImageCount, swapchainImages);
	assert(!err);

	wm->swapchain_image_resources =
			(SwapchainImageResources *)malloc(sizeof(
			SwapchainImageResources) * wm->swapchainImageCount);
	assert(wm->swapchain_image_resources);

	for(i = 0; i < wm->swapchainImageCount; i++) {
		VkImageViewCreateInfo color_image_view = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.format = wm->format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_R,
				.g = VK_COMPONENT_SWIZZLE_G,
				.b = VK_COMPONENT_SWIZZLE_B,
				.a = VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0, .levelCount = 1,
				.baseArrayLayer = 0, .layerCount = 1},
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.flags = 0,
		};

		wm->swapchain_image_resources[i].image = swapchainImages[i];

		color_image_view.image = wm->swapchain_image_resources[i].image;

		err = vkCreateImageView(wm->device, &color_image_view, NULL,
			&wm->swapchain_image_resources[i].view);
		assert(!err);
	}

	if(wm->VK_GOOGLE_display_timing_enabled) {
		VkRefreshCycleDurationGOOGLE rc_dur;
		err = wm->fpGetRefreshCycleDurationGOOGLE(wm->device,
						wm->swapchain, &rc_dur);
		assert(!err);
		wm->refresh_duration = rc_dur.refreshDuration;

		wm->syncd_with_actual_presents = false;
		// Initially target 1X the refresh duration:
		wm->target_IPD = wm->refresh_duration;
		wm->refresh_duration_multiplier = 1;
		wm->prev_desired_present_time = 0;
		wm->next_present_id = 1;
	}

	if(NULL != swapchainImages) {
		free(swapchainImages);
	}

	if(NULL != presentModes) {
		free(presentModes);
	}
}

static void wm_prepare_depth(struct Wm *wm) {
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;
	const VkImageCreateInfo image = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = depth_format,
		.extent = {wm->width, wm->height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.flags = 0,
	};

	VkImageViewCreateInfo view = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.image = VK_NULL_HANDLE,
		.format = depth_format,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel = 0, .levelCount = 1,
			.baseArrayLayer = 0, .layerCount = 1},
		.flags = 0,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
	};

	VkMemoryRequirements mem_reqs;
	VkResult U_ASSERT_ONLY err;
	bool U_ASSERT_ONLY pass;

	wm->depth.format = depth_format;

	// create image
	err = vkCreateImage(wm->device, &image, NULL, &wm->depth.image);
	assert(!err);

	vkGetImageMemoryRequirements(wm->device, wm->depth.image, &mem_reqs);
	assert(!err);

	wm->depth.mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	wm->depth.mem_alloc.pNext = NULL;
	wm->depth.mem_alloc.allocationSize = mem_reqs.size;
	wm->depth.mem_alloc.memoryTypeIndex = 0;

	pass = memory_type_from_properties(wm, mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&wm->depth.mem_alloc.memoryTypeIndex);
	assert(pass);

	// allocate memory
	err = vkAllocateMemory(wm->device, &wm->depth.mem_alloc, NULL,
		&wm->depth.mem);
	assert(!err);

	// bind memory
	err = vkBindImageMemory(wm->device, wm->depth.image, wm->depth.mem, 0);
	assert(!err);

	// create image view
	view.image = wm->depth.image;
	err = vkCreateImageView(wm->device, &view, NULL, &wm->depth.view);
	assert(!err);
}

// Convert ppm image data from header file into RGBA texture image
#include "lunarg.ppm.h"
bool loadTexture(const char *filename, uint8_t *rgba_data,
		VkSubresourceLayout *layout, int32_t *width, int32_t *height) {
	(void)filename;
	char *cPtr;
	cPtr = (char *)lunarg_ppm;
	if((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) ||
			strncmp(cPtr, "P6\n", 3)) {
		return false;
	}
	while(strncmp(cPtr++, "\n", 1));
	sscanf(cPtr, "%u %u", width, height);
	if(rgba_data == NULL) {
		return true;
	}
	while(strncmp(cPtr++, "\n", 1));
	if((unsigned char *)cPtr >= (lunarg_ppm + lunarg_ppm_len) ||
			strncmp(cPtr, "255\n", 4)) {
		return false;
	}
	while(strncmp(cPtr++, "\n", 1));
	for(int y = 0; y < *height; y++) {
		uint8_t *rowPtr = rgba_data;
		for(int x = 0; x < *width; x++) {
			memcpy(rowPtr, cPtr, 3);
			rowPtr[3] = 255; // Alpha of 1
			rowPtr += 4;
			cPtr += 3;
		}
		rgba_data += layout->rowPitch;
	}
	return true;
}

static void wm_prepare_texture_buffer(struct Wm *wm, const char *filename,
		struct texture_object *tex_obj) {
	int32_t tex_width;
	int32_t tex_height;
	VkResult U_ASSERT_ONLY err;
	bool U_ASSERT_ONLY pass;

	if(!loadTexture(filename, NULL, NULL, &tex_width, &tex_height)) {
		ERR_EXIT("Failed to load textures", "Load Texture Failure");
	}

	tex_obj->tex_width = tex_width;
	tex_obj->tex_height = tex_height;

	const VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = tex_width * tex_height * 4,
		.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL};

	err = vkCreateBuffer(wm->device, &buffer_create_info, NULL,
			&tex_obj->buffer);
	assert(!err);

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(wm->device, tex_obj->buffer, &mem_reqs);

	tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	tex_obj->mem_alloc.pNext = NULL;
	tex_obj->mem_alloc.allocationSize = mem_reqs.size;
	tex_obj->mem_alloc.memoryTypeIndex = 0;

	VkFlags requirements = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	pass = memory_type_from_properties(wm, mem_reqs.memoryTypeBits,
			requirements, &tex_obj->mem_alloc.memoryTypeIndex);
	assert(pass);

	err = vkAllocateMemory(wm->device, &tex_obj->mem_alloc, NULL,
			&(tex_obj->mem));
	assert(!err);

	// bind memory
	err = vkBindBufferMemory(wm->device, tex_obj->buffer, tex_obj->mem, 0);
	assert(!err);

	VkSubresourceLayout layout;
	memset(&layout, 0, sizeof(layout));
	layout.rowPitch = tex_width * 4;

	void *data;
	err = vkMapMemory(wm->device, tex_obj->mem, 0,
			tex_obj->mem_alloc.allocationSize, 0, &data);
	assert(!err);

	if(!loadTexture(filename, data, &layout, &tex_width, &tex_height)) {
		fprintf(stderr, "Error loading texture: %s\n", filename);
	}

	vkUnmapMemory(wm->device, tex_obj->mem);
}

static void wm_prepare_texture_image(struct Wm *wm, const char *filename,
		struct texture_object *tex_obj,
		VkImageTiling tiling, VkImageUsageFlags usage,
		VkFlags required_props) {
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
	int32_t tex_width;
	int32_t tex_height;
	VkResult U_ASSERT_ONLY err;
	bool U_ASSERT_ONLY pass;

	if(!loadTexture(filename, NULL, NULL, &tex_width, &tex_height)) {
		ERR_EXIT("Failed to load textures", "Load Texture Failure");
	}

	tex_obj->tex_width = tex_width;
	tex_obj->tex_height = tex_height;

	const VkImageCreateInfo image_create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = tex_format,
		.extent = {tex_width, tex_height, 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.flags = 0,
		.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED,
	};

	VkMemoryRequirements mem_reqs;

	err = vkCreateImage(wm->device, &image_create_info, NULL,
			&tex_obj->image);
	assert(!err);

	vkGetImageMemoryRequirements(wm->device, tex_obj->image, &mem_reqs);

	tex_obj->mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	tex_obj->mem_alloc.pNext = NULL;
	tex_obj->mem_alloc.allocationSize = mem_reqs.size;
	tex_obj->mem_alloc.memoryTypeIndex = 0;

	pass = memory_type_from_properties(wm, mem_reqs.memoryTypeBits,
			required_props, &tex_obj->mem_alloc.memoryTypeIndex);
	assert(pass);

	// allocate memory
	err = vkAllocateMemory(wm->device, &tex_obj->mem_alloc, NULL,
			&(tex_obj->mem));
	assert(!err);

	// bind memory
	err = vkBindImageMemory(wm->device, tex_obj->image, tex_obj->mem, 0);
	assert(!err);

	if(required_props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		const VkImageSubresource subres = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.arrayLayer = 0,
		};
		VkSubresourceLayout layout;
		void *data;

		vkGetImageSubresourceLayout(wm->device, tex_obj->image,
				&subres, &layout);

		err = vkMapMemory(wm->device, tex_obj->mem, 0,
				tex_obj->mem_alloc.allocationSize, 0, &data);
		assert(!err);

		if(!loadTexture(filename, data, &layout, &tex_width,
				&tex_height)) {
			fprintf(stderr, "Error loading texture: %s\n",
					filename);
		}

		vkUnmapMemory(wm->device, tex_obj->mem);
	}

	tex_obj->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static void wm_destroy_texture(struct Wm *wm, struct texture_object *tex_objs) {
	// clean up staging resources
	vkFreeMemory(wm->device, tex_objs->mem, NULL);
	if(tex_objs->image) vkDestroyImage(wm->device, tex_objs->image, NULL);
	if(tex_objs->buffer)vkDestroyBuffer(wm->device, tex_objs->buffer, NULL);
}

static void wm_prepare_textures(struct Wm *wm) {
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;
	VkFormatProperties props;
	uint32_t i;

	vkGetPhysicalDeviceFormatProperties(wm->gpu, tex_format, &props);

	for(i = 0; i < DEMO_TEXTURE_COUNT; i++) {
		VkResult U_ASSERT_ONLY err;

		if((props.linearTilingFeatures &
				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) &&
				!wm->use_staging_buffer) {
			// Device can texture using linear textures
			wm_prepare_texture_image(wm, tex_files[i],
					&wm->textures[i],
					VK_IMAGE_TILING_LINEAR,
					VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			// Nothing in the pipeline needs to be complete to
			// start, and don't allow fragment
			// shader to run until layout transition completes
			wm_set_image_layout(wm, wm->textures[i].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					wm->textures[i].imageLayout, 0,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			wm->staging_texture.image = 0;
		} else if(props.optimalTilingFeatures &
				VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
			// Must use staging buffer to copy linear texture
			// to optimized

			memset(&wm->staging_texture, 0,
					sizeof(wm->staging_texture));
			wm_prepare_texture_buffer(wm, tex_files[i],
					&wm->staging_texture);

			wm_prepare_texture_image(wm, tex_files[i],
					&wm->textures[i],
					VK_IMAGE_TILING_OPTIMAL,
					(VK_IMAGE_USAGE_TRANSFER_DST_BIT |
					VK_IMAGE_USAGE_SAMPLED_BIT),
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			wm_set_image_layout(wm, wm->textures[i].image,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_PREINITIALIZED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT);

			VkBufferImageCopy copy_region = {
				.bufferOffset = 0,
				.bufferRowLength =
						wm->staging_texture.tex_width,
				.bufferImageHeight =
						wm->staging_texture.tex_height,
				.imageSubresource =
					{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				.imageOffset = {0, 0, 0},
				.imageExtent = {wm->staging_texture.tex_width,
					wm->staging_texture.tex_height, 1},
		};

		vkCmdCopyBufferToImage(wm->cmd, wm->staging_texture.buffer,
				wm->textures[i].image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
				&copy_region);

		wm_set_image_layout(wm, wm->textures[i].image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				wm->textures[i].imageLayout,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		} else {
			// Can't support VK_FORMAT_R8G8B8A8_UNORM !?
			assert(!"No support for R8G8B8A8_UNORM "
						"as texture image format");
		}

		const VkSamplerCreateInfo sampler = {
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = NULL,
			.magFilter = VK_FILTER_NEAREST,
			.minFilter = VK_FILTER_NEAREST,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1,
			.compareOp = VK_COMPARE_OP_NEVER,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
			.unnormalizedCoordinates = VK_FALSE,
		};

		VkImageViewCreateInfo view = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.image = VK_NULL_HANDLE,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = tex_format,
			.components = {
				VK_COMPONENT_SWIZZLE_R,
				VK_COMPONENT_SWIZZLE_G,
				VK_COMPONENT_SWIZZLE_B,
				VK_COMPONENT_SWIZZLE_A,
			},
			.subresourceRange =
				{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1},
			.flags = 0,
		};

		// create sampler
		err = vkCreateSampler(wm->device, &sampler, NULL,
				&wm->textures[i].sampler);
		assert(!err);

		// create image view
		view.image = wm->textures[i].image;
		err = vkCreateImageView(wm->device, &view, NULL,
				&wm->textures[i].view);
		assert(!err);
	}
}

void wm_prepare_cube_data_buffers(struct Wm *wm) {
	VkBufferCreateInfo buf_info;
	VkMemoryRequirements mem_reqs;
	VkMemoryAllocateInfo mem_alloc;
	mat4x4 MVP, VP;
	VkResult U_ASSERT_ONLY err;
	bool U_ASSERT_ONLY pass;
	struct vktexcube_vs_uniform data;

	mat4x4_mul(VP, wm->projection_matrix, wm->view_matrix);
	mat4x4_mul(MVP, VP, wm->model_matrix);
	memcpy(data.mvp, MVP, sizeof(MVP));

	for(unsigned int i = 0; i < 12 * 3; i++) {
		data.position[i][0] = g_vertex_buffer_data[i * 3];
		data.position[i][1] = g_vertex_buffer_data[i * 3 + 1];
		data.position[i][2] = g_vertex_buffer_data[i * 3 + 2];
		data.position[i][3] = 1.0f;
		data.attr[i][0] = g_uv_buffer_data[2 * i];
		data.attr[i][1] = g_uv_buffer_data[2 * i + 1];
		data.attr[i][2] = 0;
		data.attr[i][3] = 0;
	}

	memset(&buf_info, 0, sizeof(buf_info));
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = sizeof(data);

	for(unsigned int i = 0; i < wm->swapchainImageCount; i++) {
		err = vkCreateBuffer(wm->device, &buf_info, NULL,
			&wm->swapchain_image_resources[i].uniform_buffer);
		assert(!err);

		vkGetBufferMemoryRequirements(wm->device,
				wm->swapchain_image_resources[i].uniform_buffer,
				&mem_reqs);

		mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		mem_alloc.pNext = NULL;
		mem_alloc.allocationSize = mem_reqs.size;
		mem_alloc.memoryTypeIndex = 0;

		pass = memory_type_from_properties(wm, mem_reqs.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&mem_alloc.memoryTypeIndex);
		assert(pass);

		err = vkAllocateMemory(wm->device, &mem_alloc, NULL,
			&wm->swapchain_image_resources[i].uniform_memory);
		assert(!err);

		err = vkMapMemory(wm->device,
			wm->swapchain_image_resources[i].uniform_memory, 0,
			VK_WHOLE_SIZE, 0,
                        &wm->swapchain_image_resources[i].uniform_memory_ptr);
		assert(!err);

		memcpy(wm->swapchain_image_resources[i].uniform_memory_ptr,
				&data, sizeof data);

		err = vkBindBufferMemory(wm->device,
				wm->swapchain_image_resources[i].uniform_buffer,
				wm->swapchain_image_resources[i].uniform_memory,
				0);
		assert(!err);
	}
}

static void wm_prepare_descriptor_layout(struct Wm *wm) {
	const VkDescriptorSetLayoutBinding layout_bindings[2] = {
		[0] = {
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = NULL,
		}, [1] = {
			.binding = 1,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = DEMO_TEXTURE_COUNT,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = NULL,
		},
	};
	const VkDescriptorSetLayoutCreateInfo descriptor_layout = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.bindingCount = 2,
		.pBindings = layout_bindings,
	};
	VkResult U_ASSERT_ONLY err;

	err = vkCreateDescriptorSetLayout(wm->device, &descriptor_layout, NULL,
			&wm->desc_layout);
	assert(!err);

	const VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.setLayoutCount = 1,
		.pSetLayouts = &wm->desc_layout,
	};

	err = vkCreatePipelineLayout(wm->device, &pPipelineLayoutCreateInfo,
			NULL, &wm->pipeline_layout);
	assert(!err);
}

static void wm_prepare_render_pass(struct Wm *wm) {
	// The initial layout for the color and depth attachments
	// will be LAYOUT_UNDEFINED
	// because at the start of the renderpass,
	// we don't care about their contents.
	// At the start of the subpass, the color
	// attachment's layout will be transitioned
	// to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the
	// depth stencil attachment's layout
	// will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.
	// At the end of the renderpass, the color
	// attachment's layout will be transitioned to
	// LAYOUT_PRESENT_SRC_KHR to be ready to present.
	// This is all done as part of
	// the renderpass, no barriers are necessary.
	const VkAttachmentDescription attachments[2] = {
		[0] = {
			.format = wm->format,
			.flags = 0,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		}, [1] = {
			.format = wm->depth.format,
			.flags = 0,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};
	const VkAttachmentReference color_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};
	const VkAttachmentReference depth_reference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};
	const VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.flags = 0,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_reference,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = &depth_reference,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL,
	};

	VkSubpassDependency attachmentDependencies[2] = {
		[0] = {
			// Depth buffer is shared between swapchain images
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask =
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask =
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask =
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0,
		}, [1] = {
			// Image Layout Transition
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask =
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask =
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
				VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
			.dependencyFlags = 0,
		},
	};

	const VkRenderPassCreateInfo rp_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 2,
		.pDependencies = attachmentDependencies,
	};
	VkResult U_ASSERT_ONLY err;

	err = vkCreateRenderPass(wm->device, &rp_info, NULL, &wm->render_pass);
	assert(!err);
}

static VkShaderModule wm_prepare_shader_module(struct Wm *wm,
		const uint32_t *code, size_t size) {
	VkShaderModule module;
	VkShaderModuleCreateInfo moduleCreateInfo;
	VkResult U_ASSERT_ONLY err;

	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pNext = NULL;
	moduleCreateInfo.flags = 0;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = code;

	err = vkCreateShaderModule(wm->device, &moduleCreateInfo, NULL,&module);
	assert(!err);

	return module;
}

static void wm_prepare_vs(struct Wm *wm) {
	const uint32_t vs_code[] = {
#include "cube.vert.inc"
	};
	wm->vert_shader_module = wm_prepare_shader_module(wm, vs_code,
			sizeof(vs_code));
}

static void wm_prepare_fs(struct Wm *wm) {
	const uint32_t fs_code[] = {
#include "cube.frag.inc"
	};
	wm->frag_shader_module = wm_prepare_shader_module(wm, fs_code,
			sizeof(fs_code));
}

static void wm_prepare_pipeline(struct Wm *wm) {
	VkGraphicsPipelineCreateInfo pipeline;
	VkPipelineCacheCreateInfo pipelineCache;
	VkPipelineVertexInputStateCreateInfo vi;
	VkPipelineInputAssemblyStateCreateInfo ia;
	VkPipelineRasterizationStateCreateInfo rs;
	VkPipelineColorBlendStateCreateInfo cb;
	VkPipelineDepthStencilStateCreateInfo ds;
	VkPipelineViewportStateCreateInfo vp;
	VkPipelineMultisampleStateCreateInfo ms;
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkPipelineDynamicStateCreateInfo dynamicState;
	VkResult U_ASSERT_ONLY err;

	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	memset(&dynamicState, 0, sizeof dynamicState);
	dynamicState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pDynamicStates = dynamicStateEnables;

	memset(&pipeline, 0, sizeof(pipeline));
	pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline.layout = wm->pipeline_layout;

	memset(&vi, 0, sizeof(vi));
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	memset(&ia, 0, sizeof(ia));
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	memset(&rs, 0, sizeof(rs));
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rs.depthClampEnable = VK_FALSE;
	rs.rasterizerDiscardEnable = VK_FALSE;
	rs.depthBiasEnable = VK_FALSE;
	rs.lineWidth = 1.0f;

	memset(&cb, 0, sizeof(cb));
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	VkPipelineColorBlendAttachmentState att_state[1];
	memset(att_state, 0, sizeof(att_state));
	att_state[0].colorWriteMask = 0xf;
	att_state[0].blendEnable = VK_FALSE;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;

	memset(&vp, 0, sizeof(vp));
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
			VK_DYNAMIC_STATE_VIEWPORT;
	vp.scissorCount = 1;
	dynamicStateEnables[dynamicState.dynamicStateCount++] =
			VK_DYNAMIC_STATE_SCISSOR;

	memset(&ds, 0, sizeof(ds));
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.depthBoundsTestEnable = VK_FALSE;
	ds.back.failOp = VK_STENCIL_OP_KEEP;
	ds.back.passOp = VK_STENCIL_OP_KEEP;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;
	ds.stencilTestEnable = VK_FALSE;
	ds.front = ds.back;

	memset(&ms, 0, sizeof(ms));
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.pSampleMask = NULL;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	wm_prepare_vs(wm);
	wm_prepare_fs(wm);

	// Two stages: vs and fs
	VkPipelineShaderStageCreateInfo shaderStages[2];
	memset(&shaderStages, 0, 2 * sizeof(VkPipelineShaderStageCreateInfo));

	shaderStages[0].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = wm->vert_shader_module;
	shaderStages[0].pName = "main";

	shaderStages[1].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = wm->frag_shader_module;
	shaderStages[1].pName = "main";

	memset(&pipelineCache, 0, sizeof(pipelineCache));
	pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	err = vkCreatePipelineCache(wm->device, &pipelineCache, NULL,
			&wm->pipelineCache);
	assert(!err);

	pipeline.pVertexInputState = &vi;
	pipeline.pInputAssemblyState = &ia;
	pipeline.pRasterizationState = &rs;
	pipeline.pColorBlendState = &cb;
	pipeline.pMultisampleState = &ms;
	pipeline.pViewportState = &vp;
	pipeline.pDepthStencilState = &ds;
	pipeline.stageCount = ARRAY_SIZE(shaderStages);
	pipeline.pStages = shaderStages;
	pipeline.renderPass = wm->render_pass;
	pipeline.pDynamicState = &dynamicState;

	pipeline.renderPass = wm->render_pass;

	err = vkCreateGraphicsPipelines(wm->device, wm->pipelineCache, 1,
			&pipeline, NULL, &wm->pipeline);
	assert(!err);

	vkDestroyShaderModule(wm->device, wm->frag_shader_module, NULL);
	vkDestroyShaderModule(wm->device, wm->vert_shader_module, NULL);
}

static void wm_prepare_descriptor_pool(struct Wm *wm) {
	const VkDescriptorPoolSize type_counts[2] = {
		[0] = {
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = wm->swapchainImageCount,
		}, [1] = {
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = wm->swapchainImageCount *
					DEMO_TEXTURE_COUNT,
		},
	};
	const VkDescriptorPoolCreateInfo descriptor_pool = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.maxSets = wm->swapchainImageCount,
		.poolSizeCount = 2,
		.pPoolSizes = type_counts,
	};
	VkResult U_ASSERT_ONLY err;

	err = vkCreateDescriptorPool(wm->device, &descriptor_pool, NULL,
			&wm->desc_pool);
	assert(!err);
}

static void wm_prepare_descriptor_set(struct Wm *wm) {
	VkDescriptorImageInfo tex_descs[DEMO_TEXTURE_COUNT];
	VkWriteDescriptorSet writes[2];
	VkResult U_ASSERT_ONLY err;

	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = wm->desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &wm->desc_layout};

	VkDescriptorBufferInfo buffer_info;
	buffer_info.offset = 0;
	buffer_info.range = sizeof(struct vktexcube_vs_uniform);

	memset(&tex_descs, 0, sizeof(tex_descs));
	for(unsigned int i = 0; i < DEMO_TEXTURE_COUNT; i++) {
		tex_descs[i].sampler = wm->textures[i].sampler;
		tex_descs[i].imageView = wm->textures[i].view;
		tex_descs[i].imageLayout =
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}

	memset(&writes, 0, sizeof(writes));

	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &buffer_info;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstBinding = 1;
	writes[1].descriptorCount = DEMO_TEXTURE_COUNT;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writes[1].pImageInfo = tex_descs;

	for(unsigned int i = 0; i < wm->swapchainImageCount; i++) {
		err = vkAllocateDescriptorSets(wm->device, &alloc_info,
			&wm->swapchain_image_resources[i].descriptor_set);
		assert(!err);
		buffer_info.buffer =
			wm->swapchain_image_resources[i].uniform_buffer;
		writes[0].dstSet =
			wm->swapchain_image_resources[i].descriptor_set;
			writes[1].dstSet =
			wm->swapchain_image_resources[i].descriptor_set;
			vkUpdateDescriptorSets(wm->device, 2, writes, 0, NULL);
	}
}

static void wm_prepare_framebuffers(struct Wm *wm) {
	VkImageView attachments[2];
	attachments[1] = wm->depth.view;

	const VkFramebufferCreateInfo fb_info = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = NULL,
		.renderPass = wm->render_pass,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.width = wm->width,
		.height = wm->height,
		.layers = 1,
	};
	VkResult U_ASSERT_ONLY err;
	uint32_t i;

	for(i = 0; i < wm->swapchainImageCount; i++) {
		attachments[0] = wm->swapchain_image_resources[i].view;
		err = vkCreateFramebuffer(wm->device, &fb_info, NULL,
			&wm->swapchain_image_resources[i].framebuffer);
		assert(!err);
	}
}

static void wm_prepare(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;
	if(wm->cmd_pool == VK_NULL_HANDLE) {
		const VkCommandPoolCreateInfo cmd_pool_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = NULL,
			.queueFamilyIndex = wm->graphics_queue_family_index,
			.flags = 0,
		};
		err = vkCreateCommandPool(wm->device, &cmd_pool_info, NULL,
				&wm->cmd_pool);
		assert(!err);
	}

	const VkCommandBufferAllocateInfo cmd = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = wm->cmd_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	err = vkAllocateCommandBuffers(wm->device, &cmd, &wm->cmd);
	assert(!err);
	VkCommandBufferBeginInfo cmd_buf_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = 0,
		.pInheritanceInfo = NULL,
	};
	err = vkBeginCommandBuffer(wm->cmd, &cmd_buf_info);
	assert(!err);

	wm_prepare_buffers(wm);

	if(wm->is_minimized) {
		wm->prepared = false;
		return;
	}

	wm_prepare_depth(wm);
	wm_prepare_textures(wm);
	wm_prepare_cube_data_buffers(wm);

	wm_prepare_descriptor_layout(wm);
	wm_prepare_render_pass(wm);
	wm_prepare_pipeline(wm);

	for(uint32_t i = 0; i < wm->swapchainImageCount; i++) {
		err = vkAllocateCommandBuffers(wm->device, &cmd,
				&wm->swapchain_image_resources[i].cmd);
		assert(!err);
	}

	if(wm->separate_present_queue) {
		const VkCommandPoolCreateInfo present_cmd_pool_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = NULL,
			.queueFamilyIndex = wm->present_queue_family_index,
			.flags = 0,
		};
		err = vkCreateCommandPool(wm->device, &present_cmd_pool_info,
				NULL, &wm->present_cmd_pool);
		assert(!err);
		const VkCommandBufferAllocateInfo present_cmd_info = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = NULL,
			.commandPool = wm->present_cmd_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		for(uint32_t i = 0; i < wm->swapchainImageCount; i++) {
			err = vkAllocateCommandBuffers(wm->device,
					&present_cmd_info,
					&wm->swapchain_image_resources[i]
						.graphics_to_present_cmd);
			assert(!err);
			wm_build_image_ownership_cmd(wm, i);
		}
	}

	wm_prepare_descriptor_pool(wm);
	wm_prepare_descriptor_set(wm);

	wm_prepare_framebuffers(wm);

	for(uint32_t i = 0; i < wm->swapchainImageCount; i++) {
		wm->current_buffer = i;
		wm_draw_build_cmd(wm, wm->swapchain_image_resources[i].cmd);
	}

	// Prepare functions above may generate pipeline commands
	// that need to be flushed before beginning the render loop.

	wm_flush_init_cmd(wm);
	if(wm->staging_texture.buffer) {
		wm_destroy_texture(wm, &wm->staging_texture);
	}

	wm->current_buffer = 0;
	wm->prepared = true;
}

static void wm_cleanup(struct Wm *wm) {
	uint32_t i;

	wm->prepared = false;
	vkDeviceWaitIdle(wm->device);

	// Wait for fences from present operations
	for(i = 0; i < FRAME_LAG; i++) {
		vkWaitForFences(wm->device, 1, &wm->fences[i], VK_TRUE,
				UINT64_MAX);
		vkDestroyFence(wm->device, wm->fences[i], NULL);
		vkDestroySemaphore(wm->device, wm->image_acquired_semaphores[i],
				NULL);
		vkDestroySemaphore(wm->device, wm->draw_complete_semaphores[i],
				NULL);
		if(wm->separate_present_queue) {
			vkDestroySemaphore(wm->device,
				wm->image_ownership_semaphores[i], NULL);
		}
	}

	// If the window is currently minimized, wm_resize has already done
	// some cleanup for us.
	if(!wm->is_minimized) {
		for(i = 0; i < wm->swapchainImageCount; i++) {
		vkDestroyFramebuffer(wm->device,
			wm->swapchain_image_resources[i].framebuffer, NULL);
		}
		vkDestroyDescriptorPool(wm->device, wm->desc_pool, NULL);

		vkDestroyPipeline(wm->device, wm->pipeline, NULL);
		vkDestroyPipelineCache(wm->device, wm->pipelineCache, NULL);
		vkDestroyRenderPass(wm->device, wm->render_pass, NULL);
		vkDestroyPipelineLayout(wm->device, wm->pipeline_layout, NULL);
		vkDestroyDescriptorSetLayout(wm->device, wm->desc_layout, NULL);

		for(i = 0; i < DEMO_TEXTURE_COUNT; i++) {
			vkDestroyImageView(wm->device, wm->textures[i].view,
									NULL);
			vkDestroyImage(wm->device, wm->textures[i].image, NULL);
			vkFreeMemory(wm->device, wm->textures[i].mem, NULL);
			vkDestroySampler(wm->device, wm->textures[i].sampler,
									NULL);
		}
		wm->fpDestroySwapchainKHR(wm->device, wm->swapchain, NULL);

		vkDestroyImageView(wm->device, wm->depth.view, NULL);
		vkDestroyImage(wm->device, wm->depth.image, NULL);
		vkFreeMemory(wm->device, wm->depth.mem, NULL);

		for(i = 0; i < wm->swapchainImageCount; i++) {
			vkDestroyImageView(wm->device,
				wm->swapchain_image_resources[i].view, NULL);
			vkFreeCommandBuffers(wm->device, wm->cmd_pool, 1,
					&wm->swapchain_image_resources[i].cmd);
			vkDestroyBuffer(wm->device,
			wm->swapchain_image_resources[i].uniform_buffer, NULL);
			vkUnmapMemory(wm->device,
			wm->swapchain_image_resources[i].uniform_memory);
			vkFreeMemory(wm->device,
			wm->swapchain_image_resources[i].uniform_memory, NULL);
		}
		free(wm->swapchain_image_resources);
		free(wm->queue_props);
		vkDestroyCommandPool(wm->device, wm->cmd_pool, NULL);

		if(wm->separate_present_queue) {
			vkDestroyCommandPool(wm->device, wm->present_cmd_pool,
					NULL);
		}
	}
	vkDeviceWaitIdle(wm->device);
	vkDestroyDevice(wm->device, NULL);
	if(wm->validate) {
		wm->DestroyDebugUtilsMessengerEXT(wm->inst, wm->dbg_messenger,
				NULL);
	}
	vkDestroySurfaceKHR(wm->inst, wm->surface, NULL);

	xcb_destroy_window(wm->connection, wm->xcb_window);
	xcb_disconnect(wm->connection);
	free(wm->atom_wm_delete_window);

	vkDestroyInstance(wm->inst, NULL);
}

static void wm_resize(struct Wm *wm) {
	uint32_t i;

	// Don't react to resize until after first initialization.
	if(!wm->prepared) {
		if(wm->is_minimized) {
			wm_prepare(wm);
		}
		return;
	}
	// In order to properly resize the window,
	// we must re-create the swapchain
	// AND redo the command buffers, etc.
	// First, perform part of the wm_cleanup() function:
	wm->prepared = false;
	vkDeviceWaitIdle(wm->device);

	for(i = 0; i < wm->swapchainImageCount; i++) {
		vkDestroyFramebuffer(wm->device,
			wm->swapchain_image_resources[i].framebuffer, NULL);
	}
	vkDestroyDescriptorPool(wm->device, wm->desc_pool, NULL);

	vkDestroyPipeline(wm->device, wm->pipeline, NULL);
	vkDestroyPipelineCache(wm->device, wm->pipelineCache, NULL);
	vkDestroyRenderPass(wm->device, wm->render_pass, NULL);
	vkDestroyPipelineLayout(wm->device, wm->pipeline_layout, NULL);
	vkDestroyDescriptorSetLayout(wm->device, wm->desc_layout, NULL);

	for(i = 0; i < DEMO_TEXTURE_COUNT; i++) {
		vkDestroyImageView(wm->device, wm->textures[i].view, NULL);
		vkDestroyImage(wm->device, wm->textures[i].image, NULL);
		vkFreeMemory(wm->device, wm->textures[i].mem, NULL);
		vkDestroySampler(wm->device, wm->textures[i].sampler, NULL);
	}

	vkDestroyImageView(wm->device, wm->depth.view, NULL);
	vkDestroyImage(wm->device, wm->depth.image, NULL);
	vkFreeMemory(wm->device, wm->depth.mem, NULL);

	for(i = 0; i < wm->swapchainImageCount; i++) {
		vkDestroyImageView(wm->device,
				wm->swapchain_image_resources[i].view, NULL);
		vkFreeCommandBuffers(wm->device, wm->cmd_pool, 1,
				&wm->swapchain_image_resources[i].cmd);
		vkDestroyBuffer(wm->device,
			wm->swapchain_image_resources[i].uniform_buffer, NULL);
		vkUnmapMemory(wm->device,
			wm->swapchain_image_resources[i].uniform_memory);
		vkFreeMemory(wm->device,
			wm->swapchain_image_resources[i].uniform_memory, NULL);
	}
	vkDestroyCommandPool(wm->device, wm->cmd_pool, NULL);
	wm->cmd_pool = VK_NULL_HANDLE;
	if(wm->separate_present_queue) {
		vkDestroyCommandPool(wm->device, wm->present_cmd_pool, NULL);
	}
	free(wm->swapchain_image_resources);

	// Second, re-perform the wm_prepare() function, which will re-create
	// the swapchain:
	wm_prepare(wm);
}

// On MS-Windows, make this a global, so it's available to WndProc()
//struct Wm wm;

// Return 1 (true) if all layer names specified in check_names
// can be found in given layer properties.

static VkBool32 wm_check_layers(uint32_t check_count, char **check_names,
		uint32_t layer_count, VkLayerProperties *layers) {
	for(uint32_t i = 0; i < check_count; i++) {
		VkBool32 found = 0;
		for(uint32_t j = 0; j < layer_count; j++) {
			if(!strcmp(check_names[i], layers[j].layerName)) {
				found = 1;
				break;
			}
		}
		if(!found) {
			fprintf(stderr,
				"Cannot find layer: %s\n", check_names[i]);
			return 0;
		}
	}
	return 1;
}

static void wm_init_vk(struct Wm *wm) {
	VkResult err;
	uint32_t instance_extension_count = 0;
	uint32_t instance_layer_count = 0;
	char *instance_validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
	wm->enabled_extension_count = 0;
	wm->enabled_layer_count = 0;
	wm->is_minimized = false;
	wm->cmd_pool = VK_NULL_HANDLE;

	// Look for validation layers
	VkBool32 validation_found = 0;
	if(wm->validate) {
		err =
		vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
		assert(!err);

		if(instance_layer_count > 0) {
			VkLayerProperties *instance_layers =
					malloc(sizeof(VkLayerProperties) *
					instance_layer_count);
			err = vkEnumerateInstanceLayerProperties(
					&instance_layer_count, instance_layers);
			assert(!err);

			validation_found = wm_check_layers(ARRAY_SIZE(
					instance_validation_layers),
					instance_validation_layers,
					instance_layer_count,
					instance_layers);
			if(validation_found) {
				wm->enabled_layer_count =
					ARRAY_SIZE(instance_validation_layers);
				wm->enabled_layers[0] =
						"VK_LAYER_KHRONOS_validation";
			}
			free(instance_layers);
		}

		if(!validation_found) {
			ERR_EXIT("vkEnumerateInstanceLayerProperties"
				" failed to find required validation layer.\n\n"
				"Please look at the Getting Started guide "
				"for additional information.\n",
				"vkCreateInstance Failure");
		}
	}

	// Look for instance extensions
	VkBool32 surfaceExtFound = 0;
	VkBool32 platformSurfaceExtFound = 0;
	memset(wm->extension_names, 0, sizeof(wm->extension_names));

	err = vkEnumerateInstanceExtensionProperties(NULL,
			&instance_extension_count, NULL);
	assert(!err);

	if(instance_extension_count > 0) {
		VkExtensionProperties *instance_extensions =
				malloc(sizeof(VkExtensionProperties) *
				instance_extension_count);
		err = vkEnumerateInstanceExtensionProperties(NULL,
			&instance_extension_count, instance_extensions);
		assert(!err);
		for(uint32_t i = 0; i < instance_extension_count; i++) {
			if(!strcmp(VK_KHR_SURFACE_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
				surfaceExtFound = 1;
				wm->extension_names[
					wm->enabled_extension_count++] =
					VK_KHR_SURFACE_EXTENSION_NAME;
			}
			if(!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME,
				instance_extensions[i].extensionName)) {
				platformSurfaceExtFound = 1;
				wm->extension_names[
					wm->enabled_extension_count++] =
					VK_KHR_XCB_SURFACE_EXTENSION_NAME;
			}
			if(!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
					instance_extensions[i].extensionName)) {
				if(wm->validate) {
					wm->extension_names[
					wm->enabled_extension_count++] =
					VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
				}
			}
			assert(wm->enabled_extension_count < 64);
		}

		free(instance_extensions);
	}

	if(!surfaceExtFound) {
		ERR_EXIT("vkEnumerateInstanceExtensionProperties"
			" failed to find the " VK_KHR_SURFACE_EXTENSION_NAME
			" extension.\n\n"
			"Do you have a compatible Vulkan "
			"installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for "
			"additional information.\n",
			"vkCreateInstance Failure");
	}
	if(!platformSurfaceExtFound) {
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed "
		"to find the " VK_KHR_XCB_SURFACE_EXTENSION_NAME
		" extension.\n\n"
		"Do you have a compatible Vulkan installable "
		"client driver (ICD) installed?\n"
		"Please look at the Getting Started guide for "
		"additional information.\n",
		"vkCreateInstance Failure");
	}
	const VkApplicationInfo app = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = APP_SHORT_NAME,
		.applicationVersion = 0,
		.pEngineName = APP_SHORT_NAME,
		.engineVersion = 0,
		.apiVersion = VK_API_VERSION_1_0,
	};
	VkInstanceCreateInfo inst_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.pApplicationInfo = &app,
		.enabledLayerCount = wm->enabled_layer_count,
		.ppEnabledLayerNames =
			(const char *const *)instance_validation_layers,
		.enabledExtensionCount = wm->enabled_extension_count,
		.ppEnabledExtensionNames =
			(const char *const *)wm->extension_names,
	};

	// This is info for a temp callback to use during CreateInstance.
	// After the instance is created, we use the instance-based
	// function to register the final callback.

	VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info;
	if(wm->validate) {
		// VK_EXT_debug_utils style
		dbg_messenger_create_info.sType =
			VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		dbg_messenger_create_info.pNext = NULL;
		dbg_messenger_create_info.flags = 0;
		dbg_messenger_create_info.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		dbg_messenger_create_info.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		dbg_messenger_create_info.pfnUserCallback =
			debug_messenger_callback;
		dbg_messenger_create_info.pUserData = wm;
		inst_info.pNext = &dbg_messenger_create_info;
	}

	uint32_t gpu_count;

	err = vkCreateInstance(&inst_info, NULL, &wm->inst);
	if(err == VK_ERROR_INCOMPATIBLE_DRIVER) {
		ERR_EXIT("Cannot find a compatible Vulkan "
			"installable client driver (ICD).\n\n"
			"Please look at the Getting Started guide "
			"for additional information.\n",
			"vkCreateInstance Failure");
	} else if(err == VK_ERROR_EXTENSION_NOT_PRESENT) {
		ERR_EXIT("Cannot find a specified extension library.\n"
			"Make sure your layers path is set appropriately.\n",
			"vkCreateInstance Failure");
	} else if(err) {
		ERR_EXIT("vkCreateInstance failed.\n\n"
			"Do you have a compatible Vulkan installable "
			"client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for "
			"additional information.\n",
			"vkCreateInstance Failure");
	}

	// Make initial call to query gpu_count, then second call for gpu info
	err = vkEnumeratePhysicalDevices(wm->inst, &gpu_count, NULL);
	assert(!err);

	if(gpu_count > 0) {
		VkPhysicalDevice *physical_devices =
				malloc(sizeof(VkPhysicalDevice) * gpu_count);
		err = vkEnumeratePhysicalDevices(wm->inst, &gpu_count,
				physical_devices);
		assert(!err);
		// For cube demo we just grab the first physical device
		wm->gpu = physical_devices[0];
		free(physical_devices);
	} else {
		ERR_EXIT("vkEnumeratePhysicalDevices reported "
			"zero accessible devices.\n\n"
			"Do you have a compatible Vulkan installable "
			"client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for "
			"additional information.\n",
			"vkEnumeratePhysicalDevices Failure");
	}

	// Look for device extensions
	uint32_t device_extension_count = 0;
	VkBool32 swapchainExtFound = 0;
	wm->enabled_extension_count = 0;
	memset(wm->extension_names, 0, sizeof(wm->extension_names));

	err = vkEnumerateDeviceExtensionProperties(wm->gpu, NULL,
			&device_extension_count, NULL);
	assert(!err);

	if(device_extension_count > 0) {
		VkExtensionProperties *device_extensions =
		malloc(sizeof(VkExtensionProperties) * device_extension_count);
		err = vkEnumerateDeviceExtensionProperties(wm->gpu, NULL,
				&device_extension_count, device_extensions);
		assert(!err);

		for(uint32_t i = 0; i < device_extension_count; i++) {
			if(!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
					device_extensions[i].extensionName)) {
				swapchainExtFound = 1;
				wm->extension_names[
						wm->enabled_extension_count++] =
						VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			}
			assert(wm->enabled_extension_count < 64);
		}

		if(wm->VK_KHR_incremental_present_enabled) {
            // Even though the user "enabled" the extension via the command
            // line, we must make sure that it's enumerated for use with the
            // device.  Therefore, disable it here, and re-enable it again if
            // enumerated.
			wm->VK_KHR_incremental_present_enabled = false;
			for(uint32_t i = 0; i < device_extension_count; i++) {
				if(!strcmp(
				VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME,
				device_extensions[i].extensionName)) {
					wm->extension_names[
					wm->enabled_extension_count++] =
				VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME;
					wm->VK_KHR_incremental_present_enabled =
							true;
					DbgMsg(
			"VK_KHR_incremental_present extension enabled\n");
				}
				assert(wm->enabled_extension_count < 64);
			}
			if(!wm->VK_KHR_incremental_present_enabled) {
		DbgMsg("VK_KHR_incremental_present extension NOT AVAILABLE\n");
			}
		}

		if(wm->VK_GOOGLE_display_timing_enabled) {
            // Even though the user "enabled" the extension via the command
            // line, we must make sure that it's enumerated for use with the
            // device.  Therefore, disable it here, and re-enable it again if
            // enumerated.
			wm->VK_GOOGLE_display_timing_enabled = false;
			for(uint32_t i = 0; i < device_extension_count; i++) {
				if(!strcmp(
					VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME,
					device_extensions[i].extensionName)) {
					wm->extension_names[
						wm->enabled_extension_count++] =
					VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME;
				wm->VK_GOOGLE_display_timing_enabled = true;
			DbgMsg("VK_GOOGLE_display_timing extension enabled\n");
				}
				assert(wm->enabled_extension_count < 64);
			}
			if(!wm->VK_GOOGLE_display_timing_enabled) {
		DbgMsg("VK_GOOGLE_display_timing extension NOT AVAILABLE\n");
			}
		}

		free(device_extensions);
	}

	if(!swapchainExtFound) {
		ERR_EXIT("vkEnumerateDeviceExtensionProperties "
			"failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
			" extension.\n\nDo you have a compatible Vulkan "
			"installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for "
			"additional information.\n",
			"vkCreateInstance Failure");
	}

	if(wm->validate) {
	// Setup VK_EXT_debug_utils function pointers always (we use them for
	// debug labels and names).
		wm->CreateDebugUtilsMessengerEXT =
			(PFN_vkCreateDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkCreateDebugUtilsMessengerEXT");
		wm->DestroyDebugUtilsMessengerEXT =
			(PFN_vkDestroyDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkDestroyDebugUtilsMessengerEXT");
		wm->SubmitDebugUtilsMessageEXT =
			(PFN_vkSubmitDebugUtilsMessageEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkSubmitDebugUtilsMessageEXT");
		wm->CmdBeginDebugUtilsLabelEXT =
			(PFN_vkCmdBeginDebugUtilsLabelEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkCmdBeginDebugUtilsLabelEXT");
		wm->CmdEndDebugUtilsLabelEXT =
			(PFN_vkCmdEndDebugUtilsLabelEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkCmdEndDebugUtilsLabelEXT");
		wm->CmdInsertDebugUtilsLabelEXT =
			(PFN_vkCmdInsertDebugUtilsLabelEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkCmdInsertDebugUtilsLabelEXT");
		wm->SetDebugUtilsObjectNameEXT =
			(PFN_vkSetDebugUtilsObjectNameEXT)
			vkGetInstanceProcAddr(wm->inst,
			"vkSetDebugUtilsObjectNameEXT");
		if(NULL == wm->CreateDebugUtilsMessengerEXT ||
				NULL == wm->DestroyDebugUtilsMessengerEXT ||
				NULL == wm->SubmitDebugUtilsMessageEXT ||
				NULL == wm->CmdBeginDebugUtilsLabelEXT ||
				NULL == wm->CmdEndDebugUtilsLabelEXT ||
				NULL == wm->CmdInsertDebugUtilsLabelEXT ||
				NULL == wm->SetDebugUtilsObjectNameEXT) {
			ERR_EXIT("GetProcAddr: Failed to init "
			"VK_EXT_debug_utils\n", "GetProcAddr: Failure");
		}

		err = wm->CreateDebugUtilsMessengerEXT(wm->inst,
			&dbg_messenger_create_info, NULL, &wm->dbg_messenger);
		switch(err) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			ERR_EXIT("CreateDebugUtilsMessengerEXT: out of host "
			"memory\n", "CreateDebugUtilsMessengerEXT Failure");
			break;
		default:
			ERR_EXIT("CreateDebugUtilsMessengerEXT: unknown "
			"failure\n", "CreateDebugUtilsMessengerEXT Failure");
			break;
		}
	}
	vkGetPhysicalDeviceProperties(wm->gpu, &wm->gpu_props);

	// Call with NULL data to get count
	vkGetPhysicalDeviceQueueFamilyProperties(wm->gpu,
			&wm->queue_family_count, NULL);
	assert(wm->queue_family_count >= 1);

	wm->queue_props = (VkQueueFamilyProperties *)malloc(
		wm->queue_family_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(wm->gpu,
		&wm->queue_family_count, wm->queue_props);

	// Query fine-grained feature support for this device.
	//  If app has specific feature requirements it should check supported
	//  features based on this query
	VkPhysicalDeviceFeatures physDevFeatures;
	vkGetPhysicalDeviceFeatures(wm->gpu, &physDevFeatures);

	GET_INSTANCE_PROC_ADDR(wm->inst, GetPhysicalDeviceSurfaceSupportKHR);
	GET_INSTANCE_PROC_ADDR(wm->inst,
			GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_INSTANCE_PROC_ADDR(wm->inst, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_INSTANCE_PROC_ADDR(wm->inst,
			GetPhysicalDeviceSurfacePresentModesKHR);
	GET_INSTANCE_PROC_ADDR(wm->inst, GetSwapchainImagesKHR);
}

static void wm_create_device(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;
	float queue_priorities[1] = {0.0};
	VkDeviceQueueCreateInfo queues[2];
	queues[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queues[0].pNext = NULL;
	queues[0].queueFamilyIndex = wm->graphics_queue_family_index;
	queues[0].queueCount = 1;
	queues[0].pQueuePriorities = queue_priorities;
	queues[0].flags = 0;

	VkDeviceCreateInfo device = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = queues,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = wm->enabled_extension_count,
		.ppEnabledExtensionNames =
				(const char *const *)wm->extension_names,
		.pEnabledFeatures = NULL,  // If specific features are required,
		// pass them in here
	};
	if(wm->separate_present_queue) {
		queues[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queues[1].pNext = NULL;
		queues[1].queueFamilyIndex = wm->present_queue_family_index;
		queues[1].queueCount = 1;
		queues[1].pQueuePriorities = queue_priorities;
		queues[1].flags = 0;
		device.queueCreateInfoCount = 2;
	}
	err = vkCreateDevice(wm->gpu, &device, NULL, &wm->device);
	assert(!err);
}

static void wm_create_surface(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;

	// Create a WSI surface for the window:
	VkXcbSurfaceCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.connection = wm->connection;
	createInfo.window = wm->xcb_window;

	err = vkCreateXcbSurfaceKHR(wm->inst, &createInfo, NULL, &wm->surface);
	assert(!err);
}

static void wm_init_vk_swapchain(struct Wm *wm) {
	VkResult U_ASSERT_ONLY err;

	wm_create_surface(wm);

	// Iterate over each queue to learn whether it supports presenting:
	VkBool32 *supportsPresent = (VkBool32 *)malloc(
			wm->queue_family_count * sizeof(VkBool32));
	for(uint32_t i = 0; i < wm->queue_family_count; i++) {
		wm->fpGetPhysicalDeviceSurfaceSupportKHR(wm->gpu, i,
				wm->surface, &supportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t graphicsQueueFamilyIndex = UINT32_MAX;
	uint32_t presentQueueFamilyIndex = UINT32_MAX;
	for(uint32_t i = 0; i < wm->queue_family_count; i++) {
		if((wm->queue_props[i].queueFlags &
				VK_QUEUE_GRAPHICS_BIT) != 0) {
			if(graphicsQueueFamilyIndex == UINT32_MAX) {
				graphicsQueueFamilyIndex = i;
			}

			if(supportsPresent[i] == VK_TRUE) {
				graphicsQueueFamilyIndex = i;
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	if(presentQueueFamilyIndex == UINT32_MAX) {
		// If didn't find a queue that supports both graphics and
		// present, then find a separate present queue.
		for(uint32_t i = 0; i < wm->queue_family_count; ++i) {
			if(supportsPresent[i] == VK_TRUE) {
				presentQueueFamilyIndex = i;
				break;
			}
		}
	}

	// Generate error if could not find both a graphics and a present queue
	if(graphicsQueueFamilyIndex == UINT32_MAX ||
			presentQueueFamilyIndex == UINT32_MAX) {
		ERR_EXIT("Could not find both graphics and present queues\n",
				"Swapchain Initialization Failure");
	}

	wm->graphics_queue_family_index = graphicsQueueFamilyIndex;
	wm->present_queue_family_index = presentQueueFamilyIndex;
	wm->separate_present_queue = (wm->graphics_queue_family_index !=
			wm->present_queue_family_index);
	free(supportsPresent);

	wm_create_device(wm);

	GET_DEVICE_PROC_ADDR(wm->device, CreateSwapchainKHR);
	GET_DEVICE_PROC_ADDR(wm->device, DestroySwapchainKHR);
	GET_DEVICE_PROC_ADDR(wm->device, GetSwapchainImagesKHR);
	GET_DEVICE_PROC_ADDR(wm->device, AcquireNextImageKHR);
	GET_DEVICE_PROC_ADDR(wm->device, QueuePresentKHR);
	if(wm->VK_GOOGLE_display_timing_enabled) {
		GET_DEVICE_PROC_ADDR(wm->device, GetRefreshCycleDurationGOOGLE);
		GET_DEVICE_PROC_ADDR(wm->device,
				GetPastPresentationTimingGOOGLE);
	}

	vkGetDeviceQueue(wm->device, wm->graphics_queue_family_index, 0,
			&wm->graphics_queue);

	if(!wm->separate_present_queue) {
		wm->present_queue = wm->graphics_queue;
	} else {
		vkGetDeviceQueue(wm->device, wm->present_queue_family_index, 0,
				&wm->present_queue);
	}

	// Get the list of VkFormat's that are supported:
	uint32_t formatCount;
	err = wm->fpGetPhysicalDeviceSurfaceFormatsKHR(wm->gpu, wm->surface,
			&formatCount, NULL);
	assert(!err);
	VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(
			formatCount * sizeof(VkSurfaceFormatKHR));
	err = wm->fpGetPhysicalDeviceSurfaceFormatsKHR(wm->gpu, wm->surface,
			&formatCount, surfFormats);
	assert(!err);
	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if(formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
		wm->format = VK_FORMAT_B8G8R8A8_UNORM;
	} else {
		assert(formatCount >= 1);
		wm->format = surfFormats[0].format;
	}
	wm->color_space = surfFormats[0].colorSpace;
	free(surfFormats);

	wm->quit = false;
	wm->curFrame = 0;

	// Create semaphores to synchronize acquiring presentable buffers before
	// rendering and waiting for drawing to be complete before presenting
	VkSemaphoreCreateInfo semaphoreCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
	};

	// Create fences that we can use to throttle if we get too far
	// ahead of the image presents
	VkFenceCreateInfo fence_ci = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL, .flags = VK_FENCE_CREATE_SIGNALED_BIT};
	for(uint32_t i = 0; i < FRAME_LAG; i++) {
		err = vkCreateFence(wm->device, &fence_ci, NULL,
				&wm->fences[i]);
		assert(!err);

		err = vkCreateSemaphore(wm->device, &semaphoreCreateInfo, NULL,
				&wm->image_acquired_semaphores[i]);
		assert(!err);

		err = vkCreateSemaphore(wm->device, &semaphoreCreateInfo, NULL,
				&wm->draw_complete_semaphores[i]);
		assert(!err);

		if(wm->separate_present_queue) {
			err = vkCreateSemaphore(wm->device,
				&semaphoreCreateInfo, NULL,
				&wm->image_ownership_semaphores[i]);
			assert(!err);
		}
	}
	wm->frame_index = 0;

	// Get Memory information and properties
	vkGetPhysicalDeviceMemoryProperties(wm->gpu, &wm->memory_properties);
}

static void wm_init_connection(struct Wm *wm);

static void wm_init(struct Wm *wm, int argc, char **argv) {
	vec3 eye = {0.0f, 3.0f, 5.0f};
	vec3 origin = {0, 0, 0};
	vec3 up = {0.0f, 1.0f, 0.0};

	memset(wm, 0, sizeof(*wm));
	wm->presentMode = VK_PRESENT_MODE_FIFO_KHR;
	wm->frameCount = INT32_MAX;

	wm_init_connection(wm);

	wm_init_vk(wm);

	wm->width = 500;
	wm->height = 500;

	wm->spin_angle = 4.0f;
	wm->spin_increment = 0.2f;
	wm->pause = false;

	mat4x4_perspective(wm->projection_matrix,
			(float)degreesToRadians(45.0f), 1.0f, 0.1f, 100.0f);
	mat4x4_look_at(wm->view_matrix, eye, origin, up);
	mat4x4_identity(wm->model_matrix);

	wm->projection_matrix[1][1] *= -1;  // Flip projection matrix from GL to Vulkan orientation.
}

/*
int main(int argc, char **argv) {
    struct demo demo;

    demo_init(&demo, argc, argv);
#if defined(VK_USE_PLATFORM_XCB_KHR)
    demo_create_xcb_window(&demo);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    demo_create_xlib_window(&demo);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    demo_create_window(&demo);
#endif

    demo_init_vk_swapchain(&demo);

    demo_prepare(&demo);

#if defined(VK_USE_PLATFORM_XCB_KHR)
    demo_run_xcb(&demo);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    demo_run_xlib(&demo);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    demo_run(&demo);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
    demo_run_display(&demo);
#endif

    demo_cleanup(&demo);

    return validation_error;
}
#endif
*/
