#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <iostream>


#ifdef	_WIN32
#include "wrapper_win32.hpp"
#endif

// Win32 = HWND
// Xlib  = 
using	Window	= vkexample::window;


thread_local VkResult	lastError;


struct	queue_family_index
{
	uint32_t						graphics	= UINT32_MAX;
	uint32_t						present		= UINT32_MAX;
};


struct	surface_data
{
	VkSurfaceKHR					handle		= VK_NULL_HANDLE;
	VkFormat						format		= VK_FORMAT_UNDEFINED;
};


struct	framebuffer_data
{
	VkImage							image		= VK_NULL_HANDLE;
	VkImageView						view		= VK_NULL_HANDLE;
};



struct	window_surface
{
	uint32_t						frameIndex		= 0;
	surface_data					surface;
	VkSwapchainKHR					swapChain		= VK_NULL_HANDLE;
	std::vector<framebuffer_data>	frameBuffers;
};




// ****************************************************************************
//! Initialize
// ----------------------------------------------------------------------------
VkInstance						create_vkInstance(char const* _appname, char const* _enginename, size_t _extension, char const*const* _extensions);
VkPhysicalDevice				create_vkPhysicalDevice(VkInstance _instance);
surface_data					create_vkSurface(VkInstance _instance, VkPhysicalDevice _gpu, Window const& _window);
queue_family_index				get_queueFamilyIndex(VkPhysicalDevice _gpu, VkSurfaceKHR _surface);
VkDevice						create_vkDevice(VkPhysicalDevice _gpu, uint32_t _family);
VkSwapchainKHR					create_vkSwapchain(VkPhysicalDevice _gpu, VkDevice _device, VkSurfaceKHR _surface, VkFormat _format, int _graphicsQueue, int _presentQueue);
VkRenderPass					create_vkRenderPass(VkDevice _device, VkFormat _format);
std::vector<framebuffer_data>	get_frameBuffer(VkDevice _device, VkSwapchainKHR _swapChain, VkFormat _surfaceFormat, uint32_t _width, uint32_t _height);


// ****************************************************************************
//! Util
// ----------------------------------------------------------------------------
VkFence							create_vkFence(VkDevice _device);
VkCommandPool					create_vkCommandPool(VkDevice _device, int _queueFamily);
VkCommandBuffer					create_vkCommandBuffer(VkDevice _device, VkCommandPool _pool, VkCommandBufferLevel _level);
VkFramebuffer					create_framebuffer(VkDevice _device, VkRenderPass _renderpass, VkImageView _view, VkExtent2D _size);
VkExtent2D						get_surface_size(VkPhysicalDevice _gpu, VkSurfaceKHR _surface);


// ****************************************************************************
//! CommandBuffer Util
// ----------------------------------------------------------------------------
void cmdBeginCommandBuffer(VkCommandBuffer _cmdbuff, VkFramebuffer _framebuffer);
void cmdBeginRenderPass(VkCommandBuffer _cmdbuff, VkRenderPass _renderpass, VkFramebuffer _framebuffer, VkExtent2D _size, VkClearColorValue _color);
void cmdQueueSubmit(VkQueue _queue, VkCommandBuffer _cmdbuff, VkFence _fence);


// ****************************************************************************
//! Queue Util
// ----------------------------------------------------------------------------
void queuePresent(VkQueue _queue, VkSwapchainKHR _swapChain, uint32_t _frameIndex);



