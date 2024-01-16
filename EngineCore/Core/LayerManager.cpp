#include "LayerManager.h"

kbs::LayerManager::~LayerManager()
{
	for (uint32_t i = 0;i < m_Layers.size();i++)
	{
		m_Layers[i]->OnDetach();
		m_Layers[i] = nullptr;
	}
}

void kbs::LayerManager::PushOverlay(std::shared_ptr<Layer> layer)
{
	m_Layers.push_back(layer);
	layer->OnAttach();
}

void kbs::LayerManager::PushLayer(std::shared_ptr<Layer> layer)
{
	m_Layers.insert(m_Layers.begin() + m_LayerInsertIndex, layer);
	layer->OnAttach();

	m_LayerInsertIndex += 1;
}

void kbs::LayerManager::PopLayer(std::shared_ptr<Layer> layer)
{
	uint32_t layerIdx = 0;
	for (; layerIdx < m_LayerInsertIndex; layerIdx++)
	{
		if (m_Layers[layerIdx] == layer)
		{
			break;
		}
	}

	if (layerIdx != m_LayerInsertIndex)
	{
		m_Layers[layerIdx]->OnDetach();
		m_Layers.erase(m_Layers.begin() + layerIdx);
	}
}

void kbs::LayerManager::PopOverlay(std::shared_ptr<Layer> layer)
{
	uint32_t layerIdx = m_LayerInsertIndex;
	for (; layerIdx < m_Layers.size(); layerIdx++)
	{
		if (m_Layers[layerIdx] == layer)
		{
			break;
		}
	}

	if (layerIdx != m_Layers.size())
	{
		m_Layers[layerIdx]->OnDetach();
		m_Layers.erase(m_Layers.begin() + layerIdx);
	}
}


void kbs::LayerManager::PropagateEvent(const Event& e)
{
	for (int i = m_Layers.size() - 1;i >= 0;i--)
	{
		// if event has deal by one layer, the event will stop propagate
		if (m_Layers[i]->OnEvent(e))
		{
			break;
		}
	}
}
