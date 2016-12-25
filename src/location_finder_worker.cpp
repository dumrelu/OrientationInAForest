#include "location_finder_worker.hpp"

namespace ppc
{
	LocationFinderWorker::LocationFinderWorker(mpi::communicator workers)
		: m_workers{ std::move(workers) }
	{
	}

	index_pair LocationFinderWorker::run(const Map& map)
	{
		Area area;
		mpi::scatter(m_workers, area, 0);
		PPC_LOG(debug) << "Area received: " << area;

		return {};
	}
}