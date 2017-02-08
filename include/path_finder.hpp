#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class PathFinder
	{
	public:
		//! Entrand finding configuration
		enum EntranceSearching
		{
			ANY_ENTRANCE,	//Use the first found entrance
			MIN_ENTRANCE,	//Use the entrance with the minimum table cost
			N_SEARCH,		//Searches a number of location
		};

		PathFinder(mpi::communicator workers, mpi::communicator orientee, const index_type workerID, const index_type numOfWorkers);

		//! 
		void run(const Map& map, const Area& area, boost::optional<LocationOrientationPair> locationResult = {}, EntranceSearching searching = MIN_ENTRANCE, int numOfSearchLocations = 5);

	private:
		Direction moveTo(Direction currentOrientation, const std::pair<int, int>& offsets);

		mpi::communicator m_workers;
		mpi::communicator m_orientee;
		const index_type m_workerID;
		const index_type m_numOfWorkers;
	};
}