#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class LocationFinderWorker
	{
	public:
		LocationFinderWorker(mpi::communicator workers);

		index_pair run(const Map& map);

	private:
		mpi::communicator m_workers;
	};
}