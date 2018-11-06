#include "instr_decoder_class.hpp"

namespace snow64_simulator
{

InstrDecoder::InstrDecoder()
{
}

InstrDecoder::~InstrDecoder()
{
}

void InstrDecoder::decode(InstrDecoder::Instr to_decode)
{
	__group = get_bits_with_range(to_decode, 31, 29);
	update_ddest_index(to_decode);

	switch (__group)
	{
	case 0:
		__oper = get_bits_with_range(to_decode, 15, 12);
		update_signext_imm(to_decode, 11);
		__forced_64_bit_integers
			= ((__oper == static_cast<u8>(Iog0Oper::Div_ThreeRegs))
			|| (__oper == static_cast<u8>(Iog0Oper::Shl_ThreeRegs))
			|| (__oper == static_cast<u8>(Iog0Oper::Shr_ThreeRegs))
			|| (__oper == static_cast<u8>(Iog0Oper::Not_TwoRegs)));
		__nop = (__oper >= static_cast<u8>(Iog0Oper::Bad));
		__op_type = get_bits_with_range(to_decode, 28, 28);
		update_dsrc0_index(to_decode);
		update_dsrc1_index(to_decode);
		break;

	case 1:
		__oper = get_bits_with_range(to_decode, 23, 20);
		update_signext_imm(to_decode, 19);
		__forced_64_bit_integers = false;
		__nop = (__oper >= static_cast<u8>(Iog1Oper::Bad));
		break;

	case 2:
	case 3:
		__oper = get_bits_with_range(to_decode, 15, 12);
		update_signext_imm(to_decode, 11);
		__forced_64_bit_integers = false;
		__nop = (__oper >= static_cast<u8>(Iog2Oper::Bad));
		update_dsrc0_index(to_decode);
		update_dsrc1_index(to_decode);
		break;

	default:
		__nop = true;
		break;
	}
}

} // namespace snow64_simulator
