#include "simulator_class.hpp"
#include <fstream>
#include <ctype.h>
#include <fenv.h>

namespace snow64_simulator
{

Simulator::Simulator(const std::string& s_data_filename,
	size_t s_min_mem_amount_in_bytes) : ___data_filename(s_data_filename),
	___mem_amount_in_bytes(s_min_mem_amount_in_bytes)
{
	___mem_amount_in_words = ___mem_amount_in_bytes
		/ sizeof(BasicWord);

	if ((___mem_amount_in_words * sizeof(BasicWord))
		!= ___mem_amount_in_bytes)
	{
		++___mem_amount_in_words;
	}

	////printout("___data_filename:  ", ___data_filename, "\n");
	//printout("___mem_amount_in_bytes:  ",
	//	___mem_amount_in_bytes, "\n");

	//printout("___mem_amount_in_words:  ", ___mem_amount_in_words,
	//	"\n");

	___mem.reset(new BasicWord[mem_amount_in_words()]);

	for (size_t i=0; i<mem_amount_in_words(); ++i)
	{
		___mem[i] = BasicWord();
	}


	{
	size_t word_addr = 0;
	std::ifstream data_ifstream(___data_filename);
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
				u8& n_data = ___mem[word_addr].data[i];
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
	//		//set_bits_with_range(temp, ___mem[i].data[j + 0], 31, 24);
	//		//set_bits_with_range(temp, ___mem[i].data[j + 1], 23, 16);
	//		//set_bits_with_range(temp, ___mem[i].data[j + 2], 15, 8);
	//		//set_bits_with_range(temp, ___mem[i].data[j + 3], 7, 0);

	//		u32 temp = ((___mem[i].data[j + 0] << 0)
	//			| (___mem[i].data[j + 1] << 8)
	//			| (___mem[i].data[j + 2] << 16)
	//			| (___mem[i].data[j + 3] << 24));
	//		printout(std::hex, temp, std::dec, " ");
	//	}
	//	//for (size_t j=0; j<BasicWord::NUM_DATA_ELEMS; ++j)
	//	//{
	//	//	printout(std::hex, static_cast<u32>(___mem[i].data[j]),
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
	for (;;)
	{
		//printout(std::hex, ___pc, std::dec, ":  ");
		perf_instr_fetch();
		perf_instr_decode();

		if (!perf_instr_exec())
		{
			break;
		}

		//if (___pc > 0x200)
		//{
		//	break;
		//}
	}

	return 0;
}

void Simulator::perf_instr_fetch()
{
	const auto pc_as_bw_addr = convert_addr_to_bw_addr(___pc);
	const auto pc_as_bw_instr_index = convert_addr_to_bw_instr_index(___pc);


	if (get_bits_with_range(___pc, 1, 0) != 0)
	{
		err(sconcat("Program counter value ", std::hex, ___pc, std::dec,
			" not aligned to 32 bits!"));
	}

	if (pc_as_bw_addr >= mem_amount_in_words())
	{
		err(sconcat("Program counter value ", std::hex, ___pc, std::dec,
			" out of range for amount of allocated memory ",
			"(", mem_amount_in_words(), "words)!"));
	}


	auto& line_of_instrs = ___mem[pc_as_bw_addr];

	___curr_instr = line_of_instrs.get_32(pc_as_bw_instr_index);
	//printout("perf_instr_fetch() debug stuff:  ",
	//	std::hex, ___curr_instr, "; ", ___pc, " ", pc_as_bw_addr, " ", 
	//	pc_as_bw_instr_index, std::dec, "\n");
	___pc += sizeof(InstrDecoder::Instr);
}

void Simulator::perf_instr_decode()
{
	___instr_decoder.decode(___curr_instr);

	___lar_file.read_from(___instr_decoder.ddest_index(),
		___instr_decoder.dsrc0_index(), ___instr_decoder.dsrc1_index(),
		___curr_ddest_contents, ___curr_dsrc0_contents,
		___curr_dsrc1_contents);




	if (___instr_decoder.nop())
	{
		return;
	}

	//std::string&&
	//	ddest_name = get_reg_name_str(static_cast<LarFile::RegName>
	//	(___instr_decoder.ddest_index())),
	//	dsrc0_name = get_reg_name_str(static_cast<LarFile::RegName>
	//	(___instr_decoder.dsrc0_index())),
	//	dsrc1_name = get_reg_name_str(static_cast<LarFile::RegName>
	//	(___instr_decoder.dsrc1_index()));
	//const std::string op_suffix = (!___instr_decoder.op_type()) ? "s" : "v";

	//switch (___instr_decoder.group())
	//{
	//case 0:
	//	switch (static_cast<InstrDecoder::Iog0Oper>
	//		(___instr_decoder.oper()))
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
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
	//		printout("addi" + op_suffix, " ",
	//			ddest_name, ", ", dsrc0_name, ", ",
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog0Oper::SimSyscall_ThreeRegsOneSimm12:
	//		printout("sim_syscall" + op_suffix, " ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//case 1:
	//	switch (static_cast<InstrDecoder::Iog1Oper>
	//		(___instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog1Oper::Bnz_OneRegOneSimm20:
	//		printout("btru ", ddest_name, ", ",
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog1Oper::Bzo_OneRegOneSimm20:
	//		printout("bfal ", ddest_name, ", ",
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
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
	//		(___instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog2Oper::LdU8_ThreeRegsOneSimm12:
	//		printout("ldu8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS8_ThreeRegsOneSimm12:
	//		printout("lds8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdU16_ThreeRegsOneSimm12:
	//		printout("ldu16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS16_ThreeRegsOneSimm12:
	//		printout("lds16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog2Oper::LdU32_ThreeRegsOneSimm12:
	//		printout("ldu32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS32_ThreeRegsOneSimm12:
	//		printout("lds32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdU64_ThreeRegsOneSimm12:
	//		printout("ldu64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog2Oper::LdS64_ThreeRegsOneSimm12:
	//		printout("lds64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog2Oper::LdF16_ThreeRegsOneSimm12:
	//		printout("ldf16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//case 3:
	//	switch (static_cast<InstrDecoder::Iog3Oper>
	//		(___instr_decoder.oper()))
	//	{
	//	case InstrDecoder::Iog3Oper::StU8_ThreeRegsOneSimm12:
	//		printout("stu8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS8_ThreeRegsOneSimm12:
	//		printout("sts8 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StU16_ThreeRegsOneSimm12:
	//		printout("stu16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS16_ThreeRegsOneSimm12:
	//		printout("sts16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog3Oper::StU32_ThreeRegsOneSimm12:
	//		printout("stu32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS32_ThreeRegsOneSimm12:
	//		printout("sts32 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StU64_ThreeRegsOneSimm12:
	//		printout("stu64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	case InstrDecoder::Iog3Oper::StS64_ThreeRegsOneSimm12:
	//		printout("sts64 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;

	//	case InstrDecoder::Iog3Oper::StF16_ThreeRegsOneSimm12:
	//		printout("stf16 ",
	//			strappcom(ddest_name, dsrc0_name, dsrc1_name),
	//			std::hex, ___instr_decoder.signext_imm(), std::dec, "\n");
	//		break;
	//	default:
	//		printout("eek!\n");
	//		break;
	//	}
	//	break;
	//default:
	//	break;
	//}
	//printout("perf_instr_decode():  lar data:  \n",
	//	std::hex,
	//	___curr_ddest_contents.shareddata->data, "\n",
	//	___curr_dsrc0_contents.shareddata->data, "\n",
	//	___curr_dsrc1_contents.shareddata->data, "\n",
	//	std::dec);
}

bool Simulator::perf_instr_exec()
{
	//printout(___instr_decoder.ddest_index(), "\n");

	if (___instr_decoder.nop())
	{
		err(sconcat("Invalid instruction at program counter",
			std::hex, (___pc - sizeof(InstrDecoder::Instr)), std::dec,
			"!"));
	}

	switch (___instr_decoder.group())
	{
	case 0:
		switch (static_cast<InstrDecoder::Iog0Oper>
			(___instr_decoder.oper()))
		{
		case InstrDecoder::Iog0Oper::SimSyscall_ThreeRegsOneSimm12:
			return handle_sim_syscall();
			break;

		default:
			___curr_ddest_contents.shareddata->dirty = true;
			switch (___instr_decoder.op_type())
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
			(___instr_decoder.oper()))
		{
		case InstrDecoder::Iog1Oper::Bnz_OneRegOneSimm20:
			if (___curr_ddest_contents.scalar_data() != 0)
			{
				___pc += ___instr_decoder.signext_imm();
			}
			break;
		case InstrDecoder::Iog1Oper::Bzo_OneRegOneSimm20:
			if (___curr_ddest_contents.scalar_data() == 0)
			{
				___pc += ___instr_decoder.signext_imm();
			}
			break;
		case InstrDecoder::Iog1Oper::Jmp_OneReg:
			___pc = ___curr_ddest_contents.scalar_data();
			break;
		case InstrDecoder::Iog1Oper::Bad:
			break;
		}
		break;

	case 2:
	case 3:
		{
			Address eff_addr = ___curr_dsrc0_contents.full_address()
				+ ___instr_decoder.signext_imm();
			//printout("ldst stuff:  ",
			//	std::hex,
			//	___curr_dsrc0_contents.full_address(), " ", 
			//	___instr_decoder.signext_imm(), " ",
			//	eff_addr, " ",
			//	___curr_dsrc1_contents.scalar_data(),
			//	std::dec, "\n");
			//___curr_dsrc1_contents.scalar_data()
			switch (___curr_dsrc1_contents.metadata->data_type)
			{
			case LarFile::DataType::UnsgnInt:
				switch (___curr_dsrc1_contents.metadata->type_size)
				{
				case LarFile::TypeSize::Sz8:
					eff_addr += static_cast<u64>(static_cast<u8>
						(___curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz16:
					eff_addr += static_cast<u64>(static_cast<u16>
						(___curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz32:
					eff_addr += static_cast<u64>(static_cast<u32>
						(___curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz64:
					eff_addr += ___curr_dsrc1_contents.scalar_data();
					break;
				}
				break;
			case LarFile::DataType::SgnInt:
				switch (___curr_dsrc1_contents.metadata->type_size)
				{
				case LarFile::TypeSize::Sz8:
					eff_addr += static_cast<s64>(static_cast<s8>
						(___curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz16:
					eff_addr += static_cast<s64>(static_cast<s16>
						(___curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz32:
					eff_addr += static_cast<s64>(static_cast<s32>
						(___curr_dsrc1_contents.scalar_data()));
					break;
				case LarFile::TypeSize::Sz64:
					eff_addr += ___curr_dsrc1_contents.scalar_data();
					break;
				}
				break;
			case LarFile::DataType::BFloat16:
				eff_addr += BFloat16(static_cast<u16>(___curr_dsrc1_contents
					.scalar_data())).cast_to_int<s64>();
				break;

			}
			//printout("ldst eff_addr:  ", std::hex, eff_addr, std::dec,
			//	"\n");
			LarFile::DataType n_data_type = LarFile::DataType::UnsgnInt;
			LarFile::TypeSize n_type_size = LarFile::TypeSize::Sz8;

			switch (static_cast<InstrDecoder::Iog2Oper>
				(___instr_decoder.oper()))
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

			___lar_file.perf_ldst((___instr_decoder.group() == 3),
				___instr_decoder.ddest_index(), eff_addr, n_data_type,
				n_type_size, ___mem, mem_amount_in_words());
		}
		break;
	}
	return true;
}

bool Simulator::handle_sim_syscall()
{
	const LarFile::RegName ddest_index_reg_name
		= static_cast<LarFile::RegName>(___instr_decoder.ddest_index());
	switch (static_cast<SyscallType>(___instr_decoder.signext_imm()))
	{
	case SyscallType::DispRegs:
		printout(get_reg_name_str(ddest_index_reg_name),
			" all stuff:  \n");
		break;
	case SyscallType::DispDdestVectorData:
		printout(get_reg_name_str(ddest_index_reg_name),
			" vector data:  ",
			std::hex, ___curr_ddest_contents.shareddata->data, std::dec,
			"\n");
		break;
	case SyscallType::DispDdestScalarData:
		printout(get_reg_name_str(ddest_index_reg_name),
			" scalar data:  ",
			std::hex, ___curr_ddest_contents.scalar_data(), std::dec,
			//"\n... and vector data:  ",
			//std::hex, ___curr_ddest_contents.shareddata->data, std::dec,
			"\n");
		break;
	case SyscallType::DispDdestAddr:
		printout(get_reg_name_str(ddest_index_reg_name),
			" full address:  ",
			std::hex, ___curr_ddest_contents.full_address(), std::dec,
			"\n");
		break;
	case SyscallType::Finish:
		printout("Finishing.\n");
		//exit(0);
		return false;
		break;
	}

	return true;
}

void Simulator::perf_group_0_scalar_op()
{
	switch (___curr_ddest_contents.metadata->data_type)
	{
	case LarFile::DataType::UnsgnInt:
		if (___instr_decoder.forced_64_bit_integers())
		{
			inner_perf_forced_64_bit_integer_group_0_scalar_op<u64>();
		}
		else
		{
			switch (___curr_ddest_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				inner_perf_regular_integer_group_0_scalar_op<u8>();
				break;
			case LarFile::TypeSize::Sz16:
				inner_perf_regular_integer_group_0_scalar_op<u16>();
				break;
			case LarFile::TypeSize::Sz32:
				inner_perf_regular_integer_group_0_scalar_op<u32>();
				break;
			case LarFile::TypeSize::Sz64:
				inner_perf_regular_integer_group_0_scalar_op<u64>();
				break;
			}
		}
		break;
	case LarFile::DataType::SgnInt:
		if (___instr_decoder.forced_64_bit_integers())
		{
			inner_perf_forced_64_bit_integer_group_0_scalar_op<s64>();
		}
		else
		{
			switch (___curr_ddest_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				inner_perf_regular_integer_group_0_scalar_op<s8>();
				break;
			case LarFile::TypeSize::Sz16:
				inner_perf_regular_integer_group_0_scalar_op<s16>();
				break;
			case LarFile::TypeSize::Sz32:
				inner_perf_regular_integer_group_0_scalar_op<s32>();
				break;
			case LarFile::TypeSize::Sz64:
				inner_perf_regular_integer_group_0_scalar_op<s64>();
				break;
			}
		}
		break;
	case LarFile::DataType::BFloat16:
		inner_perf_bfloat16_group_0_scalar_op();
		break;
	}
}

void Simulator::perf_group_0_vector_op()
{
	switch (___curr_ddest_contents.metadata->data_type)
	{
	case LarFile::DataType::UnsgnInt:
		if (___instr_decoder.forced_64_bit_integers())
		{
			inner_perf_forced_64_bit_integer_group_0_vector_op<u64>();
		}
		else
		{
			switch (___curr_ddest_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				inner_perf_regular_integer_group_0_vector_op<u8>();
				break;
			case LarFile::TypeSize::Sz16:
				inner_perf_regular_integer_group_0_vector_op<u16>();
				break;
			case LarFile::TypeSize::Sz32:
				inner_perf_regular_integer_group_0_vector_op<u32>();
				break;
			case LarFile::TypeSize::Sz64:
				inner_perf_regular_integer_group_0_vector_op<u64>();
				break;
			}
		}
		break;
	case LarFile::DataType::SgnInt:
		if (___instr_decoder.forced_64_bit_integers())
		{
			inner_perf_forced_64_bit_integer_group_0_vector_op<s64>();
		}
		else
		{
			switch (___curr_ddest_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				inner_perf_regular_integer_group_0_vector_op<s8>();
				break;
			case LarFile::TypeSize::Sz16:
				inner_perf_regular_integer_group_0_vector_op<s16>();
				break;
			case LarFile::TypeSize::Sz32:
				inner_perf_regular_integer_group_0_vector_op<s32>();
				break;
			case LarFile::TypeSize::Sz64:
				inner_perf_regular_integer_group_0_vector_op<s64>();
				break;
			}
		}
		break;
	case LarFile::DataType::BFloat16:
		inner_perf_bfloat16_group_0_vector_op();
		break;
	}
}

//template<typename DdestType>
//void Simulator::inner_perf_regular_group_0_scalar_op()
//{
//	const auto old_rounding_mode = fegetround();
//	fesetround(FE_TOWARDZERO);
//
//	___curr_ddest_contents.shareddata->dirty = true;
//	auto& curr_data = ___curr_ddest_contents.shareddata->data;
//
//
//	DdestType temp_ddest, temp_dsrc0, temp_dsrc1;
//
//	if constexpr (std::is_integral<DdestType>())
//	{
//		temp_ddest = 0;
//		auto get_temp_dsrc
//			= [&](const LarFile::RefLarContents& curr_dsrc_contents)
//			-> DdestType
//		{
//			switch (curr_dsrc_contents.metadata->data_type)
//			{
//			case LarFile::DataType::UnsgnInt:
//				return static_cast<DdestType>(curr_dsrc_contents
//					.scalar_data());
//			case LarFile::DataType::SgnInt:
//				return static_cast<DdestType>(curr_dsrc_contents
//					.sgn_scalar_data());
//			//case LarFile::DataType::BFloat16:
//			default:
//				return BFloat16(static_cast<s16>(curr_dsrc_contents
//					.scalar_data())).cast_to_int<DdestType>();
//			}
//		};
//
//		temp_dsrc0 = get_temp_dsrc(___curr_dsrc0_contents);
//		temp_dsrc1 = get_temp_dsrc(___curr_dsrc1_contents);
//	}
//	else if constexpr (std::is_same<DdestType, BFloat16>())
//	{
//		temp_ddest = BFloat16();
//		auto get_temp_dsrc
//			= [&](const LarFile::RefLarContents& curr_dsrc_contents)
//			-> DdestType
//		{
//			switch (curr_dsrc_contents.metadata->data_type)
//			{
//			case LarFile::DataType::UnsgnInt:
//				return BFloat16::create_from_int(curr_dsrc_contents
//					.scalar_data());
//			case LarFile::DataType::SgnInt:
//				return BFloat16::create_from_int(curr_dsrc_contents
//					.sgn_scalar_data());
//			//case LarFile::DataType::BFloat16:
//			default:
//				return BFloat16(curr_dsrc_contents.scalar_data());
//			}
//		};
//
//		temp_dsrc0 = get_temp_dsrc(___curr_dsrc0_contents);
//		temp_dsrc1 = get_temp_dsrc(___curr_dsrc1_contents);
//	}
//
//
//	switch (static_cast<InstrDecoder::Iog0Oper>
//		(___instr_decoder.oper()))
//	{
//	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16(float(temp_dsrc0) + float(temp_dsrc1));
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 + temp_dsrc1;
//		}
//		break;
//	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16(float(temp_dsrc0) - float(temp_dsrc1));
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 - temp_dsrc1;
//		}
//		break;
//	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16(float(temp_dsrc0
//				< temp_dsrc1));
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 < temp_dsrc1;
//		}
//		break;
//	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16(float(temp_dsrc0) * float(temp_dsrc1));
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 * temp_dsrc1;
//		}
//		break;
//
//	//case InstrDecoder::Iog0Oper::Div_ThreeRegs:
//	//	if constexpr (std::is_same<DdestType, BFloat16>())
//	//	{
//	//		temp_ddest = BFloat16(float(temp_dsrc0) / float(temp_dsrc1));
//	//	}
//	//	else
//	//	{
//	//		temp_ddest = temp_dsrc0 / temp_dsrc1;
//	//	}
//	//	break;
//	case InstrDecoder::Iog0Oper::And_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16();
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 & temp_dsrc1;
//		}
//		break;
//	case InstrDecoder::Iog0Oper::Orr_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16();
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 | temp_dsrc1;
//		}
//		break;
//	case InstrDecoder::Iog0Oper::Xor_ThreeRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16();
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0 ^ temp_dsrc1;
//		}
//		break;
//
//	//case InstrDecoder::Iog0Oper::Shl_ThreeRegs:
//	//	if constexpr (std::is_same<DdestType, BFloat16>())
//	//	{
//	//		temp_ddest = BFloat16();
//	//	}
//	//	else
//	//	{
//	//		temp_ddest = temp_dsrc0 << temp_dsrc1;
//	//	}
//	//	break;
//	//case InstrDecoder::Iog0Oper::Shr_ThreeRegs:
//	//	if constexpr (std::is_same<DdestType, BFloat16>())
//	//	{
//	//		temp_ddest = BFloat16();
//	//	}
//	//	else
//	//	{
//	//		// This might not be a perfect match to what the hardware
//	//		// actually does?
//	//		temp_ddest = temp_dsrc0 >> static_cast<u64>(temp_dsrc1);
//	//	}
//	//	break;
//	case InstrDecoder::Iog0Oper::Inv_TwoRegs:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16();
//		}
//		else
//		{
//			temp_ddest = ~temp_dsrc0;
//		}
//		break;
//	//case InstrDecoder::Iog0Oper::Not_TwoRegs:
//	//	if constexpr (std::is_same<DdestType, BFloat16>())
//	//	{
//	//		temp_ddest = BFloat16();
//	//	}
//	//	else
//	//	{
//	//		temp_ddest = !temp_dsrc0;
//	//	}
//	//	break;
//
//	case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16();
//		}
//		else
//		{
//			temp_ddest = static_cast<DdestType>
//				(___pc - sizeof(InstrDecoder::Instr))
//				+ static_cast<DdestType>(___instr_decoder.signext_imm());
//		}
//		break;
//	case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
//		if constexpr (std::is_same<DdestType, BFloat16>())
//		{
//			temp_ddest = BFloat16();
//		}
//		else
//		{
//			temp_ddest = temp_dsrc0
//				+ static_cast<DdestType>(___instr_decoder.signext_imm());
//		}
//		break;
//	default:
//		break;
//	}
//
//	if constexpr (std::is_same<DdestType, BFloat16>())
//	{
//		curr_data.set_16(___curr_ddest_contents.metadata->data_offset,
//			temp_ddest.data());
//	}
//	else
//	{
//		if constexpr (std::is_same<DdestType, u8>()
//			|| std::is_same<DdestType, s8>())
//		{
//			curr_data.set_8(___curr_ddest_contents.metadata->data_offset,
//				temp_ddest);
//		}
//		else if constexpr (std::is_same<DdestType, u16>()
//			|| std::is_same<DdestType, s16>())
//		{
//			curr_data.set_16(___curr_ddest_contents.metadata->data_offset,
//				temp_ddest);
//		}
//		else if constexpr (std::is_same<DdestType, u32>()
//			|| std::is_same<DdestType, s32>())
//		{
//			curr_data.set_32(___curr_ddest_contents.metadata->data_offset,
//				temp_ddest);
//		}
//		else if constexpr (std::is_same<DdestType, u64>()
//			|| std::is_same<DdestType, s64>())
//		{
//			curr_data.set_64(___curr_ddest_contents.metadata->data_offset,
//				temp_ddest);
//		}
//	}
//
//	fesetround(old_rounding_mode);
//}
//
//template<typename DdestType>
//void Simulator::inner_perf_regular_group_0_vector_op()
//{
//
//	static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<DdestType>();
//
//	DdestType temp_ddest_arr[TEMP_ARR_SIZE], temp_dsrc0_arr[TEMP_ARR_SIZE],
//		temp_dsrc1_arr[TEMP_ARR_SIZE];
//
//	___curr_ddest_contents.shareddata->dirty = true;
//
//	auto& curr_data = ___curr_ddest_contents.shareddata->data;
//	if constexpr (std::is_integral<DdestType>())
//	{
//		auto fill_temp_dsrc_arr
//			= [&](const LarFile::RefLarContents& curr_dsrc_contents,
//			DdestType* temp_dsrc_arr) -> void
//		{
//			switch (curr_dsrc_contents.metadata->data_type)
//			{
//			case LarFile::DataType::UnsgnInt:
//				switch (curr_dsrc_contents.metadata->type_size)
//				{
//				case LarFile::TypeSize::Sz8:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, u8>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz16:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, u16>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz32:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, u32>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz64:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, u64>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				}
//				break;
//			case LarFile::DataType::SgnInt:
//				switch (curr_dsrc_contents.metadata->type_size)
//				{
//				case LarFile::TypeSize::Sz8:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, s8>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz16:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, s16>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz32:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, s32>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz64:
//					cast_and_copy_to_int_temp_dsrc_arr<DdestType, s64>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				}
//				break;
//			default:
//				cast_and_copy_to_int_temp_dsrc_arr<DdestType, BFloat16>
//					(curr_dsrc_contents, temp_dsrc_arr);
//				break;
//			}
//		};
//
//		ASM_COMMENT("integer fill_temp_dsrc_arr(), dsrc0");
//		fill_temp_dsrc_arr(___curr_dsrc0_contents, temp_dsrc0_arr);
//		ASM_COMMENT("integer fill_temp_dsrc_arr(), dsrc1");
//		fill_temp_dsrc_arr(___curr_dsrc1_contents, temp_dsrc1_arr);
//
//		//printout("integer vector op temp dsrc arrs:  \n");
//		//printout(std::hex);
//		//for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//{
//		//	printout(temp_dsrc0_arr[i], " ");
//		//}
//		//printout("\n");
//		//for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//{
//		//	printout(temp_dsrc1_arr[i], " ");
//		//}
//		//printout(std::dec, "\n");
//
//		if constexpr (sizeof(DdestType) == sizeof(u8))
//		{
//			if constexpr (std::is_unsigned<DdestType>())
//			{
//				ASM_COMMENT("vector u8 ops");
//			}
//			else
//			{
//				ASM_COMMENT("vector s8 ops");
//			}
//		}
//		else if constexpr (sizeof(DdestType) == sizeof(u16))
//		{
//			if constexpr (std::is_unsigned<DdestType>())
//			{
//				ASM_COMMENT("vector u16 ops");
//			}
//			else
//			{
//				ASM_COMMENT("vector s16 ops");
//			}
//		}
//		else if constexpr (sizeof(DdestType) == sizeof(u32))
//		{
//			if constexpr (std::is_unsigned<DdestType>())
//			{
//				ASM_COMMENT("vector u32 ops");
//			}
//			else
//			{
//				ASM_COMMENT("vector s32 ops");
//			}
//		}
//		else // if constexpr (sizeof(DdestType) == sizeof(u64))
//		{
//			if constexpr (std::is_unsigned<DdestType>())
//			{
//				ASM_COMMENT("vector u64 ops");
//			}
//			else
//			{
//				ASM_COMMENT("vector s64 ops");
//			}
//		}
//		switch (static_cast<InstrDecoder::Iog0Oper>
//			(___instr_decoder.oper()))
//		{
//		case InstrDecoder::Iog0Oper::Add_ThreeRegs:
//			ASM_COMMENT("vector add");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] + temp_dsrc1_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
//			ASM_COMMENT("vector sub");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] - temp_dsrc1_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
//			ASM_COMMENT("vector slt");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] < temp_dsrc1_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
//			ASM_COMMENT("vector mul"); 
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] * temp_dsrc1_arr[i];
//			}
//			break;
//
//		//case InstrDecoder::Iog0Oper::Div_ThreeRegs:
//		//	ASM_COMMENT("vector div");
//		//	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//	{
//		//		temp_ddest_arr[i] = temp_dsrc0_arr[i] / temp_dsrc1_arr[i];
//		//	}
//		//	break;
//		case InstrDecoder::Iog0Oper::And_ThreeRegs:
//			ASM_COMMENT("vector and");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] & temp_dsrc1_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Orr_ThreeRegs:
//			ASM_COMMENT("vector orr");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] | temp_dsrc1_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Xor_ThreeRegs:
//			ASM_COMMENT("vector xor");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i] ^ temp_dsrc1_arr[i];
//			}
//			break;
//
//		//case InstrDecoder::Iog0Oper::Shl_ThreeRegs:
//		//	ASM_COMMENT("vector shl");
//		//	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//	{
//		//		temp_ddest_arr[i] = temp_dsrc0_arr[i] << temp_dsrc1_arr[i];
//		//	}
//		//	break;
//		//case InstrDecoder::Iog0Oper::Shr_ThreeRegs:
//		//	// This might not be a perfect match to what the hardware
//		//	// actually does?
//		//	ASM_COMMENT("vector shr");
//		//	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//	{
//		//		temp_ddest_arr[i] = temp_dsrc0_arr[i]
//		//			>> static_cast<u64>(temp_dsrc1_arr[i]);
//		//	}
//		//	break;
//		case InstrDecoder::Iog0Oper::Inv_TwoRegs:
//			ASM_COMMENT("vector inv");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = ~temp_dsrc0_arr[i];
//			}
//			break;
//		//case InstrDecoder::Iog0Oper::Not_TwoRegs:
//		//	ASM_COMMENT("vector not");
//		//	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//	{
//		//		temp_ddest_arr[i] = !temp_dsrc0_arr[i];
//		//	}
//		//	break;
//
//		case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
//			ASM_COMMENT("vector addi pc");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = static_cast<DdestType>
//					(___pc - sizeof(InstrDecoder::Instr))
//					+ static_cast<DdestType>(___instr_decoder
//					.signext_imm());
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
//			ASM_COMMENT("vector addi dsrc0");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_arr[i] = temp_dsrc0_arr[i]
//					+ static_cast<DdestType>(___instr_decoder
//					.signext_imm());
//			}
//			break;
//		default:
//			ASM_COMMENT("default vector op");
//			break;
//		}
//
//		ASM_COMMENT("after vector ops");
//
//		//bool found_nonzero = false;
//		//for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//{
//		//	if (temp_ddest_arr[i] != 0)
//		//	{
//		//		found_nonzero = true;
//		//		break;
//		//	}
//		//}
//		//if (!found_nonzero)
//		//{
//		//	printout("Integer vector op:  writing all zeros to destination"
//		//		" register.\n");
//		//}
//
//		
//		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		{
//			if constexpr (sizeof(DdestType) == sizeof(u8))
//			{
//				curr_data.set_8((i * sizeof(DdestType)),
//					temp_ddest_arr[i]);
//			}
//			else if constexpr (sizeof(DdestType) == sizeof(u16))
//			{
//				curr_data.set_16((i * sizeof(DdestType)),
//					temp_ddest_arr[i]);
//			}
//			else if constexpr (sizeof(DdestType) == sizeof(u32))
//			{
//				curr_data.set_32((i * sizeof(DdestType)),
//					temp_ddest_arr[i]);
//			}
//			else if constexpr (sizeof(DdestType) == sizeof(u64))
//			{
//				curr_data.set_64((i * sizeof(DdestType)),
//					temp_ddest_arr[i]);
//			}
//		}
//
//		ASM_COMMENT("after setting curr_data");
//	}
//	else if constexpr (std::is_same<DdestType, BFloat16>())
//	{
//		ASM_COMMENT("fegetround()");
//		const auto old_rounding_mode = fegetround();
//
//		ASM_COMMENT("FE_TOWARDZERO");
//		fesetround(FE_TOWARDZERO);
//
//		float temp_ddest_float_arr[TEMP_ARR_SIZE],
//			temp_dsrc0_float_arr[TEMP_ARR_SIZE],
//			temp_dsrc1_float_arr[TEMP_ARR_SIZE];
//
//		auto fill_temp_dsrc_arr
//			= [&](const LarFile::RefLarContents& curr_dsrc_contents,
//			DdestType* temp_dsrc_arr) -> void
//		{
//			switch (curr_dsrc_contents.metadata->data_type)
//			{
//			case LarFile::DataType::UnsgnInt:
//				switch (curr_dsrc_contents.metadata->type_size)
//				{
//				case LarFile::TypeSize::Sz8:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<u8>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz16:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<u16>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz32:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<u32>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz64:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<u64>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				}
//				break;
//			case LarFile::DataType::SgnInt:
//				switch (curr_dsrc_contents.metadata->type_size)
//				{
//				case LarFile::TypeSize::Sz8:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<s8>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz16:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<s16>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz32:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<s32>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				case LarFile::TypeSize::Sz64:
//					cast_and_copy_to_bfloat16_temp_dsrc_arr<s64>
//						(curr_dsrc_contents, temp_dsrc_arr);
//					break;
//				}
//				break;
//			default:
//				cast_and_copy_to_bfloat16_temp_dsrc_arr<BFloat16>
//					(curr_dsrc_contents, temp_dsrc_arr);
//				break;
//			}
//		};
//
//		ASM_COMMENT("BFloat16 fill_temp_dsrc_arr(), dsrc0");
//		fill_temp_dsrc_arr(___curr_dsrc0_contents, temp_dsrc0_arr);
//
//		ASM_COMMENT("BFloat16 fill_temp_dsrc_arr(), dsrc1");
//		fill_temp_dsrc_arr(___curr_dsrc1_contents, temp_dsrc1_arr);
//
//		ASM_COMMENT("fill temp_dsrc_float_arr");
//		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		{
//			temp_dsrc0_float_arr[i]
//				= static_cast<float>(temp_dsrc0_arr[i]);
//			temp_dsrc1_float_arr[i]
//				= static_cast<float>(temp_dsrc1_arr[i]);
//		}
//
//		switch (static_cast<InstrDecoder::Iog0Oper>
//			(___instr_decoder.oper()))
//		{
//		case InstrDecoder::Iog0Oper::Add_ThreeRegs:
//			ASM_COMMENT("vector float add");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
//					+ temp_dsrc1_float_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
//			ASM_COMMENT("vector float sub");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
//					- temp_dsrc1_float_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
//			ASM_COMMENT("vector float slt");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
//					< temp_dsrc1_float_arr[i];
//			}
//			break;
//		case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
//			ASM_COMMENT("vector float mul");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
//					* temp_dsrc1_float_arr[i];
//			}
//			break;
//
//		//case InstrDecoder::Iog0Oper::Div_ThreeRegs:
//		//	ASM_COMMENT("vector float div");
//		//	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		//	{
//		//		temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
//		//			/ temp_dsrc1_float_arr[i];
//		//	}
//		//	break;
//		default:
//			ASM_COMMENT("vector float dud");
//			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//			{
//				temp_ddest_float_arr[i]  = 0;
//			}
//			break;
//		}
//
//		ASM_COMMENT("vector float op copy to temp_ddest_arr");
//		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		{
//			temp_ddest_arr[i] = BFloat16(temp_ddest_float_arr[i]);
//		}
//
//		ASM_COMMENT("vector float op copy to curr_data");
//		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
//		{
//			curr_data.set_16((i * sizeof(BFloat16)),
//				temp_ddest_arr[i].data());
//		}
//		ASM_COMMENT("fesetround");
//		fesetround(old_rounding_mode);
//	}
//
//
//}


template<typename DdestType>
void Simulator::inner_perf_regular_integer_group_0_scalar_op()
{
	auto& curr_data = ___curr_ddest_contents.shareddata->data;

	DdestType temp_ddest, temp_dsrc0, temp_dsrc1;

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

	temp_dsrc0 = get_temp_dsrc(___curr_dsrc0_contents);
	temp_dsrc1 = get_temp_dsrc(___curr_dsrc1_contents);

	switch (static_cast<InstrDecoder::Iog0Oper>
		(___instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
		temp_ddest = temp_dsrc0 + temp_dsrc1;
		break;
	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
		temp_ddest = temp_dsrc0 - temp_dsrc1;
		break;
	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
		temp_ddest = temp_dsrc0 < temp_dsrc1;
		break;
	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
		temp_ddest = temp_dsrc0 * temp_dsrc1;
		break;

	case InstrDecoder::Iog0Oper::And_ThreeRegs:
		temp_ddest = temp_dsrc0 & temp_dsrc1;
		break;
	case InstrDecoder::Iog0Oper::Orr_ThreeRegs:
		temp_ddest = temp_dsrc0 | temp_dsrc1;
		break;
	case InstrDecoder::Iog0Oper::Xor_ThreeRegs:
		temp_ddest = temp_dsrc0 ^ temp_dsrc1;
		break;

	case InstrDecoder::Iog0Oper::Inv_TwoRegs:
		temp_ddest = ~temp_dsrc0;
		break;

	case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
		temp_ddest = static_cast<DdestType>
			(___pc - sizeof(InstrDecoder::Instr))
			+ static_cast<DdestType>(___instr_decoder.signext_imm());
		break;
	case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
		temp_ddest = temp_dsrc0
			+ static_cast<DdestType>(___instr_decoder.signext_imm());
		break;
	default:
		break;
	}

	//printout("regular integer scalar op result:  ",
	//	std::hex, temp_ddest, std::dec, "\n");
	if constexpr (std::is_same<DdestType, u8>()
		|| std::is_same<DdestType, s8>())
	{
		curr_data.set_8(___curr_ddest_contents.metadata->data_offset,
			temp_ddest);
	}
	else if constexpr (std::is_same<DdestType, u16>()
		|| std::is_same<DdestType, s16>())
	{
		curr_data.set_16(___curr_ddest_contents.metadata->data_offset,
			temp_ddest);
	}
	else if constexpr (std::is_same<DdestType, u32>()
		|| std::is_same<DdestType, s32>())
	{
		curr_data.set_32(___curr_ddest_contents.metadata->data_offset,
			temp_ddest);
	}
	else if constexpr (std::is_same<DdestType, u64>()
		|| std::is_same<DdestType, s64>())
	{
		curr_data.set_64(___curr_ddest_contents.metadata->data_offset,
			temp_ddest);
	}
}

template<typename DdestType>
void Simulator::inner_perf_forced_64_bit_integer_group_0_scalar_op()
{
	auto& curr_data = ___curr_ddest_contents.shareddata->data;

	DdestType temp_ddest, temp_dsrc0, temp_dsrc1;

	temp_dsrc0 = ___curr_dsrc0_contents.forced_64_bit_scalar_data();
	temp_dsrc1 = ___curr_dsrc1_contents.forced_64_bit_scalar_data();

	switch (static_cast<InstrDecoder::Iog0Oper>(___instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Div_ThreeRegs:
		temp_ddest = temp_dsrc0 / temp_dsrc1;
		break;
	case InstrDecoder::Iog0Oper::Shl_ThreeRegs:
		temp_ddest = temp_dsrc0 << static_cast<u64>(temp_dsrc1);
		break;
	case InstrDecoder::Iog0Oper::Shr_ThreeRegs:
		temp_ddest = temp_dsrc0 >> static_cast<u64>(temp_dsrc1);
		break;
	case InstrDecoder::Iog0Oper::Not_TwoRegs:
		temp_ddest = !temp_dsrc0;
		break;
	default:
		// Eek!
		temp_ddest = 0;
		break;
	}

	curr_data.set_64(___curr_ddest_contents.metadata->data_offset,
		temp_ddest);
}

template<typename DdestType>
void Simulator::inner_perf_regular_integer_group_0_vector_op()
{
	auto& curr_data = ___curr_ddest_contents.shareddata->data;

	static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<DdestType>();

	std::array<DdestType, TEMP_ARR_SIZE>
		temp_ddest_arr, temp_dsrc0_arr, temp_dsrc1_arr;

	auto fill_temp_dsrc_arr
		= [&](const LarFile::RefLarContents& curr_dsrc_contents,
		DdestType* temp_dsrc_arr) -> void
	{
		switch (curr_dsrc_contents.metadata->data_type)
		{
		case LarFile::DataType::UnsgnInt:
			switch (curr_dsrc_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, u8>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz16:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, u16>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz32:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, u32>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz64:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, u64>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			}
			break;
		case LarFile::DataType::SgnInt:
			switch (curr_dsrc_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, s8>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz16:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, s16>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz32:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, s32>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz64:
				cast_and_copy_to_int_temp_dsrc_arr<DdestType, s64>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			}
		case LarFile::DataType::BFloat16:
			cast_and_copy_to_int_temp_dsrc_arr<DdestType, BFloat16>
				(curr_dsrc_contents, temp_dsrc_arr);
			break;
		}
	};

	ASM_COMMENT("Integer fill_temp_dsrc_arr(), dsrc0");
	fill_temp_dsrc_arr(___curr_dsrc0_contents, temp_dsrc0_arr.data());

	ASM_COMMENT("Integer fill_temp_dsrc_arr(), dsrc1");
	fill_temp_dsrc_arr(___curr_dsrc1_contents, temp_dsrc1_arr.data());

	switch (static_cast<InstrDecoder::Iog0Oper>(___instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
		ASM_COMMENT("vector integer add");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] + temp_dsrc1_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
		ASM_COMMENT("vector integer sub");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] - temp_dsrc1_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
		ASM_COMMENT("vector integer slt");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] < temp_dsrc1_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
		ASM_COMMENT("vector integer mul");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] * temp_dsrc1_arr[i];
		}
		break;

	case InstrDecoder::Iog0Oper::And_ThreeRegs:
		ASM_COMMENT("vector integer bitwise and");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] & temp_dsrc1_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Orr_ThreeRegs:
		ASM_COMMENT("vector integer bitwise orr");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] | temp_dsrc1_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Xor_ThreeRegs:
		ASM_COMMENT("vector integer bitwise xor");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] ^ temp_dsrc1_arr[i];
		}
		break;

	case InstrDecoder::Iog0Oper::Inv_TwoRegs:
		ASM_COMMENT("vector integer bitwise invert");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = ~temp_dsrc0_arr[i];
		}
		break;

	case InstrDecoder::Iog0Oper::Addi_OneRegOnePcOneSimm12:
		ASM_COMMENT("vector integer addi pc");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = static_cast<DdestType>
				(___pc - sizeof(InstrDecoder::Instr))
				+ static_cast<DdestType>(___instr_decoder.signext_imm());
		}
		break;
	case InstrDecoder::Iog0Oper::Addi_TwoRegsOneSimm12:
		ASM_COMMENT("vector integer addi two regs");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i]
				+ static_cast<DdestType>(___instr_decoder.signext_imm());
		}
		break;
	default:
		ASM_COMMENT("vector integer eek");
		break;
	}


	//printout("regular integer vector op results:  ", std::hex);
	for (size_t i=0; i<temp_ddest_arr.size(); ++i)
	{
		//printout(temp_ddest_arr[i], " ");
		if constexpr (std::is_same<DdestType, u8>()
			|| std::is_same<DdestType, s8>())
		{
			curr_data.set_8((i * sizeof(DdestType)), temp_ddest_arr[i]);
		}
		else if constexpr (std::is_same<DdestType, u16>()
			|| std::is_same<DdestType, s16>())
		{
			curr_data.set_16((i * sizeof(DdestType)), temp_ddest_arr[i]);
		}
		else if constexpr (std::is_same<DdestType, u32>()
			|| std::is_same<DdestType, s32>())
		{
			curr_data.set_32((i * sizeof(DdestType)), temp_ddest_arr[i]);
		}
		else if constexpr (std::is_same<DdestType, u64>()
			|| std::is_same<DdestType, s64>())
		{
			curr_data.set_64((i * sizeof(DdestType)), temp_ddest_arr[i]);
		}
	}

	//printout(std::dec, "\n");
}

template<typename DdestType>
void Simulator::inner_perf_forced_64_bit_integer_group_0_vector_op()
{
	auto& curr_data = ___curr_ddest_contents.shareddata->data;

	static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<DdestType>();

	std::array<DdestType, TEMP_ARR_SIZE>
		temp_ddest_arr, temp_dsrc0_arr, temp_dsrc1_arr;

	for (size_t i=0; i<temp_ddest_arr.size(); ++i)
	{
		temp_dsrc0_arr[i] = ___curr_dsrc0_contents.shareddata->data.get_64
			(i * sizeof(DdestType));
	}
	for (size_t i=0; i<temp_ddest_arr.size(); ++i)
	{
		temp_dsrc1_arr[i] = ___curr_dsrc1_contents.shareddata->data.get_64
			(i * sizeof(DdestType));
	}

	switch (static_cast<InstrDecoder::Iog0Oper>(___instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Div_ThreeRegs:
		ASM_COMMENT("vector integer div");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i] / temp_dsrc1_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Shl_ThreeRegs:
		ASM_COMMENT("vector integer shl");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i]
				<< static_cast<u64>(temp_dsrc1_arr[i]);
		}
		break;
	case InstrDecoder::Iog0Oper::Shr_ThreeRegs:
		ASM_COMMENT("vector integer shr");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = temp_dsrc0_arr[i]
				>> static_cast<u64>(temp_dsrc1_arr[i]);
		}
		break;
	case InstrDecoder::Iog0Oper::Not_TwoRegs:
		ASM_COMMENT("vector integer not");
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = !temp_dsrc0_arr[i];
		}
		break;
	default:
		ASM_COMMENT("vector integer eek");
		// Eek!
		for (size_t i=0; i<temp_ddest_arr.size(); ++i)
		{
			temp_ddest_arr[i] = 0;
		}
		break;
	}

	ASM_COMMENT("copy to curr_data");
	for (size_t i=0; i<temp_ddest_arr.size(); ++i)
	{
		curr_data.set_64((i * sizeof(DdestType)), temp_ddest_arr[i]);
	}
}


