#include "Window.h"
#include "Core/Application.h"

kbs::Window::Window(uint32_t width, uint32_t height, Application* app, const char* title)
	:m_App(app)
{
	m_Window = gvk::Window::Create(width, height, title).value();
}

KBS_API const char*  kbs::Window::GetTitle()
{
	return title.c_str();
}

KBS_API std::shared_ptr<gvk::Window> kbs::Window::GetGvkWindow()
{
	return m_Window;
}

KBS_API void kbs::Window::Update()
{
	if (m_Window->MouseMove())
	{
		GvkVector2 offset = m_Window->GetMouseOffset();
		m_App->BoardcastEvent<MouseMovedEvent>(offset.x, offset.y);
	}

	for (int i = 0;i < GVK_KEY_NUM; i++)
	{
		GVK_KEY keyi = (GVK_KEY)i;
		if (keyi == GVK_MOUSE_1 || keyi == GVK_MOUSE_2 || keyi == GVK_MOUSE_3)
		{
			if (m_Window->KeyDown(keyi))
			{
				m_App->BoardcastEvent<MouseButtonDownEvent>(keyi);
			}
			else if (m_Window->KeyUp(keyi))
			{
				m_App->BoardcastEvent<MouseButtonReleasedEvent>(keyi);
			}
		}
		else
		{
			if (m_Window->KeyDown(keyi))
			{
				m_App->BoardcastEvent<KeyDownEvent>(keyi);
			}
			else if (m_Window->KeyHold(keyi))
			{
				m_App->BoardcastEvent<KeyHoldEvent>(keyi);
			}
			else if (m_Window->KeyUp(keyi))
			{
				m_App->BoardcastEvent<KeyReleasedEvent>(keyi);
			}
		}
	}

	m_Window->UpdateWindow();
}

KBS_API uint32_t  kbs::Window::GetWidth()
{
	return m_Width;
}

KBS_API uint32_t kbs::Window::GetHeight()
{
	return m_Height;
}