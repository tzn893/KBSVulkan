#pragma once
#include "Core/Layer.h"
#include "Core/Event.h"
#include <vector>

namespace kbs {

	// copied mostly from hazel


	class LayerManager
	{
	public:
		LayerManager() = default;
		~LayerManager();

		void PushLayer(std::shared_ptr<Layer> layer);
		void PopLayer(std::shared_ptr<Layer> layer);

		void PushOverlay(std::shared_ptr<Layer> layer);
		void PopOverlay(std::shared_ptr<Layer> layer);

		void PropagateEvent(const Event& e);

		std::vector<std::shared_ptr<Layer>>::iterator begin() { return m_Layers.begin(); }
		std::vector<std::shared_ptr<Layer>>::iterator end() { return m_Layers.end(); }
		std::vector<std::shared_ptr<Layer>>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<std::shared_ptr<Layer>>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<std::shared_ptr<Layer>>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<std::shared_ptr<Layer>>::const_iterator end()	const { return m_Layers.end(); }
		std::vector<std::shared_ptr<Layer>>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<std::shared_ptr<Layer>>::const_reverse_iterator rend() const { return m_Layers.rend(); }

        template<typename T>
        void ListenToEvent(EventManager* eventManager)
        {
			auto wapper = [&](const T& e)
			{
				PropagateEvent(e);
			};
			eventManager->AddListener<T>(wapper);
        }

		template<typename ...E>
		void ListenToEvents(EventManager* eventManger)
		{
			(
				[&]() 
				{
					ListenToEvent<E>(eventManger);
				}()
				, ...);
		}


	private:
		std::vector<std::shared_ptr<Layer>> m_Layers;
		uint32_t m_LayerInsertIndex = 0;
	};

}