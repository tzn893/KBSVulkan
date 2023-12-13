#pragma once
#include "Platform/Platform.h"


namespace kbs
{
	class KBS_API Application
	{
	public:

		int Run();

	private:

	};

	extern Application* CreateApplication();
}