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
	m_EventManager = std::make_shared<kbs::EventManager>();
	m_Window = std::make_shared<kbs::Window>(m_WindowWidth, m_WindowHeight, this, m_Title.c_str());

	BeforeRun();

	while (true)
	{
		OnUpdate();
		m_Window->Update();
	}

	return 0;
}

KBS_API kbs::LayerManager* kbs::Application::GetLayerManager()
{
	return m_LayerManager.get();
}

KBS_API kbs::ApplicationCommandLine::ApplicationCommandLine(int argc, const char** argv)
{
	commands.insert(commands.begin(), argv, argv + argc);
}
