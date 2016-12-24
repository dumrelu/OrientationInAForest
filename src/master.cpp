#include "master.hpp"
#include "forest/area.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace ppc
{
	Master::Master(mpi::communicator workers, mpi::communicator outside)
		: m_workers{ std::move(workers) }, m_outside{ std::move(outside) }
	{
		assert(m_workers.rank() == 0);
		assert(m_outside.rank() == 0);
	}

	namespace
	{
		// For now only splits verticallys
		auto split(const Map& map, const index_type numOfAreas)
		{
			std::vector<AreaZones> areas;
			areas.reserve(numOfAreas + 1);	// + 1 for scatter(master will recv an empty zone).
			areas.push_back({});

			const auto numOfRowsPerArea = static_cast<index_type>(std::ceil(map.height() / static_cast<double>(numOfAreas)));
			BOOST_LOG_TRIVIAL(debug) << "numOfAreas = " << numOfAreas << ", numOfRowsPerArea = " << numOfRowsPerArea;
			for (index_type i = 0; i < numOfAreas; ++i)
			{
				AreaZones areaZones;
				auto& area = areaZones.area;
				auto& zones = areaZones.zones;

				area.x = 0;
				area.y = i * numOfRowsPerArea;
				area.height = numOfRowsPerArea;
				area.width = map.width();

				if (i == numOfAreas - 1)
				{
					area.height = map.height() - (numOfAreas - 1) * numOfRowsPerArea;
				}

				const auto numOfZones = area.height * area.width;
				zones.reserve(numOfZones);
				const auto* mapBegin = &map[area.y][area.x];
				std::copy(mapBegin, mapBegin + numOfZones, std::back_inserter(zones));

				areas.push_back(std::move(areaZones));
			}


			return areas;
		}
	}

	Master::result Master::run(const Map& map)
	{
		const auto numOfWorkers = m_workers.size() - 1;
		assert(numOfWorkers);

		auto areas = split(map, numOfWorkers);
		mpi::scatter(m_workers, areas, areas.front(), 0);

		return {};
	}
}