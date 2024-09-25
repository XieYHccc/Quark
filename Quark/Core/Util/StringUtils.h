#pragma once
#include <string>

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
