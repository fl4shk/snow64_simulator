#include "simulator_class.hpp"

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printerr("Usage:  ", argv[0], " data_filename"
			" mem_amount_in_bytes\n");
		exit(1);
	}

	snow64_simulator::Simulator sim(std::string(argv[1]),
		convert_str_to<size_t>(argv[2]));

	return sim.run();
}
