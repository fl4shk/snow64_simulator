#include "snow64_cpu_simulator_class.hpp"


Snow64CpuSimulator::Snow64CpuSimulator(const std::string& s_data_filename,
	size_t s_min_mem_amount)
	: __data_filename(s_data_filename), __min_mem_amount(s_min_mem_amount)
{
}

Snow64CpuSimulator::~Snow64CpuSimulator()
{
}

int Snow64CpuSimulator::run()
{
	return 0;
}