int main(int _argc, char* _argv[])
{
	VkInstance			instance		= VK_NULL_HANDLE;
	VkPhysicalDevice	gpu				= VK_NULL_HANDLE;

	window_surface		ws;

	queue_family_index	queueFamilyIndex;

	VkDevice			device			= VK_NULL_HANDLE;

	VkClearColorValue	clearColor		= {0,0,0,1};

	vkexample::main_loop
	(
		// Window size.
		640, 480,

		// Created.
		[&](Window window)
		{
			// GPUを取得する
			auto	instExtension	= vkexample::get_instance_extensions();
			instance		= create_vkInstance("Exapmple", "ExampleEngine", instExtension.size(), instExtension.data());
			gpu				= create_vkPhysicalDevice(instance);

			// ウィンドウに関連付けされているサーフェースを取得する
			ws.surface			= create_vkSurface(instance, gpu, window);

			// グラフィックスキューと、ウィンドウサーフェースに対するPresent能力が持つキューを取得する
			queueFamilyIndex	= get_queueFamilyIndex(gpu, ws.surface.handle);
			std::cout << "Graphics Queue Family Index : "<< queueFamilyIndex.graphics << std::endl;
			std::cout << "Present  Queue Family Index : "<< queueFamilyIndex.present << std::endl;

			// グラフィックスキューを持つデバイスを取得する
			device				= create_vkDevice(gpu, queueFamilyIndex.graphics);

			// プレゼントキューに対してウィンドウサーフェースに対するスワップチェーンを作成する
			ws.swapChain		= create_vkSwapchain(gpu, device, ws.surface.handle, ws.surface.format, queueFamilyIndex.graphics, queueFamilyIndex.present);

			// スワップチェーンに紐づいたフレームバッファを取得する
			VkExtent2D	size	= get_surface_size(gpu, ws.surface.handle);
			ws.frameBuffers		= get_frameBuffer(device, ws.swapChain, ws.surface.format, size.width, size.height);
		},

		// Update.
		[&]()
		{
			VkFence			fence;

			
			// 次に処理すべきフレームバッファのインデックスを取得する
			fence		= create_vkFence(device);
			{
				vkAcquireNextImageKHR(device, ws.swapChain, UINT64_MAX, VK_NULL_HANDLE, fence, &ws.frameIndex);
				vkWaitForFences(device, 1, &fence, VK_FALSE, UINT64_MAX);
			}
			vkDestroyFence(device, fence, nullptr);


			// 描画用のレンダーパスと、描画先の情報（フレームバッファ）を作成。
			VkExtent2D					size		= get_surface_size(gpu, ws.surface.handle);
			VkRenderPass				renderPass	= create_vkRenderPass(device, ws.surface.format);
			VkFramebuffer				frameBuffer	= create_framebuffer(device, renderPass, ws.frameBuffers[ws.frameIndex].view, size);


			// グラフィックスキューに対するコマンドプールを作成
			VkCommandPool				cmdPool		= create_vkCommandPool(device, queueFamilyIndex.graphics);
			VkCommandBuffer				cmdBuff		= create_vkCommandBuffer(device, cmdPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);


			// コマンドバッファの登録
			cmdBeginCommandBuffer(cmdBuff, frameBuffer);
				// レンダーパスの登録
				cmdBeginRenderPass(cmdBuff, renderPass, frameBuffer, size, clearColor);
				vkCmdEndRenderPass(cmdBuff);
			vkEndCommandBuffer(cmdBuff);


			// 登録が終わったらレンダーパスとフレームバッファは削除できる
			vkDestroyFramebuffer(device, frameBuffer, nullptr);
			vkDestroyRenderPass(device, renderPass, nullptr);


			// 各種グラフィックスコマンドを書き込むキューを取得する
			VkQueue	grpQueue;
			vkGetDeviceQueue(device, queueFamilyIndex.graphics, 0, &grpQueue);

			// コマンドをサブミット
			fence		= create_vkFence(device);
			{
				cmdQueueSubmit(grpQueue, cmdBuff, fence);
				vkWaitForFences(device, 1, &fence, VK_FALSE, UINT64_MAX);
			}
			vkDestroyFence(device, fence, nullptr);


			// コマンドバッファ＆コマンドプールを破棄
			vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuff);		// サブミットが完了したらコマンドバッファは不要。
			vkDestroyCommandPool(device, cmdPool, nullptr);			// コマンドバッファがすべてなくなったらコマンドプールも削除できる





			// Present Queue を使ってプレゼントを行う
			queuePresent(grpQueue, ws.swapChain, ws.frameIndex);



			// 動作がわかりやすくなるように次フレームで画面を塗りつぶす色を変更する
			if ((clearColor.float32[2] += 0.01f) > 1) { clearColor.float32[2] = 0; }
		}
	);

	// 各リソースの破棄
	for (auto& f : ws.frameBuffers)
	{
		vkDestroyImageView(device, f.view, nullptr);
	}
	vkDestroySwapchainKHR(device, ws.swapChain, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);
	

	return	0;
}



