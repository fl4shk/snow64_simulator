#include "snow64_cpu_simulator_class.hpp"


Snow64CpuSimulator::Snow64CpuSimulator(const std::string& s_data_filename,
	size_t s_min_mem_amount_in_bytes)
	: __data_filename(s_data_filename),
	__min_mem_amount_in_bytes(s_min_mem_amount_in_bytes)
{
	__min_mem_amount_in_words = __min_mem_amount_in_bytes
		/ sizeof(MemWord);

	if ((__min_mem_amount_in_words * sizeof(MemWord))
		!= __min_mem_amount_in_bytes)
	{
		++__min_mem_amount_in_words;
	}

	//printout("__data_filename:  ", __data_filename, "\n");
	//printout("__min_mem_amount_in_bytes:  ",
	//	__min_mem_amount_in_bytes, "\n");

	//printout("__min_mem_amount_in_words:  ", __min_mem_amount_in_words,
	//	"\n");

	__mem.reset(new MemWord[min_mem_amount_in_words()]);
}

Snow64CpuSimulator::~Snow64CpuSimulator()
{
}

int Snow64CpuSimulator::run()
{
	return 0;
}
