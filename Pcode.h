#pragma once
#include<fstream>
#include<iostream>
#include<vector>
using namespace std;

/*
F,L,A 三段式指令
F段代表伪操作码
L段代表调用层与说明层的层差值
A段代表位移量（相对地址）
*/


enum class op {
	LIT,  // 常量入栈（Load Literal）
	LOD,  // 变量/参数入栈（Load）
	STO,  // 栈顶值存入变量/参数（Store）
	CAL,  // 过程调用（Call）
	INT,  // 开辟数据区（Allocate Integer）
	JMP,  // 无条件跳转（Jump）
	JPC,  // 条件跳转（Jump on Condition）
	OPR,  // 运算/操作（Operation）
	RED,  // 读入值入栈（Read）
	WRT,  // 栈顶值输出（Write）
	NOP   // 空指令（可选，用于占位）
};




class Pcode
{
private:
	typedef	struct Instruction {
		string op="";//操作码
		int L=0;
		int A=0;
	}Ins;



public:
	int PC = 0;//程序计数器
	int Label = 0;//标号计数器
	vector<Ins> code;//Pcode代码存储区

	void emit(string op, int L, int A) {
		Ins instruction;
		instruction.op = op;
		instruction.L = L;
		instruction.A = A;
		code.push_back(instruction);
		PC++;
	}


	int newLabel() {
		return Label++;
	}

	void backPatch(int offset, int A) {
		if (offset < 0 || offset >= PC) {
			cerr << "回填地址越界: " << offset << endl;
			return;
		}
		code[offset].A = A;
	}

	void printCode() {
		cout << "\n生成的Pcode代码如下：\n" << endl;
		for (int i = 0; i < code.size(); i++) {
			cout << i << ": " << code[i].op << " " << code[i].L << " " << code[i].A << endl;
		}
	}



};
