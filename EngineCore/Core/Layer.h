#pragma once
#include <string>
#include "Core/Event.h"

// kbs
namespace kbs 
{
	class Layer
	{
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		// virtual void OnUpdate(Timestep ts) {}
		// virtual void OnImGuiRender() {}
		virtual bool OnEvent(const Event& event) { return true; }

		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}