#pragma once
#include <string>

namespace tracker_session {

	std::string secureRandomHex(std::size_t byteLen = 32);

} 
