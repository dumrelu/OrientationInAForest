#include "map.hpp"

#include <istream>
#include <ostream>

namespace ppc
{
	Map::Map(index_type height, index_type width, Zones zones)
		: m_height{ height }, m_width{ width }, m_zones{ std::move(zones) }
	{
		m_zones.resize(m_width * m_height, UNKNOWN);
	}

	std::istream& operator>>(std::istream& in, Map& map)
	{
		index_type height;
		index_type width;

		in >> height >> width;
		map = { height, width };
		for (auto& zone : map.m_zones)
		{
			in >> zone;
		}

		return in;
	}

	std::ostream& operator<<(std::ostream& out, const Map& map)
	{
		out << map.height() << " " << map.width() << std::endl;

		index_type idx{ 0 };
		for (const auto zone : map.data())
		{
			out << zone << " ";
			if (++idx % map.width() == 0)
			{
				out << std::endl;
			}
		}

		return out;
	}
}