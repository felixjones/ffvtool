#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "ffv/gba.hpp"
#include "ffv/ips_writer.hpp"
#include "ffv/rom.hpp"
#include "ffv/text_mutator.hpp"
#include "ffv/text_table.hpp"

struct rpge_constants {
	static constexpr std::uint32_t crc32 = 0xf11f1026;
};

static std::vector<std::byte> to_agb( const ffv::rom& ipsRom, std::uint32_t address, std::uint32_t end, const ffv::text_table::type& sfcTextTable, const ffv::text_table::type& gbaTextTable );

static constexpr std::pair<std::string_view, std::string_view> find_replace[] = {
	{ "Ca...", "Kr..." },
	{ "Ra...", "Krile..." },

	{ "don'y", "don't" },
	{ "goin", "going" },
	{ "hellhouse", "hell-house" },
	{ "ok", "okay" },
	{ "weakpoint", "weak-point" },
	{ "youself", "yourself" },
	{ "millenium", "millennium" },

	{ "Adamantium", "Adamantite" },
	{ "Ancient Library", "Library of the Ancients" },
	{ "Boco", "Boko" },
	{ "Cara", "Krile" },
	{ "Carbunkle", "Carbuncle" },
	{ "Coco", "Koko" },
	{ "Corna", "Kornago" },
	{ "Dohlm", "Dohlme" },
	{ "Dorgan", "Dorgann" },
	{ "Elementalists", "Geomancers" },
	{ "Esper", "Summon" },
	{ "Firebute", "Fire Lash" },
	{ "Galura", "Garula" },
	{ "Garkimasera", "Garchimacera" },
	{ "Guido", "Ghido" },
	{ "Grociana", "Gloceana" },
	{ "Halikarnassos", "Halicarnassus" },
	{ "Hiryuu", "Hiryu" },
	{ "Hunter", "Ranger" },
	{ "Jacole", "Jachol" },
	{ "Kelb", "Quelb" },
	{ "Kelgar", "Kelger" },
	{ "Kuzar", "Kuza" },
	{ "Lalibo", "Rallybo" },
	{ "Lamia Harp", "Lamia's Harp" },
	{ "Lonka", "Ronka" },
	{ "Magisa", "Magissa" },
	{ "Mediator", "Beastmaster" },
	{ "Merugene", "Melusine" },
	{ "Meteo", "Meteor" },
	{ "Mimic", "Mime" },
	{ "Mua", "Moore" },
	{ "Rugor", "Regole" },
	{ "Shinryuu", "Shinryu" },
	{ "Steamship", "Fire-Powered Ship" },
	{ "Worus", "Walse" },
	{ "Zeza", "Xezat" },
	{ "Zokk", "Zok" },
};

static constexpr std::tuple<std::uint32_t, std::string_view, std::string_view> targetted_find_replace[] = {
	// 21
	{ 22, "`1704`", "`1706`" }, { 22, "`1702`", "`1704`" },
	{ 23, "`1704`", "`1706`" },
	{ 24, "`1702`", "`1704`" },
	// 25
	// 26
	// 27
	// 28

	{ 37, "meteor?`01`", "meteor?`bx`" },
	{ 40, "Huh?`01`", "Huh?`bx`" },

	{ 651, "Ancient", "Library of" },
	{ 651, "Library", "the Ancients" },
	{ 889, "HAve", "Have" },
	{ 1048, "Well,that", "Well, that" },
	{ 1084, "Faris's", "Faris'" },
	{ 1102, "Jar", "Gourd" },
	{ 1202, "Galuf:It'll", "Galuf: It'll" },
	{ 1217, "Guarde", "garde" },
	{ 1887, "Leviathan:I", "Leviathan: I" },
	{ 1921, "Ancient", "Library of" },
	{ 1921, "Library", "the Ancients" },
	{ 2029, "Jar", "Gourd" },
	{ 2069, "Ancient", "Library of" },
	{ 2069, "Library", "the Ancients" },
};

static constexpr std::string_view name_case[] = {
	"Krile",
	"Sandworm",
};

