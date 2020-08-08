#ifndef FFV_IPS_HPP
#define FFV_IPS_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <istream>
#include <type_traits>
#include <variant>

namespace ffv {
namespace ips {

static constexpr auto endianness = std::endian::big;

struct eof {
	static constexpr auto magic = std::to_array( { 'E', 'O', 'F' } );

	using data_type = std::remove_const_t<decltype( magic )>;

	data_type	data;

	constexpr bool is_end() const noexcept {
		return data == magic;
	}
};

namespace magic {
	static constexpr auto id = std::to_array( { 'P', 'A', 'T', 'C', 'H' } );
} // magic

struct record {
	static constexpr auto offset_bytes = 3;
	static constexpr auto size_bytes = 2;

	using copy_type = std::vector<char>;
	struct fill_type {
		std::uint16_t	size;
		char			data;
	};

	using data_type = std::variant<copy_type, fill_type>;	

	std::uint32_t	offset : 24;
	data_type		data;
};

} // ips

inline ips::eof& operator <<( ips::eof& eof, std::istream& streamSource ) noexcept {
	auto itor = std::begin( eof.data );
	while ( streamSource.good() && itor != std::end( eof.data ) ) {
		*itor = streamSource.get();
		itor++;
	}

	return eof;
}

inline ips::record& operator <<( ips::record& record, std::istream& streamSource ) noexcept {
	std::array<char, sizeof( std::uint32_t )> offsetBytes {};
	auto offsetItor = std::begin( offsetBytes );
	const auto offsetEnd = offsetItor + ips::record::offset_bytes;
	while ( offsetItor != offsetEnd ) {
		*offsetItor = streamSource.get();
		++offsetItor;
	}

	if constexpr ( ips::endianness != std::endian::native ) {
		std::reverse( std::begin( offsetBytes ), std::begin( offsetBytes ) + ips::record::offset_bytes );
	}

	record.offset = std::bit_cast<std::uint32_t>( offsetBytes );

	std::array<char, ips::record::size_bytes> sizeBytes {};
	auto sizeItor = std::begin( sizeBytes );
	while ( sizeItor != std::end( sizeBytes ) ) {
		*sizeItor = streamSource.get();
		sizeItor++;
	}

	if constexpr ( ips::endianness != std::endian::native ) {
		std::reverse( std::begin( sizeBytes ), std::end( sizeBytes ) );
	}

	const auto size = std::bit_cast<std::uint16_t>( sizeBytes );
	if ( size == 0 ) {
		// Fill
		sizeItor = std::begin( sizeBytes );
		while ( sizeItor != std::end( sizeBytes ) ) {
			*sizeItor = streamSource.get();
			sizeItor++;
		}

		if constexpr ( ips::endianness != std::endian::native ) {
			std::reverse( std::begin( sizeBytes ), std::end( sizeBytes ) );
		}

		record.data = ips::record::fill_type {
			std::bit_cast<std::uint16_t>( sizeBytes ),
			static_cast<char>( streamSource.get() )
		};
	} else {
		// Copy
		auto copy = ips::record::copy_type {};
		copy.resize( size, 0 );

		auto copyItor = std::begin( copy );
		while ( copyItor != std::end( copy ) ) {
			*copyItor = streamSource.get();
			copyItor++;
		}

		record.data = copy;
	}

	return record;
}

} // ffv

#endif // define FFV_IPS_HPP
