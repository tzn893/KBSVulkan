#pragma once
#include "Application.h"


int main()
{
	kbs::Application* app = kbs::CreateApplication();
	if (app == nullptr)
	{
		return -1;
	}
	return app->Run();
}