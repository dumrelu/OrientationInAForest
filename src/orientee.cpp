#include "orientee.hpp"
#include "forest/index.hpp"

#include <random>

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
			query_result result{};
			for (const auto direction : g_directions)
			{
				const auto newPosition = get_position(position, combine_directions(orientation, direction));
				result[direction] = map[newPosition.second][newPosition.first];
			}
			return result;
		}

		auto find_random_position(const Map& map)
		{
			std::random_device rd;
			std::default_random_engine engine{ rd() };
			std::uniform_int_distribution<> xDistribution{ 0, static_cast<int>(map.width()) };
			std::uniform_int_distribution<> yDistribution{ 0, static_cast<int>(map.height()) };

			index_pair position;
			auto isPositionValid = [&]()
			{
				const auto zone = map[position.second][position.first];
				return zone == ROAD || zone == OPEN;
			};

			do
			{
				position = { xDistribution(engine), yDistribution(engine) };
			} while (!isPositionValid());

			return position;
		}
	}

	index_pair Orientee::run(const Map& map)
	{
		auto tag = 0;
		auto position = find_random_position(map);
		Direction orientation = FORWARD;	//TODO: random orientation
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

				PPC_LOG(trace) << "Sending query result: " << result;
			}

			if (tag & tags::VERIFY)
			{
				PPC_LOG(trace) << "Sending position for validation...";
				m_orientee.send(status.source(), tags::OK, position);
			}

			PPC_LOG(trace) << "Middle = " << map[position.second][position.first];
		} while (!(tag & tags::STOP));

		return {};
	}
}