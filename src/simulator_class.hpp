#ifndef src_simulator_class_hpp
#define src_simulator_class_hpp

// src/simulator_class.hpp

#include "misc_includes.hpp"
#include "lar_file_class.hpp"
#include "instr_decoder_class.hpp"
#include "bfloat16_class.hpp"
#include "avx_wrapper_classes.hpp"

namespace snow64_simulator
{



class Simulator
{
public:		// enums
	enum class SyscallType
	{
		DispRegs,
		DispDdestVectorData,
		DispDdestScalarData,
		DispDdestAddr,
		Finish,
	};


private:		// variables
	LarFile __lar_file;
	InstrDecoder __instr_decoder;
	Address __pc = 0;

	LarFile::RefLarContents __curr_ddest_contents, __curr_dsrc0_contents,
		__curr_dsrc1_contents;



	InstrDecoder::Instr __curr_instr = 0;

	std::string __data_filename;

	size_t __mem_amount_in_bytes = 0, __mem_amount_in_words = 0;

	std::unique_ptr<BasicWord[]> __mem;


public:		// functions
	Simulator(const std::string& s_data_filename,
		size_t s_min_mem_amount_in_bytes);
	virtual ~Simulator();

	int run();

	gen_getter_by_val(mem_amount_in_words)

private:		// functions
	void err(const std::string& msg)
	{
		printerr("Error:  ", msg, "\n");
		exit(1);
	}


	// Standard fetch->decode->execute loop.
	void perf_instr_fetch();
	void perf_instr_decode();
	bool perf_instr_exec();

	bool handle_sim_syscall();


	void perf_group_0_scalar_op();
	void perf_group_0_vector_op();

	template<typename DdestType>
	void inner_perf_regular_integer_group_0_scalar_op();

	template<typename DdestType>
	void inner_perf_forced_64_bit_integer_group_0_scalar_op();

	template<typename DdestType>
	void inner_perf_regular_integer_group_0_vector_op();

	template<typename DdestType>
	void inner_perf_forced_64_bit_integer_group_0_vector_op();


	void inner_perf_bfloat16_group_0_scalar_op();
	void inner_perf_bfloat16_group_0_vector_op();


	std::string get_reg_name_str(LarFile::RegName some_reg_name) const;

	template<typename LarType>
	static inline constexpr size_t num_lar_elems()
	{
		return BasicWord::NUM_DATA_ELEMS / sizeof(LarType);
	}




