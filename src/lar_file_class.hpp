#ifndef src__slash__lar_file_class_hpp
#define src__slash__lar_file_class_hpp

// src/lar_file_class.hpp

#include "misc_includes.hpp"
#include "basic_word_class.hpp"

namespace snow64_simulator
{

class LarFile
{
public:		// enums
	enum class DataType
	{
		UnsgnInt,
		SgnInt,
		BFloat16,
		//Reserved
	};

	enum class IntTypeSize
	{
		Sz8,
		Sz16,
		Sz32,
		Sz64
	};


	enum class RegName
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

	static constexpr size_t WIDTH__METADATA_TAG = 4;
	static constexpr size_t WIDTH__METADATA_DATA_OFFSET = 5;
	static constexpr size_t WIDTH__METADATA_DATA_TYPE = 2;
	static constexpr size_t WIDTH__METADATA_INT_TYPE_SIZE = 2;

	static constexpr size_t WIDTH__SHAREDDATA_REF_COUNT
		= WIDTH__METADATA_TAG;


public:		// classes
	class LarMetadata
	{
	public:		// variables
		size_t tag : WIDTH__METADATA_TAG;
		size_t data_offset : WIDTH__METADATA_DATA_OFFSET;
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

		inline LarMetadata(const LarMetadata& to_copy) = default;

		inline LarMetadata& operator = (const LarMetadata& to_copy)
			= default;
	};


	class LarShareddata
	{
	public:		// variables
		BasicWord data;

		size_t ref_count : WIDTH__SHAREDDATA_REF_COUNT;

		size_t dirty : 1;

	public:		// functions
		inline LarShareddata()
		{
			ref_count = 0;
			dirty = 0;
		}

		inline LarShareddata(const LarShareddata& to_copy) = default;

		inline LarShareddata& operator = (const LarShareddata& to_copy)
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
		LarMetadata& ra_metadata, LarShareddata& ra_shareddata,
		LarMetadata& rb_metadata, LarShareddata& rb_shareddata,
		LarMetadata& rc_metadata, LarShareddata& rc_shareddata)
	{
		ra_metadata = __lar_metadata[ra_index];
		rb_metadata = __lar_metadata[rb_index];
		rc_metadata = __lar_metadata[rc_index];

		ra_shareddata = __lar_shareddata[ra_metadata.tag];
		rb_shareddata = __lar_shareddata[rb_metadata.tag];
		rc_shareddata = __lar_shareddata[rc_metadata.tag];
	}

	//inline void write_arithlog_results(size_t index,
	//	const BasicWord& n_data)
	//{
	//	if (index != 0)
	//	{
	//		__lar_shareddata[__lar_metadata[index].tag].data = n_data;
	//	}
	//}

	bool attempt_ldst(size_t index, Address eff_addr,
		DataType n_data_type, IntTypeSize n_int_type_size);
};

} // namespace snow64_simulator

#endif		// src__slash__lar_file_class_hpp
