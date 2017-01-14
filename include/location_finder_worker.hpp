#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class LocationFinderWorker
	{
	public:
		LocationFinderWorker(mpi::communicator workers);

		LocationOrientationPair run(const Map& map);

	private:
		mpi::communicator m_workers;
	};
}