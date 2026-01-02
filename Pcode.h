#pragma once
#include<fstream>
#include<iostream>
#include<vector>
#include<string>
// 包含反向迭代器所需头文件（部分编译器需显式包含）
#include<iterator>

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

struct label {
	string id;
	int place; // 标签对应的指令地址
};

// 嵌套结构体：Pcode指令结构
typedef struct Instruction {
	string op = ""; // 操作码
	int L = 0;      // 层差值
	int A = 0;      // 位移量（相对地址）
}Ins;

class Pcode
{
private:
	vector<int> jumpStack; // 跳转地址栈（辅助回填）

public:
	int PC = 0; // 程序计数器（记录指令总数）
	vector<label> labels; // 标签表
	vector<Ins> code;     // Pcode代码存储区


	void emit(string op, int L, int A) {
		Ins instruction;
		instruction.op = op;
		instruction.L = L;
		instruction.A = A;
		code.push_back(instruction);
		PC++; // 程序计数器自增
	}

	void addJump() {
		jumpStack.push_back(PC);
	}
	void fillJump(int A) {
		if (jumpStack.empty()) {
			cerr << "跳转地址栈为空，无法回填" << endl;
			return;
		}
		int addr = jumpStack.back();
		jumpStack.pop_back();
		if (addr < 0 || addr >= code.size()) {
			cerr << "回填地址越界: " << addr << endl;
			return;
		}
		code[addr].A = A;
	}


	int newLabel(string id,int place) {
		label l;
		l.id = id;
		l.place = place;
		labels.push_back(l);
		// 返回该标签在标签表中的索引（最后一个元素的索引）
		return labels.size() - 1;
	}

	// 根据指令偏移量回填A值（修正地址越界判断）
	void backPatch(int offset, int A) {
		if (offset < 0 || offset >= code.size()) { 
			cerr << "回填地址越界: " << offset << endl;
			return;
		}
		code[offset].A = A;
	}

	// 根据标签id回填地址（从后向前查找第一个匹配的标签）
	void backPatch(string id, int A) {
		vector<label>::reverse_iterator it;
		for (it = labels.rbegin(); it != labels.rend(); ++it) {
			if (it->id == id) {
				// 找到匹配标签，回填其对应的指令地址
				it->place = A;
				// 仅回填第一个（从后往前的第一个，即最后一个插入的）匹配标签，直接返回
				return;
			}
		}
		// 未找到标签时给出提示
		cerr << "未找到标签: " << id << "，回填失败" << endl;
	}

	// 打印所有生成的Pcode指令
	void printCode() {
		cout << "\n生成的Pcode代码如下：\n" << endl;
		for (int i = 0; i < code.size(); i++) {
			cout << i << ": " << code[i].op << " " << code[i].L << " " << code[i].A << endl;
		}
	}

	// 获取指定索引的指令（返回引用，支持修改）
	Ins& getInstruction(int index) { // 改用Ins&（私有结构体在类内部可正常返回）
		if (index < 0 || index >= code.size()) {
			throw runtime_error("指令索引越界: " + to_string(index));
		}
		return code[index];
	}

};