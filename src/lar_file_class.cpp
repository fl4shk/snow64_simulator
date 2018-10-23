#include "lar_file_class.hpp"

namespace snow64_simulator
{


LarFile::LarFile()
{
	for (size_t i=0; i<__lar_tag_stack.size(); ++i)
	{
		__lar_tag_stack[i] = i;
	}
	__curr_tag_stack_index = __lar_tag_stack.size() - 1;
}

LarFile::~LarFile()
{
}

void LarFile::perf_ldst(bool is_store, size_t ddest_index,
	Address eff_addr, DataType n_data_type, TypeSize n_type_size,
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
	ddest_metadata.type_size = n_type_size;

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

	const auto old_tag = ddest_metadata.tag;

	ddest_metadata.tag = tag_search;

	auto& n_shareddata = __lar_shareddata[tag_search];
	auto& curr_shareddata = __lar_shareddata[old_tag];



	// The aliased reference count always increments here.  This is because
	// we are becoming a new reference of the found element of shared data.
	++n_shareddata.ref_count;


	if (is_store)
	{
		// Make of a copy of our data to the new address.
		n_shareddata.data = curr_shareddata.data;

		// Stores mark our data as dirty.
		n_shareddata.dirty = true;
	}

	switch (curr_shareddata.ref_count)
	{
	// We didn't have ANY element of shared data yet.
	case 0:
		break;

	// Our element of shared data is to be deallocated
	case 1:
		dealloc_tag(old_tag);

		// As we've been deallocated, we need to write our old data
		// back to memory if it's not already up to date.
		if (curr_shareddata.dirty)
		{
			store_to_mem(curr_shareddata.data, curr_shareddata.base_addr,
				mem);
		}

		// This is also part of deallocating the element of shared data,
		// especially setting curr_shareddata.ref_count to zero, as that
		// prevents this element of shared data being used during the tag
		// search.
		curr_shareddata.ref_count = 0;
		curr_shareddata.dirty = false;
		break;

	// There was at least one other reference to us, so don't deallocate
	// anything, but do decrement our old reference count.
	default:
		--curr_shareddata.ref_count;
		break;
	}

}

void LarFile::handle_ldst_miss(bool is_store, LarMetadata& ddest_metadata,
	Address n_base_addr, Address n_data_offset,
	std::unique_ptr<BasicWord[]>& mem)
{
	const auto old_metadata = ddest_metadata;
	auto& curr_shareddata = __lar_shareddata[old_metadata.tag];
	const auto old_base_addr = curr_shareddata.base_addr;

	switch (curr_shareddata.ref_count)
	{
	// This is from before we were allocated.
	case 0:
		{
			// Allocate a new element of shared data.
			ddest_metadata.tag = alloc_tag();
			auto& n_shareddata = __lar_shareddata[ddest_metadata.tag];
			n_shareddata.base_addr = n_base_addr;
			n_shareddata.ref_count = 1;

			switch (is_store)
			{
			case false:
				load_from_mem(n_shareddata.data, n_base_addr, mem);
				//n_shareddata.dirty = false;
				break;

			case true:
				//n_shareddata.data = 0;
				// Stores mark the data as dirty.
				n_shareddata.dirty = true;
				break;
			}
		}
		break;

	// We were the only reference, so don't perform any allocation or
	// deallocation, and don't change the reference count.
	case 1:
		curr_shareddata.base_addr = n_base_addr;

		switch (curr_shareddata.dirty)
		{
		case true:
			// When an address's data is no longer in the LAR file, it must
			// be sent to memory iff the data has been changed.
			store_to_mem(curr_shareddata.data, old_base_addr, mem);
			if (!is_store)
			{
				// Loads of fresh data mark us as clean.
				load_from_mem(curr_shareddata.data, n_base_addr, mem);
				curr_shareddata.dirty = false;
			}
			break;

		case false:
			// Even though we were the only LAR that had data from our
			// address, the data hasn't been changed, so we don't need to
			// send it back to memory.
			switch (is_store)
			{
			case false:
				load_from_mem(curr_shareddata.data, n_base_addr, mem);
				break;

			case true:
				// Stores to an address nobody has yet marks the data as
				// dirty.
				curr_shareddata.dirty = true;
				break;
			}
			break;
		}
		break;

	// There are other LARs that have our old data, but no LAR has the data
	// from our new address.
	default:
		{
			// Allocate a new element of shared data.
			ddest_metadata.tag = alloc_tag();
			auto& n_shareddata = __lar_shareddata[ddest_metadata.tag];
			n_shareddata.base_addr = n_base_addr;
			n_shareddata.ref_count = 1;

			--curr_shareddata.ref_count;

			switch (is_store)
			{
			case false:
				// Because at least one other LAR has our previous data, we
				// do not need to store it back to memory yet, but since
				// nobody has our new address, we need to load that
				// address's data from memory.
				load_from_mem(n_shareddata.data, n_base_addr, mem);
				// Loads of fresh data mark us as clean
				n_shareddata.dirty = false;
				break;

			case true:
				// Make a copy of our old data over to the freshly
				// allocated element of shared data.
				n_shareddata.data = curr_shareddata.data;

				// Also, since this is a store, mark the **copy** of our
				// old data as dirty.
				n_shareddata.dirty = true;

				break;
			}
		}
		break;
	}
}


} // namespace snow64_simulator
