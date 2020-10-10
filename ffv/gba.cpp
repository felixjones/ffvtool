#include "gba.hpp"

#include <algorithm>
#include <string>

#include "agb_huff.hpp"
#include "istream_find.hpp"
#include "tree.hpp"

using namespace ffv;

namespace detail {

	template <typename... Ts>
	constexpr auto make_byte_array( Ts&&... args ) noexcept -> std::array<std::byte, sizeof...( Ts )> {
		return { std::byte( std::forward<Ts>( args ) )... };
	}

	template <typename Type>
	auto& read( std::istream& stream, Type& outValue ) noexcept {
		std::array<char, sizeof( Type )> buffer;
		stream.read( buffer.data(), buffer.size() );
		outValue = std::bit_cast<Type>( buffer );
		return outValue;
	}

	template <typename Type>
	auto read( std::istream& stream ) noexcept {
		std::array<char, sizeof( Type )> buffer;
		stream.read( buffer.data(), buffer.size() );
		return std::bit_cast<Type>( buffer );
	}

	std::uint8_t read_complement( std::istream& stream ) noexcept {
		std::array<char, 28> buffer;
		stream.read( buffer.data(), buffer.size() );
		std::uint8_t complement = 0;
		for ( const auto& byte : buffer ) {
			complement += byte;
		}
		return -( complement + 0x19 );
	}

	static constexpr auto font_table_header = detail::make_byte_array(
		0x00, 0x00, 0x00, 0x00, 'F', 'O', 'N', 'T'
	);

} // detail

static bool check_nintendo_logo( std::istream& streamSource ) noexcept {
	static constexpr auto nintendo_logo_tree = make_huff(
		0x40, 0x00, 0x00, 0x00, 0x01, 0x81, 0x82, 0x82, 0x83, 0x0F, 0x83, 0x0C, 0xC3, 0x03, 0x83, 0x01,
		0x83, 0x04, 0xC3, 0x08, 0x0E, 0x02, 0xC2, 0x0D, 0xC2, 0x07, 0x0B, 0x06, 0x0A, 0x05, 0x09
	);

	const auto decompressed = nintendo_logo_tree.decompress4( streamSource, 156 );

	std::array<std::byte, 4> word;
	std::copy( std::cbegin( decompressed ), std::cbegin( decompressed ) + word.size(), std::begin( word ) );
	auto size = std::bit_cast<std::uint32_t>( word ) >> 8;

	static constexpr auto nintendo_logo_tiles_width = std::size_t { 13 };
	static constexpr auto nintendo_logo_bitmap = detail::make_byte_array(
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x3c, 0xf0, 0x3c, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xc0, 0x03, 0x00, 0x21, 0x7c, 0xf0, 0x3c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00,
		0xc0, 0x03, 0x80, 0x4e, 0x7c, 0xf0, 0x3c, 0x00, 0xc0, 0x03, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x80,
		0x52, 0xfc, 0xf0, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x80, 0x4e, 0xfc, 0xf1,
		0x3c, 0xef, 0xf1, 0x8f, 0x1f, 0xde, 0x03, 0xde, 0xc3, 0x8f, 0x52, 0xbc, 0xf1, 0x3c, 0xff, 0xc7,
		0xc3, 0x39, 0xfe, 0x0f, 0xff, 0xe3, 0x1c, 0x21, 0xbc, 0xf3, 0x3c, 0x1f, 0xcf, 0xe3, 0x70, 0x3e,
		0x9e, 0xc7, 0x73, 0x38, 0x1e, 0x3c, 0xf7, 0x3c, 0x0f, 0xcf, 0xf3, 0xf0, 0x1e, 0xde, 0xc3, 0x7b,
		0x78, 0x00, 0x3c, 0xf6, 0x3c, 0x0f, 0xcf, 0xf3, 0xff, 0x1e, 0xde, 0xc3, 0x7b, 0x78, 0x00, 0x3c,
		0xfe, 0x3c, 0x0f, 0xcf, 0xf3, 0x00, 0x1e, 0xde, 0xc3, 0x7b, 0x78, 0x00, 0x3c, 0xfc, 0x3c, 0x0f,
		0xcf, 0xf3, 0xf0, 0x1e, 0xde, 0xc3, 0x7b, 0x78, 0x00, 0x3c, 0xf8, 0x3c, 0x0f, 0xcf, 0xe3, 0xf0,
		0x1e, 0x9e, 0xc3, 0x73, 0x38, 0x00, 0x3c, 0xf8, 0x3c, 0x0f, 0xcf, 0xc3, 0x79, 0x1e, 0x1e, 0xe7,
		0xe3, 0x1c, 0x00, 0x3c, 0xf0, 0x3c, 0x0f, 0xcf, 0x83, 0x1f, 0x1e, 0x1e, 0xfe, 0xc3, 0x0f, 0x00
	);

	std::size_t tileStart = 0;
	std::size_t tileY = 0;

	std::uint16_t sum = 0;
	auto read = std::cbegin( decompressed ) + 4;
	while ( size ) {
		std::array<std::byte, 2> s16;
		std::copy( read, read + 2, std::begin( s16 ) );
		sum += std::bit_cast<std::uint16_t>( s16 );

		s16 = std::bit_cast<std::array<std::byte, 2>>( sum );

		if ( nintendo_logo_bitmap[tileStart + tileY * nintendo_logo_tiles_width] != s16[0] ) {
			return false;
		}
		++tileY;

		if ( nintendo_logo_bitmap[tileStart + tileY * nintendo_logo_tiles_width] != s16[1] ) {
			return false;
		}
		++tileY;

		if ( tileY == 8 ) {
			tileY = 0;
			++tileStart;
			if ( ( tileStart % nintendo_logo_tiles_width ) == 0 ) {
				tileStart += nintendo_logo_tiles_width * 7;
			}
		}

		read += 2;
		size -= 2;
	}

	return true;
}

