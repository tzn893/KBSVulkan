#include "gtest/gtest.h"
#include "gvk.h"
#include "Core/Hasher.h"
#include <vector>

TEST(Hasher, SamplerCreateInfo)
{
	VkFilter filters[] = { VK_FILTER_NEAREST,VK_FILTER_LINEAR,VK_FILTER_CUBIC_EXT};
	VkSamplerMipmapMode mipmapSampler[] = {VK_SAMPLER_MIPMAP_MODE_NEAREST , VK_SAMPLER_MIPMAP_MODE_LINEAR};

	std::vector<uint64_t> infos;
	for (uint32_t i = 0;i < _countof(filters);i++)
	{
		for (uint32_t j = 0;j < _countof(filters);j++)
		{
			for (uint32_t k = 0;k < _countof(mipmapSampler);k++)
			{
				infos.push_back(kbs::Hasher::HashMemoryContent(GvkSamplerCreateInfo(filters[i], filters[j], mipmapSampler[k])));
			}
		}
	}

	for (uint32_t i = 0; i < infos.size();i++)
	{
		for (uint32_t j = i + 1;j < infos.size();j++)
		{
			ASSERT_NE(infos[i], infos[j]);
		}
	}


	for (uint32_t i = 0; i < _countof(filters); i++)
	{
		for (uint32_t j = 0; j < _countof(filters); j++)
		{
			for (uint32_t k = 0; k < _countof(mipmapSampler); k++)
			{
				ASSERT_EQ(infos[i * _countof(filters) * _countof(mipmapSampler) + j * _countof(mipmapSampler) + k], 
					kbs::Hasher::HashMemoryContent(GvkSamplerCreateInfo(filters[i], filters[j], mipmapSampler[k])));
			}
		}
	}
}


int main() 
{
	testing::InitGoogleTest();
	RUN_ALL_TESTS();
}