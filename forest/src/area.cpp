#include "area.hpp"

namespace ppc
{
	namespace
	{
		const bool is_area_valid(const Map& map, const Area& area)
		{
			return area.x + area.width <= map.width()
				&& area.y + area.height <= map.height();
		}
	}

	boost::optional<AreaZones> extract_zones(const Map& map, const Area& area)
	{
		if (!is_area_valid(map, area))
		{
			return {};
		}

		AreaZones az{ area, {} };
		az.zones.reserve(area.height * area.width);
		for (size_type y = area.y; y < area.y + area.height; ++y)
		{
			for (size_type x = area.x; x < area.x + area.width; ++x)
			{
				az.zones.push_back(map[y][x]);
			}
		}

		return { std::move(az) };
	}

	Map create_map(AreaZones areaZones)
	{
		const auto& area = areaZones.area;
		return { area.height, area.width, std::move(areaZones.zones) };
	}

	const bool update_map(Map& map, const AreaZones& areaZones)
	{
		if (!is_area_valid(map, areaZones.area))
		{
			return false;
		}

		const auto& area = areaZones.area;
		const auto& zones = areaZones.zones;
		size_type idx = 0;
		for (size_type y = area.y; y < area.y + area.height; ++y)
		{
			for (size_type x = area.x; x < area.x + area.width; ++x)
			{
				map[y][x] = zones[idx++];
			}
		}

		return true;
	}
}