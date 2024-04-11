#include "Scene/UUID.h"

// copied mostly from hazel
#include <random>
#include <unordered_map>

namespace kbs {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_Engine(s_RandomDevice());
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	UUID::UUID()
		: m_UUID()
	{
		uint64_t id = s_UniformDistribution(s_Engine);
		if(id == 0xffffffffffffffff)
		{
			id = s_UniformDistribution(s_Engine);
		}
		this->m_UUID = id;
	}

	UUID::UUID(uint64_t uuid)
		: m_UUID(uuid)
	{
	}

	UUID UUID::Create()
	{
		return UUID();
	}
	UUID UUID::Invalid()
	{
		return UUID{ 0xffffffffffffffff };
	}

	bool UUID::IsInvalid()
	{
		return 0xffffffffffffffff == m_UUID;
	}

}