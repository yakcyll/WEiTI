/****************************************************************************/
/*									ASFS									*/
/*				A file system with a completely unrelated name				*/
/*																			*/
/*							  Main console file								*/
/*						   Author: Marcin Dzie¿yc							*/
/****************************************************************************/

#include <iostream>
#include <string>
#include <stdlib.h>

#include "asfs.h"

using namespace std;

int main (int argc, char* argv[]) {

	long int result = 0;

	if (argc > 1)
		result = strtol(argv[1],NULL,10);
	else {
		cout << "Specify the size of a new file system in bytes: "; cin >> result;
	}

	if (result > 0) {
		cout << "Do you want to create a new file system of " << result << " bytes? (y/n): ";
		if (getchar() != 'y') 
			return 0;
	}
	else return 0;


	
}