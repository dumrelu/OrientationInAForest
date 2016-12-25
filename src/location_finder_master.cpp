#include "location_finder_master.hpp"

#include <cassert>
#include <cmath>

namespace ppc
{
	LocationFinderMaster::LocationFinderMaster(mpi::communicator workers, mpi::communicator orientee)
		: m_workers{ std::move(workers) }, m_orientee{ std::move(orientee) }
	{
		assert(m_workers.rank() == 0);
		assert(m_orientee.rank() == 0);
	}

	namespace
	{
		//! For now splitting is done only vertically.
		auto split(const Map& map, index_type numOfAreas)
		{
			std::vector<Area> areas;
			areas.reserve(numOfAreas + 1);	// + 1 so it can be used for scatter
			areas.push_back({});			//master doesn't need an area

			const auto numOfRowsPerArea = static_cast<index_type>(std::ceil(map.height() / static_cast<double>(numOfAreas)));
			PPC_LOG(debug) << "Number of rows per area: " << numOfRowsPerArea;
			for (index_type i = 0; i < numOfAreas; ++i)
			{
				auto area = Area{ 0, i * numOfRowsPerArea, numOfRowsPerArea, map.width() };
				if (i == numOfAreas - 1)
				{
					area.height = map.height() - (numOfAreas - 1) * numOfRowsPerArea;
				}

				areas.push_back(std::move(area));
			}

			return areas;
		}
	}

	index_pair LocationFinderMaster::run(const Map& map)
	{
		const auto numOfWorkers = m_workers.size() - 1;
		assert(numOfWorkers);
		PPC_LOG(debug) << "Number of workers available for location finding: " << numOfWorkers;

		auto areas = split(map, numOfWorkers);
		mpi::scatter(m_workers, areas, areas.front(), 0);

		PPC_LOG(trace) << "Sending query...";
		Direction dir = FORWARD;
		m_orientee.send(1, tags::QUERY | tags::MOVE, dir);

		query_result result;
		auto status = m_orientee.recv(mpi::any_source, mpi::any_tag, result);
		if (status.tag() != tags::OK)
		{
			PPC_LOG(fatal) << "Invalid query";
			std::exit(1);
		}
		PPC_LOG(trace) << "Received query result: ( FWD = "
			<< result[FORWARD] << ", BCK = " << result[BACKWARDS]
			<< ", LEFT = " << result[LEFT] << ", RIGHT = " << result[RIGHT] << ")";

		m_orientee.send(1, tags::STOP, dir);

		return {};
	}
}