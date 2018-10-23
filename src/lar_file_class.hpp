#ifndef src__slash__lar_file_class_hpp
#define src__slash__lar_file_class_hpp

// src/lar_file_class.hpp

#include "misc_includes.hpp"
#include "basic_word_class.hpp"

namespace snow64_simulator
{

class LarFile
{
public:		// enums and typedefs
	typedef u64 ScalarData;

	enum class DataType : u8
	{
		UnsgnInt,
		SgnInt,
		BFloat16,
		//Reserved
	};

	enum class TypeSize : u8
	{
		Sz8,
		Sz16,
		Sz32,
		Sz64
	};


	enum class RegName : u8
	{
		Dzero, Du0, Du1, Du2,
		Du3, Du4, Du5, Du6,
		Du7, Du8, Du9, Du10,
		Du11, Dlr, Dfp, Dsp,

		Ddummy
	};

public:		// constants
	static constexpr size_t ARR_SIZE__NUM_LARS
		= static_cast<size_t>(RegName::Ddummy);

	static constexpr size_t WIDTH__METADATA_TAG
		= constants::lar_file::WIDTH__METADATA_TAG;
	static constexpr size_t WIDTH__METADATA_DATA_OFFSET
		= constants::lar_file::WIDTH__METADATA_DATA_OFFSET;
	static constexpr size_t WIDTH__METADATA_DATA_TYPE
		= constants::lar_file::WIDTH__METADATA_DATA_TYPE;
	static constexpr size_t WIDTH__METADATA_INT_TYPE_SIZE
		= constants::lar_file::WIDTH__METADATA_INT_TYPE_SIZE;

	static constexpr size_t WIDTH__SHAREDDATA_REF_COUNT
		= constants::lar_file::WIDTH__SHAREDDATA_REF_COUNT;
	static constexpr size_t WIDTH__SHAREDDATA_BASE_ADDR
		= constants::lar_file::WIDTH__SHAREDDATA_BASE_ADDR;


public:		// classes
	class LarMetadata
	{
	public:		// variables
		u8 tag = 0;
		u8 data_offset = 0;
		DataType data_type = DataType::UnsgnInt;
		TypeSize type_size = TypeSize::Sz8;

	public:		// functions
		inline LarMetadata()
		{
		}

		inline LarMetadata(const LarMetadata& to_copy) = default;
		inline LarMetadata& operator = (const LarMetadata& to_copy)
			= default;
	};


	class LarShareddata
	{
	public:		// variables
		BasicWord data;

		u8 ref_count = 0;
		u64 base_addr = 0;

		bool dirty = false;

	public:		// functions
		inline LarShareddata()
		{
		}

		inline LarShareddata(const LarShareddata& to_copy) = default;
		inline LarShareddata& operator = (const LarShareddata& to_copy)
			= default;
	};

	class RefLarContents
	{
	public:		// variables
		LarMetadata* metadata = nullptr;
		LarShareddata* shareddata = nullptr;

	public:		// functions
		inline RefLarContents()
		{
		}

		inline RefLarContents(const RefLarContents& to_copy) = default;

		inline RefLarContents& operator = (const RefLarContents& to_copy)
			= default;

		inline Address full_address() const
		{
			Address ret = 0;
			ret |= (shareddata->base_addr << WIDTH__METADATA_DATA_OFFSET);
			ret |= metadata->data_offset;
			return ret;
		}

		inline u64 scalar_data() const
		{
			switch (metadata->type_size)
			{
			case TypeSize::Sz8:
				return shareddata->data.get_8(metadata->data_offset);
			case TypeSize::Sz16:
				return shareddata->data.get_16(metadata->data_offset);
			case TypeSize::Sz32:
				return shareddata->data.get_32(metadata->data_offset);
			case TypeSize::Sz64:
				return shareddata->data.get_64(metadata->data_offset);
			}

			// This should never occur!
			return 9001;
		}
	};


private:		// variables
	std::array<LarMetadata, ARR_SIZE__NUM_LARS> __lar_metadata;
	std::array<LarShareddata, ARR_SIZE__NUM_LARS> __lar_shareddata;
	std::array<u8, ARR_SIZE__NUM_LARS> __lar_tag_stack;
	u8 __curr_tag_stack_index;

public:		// functions
	LarFile();
	virtual ~LarFile();

	inline void read_from(size_t ddest_index, size_t dsrc0_index,
		size_t dsrc1_index,
		RefLarContents& ddest_contents, RefLarContents& dsrc0_contents,
		RefLarContents& dsrc1_contents)
	{
		ddest_contents.metadata = &__lar_metadata[ddest_index];
		ddest_contents.shareddata = &__lar_shareddata[ddest_contents
			.metadata->tag];

		dsrc0_contents.metadata = &__lar_metadata[dsrc0_index];
		dsrc0_contents.shareddata = &__lar_shareddata[dsrc0_contents
			.metadata->tag];

		dsrc1_contents.metadata = &__lar_metadata[dsrc1_index];
		dsrc1_contents.shareddata = &__lar_shareddata[dsrc1_contents
			.metadata->tag];
	}
	

	inline void write_arithlog_results(const RefLarContents& ddest_contents,
		const BasicWord& n_data)
	{
		if (ddest_contents.metadata->tag != 0)
		{
			// Oh yeah, this marks us as dirty!
			ddest_contents.shareddata->dirty = true;
			ddest_contents.shareddata->data = n_data;
		}
	}

	void perf_ldst(bool is_store, size_t ddest_index, Address eff_addr,
		DataType n_data_type, TypeSize n_type_size,
		std::unique_ptr<BasicWord[]>& mem, size_t mem_amount_in_words);


	//void continue_ldst(const BasicWord& )

private:		// functions
	inline u8 perf_temp_tag_search(size_t i, Address n_base_addr) const
	{
		return (((__lar_shareddata[i].ref_count != 0)
			&& (__lar_shareddata[i].base_addr == n_base_addr))
			? i : 0);
	}

	void handle_ldst_hit(bool is_store, LarMetadata& ddest_metadata,
		Address n_base_addr, Address n_data_offset,
		std::unique_ptr<BasicWord[]>& mem, u8 tag_search);
	void handle_ldst_miss(bool is_store, LarMetadata& ddest_metadata,
		Address n_base_addr, Address n_data_offset,
		std::unique_ptr<BasicWord[]>& mem);

	inline u8 alloc_tag()
	{
		const auto ret = __lar_tag_stack[__curr_tag_stack_index];
		--__curr_tag_stack_index;

		return ret;
	}
	inline void dealloc_tag(u8 old_tag)
	{
		++__curr_tag_stack_index;
		__lar_tag_stack[__curr_tag_stack_index] = old_tag;
	}

	inline void load_from_mem(BasicWord& ret, Address base_addr,
		std::unique_ptr<BasicWord[]>& mem)
	{
		ret = mem[convert_addr_to_bw_addr(base_addr)];
	}
	inline void store_to_mem(const BasicWord& to_store, Address base_addr,
		std::unique_ptr<BasicWord[]>& mem)
	{
		mem[convert_addr_to_bw_addr(base_addr)] = to_store;
	}
};

} // namespace snow64_simulator

#endif		// src__slash__lar_file_class_hpp
