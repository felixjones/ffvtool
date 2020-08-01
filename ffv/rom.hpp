#ifndef FFV_ROM_HPP
#define FFV_ROM_HPP

#include <istream>

namespace ffv {

class rom {
public:
	static rom read_ips( std::istream& streamSource );

protected:
	// rom() : m_memory(  ) {}

	//const std::unique_ptr<const char[]>	m_memory;

};

} // ffv

#endif // define FFV_ROM_HPP
