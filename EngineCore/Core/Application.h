#pragma once
#include "Platform/Platform.h"
#include "Common.h"
#include "Core/Event.h"
#include "Core/LayerManager.h"
#include "Core/Window.h"

namespace kbs
{
	struct KBS_API ApplicationCommandLine
	{
		::std::vector<std::string> commands;

		ApplicationCommandLine(int argc, const char** argv);
	};

	class KBS_API Application
	{
	public:

		Application(ApplicationCommandLine& commandLine);

		int Run();

		template<typename T, typename...Args>
		void BoardcastEvent(Args...args)
		{
			m_EventManager->BoardcastEvent(T(args...));
		}

		template<typename T>
		void AddListener(std::function<void(const T&)> eventListener)
		{
			m_EventManager->AddListener(eventListener);
		}

		virtual void OnUpdate();
		virtual void BeforeRun();

		LayerManager* GetLayerManager();
		Window*		  GetWindow();


	protected:
		ptr<EventManager>	m_EventManager;
		ptr<LayerManager>	m_LayerManager;
		ptr<Window>			m_Window;

		int m_WindowWidth;
		int m_WindowHeight;
		std::string m_Title;
	};

	extern Application* CreateApplication(ApplicationCommandLine& commandLines);
}