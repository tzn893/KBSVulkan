#include "KBS.h"
#include "Core/Entry.h"
#include <iostream>

class HelloWorldApplication : public kbs::Application
{
public:

};


namespace kbs
{
	Application* CreateApplication()
	{
		return new HelloWorldApplication();
	}
}
