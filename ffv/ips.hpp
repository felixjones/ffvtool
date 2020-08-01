#ifndef FFV_IPS_HPP
#define FFV_IPS_HPP

#include <array>

namespace ffv {
namespace ips {

namespace magic {
	static constexpr auto id = std::to_array( { 'P', 'A', 'T', 'C', 'H' } );
	static constexpr auto eof = std::to_array( { 'E', 'O', 'F' } );
} // magic

} // ips
} // ffv

#endif // define FFV_IPS_HPP
