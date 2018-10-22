#include "lar_file_class.hpp"

namespace snow64_simulator
{


LarFile::LarFile()
{
	for (size_t i=0; i<__lar_tag_stack.size(); ++i)
	{
		__lar_tag_stack[i] = i;
	}
}

LarFile::~LarFile()
{
}

void LarFile::perf_ldst(bool is_store, size_t ddest_index, Address eff_addr,
	DataType n_data_type, IntTypeSize n_int_type_size,
	std::unique_ptr<BasicWord[]>& mem, size_t mem_amount_in_words)
{
	if (ddest_index == 0)
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


	u8 temp_tag_search_arr[ARR_SIZE__NUM_LARS];
	u8 tag_search = 0;

	for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	{
		temp_tag_search_arr[i] = perf_temp_tag_search(i, n_base_addr);
	}

	for (size_t i=0; i<ARR_SIZE__NUM_LARS; ++i)
	{
		tag_search |= temp_tag_search_arr[i];
	}

	auto& ddest_metadata = __lar_metadata[ddest_index];
	ddest_metadata.data_offset = n_data_offset;
	ddest_metadata.data_type = n_data_type;
	ddest_metadata.int_type_size = n_int_type_size;

	switch (tag_search != 0)
	{
	case false:
		handle_ldst_miss(is_store, ddest_metadata, n_base_addr,
			n_data_offset, mem);
		break;

	case true:
		handle_ldst_hit(is_store, ddest_metadata, n_base_addr,
			n_data_offset, mem, tag_search);
		break;
	}

}


void LarFile::handle_ldst_hit(bool is_store, LarMetadata& ddest_metadata,
	Address n_base_addr, Address n_data_offset,
	std::unique_ptr<BasicWord[]>& mem, u8 tag_search)
{
	if (ddest_metadata.tag == tag_search)
	{
		return;
	}

	ddest_metadata.tag = tag_search;
}
void LarFile::handle_ldst_miss(bool is_store, LarMetadata& ddest_metadata,
	Address n_base_addr, Address n_data_offset,
	std::unique_ptr<BasicWord[]>& mem)
{
}


} // namespace snow64_simulator