VkInstance create_vkInstance(char const* _appname, char const* _enginename, size_t _extension, char const*const* _extensions)
{
	VkApplicationInfo		app = {};
	app.sType							= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app.pNext							= nullptr;
	app.pApplicationName				= _appname;
	app.applicationVersion				= 1;
	app.pEngineName						= _enginename;
	app.engineVersion					= 1,
	app.apiVersion						= VK_API_VERSION_1_0;


	VkInstanceCreateInfo	inst_info = {};
	inst_info.sType						= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pNext						= nullptr;
	inst_info.pApplicationInfo			= &app;
	inst_info.enabledLayerCount			= 0;
	inst_info.ppEnabledLayerNames		= nullptr;
	inst_info.enabledExtensionCount		= static_cast<uint32_t>(_extension);
	inst_info.ppEnabledExtensionNames	= _extensions;

	VkInstance	retval	= VK_NULL_HANDLE;
	lastError	= vkCreateInstance(&inst_info, nullptr, &retval);
	return	retval;
}


VkPhysicalDevice create_vkPhysicalDevice(VkInstance _instance)
{
	uint32_t						deviceNums;
	std::vector<VkPhysicalDevice>	devices;
	lastError	= vkEnumeratePhysicalDevices(_instance, &deviceNums, nullptr);

	devices.resize(static_cast<size_t>(deviceNums));
	lastError	= vkEnumeratePhysicalDevices(_instance, &deviceNums, devices.data());

	return	devices.front();
}


surface_data create_vkSurface(VkInstance _instance, VkPhysicalDevice _gpu, Window const& _window)
{
	surface_data	retval;

	// Surfaceの取得はプラットフォームによって異なる。
	lastError	= vkexample::create_vkSurfaceWithPlatform(&retval.handle, _instance, _window);


	// Formatを取得
	uint32_t						formatCount;
	std::vector<VkSurfaceFormatKHR>	formats;

	lastError	= vkGetPhysicalDeviceSurfaceFormatsKHR(_gpu, retval.handle, &formatCount, nullptr);		
	if (formatCount == 0)
	{
		// おそらく VkInstance に VK_KHR_SURFACE_EXTENSION_NAME がセットされていない
		lastError = VK_ERROR_INITIALIZATION_FAILED;
		return	retval;
	}


	formats.resize((size_t)formatCount);
	lastError	= vkGetPhysicalDeviceSurfaceFormatsKHR(_gpu, retval.handle, &formatCount, formats.data());

	if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		retval.format	= VK_FORMAT_B8G8R8A8_UNORM;;
	}
	else
	{
		retval.format	= formats[0].format;;
	}
	return	retval;
}


queue_family_index get_queueFamilyIndex(VkPhysicalDevice _gpu, VkSurfaceKHR _surface)
{
	queue_family_index	retval;

	uint32_t								count;
	std::vector<VkQueueFamilyProperties>	props;

	vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &count, nullptr);
	props.resize((size_t)count);
	vkGetPhysicalDeviceQueueFamilyProperties(_gpu, &count, props.data());

	// Graphics能力を持つキューを取得する
	for (uint32_t index = 0; index < props.size(); ++index)
	{
		if (props[index].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			retval.graphics	= index;
			break;
		}
	}

	// サーフェースに対するPresent能力を持つキューを取得する
	for (uint32_t index = 0; index < props.size(); ++index)
	{
		VkBool32	spf;
		vkGetPhysicalDeviceSurfaceSupportKHR(_gpu, index, _surface, &spf);
		if (spf)
		{
			retval.present	= index;
//			break;
			// テストのために graphicsとpresentでなるべく違うキューを使ってみる
		}
	}

	// ※ Graphics と Present は概ね同じものを指すが、マルチGPUなどでは Present が異なる場合がある。

	return	retval;
}


