#ifndef src__slash__snow64_cpu_simulator_class_hpp
#define src__slash__snow64_cpu_simulator_class_hpp

// src/snow64_cpu_simulator_class.hpp

#include "misc_includes.hpp"

class Snow64CpuSimulator
{
public:		// classes
	class MemWord
	{
	public:		// constants
		static constexpr size_t num_data_elems = 8;
	public:		// variables
		u32 data[num_data_elems];

	public:		// functions
		inline MemWord()
		{
			for (size_t i=0; i<num_data_elems; ++i)
			{
				data[i] = 0;
			}
		}

		inline MemWord(const MemWord& to_copy)
		{
			*this = to_copy;
		}

		inline MemWord& operator = (const MemWord& to_copy)
		{
			for (size_t i=0; i<num_data_elems; ++i)
			{
				data[i] = to_copy.data[i];
			}

			return *this;
		}
	};

private:		// variables
	std::string __data_filename;

	// This is in bytes
	size_t __min_mem_amount;

	std::unique_ptr<MemWord[]> __mem;

public:		// functions
	Snow64CpuSimulator(const std::string& s_data_filename,
		size_t s_min_mem_amount);
	virtual ~Snow64CpuSimulator();

	int run();

};


#endif		// src__slash__snow64_cpu_simulator_class_hpp
