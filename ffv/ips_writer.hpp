#ifndef FFV_IPS_WRITER_HPP
#define FFV_IPS_WRITER_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <ostream>
#include <type_traits>
#include <vector>

#include "ips.hpp"

namespace ffv {
namespace ips {

class writer {
private:
	struct entry {
		std::streamoff			offset;
		std::vector<std::byte>	data;
	};

public:
	writer& seekg( std::streamoff pos ) noexcept {
		if ( !m_buffer.empty() ) {
			m_entries.emplace_back( entry { m_pos, m_buffer } );
			m_buffer.clear();
		}
		m_pos = pos;
		return *this;
	}

	template <class Iter>
	constexpr writer& write( Iter first, Iter last ) noexcept {
		using array_type = std::array<std::byte, sizeof( Iter::value_type )>;

		for ( ; first != last; ++first ) {
			const auto valueBytes = std::bit_cast<array_type>( *first );
			std::copy( std::cbegin( valueBytes ), std::cend( valueBytes ), std::back_inserter( m_buffer ) );
		}
		
		return *this;
	}

	template <class Type>
	constexpr writer& write( const Type& value ) noexcept {
		const auto valueBytes = std::bit_cast<std::array<std::byte, sizeof( value )>>( value );
		return write( std::cbegin( valueBytes ), std::cend( valueBytes ) );
	}

	template <class Type>
	constexpr writer& operator <<( const Type& value ) noexcept {
		return write( value );
	}

	void compile( std::ostream& stream ) noexcept {
		if ( !m_buffer.empty() ) {
			m_entries.emplace_back( entry { m_pos, m_buffer } );
			m_buffer.clear();
			m_pos = 0;
		}

		std::vector<entry> compiled;
		for ( const auto& e : m_entries ) {
			const auto start = e.offset;
			const auto end = start + e.data.size();

			// Find if start is within existing compiled vector
			auto iter = std::find_if( std::begin( compiled ), std::end( compiled ), [start, end]( const auto& c ) {
				return start >= c.offset && end <= ( c.offset + c.data.size() );
			} );

			if ( iter == std::cend( compiled ) ) {
				compiled.push_back( e ); // Not found, add
			} else {
				// TODO : This
			}
		}

		stream.write( magic::id.data(), magic::id.size() );

		for ( const auto& entry : compiled ) {
			std::vector<record> records;
			const auto& data = entry.data;
			auto offset = entry.offset;

			auto begin = std::cbegin( data );
			while ( begin != std::cend( data ) ) {
				auto end = begin;
				std::ptrdiff_t distance = 0;
				while ( distance < std::numeric_limits<std::uint16_t>::max() && end != std::cend( data ) && *end == *begin ) {
					distance = std::distance( begin, ++end );
				}

				if ( distance > 3 ) {
					// RLE data
					record r;
					r.offset = offset;
					r.data = record::fill_type { static_cast<std::uint16_t>( distance ), *begin };
					records.push_back( r );
				} else {
					// Normal data
					if ( records.empty() || !std::holds_alternative<record::copy_type>( records.back().data ) ) {
						record r;
						r.offset = offset;
						r.data = record::copy_type {};
						records.push_back( r );
					}
					
					std::copy( begin, end, std::back_inserter( std::get<record::copy_type>( records.back().data ) ) );
				}

				begin += distance;
				offset += distance;
			}

			// ips endianness
			for ( const auto& record : records ) {
				stream_write( stream, record );
			}
		}

		stream.write( eof::magic.data(), eof::magic.size() );
	}

protected:
	std::streamoff			m_pos;
	std::vector<std::byte>	m_buffer;

private:
	std::vector<entry>	m_entries;

	static void stream_write( std::ostream& ostream, const ips::record& record ) {const auto offset = record.offset;
		auto offsetBytes = std::bit_cast<std::array<char, sizeof( offset )>>( offset );
		if constexpr ( ips::endianness != std::endian::native ) {
			std::reverse( std::begin( offsetBytes ), std::begin( offsetBytes ) + ips::record::offset_bytes );
		}

		ostream.write( offsetBytes.data(), ips::record::offset_bytes );
		
		if ( std::holds_alternative<ips::record::copy_type>( record.data ) ) {
			const auto& copy = std::get<ips::record::copy_type>( record.data );

			auto sizeBytes = std::bit_cast<std::array<char, sizeof( ips::record::copy_type::size_type )>>( copy.size() );
			if constexpr ( ips::endianness != std::endian::native ) {
				std::reverse( std::begin( sizeBytes ), std::begin( sizeBytes ) + ips::record::size_bytes );
			}

			ostream.write( sizeBytes.data(), ips::record::size_bytes );
			for ( const auto& byte : copy ) {
				const auto b = static_cast<char>( byte );
				ostream.write( &b, 1 );
			}
		} else if ( std::holds_alternative<ips::record::fill_type>( record.data ) ) {
			constexpr auto zero_size = std::bit_cast<std::array<char, ips::record::size_bytes>>( std::uint16_t { 0 } );

			ostream.write( zero_size.data(), zero_size.size() );

			const auto& fill = std::get<ips::record::fill_type>( record.data );
			auto fillSize = std::bit_cast<std::array<char, ips::record::size_bytes>>( fill.size );
			if constexpr ( ips::endianness != std::endian::native ) {
				std::reverse( std::begin( fillSize ), std::begin( fillSize ) + ips::record::size_bytes );
			}

			ostream.write( fillSize.data(), fillSize.size() );

			const auto b = static_cast<char>( fill.data );
			ostream.write( &b, 1 );
		}
	}

};

} // ips
} // ffv

#endif // define FFV_IPS_WRITER_HPP