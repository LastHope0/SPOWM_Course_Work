#include "Controller.h"

int main(int argc, char* argv[]) {
	system("chcp 1251 & cls");
	try {
		Controller controller(argc, argv);
		if (controller.isHelp()) return 0;
		if (!controller.parseCommandLine()) return -1;
		if (!controller.start()) return -2;
	}
	catch (std::bad_alloc ba)
	{
		std::cout << ba.what() << std::endl;
		return -3;
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
		return -3;
	}
	return 0;
}

