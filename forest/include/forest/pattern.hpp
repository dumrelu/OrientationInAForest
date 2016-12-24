#pragma once

#include "zone.hpp"
#include "map.hpp"
#include "area.hpp"

#include <boost/optional.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

namespace ppc
{
	struct Pattern;
	struct Location;

	//! Searches for the given pattern in the given map.
	/*!
		The MapConcept has to provide the following methods:
			1. map.height()
			2. map.width()
			3. map[y][x]
		Note: UNKNOWN zones in the pattern will match any zone, on the other hand, 
	UNKNOWN zones in the map will never match.
	*/
	template <typename MapConcept>
	std::vector<Location> match_pattern(const MapConcept& map, const Pattern& pattern, boost::optional<Area> searchArea = {});

	//! Represents a location on the map where the pattern matched.
	struct Location
	{
		size_type x;
		size_type y;
	};

	//! Represents a rectangular pattern of zones.
	struct Pattern
	{
		size_type height;
		size_type width;
		Map::Zones zones;

	private:
		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar & height;
			ar & width;
			ar & zones;
		}
	};

	namespace detail
	{
		const bool is_pattern_inside_area(const Area& area, const size_type x, const size_type y, const Pattern& pattern)
		{
			return x >= area.x && x + pattern.width <= area.x + area.width
				&& y >= area.y && y + pattern.height <= area.y + area.height;
		}

		template <typename MapConcept>
		const bool matches_pattern(const MapConcept& map, const Area& area, size_type x, size_type y, const Pattern& pattern)
		{
			if (!is_pattern_inside_area(area, x, y, pattern))
			{
				return false;
			}

			size_type patternIdx = 0;
			for (size_type currentY = y; currentY < y + pattern.height; ++currentY)
			{
				for (size_type currentX = x; currentX < x + pattern.width; ++currentX)
				{
					const auto mapZone = map[currentY][currentX];
					if (mapZone == UNKNOWN)
					{
						return false;
					}

					const auto patternZone = pattern.zones[patternIdx++];
					if (!(mapZone & patternZone))
					{
						return false;
					}
				}
			}
			return true;
		}
	}

	template <typename MapConcept>
	std::vector<Location> match_pattern(const MapConcept& map, const Pattern& pattern, boost::optional<Area> searchArea)
	{
		Area area{};
		if (searchArea)
		{
			area = *searchArea;
		}
		else
		{
			area = { 0, 0, map.height(), map.width() };
		}

		std::vector<Location> matches;
		for (size_type y = area.y; y < area.y + area.height; ++y)	//TODO: possible optimization: if pattern doesn't fit, skip to the next row
		{
			for (size_type x = area.x; x < area.x + area.width; ++x)
			{
				if (detail::matches_pattern(map, area, x, y, pattern))
				{
					matches.push_back({ x, y });
				}
			}
		}

		return matches;
	}
}