#pragma once
#include <string>

namespace quark::internal {

	template<typename T>
	void join_helper(std::ostringstream& stream, T&& t)
	{
		stream << std::forward<T>(t);
	}

	template<typename T, typename... Ts>
	void join_helper(std::ostringstream& stream, T&& t, Ts &&... ts)
	{
		stream << std::forward<T>(t);
		join_helper(stream, std::forward<Ts>(ts)...);
	}

}
namespace quark::util {

	namespace string
	{
		template<typename... Ts>
		inline std::string Join(Ts&&... ts)
		{
			std::ostringstream stream;
			(stream << ... << std::forward<Ts>(ts));
			return stream.str();
		}

		std::string& ToLower(std::string& string);
		std::string ToLowerCopy(const std::string_view string);
		std::vector<std::string> Split(const std::string& string, const char* delimiter);
		std::vector<std::string> SplitNoEmpty(const std::string& string, const char* delimiter);
	}
}
