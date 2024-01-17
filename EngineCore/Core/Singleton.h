#pragma once
#include "Common.h"


namespace kbs
{
	class Is_Singleton
	{
	public:
		Is_Singleton() = default;
		Is_Singleton(const Is_Singleton&) = delete;
		virtual ~Is_Singleton() {}
	};

	class Singleton
	{
	public:
		template<typename T>
		static T* GetInstance()
		{
			static_assert(std::is_base_of_v<Is_Singleton, T>, "type T must be derived from Is_Singleton class");
			static ptr<T> singleton;
			if (singleton == nullptr)
			{
				singleton = std::make_shared<T>();
			}
			return singleton.get();
		}
	};
}
