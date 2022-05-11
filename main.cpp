#include "Controller.h"

//extern const DWORD g_BYTES_PER_ATTRIBUTE_RESIDENT_DATA;
//
//extern const DWORD g_STANDARD_INFORMATION_ATTRIBUTE_TYPECODE;
//extern const DWORD g_FILE_NAME_ATTRIBUTE_TYPECODE;
//extern const DWORD g_DATA_ATTRIBUTE_TYPECODE;

/*
format of argv[1]
D:\
D:\bla\bla\
D:\bla\bla\bla.txt
*/

/*
format of argv[2]
D:\
D:\bla\bla
.\
.\bla
bla
*/
/*
keys:
	-mkrp - создать путь для востаноления, если он отсутствует
	-rd - восстанавливать структуру папок
*/

int main(int argc, char* argv[]) {
	system("chcp 1251 & cls");
	Controller controller(argc, argv);
	if (controller.isHelp()) return 0;
	if (!controller.parseCommandLine()) return -1;
	if (!controller.start()) return -2;

	return 0;
}

