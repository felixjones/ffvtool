#ifndef FFV_ISTREAM_FIND_HPP
#define FFV_ISTREAM_FIND_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <istream>
#include <iterator>

namespace ffv {

static constexpr auto find_buffer_size = 0x1000;

template <class Type>
std::istream& find( std::istream& stream, const Type& value ) noexcept {
	// Without needing to set a sized read buffer for istream; it's faster to read in chunks than to use stream_iterator :(
	static constexpr auto seek_back = -static_cast<int>( sizeof( Type ) );
	const auto valueBytes = std::bit_cast<std::array<char, sizeof( Type )>>( value );

	std::array<char, find_buffer_size> readBuffer;
	const auto readBufferBegin = std::cbegin( readBuffer );
	while ( true ) {
		const auto start = stream.tellg();
		stream.read( readBuffer.data(), readBuffer.size() );
		const auto good = stream.good();
		if ( !good ) [[unlikely]] {
			stream.clear();
			stream.seekg( 0, std::istream::end );
		}
		const auto readLength = stream.tellg() - start;

		const auto readBufferEnd = readBufferBegin + readLength;
		const auto iter = std::search( readBufferBegin, readBufferEnd, std::cbegin( valueBytes ), std::cend( valueBytes ) );
		if ( iter != readBufferEnd ) {
			const auto offset = std::distance( readBufferBegin, iter );
			stream.seekg( -readLength + offset, std::istream::cur );
			break;
		}

		if ( !good ) [[unlikely]] {
			break;
		}

		stream.seekg( seek_back, std::istream::cur );
	}

	return stream;
}

} // ffv

#endif // define FFV_ISTREAM_FIND_HPP
