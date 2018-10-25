#ifndef src__slash__avx_wrapper_classes_hpp
#define src__slash__avx_wrapper_classes_hpp

// src/avx_wrapper_classes.hpp

#include "misc_includes.hpp"

namespace snow64_simulator
{
//class Avx256i8
//{
//public:		// constants
//	static constexpr size_t NUM_BYTES__DATA = 256 / 8;
//
//public:		// variables
//	__m256i data;
//
//public:		// functions
//	inline Avx256i8()
//	{
//	}
//
//	inline Avx256i8(__m256i s_data)
//		: data(s_data)
//	{
//	}
//
//	inline Avx256i8(const Avx256i8& to_copy) = default;
//
//	inline Avx256i8& operator = (const Avx256i8& to_copy) = default;
//
//	inline Avx256i8& operator += (const Avx256i8& other)
//	{
//		*this = *this + other;
//		return *this;
//	}
//	inline Avx256i8& operator -= (const Avx256i8& other)
//	{
//		*this = *this - other;
//		return *this;
//	}
//	inline Avx256i8& operator *= (const Avx256i8& other)
//	{
//		*this = *this * other;
//		return *this;
//	}
//	inline Avx256i8& operator /= (const Avx256i8& other)
//	{
//		*this = *this / other;
//		return *this;
//	}
//
//	inline Avx256i8 operator + (const Avx256i8& other) const
//	{
//		return Avx256i8(_mm256_add_epi8(data, other.data));
//	}
//	inline Avx256i8 operator - (const Avx256i8& other) const
//	{
//		return Avx256i8(_mm256_sub_epi8(data, other.data));
//	}
//
//	Avx256i8 operator * (const Avx256i8& other) const;
//	Avx256i8 operator / (const Avx256i8& other) const;
//	Avx256i8 operator < (const Avx256i8& other) const;
//
//};
}


#endif		// src__slash__avx_wrapper_classes_hpp
