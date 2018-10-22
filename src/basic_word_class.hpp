#ifndef src__slash__basic_word_class_hpp
#define src__slash__basic_word_class_hpp

// src/basic_word_class.hpp

#include "misc_includes.hpp"
#include "global_constants.hpp"

namespace snow64_simulator
{
typedef u64 Address;
inline Address convert_addr_to_bw_addr(Address to_convert);
inline Address convert_addr_to_bw_instr_offset(Address to_convert);



class BasicWord
{
public:		// constants
	static constexpr size_t num_data_elems
		= (1 << constants::lar_file::WIDTH__METADATA_DATA_OFFSET);
public:		// variables
	u8 data[num_data_elems];

public:		// functions
	inline BasicWord()
	{
		for (size_t i=0; i<num_data_elems; ++i)
		{
			data[i] = 0;
		}
	}

	inline BasicWord(const BasicWord& to_copy)
	{
		*this = to_copy;
	}

	inline BasicWord& operator = (const BasicWord& to_copy)
	{
		for (size_t i=0; i<num_data_elems; ++i)
		{
			data[i] = to_copy.data[i];
		}

		return *this;
	}

};

inline Address convert_addr_to_bw_addr(Address to_convert)
{
	return (to_convert / sizeof(BasicWord::num_data_elems));
}

} // namespace snow64_simulator


#endif		// src__slash__basic_word_class_hpp
