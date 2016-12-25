#include "map.hpp"

#include <istream>
#include <ostream>

namespace ppc
{
	Map::Map(index_type height, index_type width, Zones zones)
		: m_height{ height }, m_width{ width }, m_zones{ std::move(zones) }
	{
		m_zones.resize(m_height, { m_width, UNKNOWN });
	}

	std::istream& operator>>(std::istream& in, Map& map)
	{
		index_type height;
		index_type width;

		in >> height >> width;
		map = { height, width };
		for (auto& row : map.m_zones)
		{
			for (auto& zone : row)
			{
				in >> zone;
			}
		}

		return in;
	}

	std::ostream& operator<<(std::ostream& out, const Map& map)
	{
		out << map.height() << " " << map.width() << std::endl;

		for (const auto& row : map.data())
		{
			for (const auto zone : row)
			{
				out << zone << " ";
			}
			std::endl(out);
		}

		return out;
	}
}