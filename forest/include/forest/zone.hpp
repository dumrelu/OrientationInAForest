#include <cstdint>
#include <iosfwd>

namespace ppc
{
	//! Indicates the type of a single square on the map/area
	enum ZoneType : std::uint8_t
	{
		OPEN = 0b0001, 
		ROAD = 0b0010, 
		TREE = 0b0100, 
		CLIFF = 0b1000, 
		UNKNOWN = 0b1111
	};

	//! Reads a zone from a stream.
	/*!
		Zones will be in character form, as follows:
			- 'O' = ZoneType::OPEN
			- 'R' = ZoneType::ROAD
			- 'T' = ZoneType::TREE
			- 'C' = ZoneType::CLIFF
		Note: Unknown zones are not allowed!
	*/
	std::istream& operator>>(std::istream& in, ZoneType& zone);

	//! Writes a zone to a stream.
	/*!
		See operator>> for more details.
	*/
	std::ostream& operator<<(std::ostream& out, ZoneType zone);
}