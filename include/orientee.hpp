#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class Orientee
	{
	public:
		Orientee(mpi::communicator orientee);

		//! Returns the final position.
		index_pair run(const Map& map);

	private:
		mpi::communicator m_orientee;
	};
}