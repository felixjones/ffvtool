#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "ffv/gba.hpp"
#include "ffv/gba_texts.hpp"
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
	{ "Dohlm", "Dhorme" },
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
	{ "Mold Wind", "Moldwynd" },
	{ "Mua", "Moore" },
	{ "Rugor", "Regole" },
	{ "Shinryuu", "Shinryu" },
	{ "Steamship", "Fire-Powered Ship" },
	{ "Worus", "Walse" },
	{ "Zeza", "Xezat" },
	{ "Zokk", "Zok" },
};

static constexpr std::tuple<std::uint32_t, std::string_view, std::string_view> targetted_find_replace[] = {
	{ 2, "Obtained", "   Obtained" },
	{ 3, "Obtained", "   Obtained" },
	{ 4, "Obtained", "   Obtained" },

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

	{ 112, "Received 8 Potions!", "`bx``nl`Received 8 Potions!" },

	{ 144, "Received 5 Potions!", "`bx``nl`Received 5 Potions!" },

	{ 169, "         These hold the powers`01`          of the ancient heroes`01`             that dwell within.", "          These hold the powers of the`01`       ancient heroes that dwell within." },

	{ 208, "long ago...", "long ago...`nl`" },
	{ 209, "country called Walse. ", "country called Walse.`bx`" },

	{ 238, "'Wait Mode'`01`", "'Wait Mode'`nl`" },
	{ 238, "you choose your commands.  ", "you choose your commands.  `nl`" },

	{ 240, "'Double Grip'`01`", "'Double Grip'`nl`" },
	{ 240, "'Dash'`01`", "`nl`'Dash'`nl`" },
	{ 240, "Having a Thief in your party is fine`01`  as well.", "Having a Thief in your party is fine`01`  as well.`nl`" },
	{ 240, "'Blue Magic' and 'Learning'`01`", "'Blue Magic' and 'Learning'`nl`" },

	{ 259, "Please, take this!`01``01``01``01``02`:  What is it?", "Please, take this!`01``02`:  What is it?`01``01``01`" },

	{ 275, "Three Crystals...?`01`", "Three Crystals...?`bx`" },
	{ 276, " shatter...?`01`", " shatter...?`bx`" },
	{ 285, "wind...`01`  ... ... ...", "wind...`nl`...`nl`..." },

	{ 289, "It couldn't be...", "It couldn't be...`nl`" },
	{ 291, "Stand guard!!", "Stand guard!!`nl`" },
	{ 294, "Let go!", "Let go!`nl`" }, { 294, "Let go of me!!!", "Let go of me!!!`nl`" },
	{ 296, "This ship...", "This ship...`nl`" },
	{ 297, "Faris...", "Faris...`nl`" },

	{ 299, "through here?", "through here?`nl`" }, { 299, "What are you saying?", "What are you saying?`nl`" },

	{ 300, "This place seems safe.", "This place seems safe.`nl`" },
	{ 301, "clothes...", "clothes...`nl`" },
	{ 303, "How can you say that?", "How can you say that?`nl`" },
	{ 304, "Stop that!", "Stop that!`nl`" },
	{ 309, "Hmph.", "Hmph.`nl`" },
	{ 310, "Well, get over it.", "Well, get over it.`nl`" },
	{ 311, "Aaaah, slept well...", "Aaaah, slept well...`nl`" },

	{ 316, "`02`...", "`02`...`nl`" },
	{ 318, "Lenna!?....", "Lenna!?....`nl`" },
	{ 319, "Grandpa...", "Grandpa...`nl`" },
	{ 320, "you...?", "you...?`nl`" },
	{ 322, "Who the heck are you!?", "Who the heck are you!?`bx`" }, { 322, "Hrmph...", "Hrmph...`nl`" }, { 322, "you...?", "you...?`bx`" },
	{ 324, "her!", "her!`nl`" },
	{ 326, "Galuf!", "Galuf!`nl`" },

	{ 328, "Obtained", "   Obtained" },

	{ 339, "can't get to Walse.", "can't get to Walse.`nl`" },
	{ 343, " sea.", " sea.`bx`" },
	{ 354, "Mountain.", "Mountain.`bx`" },
	{ 356, "pressing the Y", "pressing the R" },
	{ 357, "hands!", "hands!`nl`" },
	{ 361, "to stop them!", "to stop them!`nl`" },
	{ 366, "What kind of dragon!?", "What kind of dragon!?`bx`" },
	{ 368, "Hiryu's wounds...", "Hiryu's wounds...`nl`" },
	{ 369, "we had the Hiryu.", "we had the Hiryu.`bx`" },

	{ 373, "Hehehe...", "Hehehe...`nl`" }, { 373, "tough `01`", "tough " },

	{ 381, "Hiryu...", "Hiryu...`nl`" },
	{ 383, "alright.", "alright.`nl`" },
	{ 384, "Hurry...", "Hurry...`nl`" },
	{ 387, "Lenna's status", "  Lenna's status" }, { 387, "restored!`01``01``01`", "restored!`bx`" },

	{ 390, "you?", "you?`nl`" },

	{ 392, "the Crystal!", "the Crystal!`nl`" },
	{ 394, "great!", "great!`nl`" },
	{ 400, "Tower.", "Tower.`nl`" },

	{ 415, "Lone Wolf!!", "Lone Wolf!!`nl`" },
	{ 416, "Lone Wolf!!", "Lone Wolf!!`nl`" }, { 416, "you guys!", "you guys!`nl`" },
	{ 420, "out...`01`", "out...`nl`" },
	{ 424, "book...", "book...`nl`" },
	{ 429, "Shiva...", "Shiva...`nl`" }, { 429, "masters?", "masters?`nl`" },

	{ 431, "King Walse!", "King Walse!`nl`" },
	{ 432, "Please!`01`", "Please!`nl`" }, { 432, "(chuckle)...", "(chuckle)...`nl`" }, { 432, "kidding...", "kidding...`nl`" }, { 432, "that!  `01`", "that!  `bx`" },
	{ 434, "be sure that same", "be sure that the same" },
	{ 438, "Quickly!`01`", "Quickly!`nl`" },

	{ 441, "after him...`01``01`", "after him...`bx`" },
	{ 443, "upstairs...`01`", "upstairs...`nl`" },
	{ 449, "Unnh...`01`", "Unnh...`nl`" },
	{ 451, "Protect the...`01`", "Protect the... `nl`" },
	{ 460, "Syldra!`01`", "Syldra!`nl`" },
	{ 461, "Syldra...`01`", "Syldra...`nl`" },
	{ 462, "Syldraaa!`01`", "Syldraaa!`nl`" },
	{ 463, "Syldra...`01`", "Syldra...`nl`" },

	{ 399, "shattered.`01`", "shattered.`bx`" },
	{ 401, "west...`01``01`", "west...`bx`" },

	{ 467, "Garula...", "Garula...`nl`" },
	{ 468, "up north.`01`", "up north.`bx`" },
	{ 470, "correct... `01`", "correct... `nl`" }, { 470, "King Walse!`01`", "King Walse!`nl`" }, { 470, "Please hurry to Karnak.", "Please hurry to Karnak.`nl`" },
	{ 471, "near Karnak...`01`", "near Karnak...`nl`" }, { 471, "waste...`01`", "waste...`nl`" },
	{ 473, "Tycoon?", "Tycoon?`nl`" },

	{ 477, "recognize it...`01`", "recognize it...`nl`" },

	{ 485, "everyone!!`01`", "everyone!!`nl`" },
	{ 486, "It's not just Tycoon...", "It's not just Tycoon...`nl`" },
	{ 487, "And...", "And...`nl`" },
	{ 489, "Father.", "Father.`nl`" },
	{ 492, "Lenna...", "Lenna...`nl`" },
	{ 494, "princess...?", "princess...?`nl`" },
	{ 495, "But...", "But...`nl`" },
	{ 496, "`02`!`01``01``01`", "`02`!`bx`" }, { 496, "`02`:  ...", "`02`:  ...`nl`" },

	{ 549, "Crystal!", "Crystal!`bx`" },
	{ 550, "coming `01`  from the Library of the Ancients.", "coming from the library." },
	{ 553, "\"Crew Dust\"?`01`", "`nl`\"Crew Dust\"?`bx`" },
	{ 555, "???`01`", "???`bx`" },

	{ 561, "Library of the Ancients", "Ancients' Library" }, { 561, "power much greater", "greater power" },
	{ 562, "shattering...", "shattering...`bx`" },
	{ 563, "we do...`01`", "we do...`bx`Cid: " },
	{ 565, "What!?`01`", "What!?`nl`" },
	{ 566, "Cid!!`01`", "Cid!!`nl`" },
	{ 567, "Huh?`01`", "Huh?`bx`" }, { 567, "power...`01`", "power...`bx`" },
	{ 572, "prepared.`01`", "prepared.`bx`" },
	{ 573, "castle!`01`", "castle!`nl`" },
	{ 575, "Crystal...`01`", "Crystal...`nl`" },
	{ 576, "way!`01`", "way!`nl`" },
	{ 578, "cracks...`01`", "cracks...`nl`" }, { 578, "attacks...`01`", "attacks...`nl`" },

	{ 591, "alright.`01`", "alright.`nl`" },

	{ 606, "Crystal of Fire.", "Crystal of Fire.`bx`Queen: " },
	{ 607, "I'm alright.", "Queen: I'm alright.`nl`" }, { 607, "Please...`01`", "Please...`nl`" },
	{ 612, "me!?`01`", "me!?`nl`" }, { 612, "remember...`01`", "remember...`nl`" },
	{ 613, "doing?`01`", "doing?`nl`" },
	{ 614, "three...`01`", "three...`nl`" }, { 614, "seal...`01`", "seal...`nl`" },
	{ 617, "go!`01`", "go!`nl`" }, { 617, "quickly!  `01`", "quickly!  `nl`" },
	{ 619, "Galuf...`01`", "Galuf...`nl`" },
	{ 621, "The Crystal...`01`", "The Crystal...`nl`" }, { 621, "probably...    `01`", "probably...    `nl`" },	
	{ 624, "Yes!!`01`", "Yes!!`nl`" },

	{ 641, "Ancients.`01`", "Ancients.`bx`" },
	{ 645, "Go away!`01`", "Go away!`nl`" },
	{ 651, "Jachol.", "Jachol.`bx`" }, { 651, " Library", " the Ancients" }, { 651, "Ancient ", "Library of " },

	{ 664, "now...`01`", "now...`bx`" },

	{ 682, "What?`01`", "What?`nl`" },
	{ 683, "...!?`01`", "...!?`bx`" },
	{ 686, "Huh?`01`", "Huh?`nl`" },
	{ 687, "earlier!`01`", "earlier!`nl`" },
	{ 688, "Look... `01`", "Look... `nl`" },
	{ 692, "What!?`01`", "What!?`nl`" },

	{ 695, "now...`01`", "now...`nl`" },
	{ 698, "Ow!`01`", "Ow!`nl`" },
	{ 701, "Mid, ...`01`", "Mid, ...`nl`" }, { 701, "again...`01`", "again...`nl`" },
	{ 705, "book...!?`01`", "book...!?`nl`" },

	{ 715, "Ouch!`01`", "Ouch!`nl`" },
	{ 721, "then!`01`", "then!`nl`" },
	{ 722, "meteor...`01`", "meteor...`nl`" },
	{ 723, "right...`01`", "right...`nl`" },

	{ 729, "slept well!`01`", "slept well!`nl`" },
	{ 731, "Please do!!`01`", "Please do!!`nl`" }, { 731, "Ancients.`01`", "Ancients.`bx`" },

	{ 738, "But something about it", "But something" },
	{ 740, "Cid and Mid.`01`", "Cid and Mid.`nl`" },

	{ 770, "frogs are still`01`", "frogs are still`01` " },
	{ 777, "behind!`01``01``01`", "behind!`bx``nl`   " },
	{ 781, "tender feelings...`01`", "tender feelings...`nl`" },

	{ 807, "Lenna: ", "" },
	{ 815, "Cid...", "Cid...`nl`" },
	{ 817, "can enter it...", "can enter it...`nl`" },
	{ 822, "Desert...", "Desert...`nl`" },
	{ 825, "bird...", "bird...`nl`" },

	{ 1024, "It's on my", "It's on the" },
	{ 1025, "much for us...", "much for us...`nl`" },
	{ 1026, "journey!", "journey!`bx`" },
	{ 1027, "`02`?`01`", "`02`?`nl`" }, { 1027, "Oh my gosh!!`01`", "Oh my gosh!!`nl`" },
	{ 1028, "`02`!!`01`", "`02`!!`nl`" }, { 1028, "You've come home!`01`", " You've come home!`bx`" }, { 1028, "so long...`01`", "so long...`bx`" },
	{ 1029, "It's me.`01`", "It's me.`nl`" },
	{ 1034, "back then....`01`", "back then...`nl`" },
	{ 1035, "No way...`01`", "No way...`nl`" }, { 1035, "`02`!?`01`", "`02`!?`nl`" }, 
	{ 1036, "?`01`", "?`nl`" }, { 1036, "I see...`01`", "I see...`nl`" },

	{ 1040, "again?    `1705`", "again?    `1704`" },
	{ 1041, "on...    `1706`", "on...    `1704`" }, { 1041, "finish...    `1706`", "finish...    `1704`" }, { 1041, "You need to get better.`01`", "You need to get better.`1704``0f``bx`" },
	{ 1042, "WERE awake...`01``01`", "WERE awake...`1702``0f``bx`" }, { 1042, "bad guys `01`  again?", "bad guys again?" }, { 1042, "from her, `01`  won't you?", "from her, won't you?`1704``0f`" }, { 1042, "right.    `1706`", "right.    `1702`" },
	{ 1043, "Stella!`01`", "Stella!`nl`" },

	{ 1046, "melody.`01`", "melody.`nl`" }, { 1046, "memory...`01`", "memory...`nl`" },

	{ 1054, "village.", "village.`nl`" }, { 1054, "little...", "little...`bx`" }, { 1054, " after that", "`02`: After that" },
	{ 1058, "A father...`01`", "A father...`nl`" },
	{ 1059, "let's go back!`01`", "let's go back!`nl`" },

	{ 29, "the Rings...`01``01``01`", "the Rings...`bx`" },

	{ 1334, "King's room.", "King's room.`nl`" },

	{ 1922, "Fire-Powered Ship...`01`", "Fire-Powered Ship...`bx`" },
	{ 1966, "This island is marvelous.`01`", "This island is marvelous.`bx`" },

	{ 1397, "Catch", "   Catch" },
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
	{ 174, "How to use the Pieces of Crystal", "       How to use the Pieces of Crystal" },
		{ 174, "Would you like an explanation of the", "    Would you like an explanation of the" },
		{ 174, "Job/Ability System?", "                  Job/Ability System?" },
	{ 175, "The warriors", "`01`The warriors" },
	{ 178, "You might make a", "`01`You might make a" }, { 178, "First, select", "`01`First, select" },
	{ 205, "The pirates", "`01` The pirates" }, { 205, "the`01`Pub...", "the Pub...`01` " }, { 205, "Hiccup!...", " Hiccup!..." },
	{ 240, "'Blue Magic' and 'Learning'", "`01`'Blue Magic' and 'Learning'" },
	{ 357, "Hic!", " Hic!" },

	{ 422, "How", " How" }, { 422, "with", " with" },
	{ 424, "\"Summon", " \"Summon" },

	{ 449, "...Sir", " ...Sir" },
	{ 451, "Fire Crystal", " Fire Crystal" },
	{ 496, "...HUHN!", " ...HUHN!" },

	{ 470, "-cough-...", " -cough-..." },

	{ 535, "That's good.", " That's good." },
	{ 549, "amazingly", " amazingly" },
	{ 553, "\"Crew Dust\"", " \"Crew Dust\"" },

	{ 605, "I must get", " I must get" },
	{ 617, "And protect", " And protect" },
	{ 621, "...has", " ...has" }, { 621, "...swallowed", " ...swallowed" },
	{ 651, " But the road", "But the road" },

	{ 688, "We might", " We might" },

	{ 701, "...`0c`...", "...`0c` ..." },

	{ 722, "...30", " ...30" },
	{ 723, "..Uhhn....", " ...Uhhn..." },

	{ 777, "Obtained", "                     Obtained" },

	{ 785, "Another", " Another" },
	{ 789, "", "`01`" }, { 789, "Learned \"Life Song\"!`01`", "Learned \"Life Song\"!`1706``01``01`" }, { 789, "vitality", " vitality" },

	{ 815, "The Fire", " The Fire" },

	{ 1026, "journey!", " journey!`01`" }, { 1026, "as long as", " as long as" },
	{ 1027, "`02` is back!!", " `02` is back!!" },
	{ 1028, "You've come home!", " You've come home!`01`" }, { 1028, "so long...", "so long...`01``01`" }, { 1028, "you when", " you when" },
	{ 1030, "I come!", " I come!" },
	{ 1036, "`02`!", " `02`!" },
	{ 1046, "Learned \"Charm Song\"!", "`01`                Learned \"Charm Song\"!" },
	{ 1052, "What's wrong?", " What's wrong?" },

	{ 1334, "You need", " You need" },

	{ 1922, " She's been taken", "She's been taken" },
};

