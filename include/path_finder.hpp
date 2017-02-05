#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class PathFinder
	{
	public:
		PathFinder(mpi::communicator workers, mpi::communicator orientee, const index_type workerID, const index_type numOfWorkers);

		//! 
		void run(const Map& map, const Area& area, boost::optional<LocationOrientationPair> locationResult = {});

	private:
		Direction moveTo(Direction currentOrientation, const std::pair<int, int>& offsets);

		mpi::communicator m_workers;
		mpi::communicator m_orientee;
		const index_type m_workerID;
		const index_type m_numOfWorkers;
	};
}