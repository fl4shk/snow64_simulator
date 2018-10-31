#ifndef src__slash__instr_decoder_class_hpp
#define src__slash__instr_decoder_class_hpp

// src/instr_decoder_class.hpp

#include "misc_includes.hpp"

namespace snow64_simulator
{

class InstrDecoder
{
public:		// typedefs
	typedef u32 Instr;

public:		// enums
	enum class Iog0Oper : u8
	{
		Add_ThreeRegs,
		Sub_ThreeRegs,
		Slt_ThreeRegs,
		Mul_ThreeRegs,

		Div_ThreeRegs,
		And_ThreeRegs,
		Orr_ThreeRegs,
		Xor_ThreeRegs,

		Shl_ThreeRegs,
		Shr_ThreeRegs,
		Inv_TwoRegs,
		Not_TwoRegs,

		Addi_OneRegOnePcOneSimm12,
		Addi_TwoRegsOneSimm12,
		SimSyscall_ThreeRegsOneSimm12,
		Bad,
	};

	enum class Iog1Oper : u8
	{
		Bnz_OneRegOneSimm20,
		Bzo_OneRegOneSimm20,
		Jmp_OneReg,
		Bad,
	};

	enum class Iog2Oper : u8
	{
		LdU8_ThreeRegsOneSimm12,
		LdS8_ThreeRegsOneSimm12,
		LdU16_ThreeRegsOneSimm12,
		LdS16_ThreeRegsOneSimm12,

		LdU32_ThreeRegsOneSimm12,
		LdS32_ThreeRegsOneSimm12,
		LdU64_ThreeRegsOneSimm12,
		LdS64_ThreeRegsOneSimm12,

		LdF16_ThreeRegsOneSimm12,
		Bad,
	};

	enum class Iog3Oper : u8
	{
		StU8_ThreeRegsOneSimm12,
		StS8_ThreeRegsOneSimm12,
		StU16_ThreeRegsOneSimm12,
		StS16_ThreeRegsOneSimm12,

		StU32_ThreeRegsOneSimm12,
		StS32_ThreeRegsOneSimm12,
		StU64_ThreeRegsOneSimm12,
		StS64_ThreeRegsOneSimm12,

		StF16_ThreeRegsOneSimm12,
		Bad,
	};

private:		// variables
	u8 __group = 0;
	u8 __oper = 0;
	u8 __ddest_index = 0, __dsrc0_index = 0, __dsrc1_index = 0;
	s64 __signext_imm = 0;
	bool __op_type = false;
	bool __nop = false;
	bool __forced_64_bit_integers = false;


public:		// functions
	InstrDecoder();
	virtual ~InstrDecoder();

	void decode(Instr to_decode);

	gen_getter_by_val(group)
	gen_getter_by_val(oper)
	gen_getter_by_val(ddest_index)
	gen_getter_by_val(dsrc0_index)
	gen_getter_by_val(dsrc1_index)
	gen_getter_by_val(signext_imm)
	gen_getter_by_val(op_type)
	gen_getter_by_val(nop)
	gen_getter_by_val(forced_64_bit_integers)

private:		// functions
	inline void update_signext_imm(Instr to_decode, size_t msb_pos)
	{
		__signext_imm = get_bits_with_range(to_decode, msb_pos, 0);
		if (get_bits_with_range(__signext_imm, msb_pos, msb_pos))
		{
			set_bits_with_range(__signext_imm,
				static_cast<decltype(__signext_imm)>(-1),
				WIDTH2MP(sizeof(__signext_imm) * 8), (msb_pos + 1));
		}
	}

	inline void update_ddest_index(Instr to_decode)
	{
		__ddest_index = get_bits_with_range(to_decode, 27, 24);
	}

	inline void update_dsrc0_index(Instr to_decode)
	{
		__dsrc0_index = get_bits_with_range(to_decode, 23, 20);
	}
	inline void update_dsrc1_index(Instr to_decode)
	{
		__dsrc1_index = get_bits_with_range(to_decode, 19, 16);
	}

};

} // namespace snow64_simulator


#endif		// src__slash__instr_decoder_class_hpp