VkDevice create_vkDevice(VkPhysicalDevice _gpu, uint32_t _family)
{
	std::vector<const char *>		extensionNames
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};


	VkDeviceQueueCreateInfo	dqci	= {};
	float					qp[]	= {0.0f};
	dqci.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	dqci.pNext				= nullptr;
	dqci.queueFamilyIndex 	= _family;
	dqci.queueCount			= 1;
	dqci.pQueuePriorities 	= qp;

	VkDeviceCreateInfo		ci		= {};
	ci.sType					= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	ci.pNext					= nullptr;
	ci.queueCreateInfoCount		= 1;
	ci.pQueueCreateInfos		= &dqci;
	ci.enabledExtensionCount	= extensionNames.size();
	ci.ppEnabledExtensionNames	= extensionNames.data();
	ci.pEnabledFeatures			= nullptr;

	VkDevice	handle	= VK_NULL_HANDLE;
	lastError	= vkCreateDevice(_gpu, &ci, NULL, &handle);
	return	handle;
}


VkSwapchainKHR create_vkSwapchain(VkPhysicalDevice _gpu, VkDevice _device, VkSurfaceKHR _targetSurface, VkFormat _format, int _graphicsQueue, int _presentQueue)
{
	// サーフェースに対するスワップチェーンを作成するために、サーフェースの能力を調べる。
	VkSurfaceCapabilitiesKHR	targetCaps;
	lastError	= vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_gpu, _targetSurface, &targetCaps);


	VkSwapchainCreateInfoKHR	ci{};
	ci.sType					= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.pNext					= nullptr;
	ci.surface					= _targetSurface;
	ci.minImageCount			= targetCaps.minImageCount;

	ci.imageArrayLayers			= 1;
	ci.imageUsage				= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	ci.imageFormat				= _format;
	ci.imageColorSpace			= VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	// ターゲットのサイズが不明な時は min, max から適切な値を設定する
	ci.imageExtent				= targetCaps.currentExtent;
	if (ci.imageExtent.width == UINT32_MAX)
	{
		ci.imageExtent.width		= ci.imageExtent.width < targetCaps.minImageExtent.width ? ci.imageExtent.width : targetCaps.minImageExtent.width;
		ci.imageExtent.width		= ci.imageExtent.width > targetCaps.minImageExtent.width ? ci.imageExtent.width : targetCaps.minImageExtent.width;
	}
	if (ci.imageExtent.height == UINT32_MAX)
	{
		ci.imageExtent.height		= ci.imageExtent.height < targetCaps.minImageExtent.height ? ci.imageExtent.height : targetCaps.minImageExtent.height;
		ci.imageExtent.height		= ci.imageExtent.height > targetCaps.minImageExtent.height ? ci.imageExtent.height : targetCaps.minImageExtent.height;
	}

	// サーフェースの回転方向
	ci.preTransform				= targetCaps.currentTransform;

	// アルファ情報
	ci.compositeAlpha			= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;		// Alphaは常に1.0として扱う
	VkCompositeAlphaFlagBitsKHR	compositeAlphaFlags[] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (VkCompositeAlphaFlagBitsKHR flg : compositeAlphaFlags)
	{
		if (targetCaps.supportedCompositeAlpha & flg)
		{
			ci.compositeAlpha = flg;
			break;
		}
	}

	ci.presentMode				= VK_PRESENT_MODE_FIFO_KHR;
	ci.oldSwapchain				= VK_NULL_HANDLE;
	ci.clipped					= true;

	uint32_t					queueFamilyIndices[2]	= {(uint32_t)_graphicsQueue, (uint32_t)_presentQueue};
	if (_graphicsQueue == _presentQueue)
	{
		// グラフィックスとプレゼントが同じキューならスワップチェーンを占有モードで作成できる
		ci.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		ci.queueFamilyIndexCount	= 0;
		ci.pQueueFamilyIndices		= nullptr;
	}
	else
	{
		// グラフィックスとプレゼントが異なるキューならスワップチェーンをキュー同士で共有する
		ci.imageSharingMode			= VK_SHARING_MODE_CONCURRENT;
		ci.queueFamilyIndexCount	= 2;
		ci.pQueueFamilyIndices		= queueFamilyIndices;
	}

	VkSwapchainKHR	handle	= VK_NULL_HANDLE;
	lastError	= vkCreateSwapchainKHR(_device, &ci, nullptr, &handle);
	return	handle;
}


