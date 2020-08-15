#ifndef FFV_ROM_HPP
#define FFV_ROM_HPP

#include <istream>
#include <vector>

#include "crc.hpp"

namespace ffv {

class rom {
public:
	static rom read_ips( std::istream& streamSource );

	constexpr auto& data() const noexcept {
		return m_data;
	}

	constexpr auto& hash() const noexcept {
		return m_hash;
	}

protected:
	rom( const std::vector<std::byte>& data, const crc32& hash ) noexcept : m_data { std::move( data ) }, m_hash { std::move( hash ) } {}

	const std::vector<std::byte>	m_data;
	const crc32						m_hash;

};

} // ffv

#endif // define FFV_ROM_HPP
