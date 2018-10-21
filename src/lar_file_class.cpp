#include "lar_file_class.hpp"

namespace snow64_simulator
{

LarFile::LarFile()
{
}

LarFile::~LarFile()
{
}

bool LarFile::attempt_ldst(size_t index, Address eff_addr,
	LarFile::DataType n_data_type, LarFile::IntTypeSize n_int_type_size)
{
	return false;
}

} // namespace snow64_simulator
