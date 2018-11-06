#ifndef src_global_constants_hpp
#define src_global_constants_hpp

// src/global_constants.hpp

#include "misc_includes.hpp"

namespace snow64_simulator
{

namespace constants
{

namespace lar_file
{

static constexpr size_t WIDTH___METADATA_TAG = 4;
static constexpr size_t WIDTH___METADATA_DATA_OFFSET = 5;
static constexpr size_t WIDTH___METADATA_DATA_TYPE = 2;
static constexpr size_t WIDTH___METADATA_INT_TYPE_SIZE = 2;

static constexpr size_t WIDTH___SHAREDDATA_REF_COUNT
	= WIDTH___METADATA_TAG;
static constexpr size_t WIDTH___SHAREDDATA_BASE_ADDR
	= (sizeof(u64) * 8) - WIDTH___METADATA_DATA_OFFSET;

} // namespace lar_file

} // namespace constants

} // namespace snow64_simulator


#endif		// src_global_constants_hpp
