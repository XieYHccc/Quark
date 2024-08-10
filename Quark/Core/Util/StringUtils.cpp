#include "Quark/QuarkPch.h"
#include "Quark/Core/Util/StringUtils.h"

namespace quark::util {
	namespace string 
	{
		std::string& ToLower(std::string& string)
		{
			std::transform(string.begin(), string.end(), string.begin(),
				[](const unsigned char c) { return std::tolower(c); });
			return string;
		}

		std::string ToLowerCopy(const std::string_view string)
		{
			std::string result(string);
			ToLower(result);
			return result;
		}
	}
}

