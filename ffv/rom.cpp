#include "rom.hpp"

#include <algorithm>
#include <array>
#include <vector>
#include "crc.hpp"
#include "ips.hpp"
#include "ips_ext.hpp"

struct rpge_constants {
	static constexpr std::uint32_t crc32 = 0xf11f1026;
};

using namespace ffv;

rom rom::read_ips( std::istream& streamSource ) {
	std::array<char, ips::magic::id.size()> header;
	streamSource.read( header.data(), header.size() );

	if ( header != ips::magic::id ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not IPS file (Magic ID mismatch)" );
	}

	crc32 hash;
	hash << header;

	std::vector<char> romBytes;

	ips::eof eof;
	ips::record record;
	while ( streamSource.good() ) {
		record << streamSource;
		hash << record;
		
		if ( std::holds_alternative<ips::record::copy_type>( record.data ) ) {
			const auto& copy = std::get<ips::record::copy_type>( record.data );
			// TODO
		} else if ( std::holds_alternative<ips::record::fill_type>( record.data ) ) {
			const auto& fill = std::get<ips::record::fill_type>( record.data );
			// TODO
		}

		const auto start = streamSource.tellg();
		eof << streamSource;
		if ( eof.is_end() ) {
			hash << eof;
			break;
		}
		streamSource.seekg( start );
	}

	if ( hash != rpge_constants::crc32 ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not RPGe v1.1" );
	}
	
	return rom {};
}
