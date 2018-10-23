#include "simulator_class.hpp"
#include <fstream>
#include <ctype.h>
#include <fenv.h>

namespace snow64_simulator
{

Simulator::Simulator(const std::string& s_data_filename,
	size_t s_min_mem_amount_in_bytes) : __data_filename(s_data_filename),
	__mem_amount_in_bytes(s_min_mem_amount_in_bytes)
{
	__mem_amount_in_words = __mem_amount_in_bytes
		/ sizeof(BasicWord);

	if ((__mem_amount_in_words * sizeof(BasicWord))
		!= __mem_amount_in_bytes)
	{
		++__mem_amount_in_words;
	}

	////printout("__data_filename:  ", __data_filename, "\n");
	//printout("__mem_amount_in_bytes:  ",
	//	__mem_amount_in_bytes, "\n");

	//printout("__mem_amount_in_words:  ", __mem_amount_in_words,
	//	"\n");

	__mem.reset(new BasicWord[mem_amount_in_words()]);

	for (size_t i=0; i<mem_amount_in_words(); ++i)
	{
		__mem[i] = BasicWord();
	}


	{
	size_t word_addr = 0;
	std::ifstream data_ifstream(__data_filename);
	std::string line;

	while (!data_ifstream.eof())
	{
		std::getline(data_ifstream, line, '\n');

		if (line.size() == 0)
		{
			continue;
		}

		if (word_addr >= mem_amount_in_words())
		{
			err(sconcat("Too few memory words requested for input data."));
		}

		// Change address
		if (line.at(0) == '@')
		{
			u64 n_addr = 0; 
			for (size_t i=1; i<line.size(); ++i)
			{
				n_addr <<= 4;
				if ((line.at(i) >= '0') && (line.at(i) <= '9'))
				{
					n_addr |= (line.at(i) - '0');
				}
				else if ((line.at(i) >= 'a') && (line.at(i) <= 'f'))
				{
					n_addr |= (line.at(i) - 'a' + 0xa);
				}
				else
				{
					err(sconcat("Invalid address change."));
				}
			}

			word_addr = n_addr;
		}
		// Raw memory word
		else
		{
			if (line.size() != (sizeof(BasicWord) * 2))
			{
				err(sconcat("Invalid line of initial memory contents."));
			}

			for (size_t i=0; i<BasicWord::NUM_DATA_ELEMS; ++i)
			{
				u8& n_data = __mem[word_addr].data[i];
				n_data = 0;

				for (size_t j=0; j<(sizeof(u8) * 2); ++j)
				{
					const size_t index
						= ((BasicWord::NUM_DATA_ELEMS - i - 1) * 2) + j;
					n_data <<= 4;

					if ((line.at(index) >= '0') && (line.at(index) <= '9'))
					{
						n_data |= (line.at(index) - '0');
					}
					else if ((line.at(index) >= 'a')
						&& (line.at(index) <= 'f'))
					{
						n_data |= (line.at(index) - 'a' + 0xa);
					}
					else
					{
						err(sconcat("Invalid line of data."));
					}
				}
			}

			++word_addr;
		}

	}

	}

	//for (size_t i=0; i<mem_amount_in_words(); ++i)
	//{
	//	for (size_t j=0; j<BasicWord::NUM_DATA_ELEMS; j+=4)
	//	{
	//		//u32 temp = 0;
	//		//set_bits_with_range(temp, __mem[i].data[j + 0], 31, 24);
	//		//set_bits_with_range(temp, __mem[i].data[j + 1], 23, 16);
	//		//set_bits_with_range(temp, __mem[i].data[j + 2], 15, 8);
	//		//set_bits_with_range(temp, __mem[i].data[j + 3], 7, 0);

	//		u32 temp = ((__mem[i].data[j + 0] << 0)
	//			| (__mem[i].data[j + 1] << 8)
	//			| (__mem[i].data[j + 2] << 16)
	//			| (__mem[i].data[j + 3] << 24));
	//		printout(std::hex, temp, std::dec, " ");
	//	}
	//	//for (size_t j=0; j<BasicWord::NUM_DATA_ELEMS; ++j)
	//	//{
	//	//	printout(std::hex, static_cast<u32>(__mem[i].data[j]),
	//	//		std::dec, " ");
	//	//}

	//	printout("\n");
	//}
}

Simulator::~Simulator()
{
}

int Simulator::run()
{
	//printout(BasicWord::NUM_DATA_ELEMS, "\n");

	for (;;)
	{
		//printout(std::hex, __pc, std::dec, ":  ");
		perf_instr_fetch();
		perf_instr_decode();

		if (!perf_instr_exec())
		{
			break;
		}

		if (__pc > 0x200)
		{
			break;
		}
	}

	return 0;
}

void Simulator::perf_instr_fetch()
{
	const auto pc_as_bw_addr = convert_addr_to_bw_addr(__pc);
	const auto pc_as_bw_instr_index = convert_addr_to_bw_instr_index(__pc);


	if (get_bits_with_range(__pc, 1, 0) != 0)
	{
		err(sconcat("Program counter value ", std::hex, __pc, std::dec,
			" not aligned to 32 bits!"));
	}

	if (pc_as_bw_addr >= mem_amount_in_words())
	{
		err(sconcat("Program counter value ", std::hex, __pc, std::dec,
			" out of range for amount of allocated memory ",
			"(", mem_amount_in_words(), "words)!"));
	}


	auto& line_of_instrs = __mem[pc_as_bw_addr];

	__curr_instr = line_of_instrs.get_32(pc_as_bw_instr_index);
	//printout("perf_instr_fetch() debug stuff:  ",
	//	std::hex, __curr_instr, "; ", __pc, " ", pc_as_bw_addr, " ", 
	//	pc_as_bw_instr_index, std::dec, "\n");
	__pc += sizeof(InstrDecoder::Instr);
}

void Simulator::perf_instr_decode()
{
	__instr_decoder.decode(__curr_instr);

	__lar_file.read_from(__instr_decoder.ddest_index(),
		__instr_decoder.dsrc0_index(), __instr_decoder.dsrc1_index(),
		__curr_ddest_contents, __curr_dsrc0_contents,
		__curr_dsrc1_contents);


	if (__instr_decoder.nop())
	{
		return;
	}

	//std::string&&
	//	ddest_name = get_reg_name_str(static_cast<LarFile::RegName>
	//	(__instr_decoder.ddest_index())),
	//	dsrc0_name = get_reg_name_str(static_cast<LarFile::RegName>
	//	(__instr_decoder.dsrc0_index())),
	//	dsrc1_name = get_reg_name_str(static_cast<LarFile::RegName>
	//	(__instr_decoder.dsrc1_index()));
	//const std::string op_suffix = (!__instr_decoder.op_type()) ? "s" : "v";

	//switch (__instr_decoder.group())
	//{
	//case 0:
	//	switch (static_cast<InstrDecoder::Iog0Oper>
	//		(__instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
	//		printout("add" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
	//		printout("sub" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
	//		printout("slt" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
	//		printout("mul" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;

	//	case InstrDecoder::Iog0Oper::Div_ThreeRegs:
	//		printout("div" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::And_ThreeRegs:
	//		printout("and" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Orr_ThreeRegs:
	//		printout("orr" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Xor_ThreeRegs:
	//		printout("xor" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;

	//	case InstrDecoder::Iog0Oper::Shl_ThreeRegs:
	//		printout("shl" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Shr_ThreeRegs:
	//		printout("shr" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name, dsrc1_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Inv_TwoRegs:
	//		printout("inv" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name), "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Not_TwoRegs:
	//		printout("not" + op_suffix, " ",
	//			strappcom2(ddest_name, dsrc0_name), "\n");
	//		break;

	//	case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
	//		printout("addi" + op_suffix, " ",
	//			ddest_name, ", pc, ",
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
	//		printout("addi" + op_suffix, " ",
	//			ddest_name, ", ", dsrc0_name, ", ",
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::SimSyscall_ThreeRegsOneSimm12:
	//		printout("sim_syscall" + op_suffix, " ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//case 1:
	//	switch (static_cast<InstrDecoder::Iog1Oper>
	//		(__instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog1Oper::Btru_OneRegOneSimm20:
	//		printout("btru ", ddest_name, ", ",
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog1Oper::Bfal_OneRegOneSimm20:
	//		printout("bfal ", ddest_name, ", ",
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog1Oper::Jmp_OneReg:
	//		printout("jmp ", ddest_name);
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//case 2:
	//	switch (static_cast<InstrDecoder::Iog2Oper>
	//		(__instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog2Oper::LdU8_ThreeRegsOneSimm12:
	//		printout("ldu8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS8_ThreeRegsOneSimm12:
	//		printout("lds8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdU16_ThreeRegsOneSimm12:
	//		printout("ldu16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS16_ThreeRegsOneSimm12:
	//		printout("lds16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog2Oper::LdU32_ThreeRegsOneSimm12:
	//		printout("ldu32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS32_ThreeRegsOneSimm12:
	//		printout("lds32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdU64_ThreeRegsOneSimm12:
	//		printout("ldu64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS64_ThreeRegsOneSimm12:
	//		printout("lds64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog2Oper::LdF16_ThreeRegsOneSimm12:
	//		printout("ldf16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//case 3:
	//	switch (static_cast<InstrDecoder::Iog3Oper>
	//		(__instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog3Oper::StU8_ThreeRegsOneSimm12:
	//		printout("stu8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS8_ThreeRegsOneSimm12:
	//		printout("sts8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StU16_ThreeRegsOneSimm12:
	//		printout("stu16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS16_ThreeRegsOneSimm12:
	//		printout("sts16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog3Oper::StU32_ThreeRegsOneSimm12:
	//		printout("stu32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS32_ThreeRegsOneSimm12:
	//		printout("sts32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StU64_ThreeRegsOneSimm12:
	//		printout("stu64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS64_ThreeRegsOneSimm12:
	//		printout("sts64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog3Oper::StF16_ThreeRegsOneSimm12:
	//		printout("stf16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, __instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//default:
	//	break;
	//}
}

bool Simulator::perf_instr_exec()
{
	//printout(__instr_decoder.ddest_index(), "\n");

	if (__instr_decoder.nop())
	{
		err(sconcat("Invalid instruction at program counter",
			std::hex, (__pc - sizeof(InstrDecoder::Instr)), std::dec,
			"!"));
	}

	switch (__instr_decoder.group())
	{
	case 0:
		switch (static_cast<InstrDecoder::Iog0Oper>
			(__instr_decoder.oper()))
		{
		case InstrDecoder::Iog0Oper::SimSyscall_ThreeRegsOneSimm12:
			return handle_sim_syscall();
			break;

		default:
			switch (__instr_decoder.op_type())
			{
			case false:
				perf_group_0_scalar_op();
				break;
			case true:
				perf_group_0_vector_op();
				break;
			}
			break;
		}
		break;

	case 1:
		switch (static_cast<InstrDecoder::Iog1Oper>
			(__instr_decoder.oper()))
		{
		case InstrDecoder::Iog1Oper::Btru_OneRegOneSimm20:
			if (__curr_ddest_contents.scalar_data() != 0)
			{
				__pc += __instr_decoder.signext_imm();
			}
			break;
		case InstrDecoder::Iog1Oper::Bfal_OneRegOneSimm20:
			if (__curr_ddest_contents.scalar_data() == 0)
			{
				__pc += __instr_decoder.signext_imm();
			}
			break;
		case InstrDecoder::Iog1Oper::Jmp_OneReg:
			__pc = __curr_ddest_contents.scalar_data();
			break;
		case InstrDecoder::Iog1Oper::Bad:
			break;
		}
		break;

	case 2:
	case 3:
		{
			Address eff_addr = __curr_dsrc0_contents.full_address()
				+ __instr_decoder.signext_imm();
			//printout("ldst stuff:  ",
			//	std::hex,
			//	__curr_dsrc0_contents.full_address(), " ", 
			//	__instr_decoder.signext_imm(), " ",
			//	eff_addr, " ",
			//	__curr_dsrc1_contents.scalar_data(),
			//	std::dec, "\n");
			//__curr_dsrc1_contents.scalar_data()
			switch (__curr_dsrc1_contents.metadata->data_type)
			{
			case LarFile::DataType::UnsgnInt:
				switch (__curr_dsrc1_contents.metadata->type_size)
				{
				case LarFile::TypeSize::Sz8:
					eff_addr += static_cast<u64>(static_cast<u8>
						(__curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz16:
					eff_addr += static_cast<u64>(static_cast<u16>
						(__curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz32:
					eff_addr += static_cast<u64>(static_cast<u32>
						(__curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz64:
					eff_addr += __curr_dsrc1_contents.scalar_data();
					break;
				}
				break;
			case LarFile::DataType::SgnInt:
				switch (__curr_dsrc1_contents.metadata->type_size)
				{
				case LarFile::TypeSize::Sz8:
					eff_addr += static_cast<s64>(static_cast<s8>
						(__curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz16:
					eff_addr += static_cast<s64>(static_cast<s16>
						(__curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz32:
					eff_addr += static_cast<s64>(static_cast<s32>
						(__curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz64:
					eff_addr += __curr_dsrc1_contents.scalar_data();
					break;
				}
				break;
			case LarFile::DataType::BFloat16:
				eff_addr += BFloat16(static_cast<u16>(__curr_dsrc1_contents
					.scalar_data())).cast_to_int<s64>();
				break;

			}
			//printout("ldst eff_addr:  ", std::hex, eff_addr, std::dec,
			//	"\n");
			LarFile::DataType n_data_type = LarFile::DataType::UnsgnInt;
			LarFile::TypeSize n_type_size = LarFile::TypeSize::Sz8;

			switch (static_cast<InstrDecoder::Iog2Oper>
				(__instr_decoder.oper()))
			{
			case InstrDecoder::Iog2Oper::LdU8_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::UnsgnInt;
				n_type_size = LarFile::TypeSize::Sz8;
				break;
			case InstrDecoder::Iog2Oper::LdS8_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::SgnInt;
				n_type_size = LarFile::TypeSize::Sz8;
				break;
			case InstrDecoder::Iog2Oper::LdU16_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::UnsgnInt;
				n_type_size = LarFile::TypeSize::Sz16;
				break;
			case InstrDecoder::Iog2Oper::LdS16_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::SgnInt;
				n_type_size = LarFile::TypeSize::Sz16;
				break;

			case InstrDecoder::Iog2Oper::LdU32_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::UnsgnInt;
				n_type_size = LarFile::TypeSize::Sz32;
				break;
			case InstrDecoder::Iog2Oper::LdS32_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::SgnInt;
				n_type_size = LarFile::TypeSize::Sz32;
				break;
			case InstrDecoder::Iog2Oper::LdU64_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::UnsgnInt;
				n_type_size = LarFile::TypeSize::Sz64;
				break;
			case InstrDecoder::Iog2Oper::LdS64_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::SgnInt;
				n_type_size = LarFile::TypeSize::Sz64;
				break;

			case InstrDecoder::Iog2Oper::LdF16_ThreeRegsOneSimm12:
				n_data_type = LarFile::DataType::BFloat16;
				n_type_size = LarFile::TypeSize::Sz16;
				break;

			case InstrDecoder::Iog2Oper::Bad:
				break;
			}

			__lar_file.perf_ldst((__instr_decoder.group() == 3),
				__instr_decoder.ddest_index(), eff_addr, n_data_type,
				n_type_size, __mem, mem_amount_in_words());
		}
		break;
	}
	return true;
}

bool Simulator::handle_sim_syscall()
{
	const LarFile::RegName ddest_index_reg_name
		= static_cast<LarFile::RegName>(__instr_decoder.ddest_index());
	switch (static_cast<SyscallType>(__instr_decoder.signext_imm()))
	{
	case SyscallType::DispRegs:
		printout(get_reg_name_str(ddest_index_reg_name),
			" all stuff:  \n");
		break;
	case SyscallType::DispDdestVectorData:
		printout(get_reg_name_str(ddest_index_reg_name),
			" vector data:  ",
			std::hex, __curr_ddest_contents.shareddata->data, std::dec,
			"\n");
		break;
	case SyscallType::DispDdestScalarData:
		printout(get_reg_name_str(ddest_index_reg_name),
			" scalar data:  ",
			std::hex, __curr_ddest_contents.scalar_data(), std::dec,
			"\n");
		break;
	case SyscallType::DispDdestAddr:
		printout(get_reg_name_str(ddest_index_reg_name),
			" full address:  ",
			std::hex, __curr_ddest_contents.full_address(), std::dec,
			"\n");
		break;
	case SyscallType::Finish:
		printout("Finishing.\n");
		//exit(0);
		return true;
		break;
	}

	return false;
}

void Simulator::perf_group_0_scalar_op()
{
	switch (__curr_ddest_contents.metadata->data_type)
	{
	case LarFile::DataType::UnsgnInt:
		switch (__curr_ddest_contents.metadata->type_size)
		{
		case LarFile::TypeSize::Sz8:
			inner_perf_group_0_scalar_op<u8>();
			break;
		case LarFile::TypeSize::Sz16:
			inner_perf_group_0_scalar_op<u16>();
			break;
		case LarFile::TypeSize::Sz32:
			inner_perf_group_0_scalar_op<u32>();
			break;
		case LarFile::TypeSize::Sz64:
			inner_perf_group_0_scalar_op<u64>();
			break;
		}
		break;
	case LarFile::DataType::SgnInt:
		switch (__curr_ddest_contents.metadata->type_size)
		{
		case LarFile::TypeSize::Sz8:
			inner_perf_group_0_scalar_op<s8>();
			break;
		case LarFile::TypeSize::Sz16:
			inner_perf_group_0_scalar_op<s16>();
			break;
		case LarFile::TypeSize::Sz32:
			inner_perf_group_0_scalar_op<s32>();
			break;
		case LarFile::TypeSize::Sz64:
			inner_perf_group_0_scalar_op<s64>();
			break;
		}
		break;
	case LarFile::DataType::BFloat16:
		inner_perf_group_0_scalar_op<BFloat16>();
		break;
	}
}

void Simulator::perf_group_0_vector_op()
{
	switch (__curr_ddest_contents.metadata->data_type)
	{
	case LarFile::DataType::UnsgnInt:
		switch (__curr_ddest_contents.metadata->type_size)
		{
		case LarFile::TypeSize::Sz8:
			inner_perf_group_0_vector_op<u8>();
			break;
		case LarFile::TypeSize::Sz16:
			inner_perf_group_0_vector_op<u16>();
			break;
		case LarFile::TypeSize::Sz32:
			inner_perf_group_0_vector_op<u32>();
			break;
		case LarFile::TypeSize::Sz64:
			inner_perf_group_0_vector_op<u64>();
			break;
		}
		break;
	case LarFile::DataType::SgnInt:
		switch (__curr_ddest_contents.metadata->type_size)
		{
		case LarFile::TypeSize::Sz8:
			inner_perf_group_0_vector_op<s8>();
			break;
		case LarFile::TypeSize::Sz16:
			inner_perf_group_0_vector_op<s16>();
			break;
		case LarFile::TypeSize::Sz32:
			inner_perf_group_0_vector_op<s32>();
			break;
		case LarFile::TypeSize::Sz64:
			inner_perf_group_0_vector_op<s64>();
			break;
		}
		break;
	case LarFile::DataType::BFloat16:
		inner_perf_group_0_vector_op<BFloat16>();
		break;
	}
}

template<typename DdestType>
void Simulator::inner_perf_group_0_scalar_op()
{
	const auto old_rounding_mode = fegetround();
	fesetround(FE_TOWARDZERO);

	__curr_ddest_contents.shareddata->dirty = true;
	auto& curr_data = __curr_ddest_contents.shareddata->data;


	DdestType temp_ddest, temp_dsrc0, temp_dsrc1;

	if constexpr (std::is_integral<DdestType>())
	{
		temp_ddest = 0;
		auto get_temp_dsrc
			= [&](const LarFile::RefLarContents& curr_dsrc_contents)
			-> DdestType
		{
			switch (curr_dsrc_contents.metadata->data_type)
			{
			case LarFile::DataType::UnsgnInt:
				return static_cast<DdestType>(curr_dsrc_contents
					.scalar_data());
			case LarFile::DataType::SgnInt:
				return static_cast<DdestType>(curr_dsrc_contents
					.sgn_scalar_data());
			//case LarFile::DataType::BFloat16:
			default:
				return BFloat16(static_cast<s16>(curr_dsrc_contents
					.scalar_data())).cast_to_int<DdestType>();
			}
		};

		temp_dsrc0 = get_temp_dsrc(__curr_dsrc0_contents);
		temp_dsrc1 = get_temp_dsrc(__curr_dsrc1_contents);
	}
	else if constexpr (std::is_same<DdestType, BFloat16>())
	{
		temp_ddest = BFloat16();
		auto get_temp_dsrc
			= [&](const LarFile::RefLarContents& curr_dsrc_contents)
			-> DdestType
		{
			switch (curr_dsrc_contents.metadata->data_type)
			{
			case LarFile::DataType::UnsgnInt:
				return BFloat16::create_from_int(curr_dsrc_contents
					.scalar_data());
			case LarFile::DataType::SgnInt:
				return BFloat16::create_from_int(curr_dsrc_contents
					.sgn_scalar_data());
			//case LarFile::DataType::BFloat16:
			default:
				return BFloat16(curr_dsrc_contents.scalar_data());
			}
		};

		temp_dsrc0 = get_temp_dsrc(__curr_dsrc0_contents);
		temp_dsrc1 = get_temp_dsrc(__curr_dsrc1_contents);
	}


	switch (static_cast<InstrDecoder::Iog0Oper>
		(__instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16(float(temp_dsrc0) + float(temp_dsrc1));
		}
		else
		{
			temp_ddest = temp_dsrc0 + temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16(float(temp_dsrc0) - float(temp_dsrc1));
		}
		else
		{
			temp_ddest = temp_dsrc0 - temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16(float(temp_dsrc0
				< temp_dsrc1));
		}
		else
		{
			temp_ddest = temp_dsrc0 < temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16(float(temp_dsrc0) * float(temp_dsrc1));
		}
		else
		{
			temp_ddest = temp_dsrc0 * temp_dsrc1;
		}
		break;

	case InstrDecoder::Iog0Oper::Div_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16(float(temp_dsrc0) / float(temp_dsrc1));
		}
		else
		{
			temp_ddest = temp_dsrc0 / temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::And_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			temp_ddest = temp_dsrc0 & temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::Orr_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			temp_ddest = temp_dsrc0 | temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::Xor_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			temp_ddest = temp_dsrc0 ^ temp_dsrc1;
		}
		break;

	case InstrDecoder::Iog0Oper::Shl_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			temp_ddest = temp_dsrc0 << temp_dsrc1;
		}
		break;
	case InstrDecoder::Iog0Oper::Shr_ThreeRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			// This might not be a perfect match to what the hardware
			// actually does?
			temp_ddest = temp_dsrc0 >> static_cast<u64>(temp_dsrc1);
		}
		break;
	case InstrDecoder::Iog0Oper::Inv_TwoRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			temp_ddest = ~temp_dsrc0;
		}
		break;
	case InstrDecoder::Iog0Oper::Not_TwoRegs:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			temp_ddest = BFloat16();
		}
		else
		{
			temp_ddest = !temp_dsrc0;
		}
		break;

	case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			//temp_ddest = BFloat16::create_from_int(__pc
			//	- sizeof(InstrDecoder::Instr))
			//	+ BFloat16::create_from_int(__instr_decoder.signext_imm());
			temp_ddest = BFloat16
				(float(BFloat16(__pc - sizeof(InstrDecoder::Instr)))
				+ float(BFloat16::create_from_int(__instr_decoder
				.signext_imm())));
		}
		else
		{
			temp_ddest = static_cast<DdestType>
				(__pc - sizeof(InstrDecoder::Instr))
				+ static_cast<DdestType>(__instr_decoder.signext_imm());
		}
		break;
	case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
		if constexpr (std::is_same<DdestType, BFloat16>())
		{
			//temp_ddest = temp_dsrc0
			//	+ BFloat16::create_from_int(__instr_decoder.signext_imm());
			temp_ddest = BFloat16(float(temp_dsrc0)
				+ float(BFloat16::create_from_int(__instr_decoder
				.signext_imm())));
		}
		else
		{
			temp_ddest = temp_dsrc0
				+ static_cast<DdestType>(__instr_decoder.signext_imm());
		}
		break;
	default:
		break;
	}

	if constexpr (std::is_same<DdestType, BFloat16>())
	{
		curr_data.set_16(__curr_ddest_contents.metadata->data_offset,
			temp_ddest.data());
	}
	else
	{
		if constexpr (std::is_same<DdestType, u8>()
			|| std::is_same<DdestType, s8>())
		{
			curr_data.set_8(__curr_ddest_contents.metadata->data_offset,
				temp_ddest);
		}
		else if constexpr (std::is_same<DdestType, u16>()
			|| std::is_same<DdestType, s16>())
		{
			curr_data.set_16(__curr_ddest_contents.metadata->data_offset,
				temp_ddest);
		}
		else if constexpr (std::is_same<DdestType, u32>()
			|| std::is_same<DdestType, s32>())
		{
			curr_data.set_32(__curr_ddest_contents.metadata->data_offset,
				temp_ddest);
		}
		else if constexpr (std::is_same<DdestType, u64>()
			|| std::is_same<DdestType, s64>())
		{
			curr_data.set_64(__curr_ddest_contents.metadata->data_offset,
				temp_ddest);
		}
	}

	fesetround(old_rounding_mode);
}

template<typename DdestType>
void Simulator::inner_perf_group_0_vector_op()
{
	const auto old_rounding_mode = fegetround();
	fesetround(FE_TOWARDZERO);

	static constexpr size_t TEMP_ARR_SIZE = BasicWord::NUM_DATA_ELEMS
		/ sizeof(DdestType);

	DdestType temp_ddest_arr[TEMP_ARR_SIZE], temp_dsrc0_arr[TEMP_ARR_SIZE],
		temp_dsrc1_arr[TEMP_ARR_SIZE];

	float temp_ddest_float_arr[TEMP_ARR_SIZE];
		temp_dsrc0_float_arr[TEMP_ARR_SIZE],
		temp_dsrc1_float_arr[TEMP_ARR_SIZE];

	if constexpr (std::is_integral<DdestType>())
	{
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_arr[i] = 0;
		}
	}
	else if constexpr (std::is_same<DdestType, BFloat16>())
	{
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_arr[i] = BFloat16();
		}

		auto fill_temp_dsrc_arr
			= [&](const LarFile::RefLarContents& curr_dsrc_contents,
			DdestType* temp_dsrc_arr) -> void
		{
			const auto& curr_data = curr_dsrc_contents.shareddata->data;

			switch (curr_dsrc_contents.metadata->data_type)
			{
			case LarFile::DataType::BFloat16:
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16(curr_data.get_16
						(i * sizeof(u16)));
				}
				break;
			default:
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16();
				}

				switch (curr_dsrc_contents.metadata->type_size)
				{
				case LarFile::TypeSize::Sz8:
					break;
				case LarFile::TypeSize::Sz16:
					break;
				case LarFile::TypeSize::Sz32:
					break;
				case LarFile::TypeSize::Sz64:
					break;
				}
				break;
			}
		};

		fill_temp_dsrc_arr(__curr_dsrc0_contents, temp_dsrc0_arr);
		fill_temp_dsrc_arr(__curr_dsrc1_contents, temp_dsrc1_arr);

		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_dsrc0_float_arr[i]
				= static_cast<float>(temp_dsrc0_arr[i]);
			temp_dsrc1_float_arr[i]
				= static_cast<float>(temp_dsrc1_arr[i]);
		}

		switch (static_cast<InstrDecoder::Iog0Oper>
			(__instr_decoder.oper()))
		{
		case InstrDecoder::Iog0Oper::Add_ThreeRegs:
			break;
		case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
			break;
		case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
			break;
		case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
			break;

		case InstrDecoder::Iog0Oper::Div_ThreeRegs:
			break;
		case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
			break;
		case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
			break;
		}
	}


	fesetround(old_rounding_mode);
}

std::string Simulator::get_reg_name_str(LarFile::RegName some_reg_name)
	const
{
	switch (some_reg_name)
	{
	case LarFile::RegName::Dzero:
		return "dzero";
	case LarFile::RegName::Du0:
		return "du0";
	case LarFile::RegName::Du1:
		return "du1";
	case LarFile::RegName::Du2:
		return "du2";
	case LarFile::RegName::Du3:
		return "du3";
	case LarFile::RegName::Du4:
		return "du4";
	case LarFile::RegName::Du5:
		return "du5";
	case LarFile::RegName::Du6:
		return "du6";
	case LarFile::RegName::Du7:
		return "du7";
	case LarFile::RegName::Du8:
		return "du8";
	case LarFile::RegName::Du9:
		return "du9";
	case LarFile::RegName::Du10:
		return "du10";
	case LarFile::RegName::Du11:
		return "du11";
	case LarFile::RegName::Dlr:
		return "dlr";
	case LarFile::RegName::Dfp:
		return "dfp";
	case LarFile::RegName::Dsp:
		return "dsp";
	default:
		return "eek!";
	}
}


} // namespace snow64_simulator

