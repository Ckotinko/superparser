#pragma once

/*
 * In MSVC, tr1::function is already in std
 * In GCC it's still in tr1
 */
#ifdef _MSCVER
#include <functional>
using std::function;
using std::bind;
using namespace std::placeholders;
#else
#include <tr1/functional>
using std::tr1::function;
using std::tr1::bind;
using namespace std::tr1::placeholders;
#endif
