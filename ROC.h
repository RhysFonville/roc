#pragma once

#include <string>
#include <fstream>
#include "MachineSpecificCodeGenerator.h"

class ROC {
public:
	void run(const std::string& line);
	void run(const std::ifstream& file);

private:
	std::shared_ptr<MachineSpecificCodeGenerator> get_appropriate_code_generator(const std::vector<IRCommand>& commands);
};

