#pragma once

#include "common.hpp"
#include "forest/pattern.hpp"

#include <random>
#include <tuple>

namespace ppc
{
	//!
	class LocationFinderMaster
	{
	public:
		LocationFinderMaster(mpi::communicator workers, mpi::communicator orientee);

		LocationOrientationPair run(const Map& map, const bool randomized, boost::optional<Statistics>& stats);

	private:
		std::pair<query_result, Pattern> initialQuery(Direction initialDirection);
		query_result query(boost::optional<Direction> direction = {});
		LocationOrientationPair computeSolution(const index_pair& patternPosition, const index_type height, const index_type width, Direction orientation);
		const bool validateSolution(const LocationOrientationPair& solution);


		mpi::communicator m_workers;
		mpi::communicator m_orientee;
	};
}