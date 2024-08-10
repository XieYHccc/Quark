#pragma once
#include <string>
namespace quark::util {

	namespace string
	{
		std::string& ToLower(std::string& string);
		std::string ToLowerCopy(const std::string_view string);
		
	}
}
