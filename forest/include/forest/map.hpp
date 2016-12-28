#pragma once

#include "zone.hpp"
#include "index.hpp"

#include <cassert>
#include <iosfwd>
#include <utility>
#include <vector>


namespace ppc
{
	//! Class which represents a satelite map.
	class Map
	{
	public:
		using Zones = std::vector<std::vector<ZoneType>>;

		Map(index_type height = 0, index_type width = 0, Zones zones = {});
		Map(const Map& map) = default;
		Map(Map&& map) noexcept = default;
		Map& operator=(const Map&) = default;
		Map& operator=(Map&&) noexcept = default;

		//! Returns the height of the map.
		const index_type height() const { return m_height; }

		//! Returns the width of the map.
		const index_type width() const { return m_width; }

		//! Returns the vector containing all the zones in the map.
		const Zones& data() const { return m_zones; }

		//! 2D access operator.
		decltype(auto) operator[](const index_type row) { return m_zones[row]; }
		decltype(auto) operator[](const index_type row) const { return m_zones[row]; }

	private:
		friend std::istream& operator >> (std::istream& in, Map& map);

		index_type m_height;
		index_type m_width;
		Zones m_zones;
	};

	//! Reads a map from the given input stream.
	/*!
		It starts with reading two integers, the height and the width of the map, 
	followed by reading height * width zones.
	*/
	std::istream& operator>>(std::istream& in, Map& map);

	//! Writes a map to the given output stream.
	/*!
		The written map has the following format:
		"
		height width
		zone00 zone01 ... zone0m
		zone10 zone11 ... zone1m
		.
		.
		.
		zonen0 zonen1 ... zonenm
		"

		Where m = width - 1 and n = height - 1.
	*/
	std::ostream& operator<<(std::ostream& out, const Map& map);
}