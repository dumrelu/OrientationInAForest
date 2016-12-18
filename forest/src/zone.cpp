#include "zone.hpp"

#include <cassert>
#include <istream>
#include <ostream>

namespace ppc
{
	std::istream& operator>>(std::istream& in, ZoneType& zone)
	{
		char c;
		in >> c;

		switch (c)
		{
		case 'O':
			zone = ZoneType::OPEN;
			break;
		case 'R':
			zone = ZoneType::ROAD;
			break;
		case 'T':
			zone = ZoneType::TREE;
			break;
		case 'C':
			zone = ZoneType::CLIFF;
			break;
		default:
			assert(false);
			break;
		}

		return in;
	}

	std::ostream& operator<<(std::ostream& out, ZoneType zone)
	{
		char c{};

		switch (zone)
		{
		case ZoneType::OPEN:
			c = 'O';
			break;
		case ZoneType::ROAD:
			c = 'R';
			break;
		case ZoneType::TREE:
			c = 'T';
			break;
		case ZoneType::CLIFF:
			c = 'C';
			break;
		default:
			break;
		}

		out << c;

		return out;
	}
}