#ifndef src__slash__simulator_class_hpp
#define src__slash__simulator_class_hpp

// src/simulator_class.hpp

#include "misc_includes.hpp"
#include "lar_file_class.hpp"
#include "instr_decoder_class.hpp"
#include "bfloat16_class.hpp"

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


private:		// variables
	LarFile __lar_file;
	InstrDecoder __instr_decoder;
	Address __pc = 0;

	LarFile::RefLarContents __curr_ddest_contents, __curr_dsrc0_contents,
		__curr_dsrc1_contents;



	InstrDecoder::Instr __curr_instr = 0;

	std::string __data_filename;

	size_t __mem_amount_in_bytes = 0, __mem_amount_in_words = 0;

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


	// Standard fetch->decode->execute loop.
	void perf_instr_fetch();
	void perf_instr_decode();
	bool perf_instr_exec();

	std::string get_reg_name_str(LarFile::RegName some_reg_name) const;
};

} // namespace snow64_simulator

#endif		// src__slash__simulator_class_hpp
