#include "snow64_cpu_simulator_class.hpp"


Snow64CpuSimulator::Snow64CpuSimulator(const std::string& s_data_filename,
	size_t s_min_mem_amount_in_bytes)
	: __data_filename(s_data_filename),
	__mem_amount_in_bytes(s_min_mem_amount_in_bytes)
{
	__mem_amount_in_words = __mem_amount_in_bytes
		/ sizeof(MemWord);

	if ((__mem_amount_in_words * sizeof(MemWord))
		!= __mem_amount_in_bytes)
	{
		++__mem_amount_in_words;
	}

	//printout("__data_filename:  ", __data_filename, "\n");
	//printout("__mem_amount_in_bytes:  ",
	//	__mem_amount_in_bytes, "\n");

	//printout("__mem_amount_in_words:  ", __mem_amount_in_words,
	//	"\n");

	__mem.reset(new MemWord[mem_amount_in_words()]);

	for (size_t i=0; i<mem_amount_in_words(); ++i)
	{
		__mem[i] = MemWord();
	}
}

Snow64CpuSimulator::~Snow64CpuSimulator()
{
}

int Snow64CpuSimulator::run()
{
	return 0;
}
