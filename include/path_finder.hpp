#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class PathFinder
	{
	public:
		PathFinder(mpi::communicator workers, mpi::communicator orientee);

		//! 
		void run(const Map& map, const Area& area, boost::optional<index_pair> startingPosition = {});

	private:
		mpi::communicator m_workers;
		mpi::communicator m_orientee;
	};
}