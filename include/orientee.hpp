#pragma once

#include "common.hpp"

namespace ppc
{
	//!
	class Orientee
	{
	public:
		Orientee(mpi::communicator orientee);

		//! Returns the path taken.
		path run(const Map& map);

	private:
		mpi::communicator m_orientee;
	};
}