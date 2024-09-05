#include "Quark/qkpch.h"
#include "Quark/Core/Util/StringUtils.h"

namespace quark::util {
	namespace string 
	{
		static std::vector<std::string> InternalSplit(const std::string& str, const char* delim, bool allow_empty)
		{
			if (str.empty())
				return {};

			std::vector<std::string> result;

			size_t start_index = 0;
			size_t index = 0;
			while ((index = str.find_first_of(delim, start_index)) != std::string::npos)
			{
				if (allow_empty || index > start_index)
					result.push_back(str.substr(start_index, index - start_index));
				start_index = index + 1;

				if (allow_empty && (index == str.size() - 1))
					result.emplace_back();
			}

			if (start_index < str.size())
				result.push_back(str.substr(start_index));

			return result;
		}


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

		std::vector<std::string> Split(const std::string& string, const char* delimiter)
		{
			return InternalSplit(string, delimiter, true);
		}

		std::vector<std::string> SplitNoEmpty(const std::string& string, const char* delimiter)
		{
			return InternalSplit(string, delimiter, false);
		}
	}
}

