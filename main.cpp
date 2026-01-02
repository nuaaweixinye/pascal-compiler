#include<iostream>
#include<vector>
#include<fstream>
#include"tokenization.h"
#include"Parser.h"

using namespace std;

int main(int argc,char* argv[])
{
	if (argc == 3) {
		tokenizationer Plexer(argv[1], argv[2]);
		Plexer.tokenize();
		Parser paser(argv[2]);
		paser.parse();

	}
	else {
		tokenizationer Plexer("pascal.txt", "out.txt");
		Plexer.tokenize();
		Parser paser("out.txt");
		paser.parse();
	}

	return 0;
}