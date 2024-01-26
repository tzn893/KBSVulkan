#pragma once
#include <stdint.h>
#include <unordered_map>

// copied mostly from hazel
namespace kbs {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		static UUID Create();

		template<typename T>
		static UUID	GenerateUncollidedID(const std::unordered_map<UUID, T>& map)
		{
			UUID id = Create();
			while (map.count(id)) id = Create();
			return id;
		}

		operator uint64_t() const { return m_UUID; }

		static UUID Invalid();
	private:
		uint64_t m_UUID;
	};

}

namespace std {
	template <typename T> struct hash;

	template<>
	struct hash<kbs::UUID>
	{
		size_t operator()(const kbs::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}
