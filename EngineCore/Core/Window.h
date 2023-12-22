#pragma once
#include <stdint.h>
#include "gvk_window.h"
#include "Platform/Platform.h"

namespace kbs
{
	class Application;

	class KBS_API Window
	{
	public:
		Window(uint32_t width, uint32_t height, Application* app, const char* title);

		void Update();

		uint32_t GetWidth();
		uint32_t GetHeight();

		const char* GetTitle();

		std::shared_ptr<gvk::Window> GetGvkWindow();
	protected:
		uint32_t m_Width, m_Height;
		Application* m_App;

		std::shared_ptr<gvk::Window> m_Window;
		std::string title;
	};
}