VkRenderPass create_vkRenderPass(VkDevice _device, VkFormat _format)
{
	VkAttachmentDescription	desc{};
	desc.format						= _format;
	desc.samples					= VK_SAMPLE_COUNT_1_BIT;
	desc.loadOp						= VK_ATTACHMENT_LOAD_OP_CLEAR;
	desc.storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
	desc.initialLayout				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	desc.finalLayout				= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference	ref{};
	ref.attachment					= 0;
	ref.layout						= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount	= 1;
	subpass.pColorAttachments		= &ref;


	VkRenderPassCreateInfo ci{};
	ci.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	ci.attachmentCount				= 1;
	ci.pAttachments					= &desc;
	ci.subpassCount					= 1;
	ci.pSubpasses					= &subpass;

	VkRenderPass	handle	= VK_NULL_HANDLE;
	lastError	= vkCreateRenderPass(_device, &ci, nullptr, &handle);
	return	handle;
}


std::vector<framebuffer_data> get_frameBuffer(VkDevice _device, VkSwapchainKHR _swapChain, VkFormat _surfaceFormat, uint32_t _width, uint32_t _height)
{
	uint32_t				imageCount;
	std::vector<VkImage>	images;
	lastError	= vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	images.resize((size_t)imageCount);
	lastError	= vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, images.data());


	std::vector<framebuffer_data>	retval;
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		framebuffer_data	fb;
		fb.image	= images[i];

		// ImageView
		VkImageViewCreateInfo	vci = {};
		vci.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vci.pNext							= nullptr;
		vci.image							= VK_NULL_HANDLE;
		vci.format							= _surfaceFormat;
		vci.components.r					= VK_COMPONENT_SWIZZLE_R;
		vci.components.g					= VK_COMPONENT_SWIZZLE_G;
		vci.components.b					= VK_COMPONENT_SWIZZLE_B;
		vci.components.a					= VK_COMPONENT_SWIZZLE_A;
		vci.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_DEPTH_BIT;
		vci.subresourceRange.baseMipLevel	= 0;
		vci.subresourceRange.levelCount		= 1;
		vci.subresourceRange.baseArrayLayer	= 0;
		vci.subresourceRange.layerCount		= 1;
		vci.viewType						= VK_IMAGE_VIEW_TYPE_2D;
		vci.flags							= 0;
		vci.image							= fb.image;
		lastError	= vkCreateImageView(_device, &vci, NULL, &fb.view);

		retval.push_back(std::move(fb));
	}

	return	retval;
}


VkFence create_vkFence(VkDevice _device)
{
	VkFenceCreateInfo	ci{};
	ci.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence	handle	= VK_NULL_HANDLE;
	lastError	= vkCreateFence(_device, &ci, nullptr, &handle);
	return	handle;
}


