#pragma once

#ifdef DEV_DEBUG

//#include <cassert>


#define DEV_ASSERT(x) assert(x)

#else

#define DEV_ASSERT(x)

#endif
