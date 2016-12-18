#include "map.hpp"

#include <boost/serialization/serialization.hpp>

namespace ppc
{
	struct Area;

	//! Extracts an area from the given map.
	/*!
		The first argument is the map from which to extract the area and 
	the second parameter considers {x, y, width, height} as input and puts
	the result in {data}.
		Returns true if area is valid, false otherwise.
	*/
	const bool extract_area(const Map& map, Area& area);

	//! Creates a map from the given area.
	Map create_map(Area area);

	//! Translates from global coordinates to local coordinates.
	Area to_local(Area area, Map::size_type x, Map::size_type y);

	//! Translate from local coordinates to global coordinates.
	Area to_global(Area area, Map::size_type x, Map::size_type y);


	//! Updates the given area on the map.
	/*!
		Returns true if the area is valid, false otherwise.
		Note: Unknown zones in the area will be left unmodified.
	*/
	const bool update_area(Map& map, const Area& area);

	//! Defines an area of a map.
	/*!
		Used to send/recv parts of a map.
	*/
	struct Area
	{
		using size_type = Map::size_type;
		using Rows = Map::Rows;

		size_type x;
		size_type y;
		size_type width;
		size_type height;
		Rows data;

	private:
		friend class boost::serialization::access;

		template <typename Archive>
		void serialize(Archive& ar, const unsigned int /*version*/)
		{
			ar & x;
			ar & y;
			ar & width;
			ar & height;
			ar & data;
		}
	};
}