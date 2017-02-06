#include "orientee.hpp"
#include "forest/index.hpp"

#include <random>
#include <tuple>

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

		auto find_random_position(const Map& map, boost::optional<index_pair> startingPosition)
		{
			std::random_device rd;
			std::default_random_engine engine{ rd() };
			std::uniform_int_distribution<> xDistribution{ 0, static_cast<int>(map.width() - 1) };
			std::uniform_int_distribution<> yDistribution{ 0, static_cast<int>(map.height() - 1) };

			index_pair position;
			auto isPositionValid = [&]()
			{
				const auto zone = map[position.second][position.first];
				return zone == ROAD || zone == OPEN;
			};

			if (startingPosition)
			{
				position = *startingPosition;

				if (!isPositionValid())
				{
					PPC_LOG(info) << "Starting position is not valid! Searching for another one...";
				}
				else
				{
					PPC_LOG(info) << "Given starting position is valid!";
				}
			}

			while (!isPositionValid())
			{
				position = { xDistribution(engine), yDistribution(engine) };
			}

			return position;
		}

		auto find_random_direction()
		{
			std::random_device rd;
			std::default_random_engine engine{ rd() };
			std::uniform_int_distribution<> directionDistribution{ 0, 3 };

			return static_cast<Direction>(directionDistribution(engine));
		}

		//TODO: For some reason ostream << index_pair doesn't work with boost::log...
		inline auto to_string(const index_pair& pair)
		{
			return static_cast<const std::ostringstream&>(std::ostringstream{} << "( x = " << pair.first << ", y = " << pair.second << " )").str();
		}
	}

	path Orientee::run(const Map& map, boost::optional<index_pair> startingPosition, boost::optional<Direction> startingDirection)
	{
		auto position = find_random_position(map, startingPosition);
		PPC_LOG(info) << "Chosen starting position: " << to_string(position);

		Direction orientation = startingDirection ? *startingDirection : find_random_direction();
		PPC_LOG(info) << "Chosen starting orientation: " << orientation;

		const auto realStartingPosition = position;
		const auto realStartingOrientation = orientation;
		index_type numOfMoves = 0u;
		index_type numOfQueries = 0u;

		path p;
		p.push_back(position);
		auto tag = 0;
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
				assert(map[position.second][position.first] & OPEN | ROAD);

				PPC_LOG(debug) << "Position updated: " << to_string(position);
				p.push_back(position);
				++numOfMoves;
			}

			if (tag & tags::QUERY)
			{
				auto result = run_query(map, position, orientation);
				m_orientee.send(status.source(), tags::OK, result);

				PPC_LOG(trace) << "Sending query result: " << result;
				++numOfQueries;
			}

			if (tag & tags::VERIFY)
			{
				PPC_LOG(trace) << "Sending position and orientation for validation...";
				LocationOrientationPair solution{ position, orientation };
				m_orientee.send(status.source(), tags::OK, solution);
			}

			if (tag & tags::STATS)
			{
				PPC_LOG(trace) << "Sending statistics...";
				m_orientee.send(status.source(), tags::OK, realStartingPosition);
				m_orientee.send(status.source(), tags::OK, realStartingOrientation);
				m_orientee.send(status.source(), tags::OK, numOfMoves);
				m_orientee.send(status.source(), tags::OK, numOfQueries);
			}

			PPC_LOG(trace) << "Middle = " << map[position.second][position.first];
		} while (!(tag & tags::STOP));

		return p;
	}
}