gba::header gba::read_header( std::istream& stream ) noexcept {
	gba::header header;

	stream.seekg( 4, std::istream::cur );
	header.logo_code = check_nintendo_logo( stream );
	stream.read( header.software_title.data(), header.software_title.size() );
	stream.read( header.game_serial.data(), header.game_serial.size() );
	stream.read( header.maker.data(), header.maker.size() );
	header.fixed = detail::read<std::uint8_t>( stream ) == 0x96;
	const auto device = detail::read<std::uint16_t>( stream );
	stream.seekg( 7, std::istream::cur );
	detail::read( stream, header.version );
	const auto complement = detail::read<std::uint8_t>( stream );

	stream.seekg( -30, std::istream::cur );
	header.complement = detail::read_complement( stream ) == complement;
	header.device = static_cast<device_type>( device & 0x7fff );
	header.debugger = device & 0x8000;
	return header;
}

std::istream& gba::find_font_table( std::istream& stream ) noexcept {
	return find( stream, detail::font_table_header );
}

gba::font_table gba::read_font_table( std::istream& stream ) {
	static constexpr auto unknown_byte_count = 256;

	using header_type = std::remove_const_t<typename decltype( detail::font_table_header )>;

	const auto header = detail::read<header_type>( stream );
	if ( header != detail::font_table_header ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not a font table" );
	}

	font_table fontTable {};
	detail::read( stream, fontTable.height );

	if ( const auto bitDepth = detail::read<std::uint8_t>( stream ); bitDepth != 2 ) [[unlikely]] {		
		throw std::invalid_argument( std::string( "Font table has unexpected bit depth (expected 2, got " ) + std::to_string( static_cast<int>( bitDepth ) ) + ")" );
	}

	fontTable.glyphs.reserve( detail::read<std::uint16_t>( stream ) );

	stream.seekg( unknown_byte_count, std::istream::cur );

	std::generate_n( std::back_inserter( fontTable.glyphs ), fontTable.glyphs.capacity(), [&stream, tableStart = stream.tellg(), height = fontTable.height]() {
		const auto offset = detail::read<std::uint32_t>( stream );
		const auto next = stream.tellg();

		stream.seekg( offset + tableStart );

		font_glyph glyph {};
		detail::read( stream, glyph.advance );
		detail::read( stream, glyph.stride );
		glyph.data.reserve( static_cast<std::size_t>( height ) * glyph.stride );

		std::generate_n( std::back_inserter( glyph.data ), glyph.data.capacity(), [&stream]() {
			return detail::read<std::byte>( stream );
		} );

		stream.seekg( next );

		return glyph;
	} );

	return fontTable;
}
