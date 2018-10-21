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

			std::vector<std::string> mem_words_str_vec;
			for (size_t i=0; i<BasicWord::num_data_elems; ++i)
			{
				mem_words_str_vec.push_back(std::string());
			}

			size_t j=0;
			for (s64 i=BasicWord::num_data_elems-1; i>=0; --i)
			{
				mem_words_str_vec.at(i) = line.substr(j, sizeof(u32) * 2);
				j += (sizeof(u32) * 2);
			}

			//for (size_t i=0; i<mem_words_str_vec.size(); ++i)
			//{
			//	printout(mem_words_str_vec.at(i), "\n");
			//}


			for (size_t i=0; i<mem_words_str_vec.size(); ++i)
			{
				u32 instr_or_data = 0;
				const auto& curr_str = mem_words_str_vec.at(i);
				for (j=0; j<curr_str.size(); ++j)
				{
					instr_or_data <<= 4;
					if ((curr_str.at(j) >= '0') && (curr_str.at(j) <= '9'))
					{
						instr_or_data |= (curr_str.at(j) - '0');
					}
					else if ((curr_str.at(j) >= 'a')
						&& (curr_str.at(j) <= 'f'))
					{
						instr_or_data |= (curr_str.at(j) - 'a' + 0xa);
					}
					else
					{
						err(sconcat("Invalid line of initial mem ",
							"contents."));
					}
				}

				__mem[word_addr].data[i] = instr_or_data;
			}


			++word_addr;
		}

	}

	}

	//for (size_t i=0; i<mem_amount_in_words(); ++i)
	//{
	//	for (size_t j=0; j<BasicWord::num_data_elems; ++j)
	//	{
	//		printout(std::hex, __mem[i].data[j], std::dec, " ");
	//	}
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