	template<typename DdestType, typename DsrcType>
	static inline void cast_and_copy_to_int_temp_dsrc_arr
		(const LarFile::RefLarContents& curr_dsrc_contents,
		DdestType* temp_dsrc_arr)
	{
		static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<DdestType>();
		static constexpr size_t DSRC_LAR_ARR_SIZE
			= num_lar_elems<DsrcType>();

		auto& curr_data = curr_dsrc_contents.shareddata->data;

		if constexpr (DSRC_LAR_ARR_SIZE < TEMP_ARR_SIZE)
		{
			ASM_COMMENT("cast_and_copy_to_int_temp_dsrc_arr():  "
				"DSRC_LAR_ARR_SIZE < TEMP_ARR_SIZE");
			size_t i = 0;

			if constexpr (std::is_same<DsrcType, u8>()
				|| std::is_same<DsrcType, s8>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u8");
				}
				else
				{
					ASM_COMMENT("DsrcType s8");
				}
				for (i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = static_cast<DdestType>
						(static_cast<DsrcType>(curr_data.get_8
						(i * sizeof(DsrcType))));
				}
			}
			else if constexpr (std::is_same<DsrcType, u16>()
				|| std::is_same<DsrcType, s16>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u16");
				}
				else
				{
					ASM_COMMENT("DsrcType s16");
				}
				for (i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = static_cast<DdestType>
						(static_cast<DsrcType>(curr_data.get_16
						(i * sizeof(DsrcType))));
				}
			}
			else if constexpr (std::is_same<DsrcType, u32>()
				|| std::is_same<DsrcType, s32>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u32");
				}
				else
				{
					ASM_COMMENT("DsrcType s32");
				}
				for (i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = static_cast<DdestType>
						(static_cast<DsrcType>(curr_data.get_32
						(i * sizeof(DsrcType))));
				}
			}
			else if constexpr (std::is_same<DsrcType, u64>()
				|| std::is_same<DsrcType, s64>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u64");
				}
				else
				{
					ASM_COMMENT("DsrcType s64");
				}
				for (i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = static_cast<DdestType>
						(static_cast<DsrcType>(curr_data.get_64
						(i * sizeof(DsrcType))));
				}
			}
			else if constexpr (std::is_same<DsrcType, BFloat16>())
			{
				ASM_COMMENT("DsrcType BFloat16");
				for (size_t i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = static_cast<DdestType>
						(static_cast<float>(BFloat16(curr_data.get_16(i
						* sizeof(DsrcType)))));
				}
			}

			ASM_COMMENT("for loop");
			for (; i<TEMP_ARR_SIZE; ++i)
			{
				temp_dsrc_arr[i] = 0;
			}
		}
		else if constexpr (DSRC_LAR_ARR_SIZE >= TEMP_ARR_SIZE)
		{
			ASM_COMMENT("cast_and_copy_to_int_temp_dsrc_arr():  "
				"DSRC_LAR_ARR_SIZE >= TEMP_ARR_SIZE");
			if constexpr (std::is_same<DsrcType, u8>()
				|| std::is_same<DsrcType, s8>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u8");
				}
				else
				{
					ASM_COMMENT("DsrcType s8");
				}
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = curr_data.get_8(i
						* sizeof(DsrcType));
				}
			}
			else if constexpr (std::is_same<DsrcType, u16>()
				|| std::is_same<DsrcType, s16>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u16");
				}
				else
				{
					ASM_COMMENT("DsrcType s16");
				}
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = curr_data.get_16(i
						* sizeof(DsrcType));
				}
			}
			else if constexpr (std::is_same<DsrcType, u32>()
				|| std::is_same<DsrcType, s32>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u32");
				}
				else
				{
					ASM_COMMENT("DsrcType s32");
				}
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = curr_data.get_32(i
						* sizeof(DsrcType));
				}
			}
			else if constexpr (std::is_same<DsrcType, u64>()
				|| std::is_same<DsrcType, s64>())
			{
				if constexpr (std::is_unsigned<DsrcType>())
				{
					ASM_COMMENT("DsrcType u64");
				}
				else
				{
					ASM_COMMENT("DsrcType s64");
				}
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = curr_data.get_64(i
						* sizeof(DsrcType));
				}
			}
			else if constexpr (std::is_same<DsrcType, BFloat16>())
			{
				ASM_COMMENT("DsrcType BFloat16");

				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = static_cast<DdestType>
						(static_cast<float>(BFloat16(curr_data.get_16(i
						* sizeof(DsrcType)))));
				}
			}
		}
	}

	//template<typename DdestType, typename DsrcType>
	//static inline void cast_and_copy_to_int_temp_dsrc_arr
	//	(const LarFile::RefLarContents& curr_dsrc_contents,
	//	DdestType* temp_dsrc_arr)
	//{
	//	static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<DdestType>();
	//	static constexpr size_t DSRC_LAR_ARR_SIZE
	//		= num_lar_elems<DsrcType>();
	//}


	template<typename DsrcType>
	static inline void cast_and_copy_to_bfloat16_temp_dsrc_arr
		(const LarFile::RefLarContents& curr_dsrc_contents,
		BFloat16* temp_dsrc_arr)
	{
		static constexpr size_t TEMP_ARR_SIZE = num_lar_elems<BFloat16>();
		static constexpr size_t DSRC_LAR_ARR_SIZE
			= num_lar_elems<DsrcType>();
		auto& curr_data = curr_dsrc_contents.shareddata->data;

		//for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
		//{
		//	temp_dsrc_arr[i] = BFloat16();
		//}

		if constexpr (!std::is_same<DsrcType, BFloat16>())
		{
			if constexpr (std::is_same<DsrcType, u8>()
				|| std::is_same<DsrcType, s8>())
			{
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16::create_from_int<DsrcType>
						(curr_data.get_8(i * sizeof(DsrcType)));
				}
			}
			else if constexpr (std::is_same<DsrcType, u16>()
				|| std::is_same<DsrcType, s16>())
			{
				for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16::create_from_int<DsrcType>
						(curr_data.get_16(i * sizeof(DsrcType)));
				}
			}
			else if constexpr (std::is_same<DsrcType, u32>()
				|| std::is_same<DsrcType, s32>())
			{
				size_t i;
				for (i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16::create_from_int<DsrcType>
						(curr_data.get_32(i * sizeof(DsrcType)));
				}
				for (; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16();
				}
			}
			else if constexpr (std::is_same<DsrcType, u64>()
				|| std::is_same<DsrcType, s64>())
			{
				size_t i;
				for (i=0; i<DSRC_LAR_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16::create_from_int<DsrcType>
						(curr_data.get_64(i * sizeof(DsrcType)));
				}
				for (; i<TEMP_ARR_SIZE; ++i)
				{
					temp_dsrc_arr[i] = BFloat16();
				}
			}
		}
		else
		{
			for (size_t i=0; i<TEMP_ARR_SIZE; ++i)
			{
				temp_dsrc_arr[i] = BFloat16(static_cast<size_t>
					(curr_data.get_16(i * sizeof(DsrcType))));
			}
		}
	}

	//template<typename DsrcType>
	//static inline void cast_and_copy_to_bfloat16_temp_dsrc_scalar
	//	(const LarFile::RefLarContents& curr_dsrc_contents,
	//	BFloat16& temp_dsrc_scalar)
	//{
	//}
};

} // namespace snow64_simulator

#endif		// src_simulator_class_hpp
