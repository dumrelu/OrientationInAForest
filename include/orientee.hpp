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
		path run(const Map& map, boost::optional<index_pair> startingPosition = {}, boost::optional<Direction> startingDirection = {});

	private:
		mpi::communicator m_orientee;
	};
}