#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class LocationFinderMaster
	{
	public:
		LocationFinderMaster(mpi::communicator workers, mpi::communicator orientee);

		index_pair run(const Map& map);

	private:
		mpi::communicator m_workers;
		mpi::communicator m_orientee;
	};
}