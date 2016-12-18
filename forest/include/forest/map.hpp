#include "zone.hpp"

#include <iosfwd>
#include <vector>

namespace ppc
{
	class Map
	{
	public:
		using size_type = std::size_t;
		using Row = std::vector<ZoneType>;
		using Rows = std::vector<Row>;

		Map(size_type height = 0, size_type width = 0);
		Map(const Map& map) = default;
		Map(Map&& map) = default;
		Map& operator=(const Map&) = default;
		Map& operator=(Map&&) = default;

		//! Returns the height of the map.
		size_type height() const;

		//! Returns the width of the map.
		size_type width() const;

		//! Returns the map data.
		const Rows& data() const { return m_data; }

		//! Returns the given row.
		Row& operator[](size_type idx) { return m_data[idx]; }
		const Row& operator[](size_type idx) const { return m_data[idx]; }

	private:
		friend std::istream& operator>>(std::istream&, Map&);

		Rows m_data;
	};

	//! Read the map from an input stream.
	/*!
		Reading is done by first reading 2 values of type Map::size_type, which indicate
	the height and the width of the map, followed by reading width * height number of zones.
	*/
	std::istream& operator>>(std::istream& in, Map& map);

	//! Writes the map to the given output stream.
	/*!
		See operator>> for more details.
	*/
	std::ostream& operator<<(std::ostream& out, const Map& map);
}