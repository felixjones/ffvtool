#ifndef FFV_GBA_HPP
#define FFV_GBA_HPP

#include <array>
#include <cstdint>
#include <istream>
#include <string_view>

namespace ffv {
namespace gba {

using game_title_type = std::array<char, 12>;
using game_serial_type = std::array<char, 4>;
using maker_type = std::array<char, 2>;

enum class device_type : std::uint16_t {
	advance_game_boy = 0x00
};

struct header {
	game_title_type		software_title;
	game_serial_type	game_serial;
	maker_type			maker;
	device_type			device;
	std::uint8_t		version;
	bool				logo_code : 1,
						fixed : 1,
						debugger : 1,
						complement : 1;
};

header	read_header( std::istream& stream ) noexcept;

} // gba
} // ffv

#endif // define FFV_GBA_HPP