static constexpr std::pair<std::uint32_t, std::string_view> dialog_mark[] = {
	{ 174, "How to use the" }, { 174, "Would you like an" },
	{ 175, "The warriors" },
	{ 205, "... Hic!" },
	{ 238, "'Wait Mode'" },
	{ 240, "'Double Grip'" }, { 240, "'Dash'" }, { 240, "'Blue Magic' and 'Learning'" },
	{ 422, "How" },
	{ 789, "This" },
	{ 1026, "`02`!?" },
	{ 1028, "`02`!!" },
	{ 1030, "...Nine" },
	{ 1042, "`02`: Are you going to" }, { 1042, "And you'll" },
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
	ffv::gba::find_texts( gbaStream );
	if ( !gbaStream.good() ) [[unlikely]] {
		throw std::invalid_argument( "Missing text table" );
	}

	const auto textStart = gbaStream.tellg();
	const auto textData = ffv::gba::read_texts( gbaStream );
	const auto textBegin = std::stoul( argv[7], nullptr, 10 );

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

	const auto itemLength = ffv::gba::max_text_length( textData, {
		{ 3313, 3431 },
		{ 3440, 3529 },
		{ 3535, 3570 },
	}, gbaTextTable, fontTable );

	const auto abilityLength = ffv::gba::max_text_length( textData, {
		{ 4579, 4853 },
	}, gbaTextTable, fontTable );

	auto address = std::stoul( argv[3], nullptr, 16 );
	const auto end = std::stoul( argv[4], nullptr, 16 );
	if ( end < address ) [[unlikely]] {
		throw std::invalid_argument( "Specified text start address greater than text end address" );
	}

	std::cout << "Translating to GBA\n";
	const auto agbData = to_agb( ipsRom, address, end, sfcTextTable, gbaTextTable );
	auto mutator = ffv::text_mutator( agbData, gbaTextTable, fontTable, itemLength, abilityLength );

	std::cout << "Marking manual dialogs\n";
	for ( const auto& p : dialog_mark ) {
		std::cout << "\t[" << p.first << "] " << p.second << '\n';
		mutator.mark_dialog( p.first, p.second );
	}

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
