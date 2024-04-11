#pragma once
#include <string>
#include <unordered_map>
#include <functional>

#include "Core/Singleton.h"
#include "gvk_window.h"

namespace kbs
{
	//mostly copied from hazel
	
	// Events in Hazel are currently blocking, meaning when an event occurs it
	// immediately gets dispatched and must be dealt with right then an there.
	// For the future, a better strategy might be to buffer events in an event
	// bus and process them during the "event" part of the update stage.

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyHold, KeyReleased, KeyDown,
		MouseButtonDown, MouseButtonReleased, MouseMoved
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = 1,
		EventCategoryInput = 2,
		EventCategoryKeyboard = 4,
		EventCategoryMouse = 8,
		EventCategoryMouseButton = 16,
	};

#define EVENT_CLASS_NAME(eve) static const char* GetName() {return #eve;}

#define EVENT_CLASS_TYPE(type) static uint64_t GetStaticType() { return (uint64_t)type; }\
								virtual uint64_t GetEventType() const override { return GetStaticType(); }\

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }

#define EVENT_CLASS(eve, type, category) EVENT_CLASS_NAME(eve); EVENT_CLASS_TYPE(type); EVENT_CLASS_CATEGORY(category);


	class Event
	{
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual uint64_t GetEventType() const = 0;
		static const char* GetName() { return "Undefined"; };
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category)
		{
			return GetCategoryFlags() & category;
		}
	};

	class EventManager : public Is_Singleton
	{
	public:
		using EventCallback = std::function<void(const Event&)>;

		template<typename T>
		void AddListener(std::function<void(const T&)> eventCallback)
		{
			static_assert(std::is_base_of_v<Event, T>, "listener must listen to a event object");

			auto wapper = [eventCallback](const Event& e)
			{
				const T& tEvent = static_cast<const T&>(e);
				eventCallback(tEvent);
			};
			
			std::string eventClassName = T::GetName();
			if (!m_CallbackTable.count(eventClassName))
			{
				m_CallbackTable[eventClassName] = std::vector<EventCallback>{};
			}

			m_CallbackTable[eventClassName].push_back(wapper);
		}

		template<typename T>
		void BoardcastEvent(const T& e)
		{
			if (m_CallbackTable.count(e.GetName()))
			{
				for (auto& callback : m_CallbackTable[e.GetName()])
				{
					callback(e);
				}
			}
		}

	private:
		std::unordered_map<std::string, std::vector<EventCallback>> m_CallbackTable;
	};

	extern std::vector<std::string> g_keyCodeStringTable;

	class KeyEvent : public Event
	{
	public:
		GVK_KEY GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(const GVK_KEY keycode)
			: m_KeyCode(keycode) {}

		GVK_KEY m_KeyCode;
	};

	class KeyHoldEvent : public KeyEvent
	{
	public:
		KeyHoldEvent(const GVK_KEY keycode)
			: KeyEvent(keycode) {}


		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyHoldEvent: " << g_keyCodeStringTable[m_KeyCode];
			return ss.str();
		}

		EVENT_CLASS_NAME(KeyHold)
		EVENT_CLASS_TYPE(EventType::KeyHold)
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(const GVK_KEY keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << g_keyCodeStringTable[m_KeyCode];
			return ss.str();
		}

		EVENT_CLASS_NAME(KeyReleased)
		EVENT_CLASS_TYPE(EventType::KeyReleased)
	};

	class KeyDownEvent : public KeyEvent
	{
	public:
		KeyDownEvent(const GVK_KEY keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << g_keyCodeStringTable[m_KeyCode];
			return ss.str();
		}

		EVENT_CLASS_NAME(KeyDown)
		EVENT_CLASS_TYPE(EventType::KeyDown)
	};

	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(const float x, const float y)
			: m_MouseX(x), m_MouseY(y) {}

		float GetX() const { return m_MouseX; }
		float GetY() const { return m_MouseY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS(MouseMoved, EventType::MouseMoved, EventCategoryMouse | EventCategoryInput)

	private:
		float m_MouseX, m_MouseY;
	};

	class MouseButtonEvent : public Event
	{
	public:
		GVK_KEY GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
	protected:
		MouseButtonEvent(const GVK_KEY button)
			: m_Button(button) {}

		GVK_KEY m_Button;
	};

	class MouseButtonDownEvent : public MouseButtonEvent
	{
	public:
		MouseButtonDownEvent(const GVK_KEY button)
			: MouseButtonEvent(button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(EventType::MouseButtonDown)
		EVENT_CLASS_NAME(MouseButtonDown)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(const GVK_KEY button)
			: MouseButtonEvent(button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(EventType::MouseButtonReleased)
		EVENT_CLASS_NAME(MouseButtonReleased)
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		unsigned int GetWidth() const { return m_Width; }
		unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS(WindowResize, EventType::WindowResize, EventCategoryApplication)
	private:
		unsigned int m_Width, m_Height;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS(WindowClose, EventType::WindowClose, EventCategoryApplication)
	};

	class AppTickEvent : public Event
	{
	public:
		AppTickEvent() = default;

		EVENT_CLASS(AppTick, EventType::AppTick, EventCategoryApplication)
	};

	class AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() = default;

		EVENT_CLASS(AppUpdate, EventType::AppUpdate, EventCategoryApplication)
	};

	class AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() = default;

		EVENT_CLASS(AppRender, EventType::AppRender, EventCategoryApplication)
	};
}