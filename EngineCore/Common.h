#pragma once

#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <memory>
#include <optional>
#include <tuple>
#include <string>
#include <algorithm>
#include <functional>
#include "Core/Log.h"

namespace kbs
{
	template<typename T>
	using ptr = std::shared_ptr<T>;

	template<typename T>
	using opt = std::optional<T>;

	template<typename T>
	using ref = T*;

	template<typename ...Args>
	using tpl = std::tuple<Args...>;

	inline std::vector<std::string> string_split(const std::string& _str, char dim)
	{
		// this will promise the last splited sub string will be added to list
		std::string str = _str + dim;
		
		std::vector<std::string> res;
		uint32_t p = 0, s = 0;
		while (p != str.size()) 
		{
			if (str[p] == dim)
			{
				// prevent empty string
				if (p != s)
				{
					res.push_back(str.substr(s, p - s));
				}
				s = p + 1;
			}
			p++;
		}

		return res;
	}

	inline std::string lower_string(const std::string& _str)
	{
		std::string str = _str;
		std::transform(str.begin(), str.end(), str.begin(), [](char c) {return std::tolower(c); });
		return str;
	}

	inline bool string_contains(const std::string& src, const std::string& subStr)
	{
		return src.find(subStr) != std::string::npos;
	}
	
	inline std::string string_strip(const std::string& str)
	{
		int32_t s = 0;
		while (str[s] == ' ' && s != str.size()) s++;
		if (s == str.size()) return "";

		int32_t e = str.size() - 1;
		while (str[e] == ' ' && e > s) e--;
		return str.substr(s, e - s + 1);
	}

	template<typename FlagT,typename FlagBitT>
	bool kbs_contains_flags(FlagT src, FlagBitT target)
	{
		return (src & target) != 0;
	}

	template<typename FlagT, typename FlagBitT>
	bool kbs_cover_flags(FlagT src, FlagBitT target)
	{
		return (src & target) == target;
	}

	template<uint32_t dst>
	uint32_t round_up(uint32_t num) 
	{
		return ((num + (dst - 1)) / dst) * dst;
	}

	template<typename T>
	class View
	{
	public:
		View() { data = nullptr, count = 0; }
		View(std::vector<T>& arr)
		{
			data = arr.empty() ? nullptr : arr.data();
			count = arr.size();
		}
		View(T* arr, uint32_t size)
		{
			data = arr;
			count = size;
		}

		View(const View& other) = default;
		View& operator=(const View& other) = default;
		
		bool empty()
		{
			return data != nullptr && count != 0;
		}
		T& operator[](uint32_t idx)
		{
			KBS_ASSERT(count > idx, "view out of boundary");
			return data[idx];
		}
		const T& operator[](uint32_t idx) const
		{
			return (*const_cast<View*>(this))[idx];
		}

		class Iterator
		{
		public:
			Iterator() :view(nullptr), pos(0) {}
			Iterator(View* view, uint32_t pos)
				:view(view),pos(pos)
			{}
			Iterator(const Iterator& iter) = default;

			bool operator==(const Iterator& other) const
			{
				return other.view == view && pos == other.pos;
			}
			bool operator!=(const Iterator& other) const
			{
				return !this->operator==(other);
			}
			T& operator*()
			{
				return (*view)[pos];
			}
			const T& operator*() const
			{
				return (*view)[pos];
			}
			Iterator& operator++() { pos++; return *this; }
		private:
			View*    view;
			uint32_t pos;
		};

		Iterator begin()
		{
			return Iterator(this, 0);
		}
		Iterator end()
		{
			return Iterator(this, count);
		}

	private:
		T* data;
		uint32_t count;
	};

}