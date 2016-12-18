#include "map.hpp"

#include <cassert>
#include <istream>
#include <ostream>
#include <type_traits>

namespace ppc
{
	Map::Map(size_type height, size_type width)
	{
		m_data = Rows(height, Row(width, ZoneType::UNKNOWN));
	}
	
	Map::Map(Rows data)
		: m_data(std::move(data))
	{
	}

	Map::size_type Map::height() const
	{
		return m_data.size();
	}

	Map::size_type Map::width() const
	{
		if (m_data.empty())
		{
			return 0;
		}
		return m_data.size();
	}

	std::istream& operator>>(std::istream& in, Map& map)
	{
		Map::size_type height{};
		Map::size_type width{};

		in >> height;
		in >> width;

		map = Map{ height, width };
		for (auto& row : map.m_data)
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
		const auto height = map.height();
		const auto width = map.width();

		out << height << std::endl << width << std::endl;
		for (const auto& row : map.data())
		{
			for (auto zone : row)
			{
				out << zone;
			}
			out << std::endl;
		}

		return out;
	}
}