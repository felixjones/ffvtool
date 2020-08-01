#include <fstream>
#include "ffv/rom.hpp"

int main( int argc, char * argv[] ) {
	const auto ips = ffv::rom::read_ips( std::ifstream( argv[1], std::ios::binary ) );

	return 0;
}
