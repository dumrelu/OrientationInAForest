#pragma once

#include "mpi.hpp"

#include "forest/map.hpp"

#include <string>

namespace ppc
{
	//! Represents the master process.
	/*!
	 *	The master process uses 2 communicators: one communicator to talk to the slaves
	 * and one communicator to talk to the outside process(the one requiring aid orientating in
	 * the forest). In both communicators, the master should have rank 0.
	 */
	class Master
	{
	public:
		Master(mpi::communicator workers, mpi::communicator outside);

		//! 
		void run(const std::string& filename);

	private:
		mpi::communicator m_workers;
		mpi::communicator m_outside;
	};
}