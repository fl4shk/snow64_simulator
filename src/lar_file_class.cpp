#include "lar_file_class.hpp"

namespace snow64_simulator
{


LarFile::LarFile()
{
}

LarFile::~LarFile()
{
}

void LarFile::perf_ldst(bool is_store, size_t index, Address eff_addr,
	DataType n_data_type, IntTypeSize n_int_type_size,
	std::unique_ptr<BasicWord[]>& mem, size_t mem_amount_in_words)
{
	if (index == 0)
	{
		return;
	}

	const Address word_addr = convert_addr_to_bw_addr(eff_addr);
	if (word_addr >= mem_amount_in_words)
	{
		printerr("Error:  LarFile::perf_ldst() address ",
			std::hex, eff_addr, std::dec, " out of range for allocated",
			" memory.\n");
		exit(1);
	}


	const Address n_base_addr = get_bits_with_range(eff_addr,
		((sizeof(Address) * 8) - 1), WIDTH__METADATA_DATA_OFFSET);
	const Address n_data_offset = get_bits_with_range(eff_addr,
		(WIDTH__METADATA_DATA_OFFSET - 1), 0);

	//LarMetadata& metadata = __lar_metadata[index];

	//u8 tag_search_arr[ARR_SIZE__NUM_LARS];
	//bool cmp_ref_count_arr[ARR_SIZE__NUM_LARS];
	//bool cmp_base_addr_arr[ARR_SIZE__NUM_LARS];
	//bool cmp_result_arr[ARR_SIZE__NUM_LARS];
	//u8 mask_arr[ARR_SIZE__NUM_LARS];


	////u8 tag_search__2_to_3, tag_search__4_to_5, tag_search__6_to_7,
	////	tag_search__8_to_9, tag_search__10_to_11, tag_search__12_to_13,
	////	tag_search__14_to_15;

	////for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	////{
	////	tag_search_arr[i] = perf_tag_search(i, index, n_base_addr);
	////}

	////for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	////{
	////}
	////for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	////{
	////}
	//for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	//{
	//	cmp_ref_count_arr[i] = (__lar_shareddata[i].ref_count != 0);
	//	cmp_base_addr_arr[i] = (__lar_shareddata[i].base_addr
	//		== n_base_addr);
	//	cmp_result_arr[i] = (cmp_ref_count_arr[i] && cmp_base_addr_arr[i]);
	//}

	//asm_comment("Will this be vectorized?");
	//for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	//{
	//	tag_search_arr[i] = cmp_result_arr[i] ? index : 0;
	//}

	//asm_comment("Let's see.");

	//for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	//{
	//	printout(tag_search_arr[i], "\n");
	//}

}

} // namespace snow64_simulator
