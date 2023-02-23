#include "Dependencies.h"

#include "Viewer.h"

int main()
{
	string file_name;
	string listing_name;
	cout << "Enter source code file name. Enter name, and then add .ASM, or .asm " << endl;
	cin >> file_name;
	cout << "Enter listing file name, do not add .lst or .LST, program will do it automatically" << endl;
	cin >> listing_name;

	do_inits();
	first_view(file_name);
	second_view(file_name, listing_name);
}