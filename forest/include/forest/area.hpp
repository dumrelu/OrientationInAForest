#pragma once

#include "map.hpp"

#include <boost/optional.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

namespace ppc
{
	struct Area;
	struct AreaZones;

	//! Extrancs the zones from the map in the given area.
	boost::optional<AreaZones> extract_zones(const Map& map, const Area& area);

	//! Creates a map from the given area zones.
	Map create_map(AreaZones areaZones);

	//! Updates the given area of the map.
	const bool update_map(Map& map, const AreaZones& zones);

	//! Represents a rectangular area of a map.
	struct Area
	{
		size_type x;
		size_type y;
		size_type height;
		size_type width;

	private:
		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar & x;
			ar & y;
			ar & width;
			ar & height;
		}
	};

	//! Stores the zones in the given area.
	struct AreaZones
	{
		Area area;
		Map::Zones zones;

	private:
		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar & area;
			ar & zones;
		}
	};
}