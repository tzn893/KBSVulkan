#pragma once

#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <memory>
#include <optional>
#include <tuple>
#include <string>
#include <functional>

namespace kbs
{
	template<typename T>
	using ptr = std::shared_ptr<T>;

	template<typename T>
	using opt = std::optional<T>;

	template<typename ...Args>
	using tpl = std::tuple<Args...>;

	inline std::vector<std::string> string_split(const std::string& str, char dim)
	{
		std::vector<std::string> res;
		uint32_t p = 0, s = 0;
		while (p != str.size()) 
		{
			if (str[p] == dim)
			{
				if (p != s)
				{
					res.push_back(str.substr(s, p - s));
				}
				s = p + 1;
			}
			p++;
		}
		res.push_back(str.substr(s, p - s));
		return res;
	}
}