#include "KBS.h"
#include "Core/Entry.h"
#include <iostream>

class HelloWorldApplication : public kbs::Application
{
public:

	HelloWorldApplication(kbs::ApplicationCommandLine& commandLine)
		: Application(commandLine){}

	virtual void OnUpdate() override;

	virtual void BeforeRun() override;
};


namespace kbs
{
	Application* CreateApplication(ApplicationCommandLine& commandLines)
	{
		KBS_LOG("Hello World Application!");
		auto app = new HelloWorldApplication(commandLines);
		
		
		return app;
	}
}


void HelloWorldApplication::OnUpdate()
{
}

void HelloWorldApplication::BeforeRun()
{
	AddListener<kbs::KeyHoldEvent>(
		[](const kbs::KeyHoldEvent& e)
		{
			KBS_LOG("Key down {}", e.ToString().c_str());
		}
	);
}