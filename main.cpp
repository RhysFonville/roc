#include "ROC.h"

int main() {
	ROC roc{};

	roc.run(std::ifstream{"code"});
	/*std::string line{};
	while (true) {
		std::cout << "> ";
		std::getline(std::cin, line);
		roc.run(line);
	}*/
	
	return 0;
}
