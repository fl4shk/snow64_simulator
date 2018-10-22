#ifndef src__slash__simulator_class_hpp
#define src__slash__simulator_class_hpp

// src/simulator_class.hpp

#include "misc_includes.hpp"
#include "basic_word_class.hpp"

namespace snow64_simulator
{



class Simulator
{
public:		// enums
	enum class SyscallType
	{
		DispRegs,
		DispDdestVectorData,
		DispDdestScalarData,
		DispDdestAddr,
		Finish,
	};

public:		// classes

private:		// variables
	std::string __data_filename;

	size_t __mem_amount_in_bytes, __mem_amount_in_words;

	std::unique_ptr<BasicWord[]> __mem;

public:		// functions
	Simulator(const std::string& s_data_filename,
		size_t s_min_mem_amount_in_bytes);
	virtual ~Simulator();

	int run();

	gen_getter_by_val(mem_amount_in_words)

private:		// functions
	void err(const std::string& msg)
	{
		printerr("Error:  ", msg, "\n");
		exit(1);
	}
};

} // namespace snow64_simulator

#endif		// src__slash__simulator_class_hpp
