#ifndef FFV_AGB_HUFF_HPP
#define FFV_AGB_HUFF_HPP

#include <array>
#include <bit>

namespace ffv {

template <unsigned TreeLength>
class agb_huff {
public:
	static constexpr auto tree_length = TreeLength;

	constexpr agb_huff( const std::array<std::byte, tree_length>& bytes ) noexcept : m_data { to_node_array( bytes ) } {}

	auto decompress4( std::istream& stream, std::size_t length ) const noexcept {
		length &= ~0x3; // Round down to multiple of 4

		std::uint32_t word32;
		std::size_t bits = 0;

		std::vector<std::byte> uncompressed;
		std::size_t pos = 0;

		bool lo = true;
		while ( length || bits ) {
			if ( bits == 0 ) {
				std::array<char, 4> word;
				stream.read( word.data(), word.size() );
				length -= 4;

				word32 = std::bit_cast<std::uint32_t>( word );
				bits = 32;
			}

			--bits;
			if ( word32 & ( 1 << bits ) ) {
				const auto& n = m_data[pos];
				pos = index_right( n );
				if ( pos >= m_data.size() ) break;

				if ( n.end_right ) {
					if ( lo ) {
						uncompressed.push_back( std::bit_cast<std::byte>( m_data[pos] ) );
					} else {
						uncompressed.back() |= std::bit_cast<std::byte>( m_data[pos] ) << 4;
					}
					lo = !lo;
					pos = 0;
				}
			} else {
				const auto& n = m_data[pos];
				pos = index_left( n );
				if ( pos >= m_data.size() ) break;

				if ( n.end_left ) {
					if ( lo ) {
						uncompressed.push_back( std::bit_cast<std::byte>( m_data[pos] ) );
					} else {
						uncompressed.back() |= std::bit_cast<std::byte>( m_data[pos] ) << 4;
					}
					lo = !lo;
					pos = 0;
				}
			}
		}

		return uncompressed;
	}

protected:
	struct node {
		std::uint8_t	next : 6;
		bool			end_left : 1;
		bool			end_right : 1;

		constexpr bool operator ==( const node& other ) const noexcept {
			return this == &other;
		}
	};

	using node_array_type = std::array<node, TreeLength>;

	static constexpr auto to_node_array( const std::array<std::byte, tree_length>& bytes ) noexcept -> node_array_type {
		std::array<node, tree_length> na;
		auto ii = na.size();
		while ( ii-- ) {
			na[ii].next = static_cast<std::uint8_t>( bytes[ii] ) & 0x3f;
			na[ii].end_left = static_cast<bool>( bytes[ii] & std::byte { 0x80 } );
			na[ii].end_right = static_cast<bool>( bytes[ii] & std::byte { 0x40 } );
		}
		return na;
	}

	constexpr auto index_of( const node& node ) const noexcept {
		return std::distance( std::cbegin( m_data ), std::find( std::cbegin( m_data ), std::cend( m_data ), node ) );
	}

	constexpr auto index_left( const node& node ) const noexcept {
		return ( ( index_of( node ) + 1 ) & ~1u ) + ( static_cast<std::size_t>( node.next ) * 2 ) + 1;
	}

	constexpr auto index_right( const node& node ) const noexcept {
		return ( ( index_of( node ) + 1 ) & ~1u ) + ( static_cast<std::size_t>( node.next ) * 2 ) + 2;
	}

	const node_array_type	m_data;

};

template <typename... Ts>
static constexpr auto make_huff( Ts&&... args ) noexcept -> agb_huff<sizeof...( Ts )> {
	return { std::array<std::byte, sizeof...( Ts )> { std::byte( std::forward<Ts>( args ) )... } };
}

} // ffv

#endif // define FFV_AGB_HUFF_HPP