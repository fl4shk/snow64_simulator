#ifndef src__slash__global_constants_hpp
#define src__slash__global_constants_hpp

// src/global_constants.hpp

#include "misc_includes.hpp"

namespace snow64_simulator
{

namespace constants
{

namespace lar_file
{

static constexpr size_t WIDTH__METADATA_TAG = 4;
static constexpr size_t WIDTH__METADATA_DATA_OFFSET = 5;
static constexpr size_t WIDTH__METADATA_DATA_TYPE = 2;
static constexpr size_t WIDTH__METADATA_INT_TYPE_SIZE = 2;

static constexpr size_t WIDTH__SHAREDDATA_REF_COUNT
	= WIDTH__METADATA_TAG;
static constexpr size_t WIDTH__SHAREDDATA_BASE_ADDR
	= (sizeof(u64) * 8) - WIDTH__METADATA_DATA_OFFSET;

} // namespace lar_file

} // namespace constants

} // namespace snow64_simulator


#endif		// src__slash__global_constants_hpp
