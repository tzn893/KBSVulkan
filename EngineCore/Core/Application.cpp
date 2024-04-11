#include "Core/Application.h"
#include "Core/Log.h"


KBS_API kbs::Application::Application(ApplicationCommandLine& commandLine)
{
	m_WindowWidth = 600, m_WindowHeight = 600;

	m_Title = "kbs";
	for (int i = 0;i < commandLine.commands.size(); i++)
	{
		if (commandLine.commands[i].substr(0,2) == "-r")
		{
			std::string resolutionStr = commandLine.commands[i].substr(2, commandLine.commands[i].size() - 2);
			std::vector<std::string> splitedResolutionStr = string_split(resolutionStr, 'x');

			if (splitedResolutionStr.size() != 2)
			{
				KBS_WARN("invalid resolution parameter {} : valid useage -r[Width]x[Height]", commandLine.commands[i].c_str());
				continue;
			}
			
			try
			{
				m_WindowWidth = std::stoi(splitedResolutionStr[0]);
				m_WindowHeight = std::stoi(splitedResolutionStr[1]);
			}
			catch (...)
			{
				KBS_WARN("invalid resolution parameter {} : valid useage -r[Width]x[Height]", commandLine.commands[i].c_str());
				continue;
			}
		}
		else if (commandLine.commands[i].substr(0,2) == "-t")
		{
			std::string resolutionStr = commandLine.commands[i].substr(2, commandLine.commands[i].size() - 2);
			m_Title = resolutionStr;
		}
		else
		{
			KBS_WARN("unknown command line parameter {}", commandLine.commands[i].c_str());
		}

		m_Timer = std::make_shared<Timer>();
	}
}


KBS_API  void kbs::Application::BeforeRun()
{

}

KBS_API void kbs::Application::OnUpdate()
{
}

KBS_API int kbs::Application::Run()
{
	//m_EventManager = std::make_shared<kbs::EventManager>();
	m_LayerManager = std::make_shared<kbs::LayerManager>();
	m_Window = std::make_shared<kbs::Window>(m_WindowWidth, m_WindowHeight, this, m_Title.c_str());

	m_LayerManager->ListenToEvents<KeyDownEvent, KeyHoldEvent, KeyReleasedEvent, MouseButtonEvent, MouseButtonDownEvent,
		MouseButtonReleasedEvent, MouseMovedEvent, WindowResizeEvent, WindowCloseEvent>(Singleton::GetInstance<EventManager>());

	BeforeRun();

	kbs::Singleton::GetInstance<kbs::EventManager>()->BoardcastEvent(kbs::WindowResizeEvent(m_Window->GetWidth(), m_Window->GetHeight()));

	while (true)
	{
		OnUpdate();
		m_Window->Update();
		m_Timer->Tick();
	}

	return 0;
}

KBS_API kbs::LayerManager* kbs::Application::GetLayerManager()
{
	return m_LayerManager.get();
}

kbs::Window* kbs::Application::GetWindow()
{
	return m_Window.get();
}

KBS_API kbs::ApplicationCommandLine::ApplicationCommandLine(int argc, const char** argv)
{
	commands.insert(commands.begin(), argv, argv + argc);
}
