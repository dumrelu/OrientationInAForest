#pragma once

#include "common.hpp"
#include "forest/pattern.hpp"

#include <random>
#include <tuple>

#include <boost/optional.hpp>

namespace ppc
{
	//!
	class LocationFinderMaster
	{
	public:
		LocationFinderMaster(mpi::communicator workers, mpi::communicator orientee);

		index_pair run(const Map& map);

	private:
		std::pair<query_result, Pattern> initialQuery(Direction initialDirection);
		query_result query(boost::optional<Direction> direction = {});
		index_pair computeFinalLocation(const index_pair& patternPosition);
		const bool validateSolution(const index_pair& finalLocation);


		mpi::communicator m_workers;
		mpi::communicator m_orientee;

		std::default_random_engine m_engine;
	};
}