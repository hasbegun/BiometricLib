#include "portability.h"

#ifdef _WIN32
#include <cfloat>
#else
#include <cmath>
#endif

using namespace portability;

/**
 * Private constructor. Never needs to be used.
 * @return
 */
Math::Math()
{
}

bool Math::IsNaN(const double x) {
#ifdef _WIN32
    return std::_isnan(x) != 0;
#else
    return std::isnan(x) != 0;
#endif
}

double Math::GetNaN()
{
    return sqrt((double) -1);
}
