#pragma once

#include <string>
#include <fstream>

class ROC {
public:
	void run(const std::string& line);
	void run(const std::ifstream& file);
};

