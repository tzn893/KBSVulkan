#include "Common.h"
#include "Core/Singleton.h"
#include <filesystem>

namespace kbs
{

	template<typename T>
	class FileSystem : public Is_Singleton
	{
	public:
		FileSystem() = default;

		void AddSearchPath(const std::string& _path)
		{
			namespace fs = std::filesystem;

			auto p = fs::path(_path);
			KBS_ASSERT(fs::exists(p), "path added to filesytem must be exits!");
			KBS_ASSERT(p.is_absolute(), "path added to filesystem's search path must be absolute");
			KBS_ASSERT(fs::is_directory(p), "path added to filesystem's search path must be a valid directory");

			if (std::find(m_SearchPathes.begin(), m_SearchPathes.end(), _path) == m_SearchPathes.end())
			{
				m_SearchPathes.push_back(_path);
			}
		}

		opt<std::string> FindAbsolutePath(const std::string& path)
		{
			namespace fs = std::filesystem;

			if (fs::path(path).is_absolute())
			{
				return path;
			}
			for (auto& s : m_SearchPathes)
			{
				auto searchedPath = fs::path(s) / path;
				if (fs::exists(searchedPath))
				{
					return searchedPath.string();
				}
			}

			return std::nullopt;
		}

		View<std::string> GetSearchPathes()
		{
			return View(m_SearchPathes);
		}

		std::string GetFileName(const std::string& path)
		{
			namespace fs = std::filesystem;
			fs::path p(path);
			return p.filename().string();
		}

	private:
		std::vector<std::string> m_SearchPathes;
	};

}