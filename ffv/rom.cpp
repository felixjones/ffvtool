#include "rom.hpp"

#include <array>
#include <vector>
#include "crc32.hpp"
#include "ips.hpp"

static constexpr auto rpge_crc32 = 0xf11f1026;

using namespace ffv;

rom rom::read_ips( std::istream& streamSource ) {
	std::array<char, ips::magic::id.size()> header;
	streamSource.read( header.data(), header.size() );

	if ( header != ips::magic::id ) {
		throw std::invalid_argument( "Stream is not IPS file (Magic ID mismatch)" );
	}

	crc32 hash;
	hash << header;
	hash << std::string( std::istreambuf_iterator<char>( streamSource ), {} );

	if ( hash != rpge_crc32 ) {
		throw std::invalid_argument( "Stream is not RPGe v1.1" );
	}

	return rom {};
}
