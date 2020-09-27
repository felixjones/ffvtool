#ifndef FFV_ROM_HPP
#define FFV_ROM_HPP

#include <istream>
#include <vector>

#include "crc.hpp"

namespace ffv {

class rom {
public:
	using vector_type = std::vector<std::byte>;
	using size_type = vector_type::size_type;

	static rom	read_ips( std::istream& streamSource );

	constexpr auto& data() const noexcept {
		return m_data;
	}

	constexpr auto& hash() const noexcept {
		return m_hash;
	}

	auto at( const size_type pos ) const noexcept {
		return m_data.at( pos );
	}

protected:
	rom( const vector_type& data, const crc32& hash ) noexcept : m_data { data }, m_hash { hash } {}

	const vector_type	m_data;
	const crc32			m_hash;

};

} // ffv

#endif // define FFV_ROM_HPP
