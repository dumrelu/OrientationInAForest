#pragma once

#include "mpi.hpp"

#include "forest/map.hpp"

#include <array>
#include <string>
#include <tuple>

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
		//! (initial_position, starting_position, path_to_bottom).
		using result = std::tuple<index_pair, index_pair, path>;

		Master(mpi::communicator workers, mpi::communicator outside);

		//! 
		result run(const Map& map);

	private:

		// Interacting with the outside
		std::array<ZoneType, 4> query();
		void move(Direction direction);

		mpi::communicator m_workers;
		mpi::communicator m_outside;
	};
}