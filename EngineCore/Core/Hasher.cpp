#include "Hasher.h"

namespace kbs
{
	uint64_t Hasher::HashMemoryContent(const void* data, uint64_t dataSize)
	{
        return HashContent(
        [data](uint64_t offset) 
        {
            return static_cast<const char*>(data)[offset];
        }, 
        dataSize);
	}

    uint64_t Hasher::HashContent(HasherContentVisiter visiter, uint64_t dataSize)
    {
        std::size_t hash_value = 0;
        std::hash<char> hasher;
        for (std::size_t i = 0; i < dataSize; ++i) {
            hash_value ^= hasher(visiter(i)) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
        }
        return hash_value;
    }
}