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
		tokenizationer Plexer("pascal.txt", "outTokens.txt");
		Plexer.tokenize();
		Parser paser("out.txt");
		paser.parse();
	}

	//
	cout << "\n\n解释执行pcode..." << endl;
	pcode.interpret(symTable, "pcode.txt");
	cout << "\n\n活动记录栈可在pcode_output.txt文件中查看； 或者运行main.py程序展示动画过程" << endl;
	return 0;
}