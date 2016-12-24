#include "master.hpp"

namespace ppc
{
	Master::Master(mpi::communicator workers, mpi::communicator outside)
		: m_workers{ std::move(workers) }, m_outside{ std::move(outside) }
	{
	}
}