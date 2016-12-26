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
		std::tuple<Pattern, index_pair, Direction> initialQuery(Direction orientation);
		query_result query(boost::optional<Direction> direction = {});


		mpi::communicator m_workers;
		mpi::communicator m_orientee;

		std::default_random_engine m_engine;
	};
}