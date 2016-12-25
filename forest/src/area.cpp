#include "area.hpp"

#include <ostream>

namespace ppc
{
	std::ostream& operator<<(std::ostream& out, const Area& area)
	{
		out << "( x=" << area.x << ", y = " << area.y << ", height = " << area.height << ", width = " << area.width << " )";
		return out;
	}
}