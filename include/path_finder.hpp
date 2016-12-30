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
		void run(const Map& map, const Area& area);

	private:
		mpi::communicator m_workers;
		mpi::communicator m_orientee;
	};
}