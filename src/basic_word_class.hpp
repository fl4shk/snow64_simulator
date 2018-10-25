#ifndef src__slash__basic_word_class_hpp
#define src__slash__basic_word_class_hpp

// src/basic_word_class.hpp

#include "misc_includes.hpp"
#include "global_constants.hpp"
#include <math.h>

namespace snow64_simulator
{
typedef u64 Address;
inline Address convert_addr_to_bw_addr(Address to_convert);



class alignas(32) BasicWord
{
public:		// constants
	static constexpr size_t NUM_DATA_ELEMS
		= (1 << constants::lar_file::WIDTH__METADATA_DATA_OFFSET);

	//static constexpr Address
	//	shift_amounts[] = {0, 8, 16, 24, 32, 40, 48, 56};
public:		// variables
	u8 data[NUM_DATA_ELEMS];

public:		// functions
	inline BasicWord()
	{
		for (size_t i=0; i<NUM_DATA_ELEMS; ++i)
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
		for (size_t i=0; i<NUM_DATA_ELEMS; ++i)
		{
			data[i] = to_copy.data[i];
		}
		return *this;
	}

	inline u8 get_8(Address index) const
	{
		return data[index];
	}
	inline void set_8(Address index, u8 val)
	{
		data[index] = val;
	}

	inline u16 get_16(Address index)
	{
		const Address aligned_index = index & (~static_cast<Address>(0b1));


		u16 ret = 0;


		set_bits_with_range(ret, data[aligned_index + 0], 7, 0);
		set_bits_with_range(ret, data[aligned_index + 1], 15, 8);


		return ret;
	}
	inline void set_16(Address index, u16 val)
	{
		const Address aligned_index = index & (~static_cast<Address>(0b1));

		//static constexpr size_t NUM_ITERATIONS = sizeof(u16);

		//for (size_t i=0; i<NUM_ITERATIONS; ++i)
		//{
		//	data[aligned_index + i] = (val >> shift_amount(i));
		//}

		data[aligned_index + 0] = get_bits_with_range(val, 7, 0);
		data[aligned_index + 1] = get_bits_with_range(val, 15, 8);
	}

	inline u32 get_32(Address index) const
	{
		const Address aligned_index = index
			& (~static_cast<Address>(0b11));

		u32 ret = 0;

		set_bits_with_range(ret, data[aligned_index + 0], 7, 0);
		set_bits_with_range(ret, data[aligned_index + 1], 15, 8);
		set_bits_with_range(ret, data[aligned_index + 2], 23, 16);
		set_bits_with_range(ret, data[aligned_index + 3], 31, 24);

		return ret;
	}

	inline void set_32(Address index, u32 val)
	{
		const Address aligned_index = index
			& (~static_cast<Address>(0b11));

		//static constexpr size_t NUM_ITERATIONS = sizeof(u32);

		//for (size_t i=0; i<NUM_ITERATIONS; ++i)
		//{
		//	data[aligned_index + i] = (val >> shift_amount(i));
		//}

		data[aligned_index + 0] = get_bits_with_range(val, 7, 0);
		data[aligned_index + 1] = get_bits_with_range(val, 15, 8);
		data[aligned_index + 2] = get_bits_with_range(val, 23, 16);
		data[aligned_index + 3] = get_bits_with_range(val, 31, 24);
	}

	inline u64 get_64(Address index) const
	{
		const Address aligned_index = index
			& (~static_cast<Address>(0b111));

		u64 ret = 0;

		set_bits_with_range(ret, data[aligned_index + 0], 7, 0);
		set_bits_with_range(ret, data[aligned_index + 1], 15, 8);
		set_bits_with_range(ret, data[aligned_index + 2], 23, 16);
		set_bits_with_range(ret, data[aligned_index + 3], 31, 24);
		set_bits_with_range(ret, data[aligned_index + 4], 39, 32);
		set_bits_with_range(ret, data[aligned_index + 5], 47, 40);
		set_bits_with_range(ret, data[aligned_index + 6], 55, 48);
		set_bits_with_range(ret, data[aligned_index + 7], 63, 56);

		return ret;
	}

	inline void set_64(Address index, u64 val)
	{
		const Address aligned_index = index
			& (~static_cast<Address>(0b111));

		//static constexpr size_t NUM_ITERATIONS = sizeof(u64);

		//for (size_t i=0; i<NUM_ITERATIONS; ++i)
		//{
		//	data[aligned_index + i] = (val >> shift_amount(i));
		//}

		data[aligned_index + 0] = get_bits_with_range(val, 7, 0);
		data[aligned_index + 1] = get_bits_with_range(val, 15, 8);
		data[aligned_index + 2] = get_bits_with_range(val, 23, 16);
		data[aligned_index + 3] = get_bits_with_range(val, 31, 24);
		data[aligned_index + 4] = get_bits_with_range(val, 39, 32);
		data[aligned_index + 5] = get_bits_with_range(val, 47, 40);
		data[aligned_index + 6] = get_bits_with_range(val, 55, 48);
		data[aligned_index + 7] = get_bits_with_range(val, 63, 56);
	}

private:		// functions
	inline size_t shift_amount(size_t to_shift) const
	{
		return (to_shift << 3);
	}

};

inline Address convert_addr_to_bw_addr(Address to_convert)
{
	return (to_convert / BasicWord::NUM_DATA_ELEMS);
}
inline Address convert_addr_to_bw_instr_index(Address to_convert)
{
	return (to_convert % BasicWord::NUM_DATA_ELEMS);
}

} // namespace snow64_simulator

inline std::ostream& operator << (std::ostream& os,
	const snow64_simulator::BasicWord& to_show)
{
	for (s64 i=to_show.NUM_DATA_ELEMS-1; i>=0; --i)
	//for (size_t i=0; i<to_show.NUM_DATA_ELEMS; ++i)
	{
		osprintout(os, static_cast<u32>(to_show.data[i]));
	}
	return os;
}

#endif		// src__slash__basic_word_class_hpp