static constexpr std::tuple<std::uint32_t, std::string_view, std::string_view> targetted_post_find_replace[] = {
	{ 41, "Help...", "`01` Help..." },
	{ 175, "The warriors", "`01`The warriors" },
	{ 178, "You might make a", "`01`You might make a" }, { 178, "First, select", "`01`First, select" },
	{ 205, "The pirates", "`01` The pirates" }, { 205, "the`01`Pub...", "the Pub...`01` " },
};

static constexpr std::pair<std::uint32_t, std::string_view> dialog_mark[] = {
	{ 175, "The warriors" },
	{ 205, "... Hic!" },
};

int main( int argc, char * argv[] ) {
	const auto ipsRom = ffv::rom::read_ips( std::ifstream( argv[1], std::istream::binary ) );
	if ( ipsRom.hash() != rpge_constants::crc32 ) [[unlikely]] {
		throw std::invalid_argument( "Stream is not RPGe v1.1" );
	}

	const auto sfcTextTable = ffv::text_table::read( std::ifstream( argv[2] ) );
	if ( sfcTextTable.empty() ) [[unlikely]] {
		throw std::invalid_argument( "Invalid or corrupt SFC text table" );
	}

	auto gbaStream = std::ifstream( argv[5], std::istream::binary );
	const auto gbaStart = gbaStream.tellg();

	const auto gbaHeader = ffv::gba::read_header( gbaStream );
	if ( !gbaHeader.logo_code ) {
		std::cout << "Warning: GBA header missing logo\n";
	}
	if ( !gbaHeader.fixed ) {
		std::cout << "Warning: GBA header missing fixed byte 0x96\n";
	}
	if ( !gbaHeader.complement ) {
		std::cout << "Warning: GBA header complement check fail\n";
	}

	gbaStream.seekg( gbaStart );
	ffv::gba::find_fonts( gbaStream );
	if ( !gbaStream.good() ) [[unlikely]] {
		throw std::invalid_argument( "Missing font table" );
	}

	const auto fontTable = ffv::gba::read_fonts( gbaStream );

	const auto gbaTextTable = ffv::text_table::read( std::ifstream( argv[6] ) );
	if ( gbaTextTable.empty() ) [[unlikely]] {
		throw std::invalid_argument( "Invalid or corrupt GBA text table" );
	}

	auto address = std::stoul( argv[3], nullptr, 16 );
	const auto end = std::stoul( argv[4], nullptr, 16 );
	if ( end < address ) [[unlikely]] {
		throw std::invalid_argument( "Specified text start address greater than text end address" );
	}

	std::cout << "Translating to GBA\n";
	const auto agbData = to_agb( ipsRom, address, end, sfcTextTable, gbaTextTable );
	auto mutator = ffv::text_mutator( agbData, gbaTextTable, fontTable );

	std::cout << "Find replace\n";
	for ( const auto& pair : find_replace ) {
		std::cout << '\t' << pair.first << " -> " << pair.second << '\n';
		mutator.find_replace( pair.first, pair.second );
	}

	std::cout << "Indexed find replace\n";
	for ( const auto& tuple : targetted_find_replace ) {
		std::cout << "\t[" << std::get<0>( tuple ) << "] " << std::get<1>( tuple ) << " -> " << std::get<2>( tuple );
		if ( !mutator.target_find_replace( std::get<0>( tuple ), std::get<1>( tuple ), std::get<2>( tuple ) ) ) [[unlikely]] {
			std::cout << " WARNING Nothing found\n";
		} else [[likely]] {
			std::cout << '\n';
		}
	}

	std::cout << "Applying name casing\n";
	for ( const auto& name : name_case ) {
		std::cout << '\t' << name << '\n';
		mutator.name_case( name );
	}

	std::cout << "Marking manual dialogs\n";
	for ( const auto& p : dialog_mark ) {
		std::cout << "\t[" << p.first << "] " << p.second;
		if ( !mutator.mark_dialog( p.first, p.second ) ) [[unlikely]] {
			std::cout << " WARNING Nothing found\n";
		} else [[likely]] {
			std::cout << '\n';
		}
	}

	std::cout << "Reflowing dialog\n";
	mutator.dialog_reflow();

	std::cout << "Reflowing the not dialog\n";
	mutator.text_reflow();

	std::cout << "Post indexed find replace\n";
	for ( const auto& tuple : targetted_post_find_replace ) {
		std::cout << "\t[" << std::get<0>( tuple ) << "] " << std::get<1>( tuple ) << " -> " << std::get<2>( tuple );
		if ( !mutator.target_find_replace( std::get<0>( tuple ), std::get<1>( tuple ), std::get<2>( tuple ) ) ) [[unlikely]] {
			std::cout << " WARNING Nothing found\n";
		} else [[likely]] {
			std::cout << '\n';
		}
	}

	std::cout << "Writing IPS\n";
	gbaStream.seekg( gbaStart );
	ffv::gba::find_texts( gbaStream );
	if ( !gbaStream.good() ) [[unlikely]] {
		throw std::invalid_argument( "Missing text table" );
	}

	const auto textStart = gbaStream.tellg();
	const auto textData = ffv::gba::read_texts( gbaStream );
	const auto textBegin = std::stoul( argv[7], nullptr, 10 );

	auto writer = ffv::ips::writer();

	writer.seekg( textData.offsets[textBegin] + textStart );

	int offsetIndex = 0;
	std::uint32_t offset = textData.offsets[textBegin];
	std::vector<std::uint32_t> offsets;
	for ( const auto& line : mutator.lines() ) {
		if ( offsetIndex == 2009 ) {
			offsets.push_back( textData.offsets[2096] );
			offsets.push_back( textData.offsets[2097] );
			offsets.push_back( textData.offsets[2098] );
		}
		offsetIndex++;

		offsets.push_back( offset );

		auto begin = std::cbegin( line );
		while ( begin != std::cend( line ) ) {
			auto end = begin + 1;
			auto key = gbaTextTable.rfind( std::string( begin, end ) );
			while ( key.empty() ) {
				if ( end == std::cend( line ) ) {
					break;
				}
				++end;
				key = gbaTextTable.rfind( std::string( begin, end ) );
			}

			if ( key.empty() ) [[unlikely]] {
				std::cout << " WARNING No key for string \"" << std::string( begin, end ) << "\"\n";
			}

			offset += static_cast<std::uint32_t>( key.size() );
			writer.write( std::cbegin( key ), std::cend( key ) );

			begin = end;
		}
	}

	writer.seekg( sizeof( textData.header ) + ( 4 * static_cast<std::size_t>( textBegin ) ) + textStart );
	for ( const auto offset : offsets ) {
		writer.write( offset );
		offsetIndex++;
	}

	auto ips = std::ofstream( "C:\\Users\\felixjones\\source\\repos\\ffvtool\\roms\\testU\\out.ips", std::ostream::binary );
	writer.compile( ips );
	ips.close();

	return 0;
}

std::vector<std::byte> to_agb( const ffv::rom& ipsRom, std::uint32_t address, std::uint32_t end, const ffv::text_table::type& sfcTextTable, const ffv::text_table::type& gbaTextTable ) {
	std::vector<std::byte> data;

	auto first = std::cbegin( ipsRom.data() ) + address;
	const auto last = std::cbegin( ipsRom.data() ) + end;
	while ( first != last ) {
		auto begin = first;
		const auto it = sfcTextTable.find( first, last );
		++first;

		if ( it->value().has_value() ) {
			const auto gbaKey = gbaTextTable.rfind( it->value().value() );
			if ( !gbaKey.empty() ) {
				data.insert( std::end( data ), std::cbegin( gbaKey ), std::cend( gbaKey ) );
			} else [[unlikely]] {
				std::cout << "Warning: Missing GBA character for code ";
				while ( begin != first ) {
					std::cout << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<int>( *begin++ );
				}
				std::cout << '\n';
			}
		} else [[unlikely]] {
			std::cout << "Warning: Missing SFC character for code ";
			while ( begin != first ) {
				std::cout << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<int>( *begin++ );
			}
			std::cout << '\n';
		}
	}

	data.shrink_to_fit();
	return data;
}
