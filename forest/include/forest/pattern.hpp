#pragma once

#include "zone.hpp"
#include "map.hpp"
#include "area.hpp"

#include <boost/optional.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include <ostream>

namespace ppc
{
	struct Pattern;

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
	std::vector<index_pair> match_pattern(const MapConcept& map, const Pattern& pattern, boost::optional<Area> searchArea = {});

	//! Represents a rectangular pattern of zones.
	struct Pattern
	{
		index_type height;
		index_type width;
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
		inline const bool is_pattern_inside_area(const Area& area, const index_type x, const index_type y, const Pattern& pattern)
		{
			return x >= area.x && x + pattern.width <= area.x + area.width
				&& y >= area.y && y + pattern.height <= area.y + area.height;
		}

		template <typename MapConcept>
		const bool matches_pattern(const MapConcept& map, const Area& area, index_type x, index_type y, const Pattern& pattern)
		{
			if (!is_pattern_inside_area(area, x, y, pattern))
			{
				return false;
			}

			index_type patternIdx = 0;
			for (index_type currentY = y; currentY < y + pattern.height; ++currentY)
			{
				for (index_type currentX = x; currentX < x + pattern.width; ++currentX)
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
	std::vector<index_pair> match_pattern(const MapConcept& map, const Pattern& pattern, boost::optional<Area> searchArea)
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

		std::vector<index_pair> matches;
		for (index_type y = area.y; y < area.y + area.height; ++y)	//TODO: possible optimization: if pattern doesn't fit, skip to the next row
		{
			for (index_type x = area.x; x < area.x + area.width; ++x)
			{
				if (detail::matches_pattern(map, area, x, y, pattern))
				{
					matches.push_back({ x, y });
				}
			}
		}

		return matches;
	}

	//! Debug purpose output stream operator.
	static std::ostream& operator<<(std::ostream& out, const Pattern& pattern)
	{
		out << pattern.height << " " << pattern.width << std::endl;
		for (const auto& row : pattern.zones)
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