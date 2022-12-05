/**
 * @file Profiler.hpp
 * @author Alexey Tsurikov
 *
 */

#ifndef _PROFILER_H
#define _PROFILER_H
#define PROF_FUNCTIONS 3500

#include <time.h>

struct _funcprof
{
unsigned int function_index;
unsigned int number_of_calls;
timespec tm;
unsigned int function_graph;
timespec child_tm;
unsigned int main_function;
};

class Profiling
{
protected:
unsigned int _getspc;
unsigned int _func_index;
unsigned int prev_func_index;
timespec temp_tm;
timespec tm1;
timespec tm2;
clockid_t clock_id1;
clockid_t clock_id2;

public:
 Profiling(unsigned int func_index);
 ~Profiling();
 static void SaveLog();
 static void CreateMyKey(void);
protected:
 void
 add_time(timespec& tm) throw ();
};
#endif