VkCommandPool create_vkCommandPool(VkDevice _device, int _queueFamily)
{
	VkCommandPoolCreateInfo	ci{};
	ci.sType				= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	ci.pNext				= nullptr;
	ci.queueFamilyIndex		= _queueFamily;
	ci.flags				= 0;

	VkCommandPool	handle	= VK_NULL_HANDLE;
	lastError	= vkCreateCommandPool(_device, &ci, NULL, &handle);
	return	handle;
}


VkCommandBuffer create_vkCommandBuffer(VkDevice _device, VkCommandPool _pool, VkCommandBufferLevel _level)
{
	VkCommandBufferAllocateInfo	ai{};
	ai.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.pNext				= nullptr;
	ai.commandPool			= _pool;
	ai.level				= _level;
	ai.commandBufferCount	= 1;

	VkCommandBuffer	handle	= VK_NULL_HANDLE;
	lastError	= vkAllocateCommandBuffers(_device, &ai, &handle);
	return	handle;
}


void cmdBeginCommandBuffer(VkCommandBuffer _cmdbuff, VkFramebuffer _framebuffer)
{
	VkCommandBufferInheritanceInfo 	ii{};
	ii.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	ii.framebuffer			= _framebuffer;

	VkCommandBufferBeginInfo		bi{};
	bi.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.pInheritanceInfo		= &ii;

	vkBeginCommandBuffer(_cmdbuff, &bi);
}


void cmdBeginRenderPass(VkCommandBuffer _cmdbuff, VkRenderPass _renderpass, VkFramebuffer _framebuffer, VkExtent2D _size, VkClearColorValue _color)
{
	VkClearValue			cv;
	cv.color		= _color;
	cv.depthStencil	= {0, 0};

	VkRenderPassBeginInfo	rpbi{};
	rpbi.sType						= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpbi.framebuffer				= _framebuffer;
	rpbi.renderPass					= _renderpass;
	rpbi.renderArea.extent.width	= _size.width;
	rpbi.renderArea.extent.height	= _size.height;
	rpbi.clearValueCount			= 1;
	rpbi.pClearValues				= &cv;
	vkCmdBeginRenderPass(_cmdbuff, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
}


void cmdQueueSubmit(VkQueue _queue, VkCommandBuffer _cmdbuff, VkFence _fence)
{
	VkSubmitInfo			si{};
	VkPipelineStageFlags	waitStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

	si.sType 				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	si.commandBufferCount	= 1;
	si.pCommandBuffers		= &_cmdbuff;
	si.pWaitDstStageMask	= &waitStageMask;
	lastError	= vkQueueSubmit(_queue, 1, &si, _fence);
}


void queuePresent(VkQueue _queue, VkSwapchainKHR _swapChain, uint32_t _frameIndex)
{
	VkPresentInfoKHR	pi{};
	pi.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pi.pNext				= nullptr;
	pi.swapchainCount		= 1;
	pi.pSwapchains			= &_swapChain;
	pi.pImageIndices		= &_frameIndex;
	vkQueuePresentKHR(_queue, &pi);
}


VkFramebuffer create_framebuffer(VkDevice _device, VkRenderPass _renderpass, VkImageView _view, VkExtent2D _size)
{
	VkFramebufferCreateInfo	fci{};
	fci.sType							= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fci.renderPass						= _renderpass;
	fci.width							= _size.width;
	fci.height							= _size.height;
	fci.layers							= 1;
	fci.attachmentCount					= 1;
	fci.pAttachments					= &_view;

	VkFramebuffer	handle	= VK_NULL_HANDLE;
	lastError	= vkCreateFramebuffer(_device, &fci, nullptr, &handle);
	return	handle;
}


VkExtent2D get_surface_size(VkPhysicalDevice _gpu, VkSurfaceKHR _surface)
{
	VkSurfaceCapabilitiesKHR	caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_gpu, _surface, &caps);
	return	caps.currentExtent;
}
