#pragma once
#include "Common.h"



namespace kbs
{
	using HasherContentVisiter = std::function<char(uint64_t offset)>;

	class Hasher
	{
	public:
		static uint64_t HashMemoryContent(const void* data, uint64_t dataSize);
		static uint64_t HashContent(HasherContentVisiter visiter, uint64_t dataSize);

		template<typename T>
		static uint64_t HashMemoryContent(const T& data)
		{
			return HashMemoryContent(&data, sizeof(data));
		}
	};
}