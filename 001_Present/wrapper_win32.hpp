#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#include <vector>
namespace vkexample
{

using	window	= ::HWND;

inline std::vector<char const*> get_instance_extensions()
{
	return	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME
	};
}


inline VkResult create_vkSurfaceWithPlatform(VkSurfaceKHR* _dest, VkInstance _instance, HWND _hwnd)
{
	VkWin32SurfaceCreateInfoKHR		ci = {};
	ci.sType		= VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	ci.pNext		= nullptr;
	ci.hinstance	= ::GetModuleHandle(nullptr);
	ci.hwnd			= _hwnd;

	*_dest			= VK_NULL_HANDLE;
	return	vkCreateWin32SurfaceKHR(_instance, &ci, nullptr, _dest);
}



template<class Created, class Update>
void main_loop(int _width, int _height, Created&& _created, Update&& _update)
{
	// ==============================================================
	// Windows Class
	// --------------------------------------------------------------
	WNDCLASSEX	wcls{};
	wcls.cbSize			= sizeof(wcls);
	wcls.style			= CS_HREDRAW | CS_VREDRAW;
	wcls.cbWndExtra		= 0;
	wcls.hInstance		= GetModuleHandle(nullptr);
	wcls.hIcon			= LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
	wcls.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcls.hbrBackground	= nullptr;//(HBRUSH)GetStockObject(WHITE_BRUSH);
	wcls.lpszClassName	= "Example";
	wcls.hIconSm		= LoadIcon(GetModuleHandle(NULL), IDI_APPLICATION);
	wcls.lpfnWndProc	= [](HWND _hwnd, UINT _msg, WPARAM _wparam, LPARAM _lparam) -> LRESULT
	{
		switch (_msg)
		{
			case WM_NCDESTROY :
			{
				PostQuitMessage(0);
				break;
			}
		}
		return	DefWindowProc(_hwnd, _msg, _wparam, _lparam);
	}; // wcls.lpfnWndProc
	RegisterClassEx(&wcls);


	// ==============================================================
	// Windows Create
	// --------------------------------------------------------------
	HWND	hwnd	= CreateWindowEx(
						WS_EX_APPWINDOW,
						wcls.lpszClassName,
						"Example Window",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,
						CW_USEDEFAULT,
						_width,
						_height,
						nullptr,
						nullptr,
						GetModuleHandle(nullptr),
						nullptr
					);
	ShowWindow(hwnd, TRUE);

	// Create callback
	_created(hwnd);


	// ==============================================================
	// Message loop
	// --------------------------------------------------------------
	for (;;)
	{
		MSG		msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Update callback
		_update();
	}
}



}