void Simulator::inner_perf_bfloat16_group_0_scalar_op()
{
	ASM_COMMENT("fegetround()");
	const auto old_rounding_mode = fegetround();

	ASM_COMMENT("FE_TOWARDZERO");
	fesetround(FE_TOWARDZERO);


	BFloat16 temp_ddest, temp_dsrc0, temp_dsrc1;

	float temp_ddest_float, temp_dsrc0_float, temp_dsrc1_float;

	auto set_temp_dsrc_scalar
		= [&](const LarFile::RefLarContents& curr_dsrc_contents,
		BFloat16& temp_dsrc_scalar) -> void
	{
		auto metadata = curr_dsrc_contents.metadata;
		auto shareddata = curr_dsrc_contents.shareddata;
		switch (metadata->data_type)
		{
		case LarFile::DataType::UnsgnInt:
			switch (metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				temp_dsrc_scalar = BFloat16::create_from_int<u8>
					(shareddata->data.get_8(metadata->data_offset));
				break;
			case LarFile::TypeSize::Sz16:
				temp_dsrc_scalar = BFloat16::create_from_int<u16>
					(shareddata->data.get_16(metadata->data_offset));
				break;
			case LarFile::TypeSize::Sz32:
				temp_dsrc_scalar = BFloat16::create_from_int<u32>
					(shareddata->data.get_32(metadata->data_offset));
				break;
			case LarFile::TypeSize::Sz64:
				temp_dsrc_scalar = BFloat16::create_from_int<u64>
					(shareddata->data.get_64(metadata->data_offset));
				break;
			}
			break;
		case LarFile::DataType::SgnInt:
			switch (metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				temp_dsrc_scalar = BFloat16::create_from_int<s8>
					(shareddata->data.get_8(metadata->data_offset));
				break;
			case LarFile::TypeSize::Sz16:
				temp_dsrc_scalar = BFloat16::create_from_int<s16>
					(shareddata->data.get_16(metadata->data_offset));
				break;
			case LarFile::TypeSize::Sz32:
				temp_dsrc_scalar = BFloat16::create_from_int<s32>
					(shareddata->data.get_32(metadata->data_offset));
				break;
			case LarFile::TypeSize::Sz64:
				temp_dsrc_scalar = BFloat16::create_from_int<s64>
					(shareddata->data.get_64(metadata->data_offset));
				break;
			}
			break;
		case LarFile::DataType::BFloat16:
			temp_dsrc_scalar = BFloat16(static_cast<u32>(shareddata->data
				.get_16(metadata->data_offset)));
			break;
		}
	};

	set_temp_dsrc_scalar(___curr_dsrc0_contents, temp_dsrc0);
	set_temp_dsrc_scalar(___curr_dsrc1_contents, temp_dsrc1);


	temp_dsrc0_float = static_cast<float>(temp_dsrc0);
	temp_dsrc1_float = static_cast<float>(temp_dsrc1);

	switch (static_cast<InstrDecoder::Iog0Oper>(___instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
		temp_ddest_float = temp_dsrc0_float + temp_dsrc1_float;
		break;
	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
		temp_ddest_float = temp_dsrc0_float - temp_dsrc1_float;
		break;
	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
		temp_ddest_float = temp_dsrc0_float < temp_dsrc1_float;
		break;
	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
		temp_ddest_float = temp_dsrc0_float * temp_dsrc1_float;
		break;
	case InstrDecoder::Iog0Oper::Div_ThreeRegs:
		temp_ddest_float = temp_dsrc0_float / temp_dsrc1_float;
		break;
	default:
		temp_ddest_float = 0;
		break;
	}

	temp_ddest = temp_ddest_float;

	___curr_ddest_contents.shareddata->data.set_16(___curr_dsrc0_contents
		.metadata->data_offset, temp_ddest.data());;

	ASM_COMMENT("fesetround");
	fesetround(old_rounding_mode);
}

void Simulator::inner_perf_bfloat16_group_0_vector_op()
{
	ASM_COMMENT("fegetround()");
	const auto old_rounding_mode = fegetround();

	ASM_COMMENT("FE_TOWARDZERO");
	fesetround(FE_TOWARDZERO);

	static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<BFloat16>();

	BFloat16 temp_ddest_arr[TEMP_ARR_SIZE], temp_dsrc0_arr[TEMP_ARR_SIZE],
		temp_dsrc1_arr[TEMP_ARR_SIZE];

	float temp_ddest_float_arr[TEMP_ARR_SIZE],
		temp_dsrc0_float_arr[TEMP_ARR_SIZE],
		temp_dsrc1_float_arr[TEMP_ARR_SIZE];

	auto& curr_data = ___curr_ddest_contents.shareddata->data;

	auto fill_temp_dsrc_arr
		= [&](const LarFile::RefLarContents& curr_dsrc_contents,
		BFloat16* temp_dsrc_arr) -> void
	{
		switch (curr_dsrc_contents.metadata->data_type)
		{
		case LarFile::DataType::UnsgnInt:
			switch (curr_dsrc_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<u8>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz16:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<u16>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz32:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<u32>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz64:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<u64>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			}
			break;
		case LarFile::DataType::SgnInt:
			switch (curr_dsrc_contents.metadata->type_size)
			{
			case LarFile::TypeSize::Sz8:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<s8>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz16:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<s16>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz32:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<s32>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			case LarFile::TypeSize::Sz64:
				cast_and_copy_to_bfloat16_temp_dsrc_arr<s64>
					(curr_dsrc_contents, temp_dsrc_arr);
				break;
			}
			break;
		default:
			cast_and_copy_to_bfloat16_temp_dsrc_arr<BFloat16>
				(curr_dsrc_contents, temp_dsrc_arr);
			break;
		}
	};

	ASM_COMMENT("BFloat16 fill_temp_dsrc_arr(), dsrc0");
	fill_temp_dsrc_arr(___curr_dsrc0_contents, temp_dsrc0_arr);

	ASM_COMMENT("BFloat16 fill_temp_dsrc_arr(), dsrc1");
	fill_temp_dsrc_arr(___curr_dsrc1_contents, temp_dsrc1_arr);

	ASM_COMMENT("fill temp_dsrc_float_arr");
	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
	{
		temp_dsrc0_float_arr[i] = static_cast<float>(temp_dsrc0_arr[i]);
		temp_dsrc1_float_arr[i] = static_cast<float>(temp_dsrc1_arr[i]);
	}

	switch (static_cast<InstrDecoder::Iog0Oper>(___instr_decoder.oper()))
	{
	case InstrDecoder::Iog0Oper::Add_ThreeRegs:
		ASM_COMMENT("vector float add");
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
				+ temp_dsrc1_float_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Sub_ThreeRegs:
		ASM_COMMENT("vector float sub");
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
				- temp_dsrc1_float_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Slt_ThreeRegs:
		ASM_COMMENT("vector float slt");
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
				< temp_dsrc1_float_arr[i];
		}
		break;
	case InstrDecoder::Iog0Oper::Mul_ThreeRegs:
		ASM_COMMENT("vector float mul");
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
				* temp_dsrc1_float_arr[i];
		}
		break;

	case InstrDecoder::Iog0Oper::Div_ThreeRegs:
		ASM_COMMENT("vector float div");
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_float_arr[i] = temp_dsrc0_float_arr[i]
				/ temp_dsrc1_float_arr[i];
		}
		break;
	default:
		ASM_COMMENT("vector float dud");
		for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		{
			temp_ddest_float_arr[i]  = 0;
		}
		break;
	}

	ASM_COMMENT("vector float op copy to temp_ddest_arr");
	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
	{
		temp_ddest_arr[i] = BFloat16(temp_ddest_float_arr[i]);
	}

	ASM_COMMENT("vector float op copy to curr_data");
	for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
	{
		curr_data.set_16((i * sizeof(BFloat16)),
			temp_ddest_arr[i].data());
	}

	ASM_COMMENT("fesetround");
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

