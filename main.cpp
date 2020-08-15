#include <fstream>
#include "ffv/rom.hpp"

struct rpge_constants {
	static constexpr std::uint32_t crc32 = 0xf11f1026;

	using offset_type = std::tuple<std::string_view, std::uint32_t, std::uint32_t>;
	static constexpr offset_type offsets[] = {
		{ "monster_names", 0x200000, 0xf50 },
		{ "ability_names", 0x201150, 0x2a0 }
	};
};

int main( int argc, char * argv[] ) {
	const auto ips = ffv::rom::read_ips( std::ifstream( argv[1], std::ios::binary ) );

	if ( ips.hash() != rpge_constants::crc32 ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not RPGe v1.1" );
	}

	for ( auto& offset : rpge_constants::offsets ) {
		const auto& file = std::get<0>( offset );
		const auto& address = std::get<1>( offset );
		const auto& length = std::get<2>( offset );
	}

	return 0;
}
