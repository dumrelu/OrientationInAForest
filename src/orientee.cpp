#include "orientee.hpp"

namespace ppc
{
	Orientee::Orientee(mpi::communicator orientee)
		: m_orientee{ std::move(orientee) }
	{
	}

	namespace
	{
		query_result run_query(const Map& map, const index_pair& position, const Direction orientation)
		{
			query_result result;
			for (const auto direction : g_directions)
			{
				const auto newPosition = get_position(position, combine_directions(orientation, direction));
				result[direction] = map[newPosition.second][newPosition.first];
			}
			return result;
		}
	}

	index_pair Orientee::run(const Map& map)
	{
		auto tag = 0;
		index_pair position{ 3, 3 };
		Direction orientation = RIGHT;
		do
		{
			Direction moveDirection = FORWARD;
			auto status = m_orientee.recv(mpi::any_source, mpi::any_tag, moveDirection);
			tag = status.tag();
			PPC_LOG(trace) << "Received request with tag: " << tag;

			if (tag & tags::MOVE)
			{
				orientation = combine_directions(orientation, moveDirection);
				position = get_position(position, orientation);
				assert(map[position.second][position.first] != CLIFF);
			}

			if (tag & tags::QUERY)
			{
				auto result = run_query(map, position, orientation);
				m_orientee.send(status.source(), tags::OK, result);

				PPC_LOG(trace) << "Sending query result: ( FWD = "
					<< result[FORWARD] << ", BCK = " << result[BACKWARDS]
					<< ", LEFT = " << result[LEFT] << ", RIGHT = " << result[RIGHT] << ")";
			}
		} while (tag & tags::STOP);

		return {};
	}
}