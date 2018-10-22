#include "simulator_class.hpp"
#include <fstream>
#include <ctype.h>

namespace snow64_simulator
{

Simulator::Simulator(const std::string& s_data_filename,
	size_t s_min_mem_amount_in_bytes) : __data_filename(s_data_filename),
	__mem_amount_in_bytes(s_min_mem_amount_in_bytes)
{
	__mem_amount_in_words = __mem_amount_in_bytes
		/ sizeof(BasicWord);

	if ((__mem_amount_in_words * sizeof(BasicWord))
		!= __mem_amount_in_bytes)
	{
		++__mem_amount_in_words;
	}

	//printout("__data_filename:  ", __data_filename, "\n");
	printout("__mem_amount_in_bytes:  ",
		__mem_amount_in_bytes, "\n");

	printout("__mem_amount_in_words:  ", __mem_amount_in_words,
		"\n");

	__mem.reset(new BasicWord[mem_amount_in_words()]);

	for (size_t i=0; i<mem_amount_in_words(); ++i)
	{
		__mem[i] = BasicWord();
	}


	{
	size_t word_addr = 0;
	std::ifstream data_ifstream(__data_filename);
	std::string line;

	while (!data_ifstream.eof())
	{
		std::getline(data_ifstream, line, '\n');

		if (line.size() == 0)
		{
			continue;
		}

		if (word_addr >= mem_amount_in_words())
		{
			err(sconcat("Too few memory words requested for input data."));
		}

		// Change address
		if (line.at(0) == '@')
		{
			u64 n_addr = 0; 
			for (size_t i=1; i<line.size(); ++i)
			{
				n_addr <<= 4;
				if ((line.at(i) >= '0') && (line.at(i) <= '9'))
				{
					n_addr |= (line.at(i) - '0');
				}
				else if ((line.at(i) >= 'a') && (line.at(i) <= 'f'))
				{
					n_addr |= (line.at(i) - 'a' + 0xa);
				}
				else
				{
					err(sconcat("Invalid address change."));
				}
			}

			word_addr = n_addr;
		}
		// Raw memory word
		else
		{
			if (line.size() != (sizeof(BasicWord) * 2))
			{
				err(sconcat("Invalid line of initial memory contents."));
			}

			for (size_t i=0; i<BasicWord::num_data_elems; ++i)
			{
				u8& n_data = __mem[word_addr].data[i];
				n_data = 0;

				for (size_t j=0; j<(sizeof(u8) * 2); ++j)
				{
					const size_t index
						= ((BasicWord::num_data_elems - i - 1) * 2) + j;
					n_data <<= 4;

					if ((line.at(index) >= '0') && (line.at(index) <= '9'))
					{
						n_data |= (line.at(index) - '0');
					}
					else if ((line.at(index) >= 'a')
						&& (line.at(index) <= 'f'))
					{
						n_data |= (line.at(index) - 'a' + 0xa);
					}
					else
					{
						err(sconcat("Invalid line of data."));
					}
				}
			}

			++word_addr;
		}

	}

	}

	//for (size_t i=0; i<mem_amount_in_words(); ++i)
	//{
	//	for (size_t j=0; j<BasicWord::num_data_elems; j+=4)
	//	{
	//		//u32 temp = 0;
	//		//set_bits_with_range(temp, __mem[i].data[j + 0], 31, 24);
	//		//set_bits_with_range(temp, __mem[i].data[j + 1], 23, 16);
	//		//set_bits_with_range(temp, __mem[i].data[j + 2], 15, 8);
	//		//set_bits_with_range(temp, __mem[i].data[j + 3], 7, 0);

	//		u32 temp = ((__mem[i].data[j + 0] << 0)
	//			| (__mem[i].data[j + 1] << 8)
	//			| (__mem[i].data[j + 2] << 16)
	//			| (__mem[i].data[j + 3] << 24));
	//		printout(std::hex, temp, std::dec, " ");
	//	}
	//	//for (size_t j=0; j<BasicWord::num_data_elems; ++j)
	//	//{
	//	//	printout(std::hex, static_cast<u32>(__mem[i].data[j]),
	//	//		std::dec, " ");
	//	//}

	//	printout("\n");
	//}
}

Simulator::~Simulator()
{
}

int Simulator::run()
{
	return 0;
}

} // namespace snow64_simulator

