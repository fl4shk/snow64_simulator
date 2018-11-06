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
	___group = get_bits_with_range(to_decode, 31, 29);
	update_ddest_index(to_decode);

	switch (___group)
	{
	case 0:
		___oper = get_bits_with_range(to_decode, 15, 12);
		update_signext_imm(to_decode, 11);
		___forced_64_bit_integers
			= ((___oper == static_cast<u8>(Iog0Oper::Div_ThreeRegs))
			|| (___oper == static_cast<u8>(Iog0Oper::Shl_ThreeRegs))
			|| (___oper == static_cast<u8>(Iog0Oper::Shr_ThreeRegs))
			|| (___oper == static_cast<u8>(Iog0Oper::Not_TwoRegs)));
		___nop = (___oper >= static_cast<u8>(Iog0Oper::Bad));
		___op_type = get_bits_with_range(to_decode, 28, 28);
		update_dsrc0_index(to_decode);
		update_dsrc1_index(to_decode);
		break;

	case 1:
		___oper = get_bits_with_range(to_decode, 23, 20);
		update_signext_imm(to_decode, 19);
		___forced_64_bit_integers = false;
		___nop = (___oper >= static_cast<u8>(Iog1Oper::Bad));
		break;

	case 2:
	case 3:
		___oper = get_bits_with_range(to_decode, 15, 12);
		update_signext_imm(to_decode, 11);
		___forced_64_bit_integers = false;
		___nop = (___oper >= static_cast<u8>(Iog2Oper::Bad));
		update_dsrc0_index(to_decode);
		update_dsrc1_index(to_decode);
		break;

	default:
		___nop = true;
		break;
	}
}

} // namespace snow64_simulator
