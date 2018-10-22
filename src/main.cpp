#include "simulator_class.hpp"
//#include "bfloat16_class.hpp"
#include <fenv.h>

void test_float_vectorization();

int main(int argc, char** argv)
{
	fesetround(FE_TOWARDZERO);

	if (argc != 3)
	{
		printerr("Usage:  ", argv[0], " data_filename"
			" mem_amount_in_bytes\n");
		exit(1);
	}

	snow64_simulator::Simulator sim(std::string(argv[1]),
		convert_str_to<size_t>(argv[2]));

	return sim.run();
}


//void test_float_vectorization()
//{
//	const int old_rounding_mode = fegetround();
//	fesetround(FE_TOWARDZERO);
//
//	static constexpr size_t ARR_SIZE__TEST_VECTORIZED_FLOAT = 16;
//	std::array<u32, ARR_SIZE__TEST_VECTORIZED_FLOAT>
//		arr_a_u32, arr_b_u32, arr_c_u32;
//
//	std::array<u16, ARR_SIZE__TEST_VECTORIZED_FLOAT>
//		arr_a_u16, arr_b_u16, arr_c_u16;
//	std::array<BFloat16, ARR_SIZE__TEST_VECTORIZED_FLOAT>
//		arr_a_bfloat16, arr_b_bfloat16, arr_c_bfloat16;
//
//	std::array<float, ARR_SIZE__TEST_VECTORIZED_FLOAT>
//		arr_a_float, arr_b_float, arr_c_float;
//
//	liborangepower::time::Prng prng;
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		arr_a_u32[i] = prng();
//		arr_b_u32[i] = prng();
//	}
//
//	ASM_COMMENT("finalize integer arrays");
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		arr_a_u32[i] &= (~static_cast<u32>(0xffff));
//		arr_b_u32[i] &= (~static_cast<u32>(0xffff));
//		arr_a_u16[i] = get_bits_with_range(arr_a_u32[i], 31, 16);
//		arr_b_u16[i] = get_bits_with_range(arr_b_u32[i], 31, 16);
//	}
//
//	ASM_COMMENT("copy to float arrays");
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		//arr_a_float[i] = *(reinterpret_cast<float*>(&arr_a_u32[i]));
//		//arr_b_float[i] = *(reinterpret_cast<float*>(&arr_b_u32[i]));
//		arr_a_bfloat16[i] = BFloat16(arr_a_u16[i]);
//		arr_b_bfloat16[i] = BFloat16(arr_b_u16[i]);
//
//		arr_a_float[i] = static_cast<float>(arr_a_bfloat16[i]);
//		arr_b_float[i] = static_cast<float>(arr_b_bfloat16[i]);
//	}
//
//	ASM_COMMENT("perf float operations");
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		arr_c_float[i] = arr_a_float[i] + arr_b_float[i];
//	}
//
//	ASM_COMMENT("copy to arr_c_u32");
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		arr_c_u32[i] = *(reinterpret_cast<u32*>(&arr_c_float[i]))
//			& (~static_cast<u32>(0xffff));
//	}
//
//	ASM_COMMENT("perf BFloat16 operations");
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		arr_c_bfloat16[i] = arr_a_bfloat16[i] + arr_b_bfloat16[i];
//	}
//
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		arr_c_u16[i] = arr_c_bfloat16[i].data();
//	}
//
//
//	//for (size_t i=0; i<arr_a_u32.size(); ++i)
//	//{
//	//	printout(arr_a_float[i], " + ", arr_b_float[i], ":  ",
//	//		arr_c_float[i], "\n");
//	//}
//
//	ASM_COMMENT("printout()");
//	for (size_t i=0; i<arr_a_u32.size(); ++i)
//	{
//		const u16 from_arr_c_u32 = get_bits_with_range(arr_c_u32[i], 31,
//			16);
//		//printout(std::hex, arr_a_u32[i], " ", arr_b_u32[i], " ",
//		//	arr_c_u32[i], std::dec, "\n");
//		//printout(std::hex, from_arr_c_u32, " ", arr_c_u16[i], " ",
//		//	(from_arr_c_u32 == arr_c_u16[i]), std::dec, "\n");
//		if (from_arr_c_u32 != arr_c_u16[i])
//		{
//			printout("Oh no!\n");
//		}
//	}
//
//	fesetround(old_rounding_mode);
//}
