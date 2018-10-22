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

	enum class IntTypeSize : u8
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
		u8 tag;
		u8 data_offset;
		DataType data_type;
		IntTypeSize int_type_size;

	public:		// functions
		inline LarMetadata()
		{
			tag = 0;
			data_offset = 0;
			data_type = DataType::UnsgnInt;
			int_type_size = IntTypeSize::Sz8;
		}

		inline LarMetadata(const LarMetadata& to_copy)
		{
			*this = to_copy;
		}

		inline LarMetadata& operator = (const LarMetadata& to_copy)
		{
			tag = to_copy.tag;
			data_offset = to_copy.data_offset;
			data_type = to_copy.data_type;
			int_type_size = to_copy.int_type_size;

			return *this;
		}
	};


	class LarShareddata
	{
	public:		// variables
		BasicWord data;

		u8 ref_count;
		size_t base_addr;

		bool dirty;

	public:		// functions
		inline LarShareddata()
		{
			ref_count = 0;
			base_addr = 0;
			dirty = 0;
		}

		inline LarShareddata(const LarShareddata& to_copy)
		{
			*this = to_copy;
		}

		inline LarShareddata& operator = (const LarShareddata& to_copy)
		{
			data = to_copy.data;
			ref_count = to_copy.ref_count;
			base_addr = to_copy.base_addr;
			dirty = to_copy.dirty;
			return *this;
		}
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
	};


private:		// variables
	std::array<LarMetadata, ARR_SIZE__NUM_LARS> __lar_metadata;
	std::array<LarShareddata, ARR_SIZE__NUM_LARS> __lar_shareddata;

public:		// functions
	LarFile();
	virtual ~LarFile();

	inline void read_from(size_t ra_index, size_t rb_index,
		size_t rc_index,
		RefLarContents& ra_contents, RefLarContents& rb_contents,
		RefLarContents& rc_contents)
	{
		ra_contents.metadata = &__lar_metadata[ra_index];
		ra_contents.shareddata = &__lar_shareddata[ra_contents.metadata
			->tag];

		rb_contents.metadata = &__lar_metadata[rb_index];
		rb_contents.shareddata = &__lar_shareddata[rb_contents.metadata
			->tag];

		rc_contents.metadata = &__lar_metadata[rc_index];
		rc_contents.shareddata = &__lar_shareddata[rc_contents.metadata
			->tag];
	}
	

	//inline void write_arithlog_results(size_t index,
	//	const BasicWord& n_data)
	//{
	//	if (index != 0)
	//	{
	//		__lar_shareddata[__lar_metadata[index].tag].data = n_data;
	//	}
	//}

	void perf_ldst(bool is_store, size_t index, Address eff_addr,
		DataType n_data_type, IntTypeSize n_int_type_size,
		std::unique_ptr<BasicWord[]>& mem, size_t mem_amount_in_words);


	//void continue_ldst(const BasicWord& )

private:		// functions
	//inline size_t perf_tag_search(size_t i, size_t index,
	//	size_t n_base_addr)
	//{
	//	return (((__lar_shareddata[i].ref_count != 0)
	//		&& (__lar_shareddata[i].base_addr == n_base_addr))
	//		? index : 0);
	//}
};

} // namespace snow64_simulator

#endif		// src__slash__lar_file_class_hpp
