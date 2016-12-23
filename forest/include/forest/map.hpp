#include "zone.hpp"

#include <cassert>
#include <iosfwd>
#include <vector>


namespace ppc
{
	//! Unsigned integral type used as the map index.
	using size_type = std::size_t;

	//! Class which represents a satelite map.
	class Map
	{
	public:
		//! Facilites acces to a column from a given row.
		template <typename M>
		struct AccessProxy
		{
			AccessProxy(M map, size_type row)
				: m_map{ map }, m_row{ row }
			{
				assert(m_row < m_map.m_height);
			}

			decltype(auto) operator[](const size_type column)
			{
				assert(column < m_map.m_width);
				return m_map.m_zones[m_row * m_map.m_width + column];
			}

		private:
			M m_map;
			size_type m_row;
		};

		using Zones = std::vector<ZoneType>;

		Map(size_type height = 0, size_type width = 0, Zones zones = {});
		Map(const Map& map) = default;
		Map(Map&& map) noexcept = default;
		Map& operator=(const Map&) = default;
		Map& operator=(Map&&) noexcept = default;

		//! Returns the height of the map.
		const size_type height() const { return m_height; }

		//! Returns the width of the map.
		const size_type width() const { return m_width; }

		//! Returns the vector containing all the zones in the map.
		const Zones& data() const { return m_zones; }

		//! 2D access operator.
		auto operator[](const size_type row) { return AccessProxy<decltype(*this)>(*this, row); }
		auto operator[](const size_type row) const { return AccessProxy<decltype(*this)>(*this, row); }

	private:
		friend std::istream& operator >> (std::istream& in, Map& map);

		size_type m_height;
		size_type m_width;
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