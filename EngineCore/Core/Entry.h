#pragma once
#include "Application.h"
#include "Event.h"


int main(int argc, const char** argv)
{
	kbs::Application* app = kbs::CreateApplication(kbs::ApplicationCommandLine(argc, argv));
	
	
	
	if (app == nullptr)
	{
		return -1;
	}
	return app->Run();
}