#pragma once
#include<fstream>
#include<iostream>
#include<vector>
#include<string>
#include<fstream>
// 包含反向迭代器所需头文件（部分编译器需显式包含）
#include<iterator>
#include"SymbolTable.h"

using namespace std;

fstream File;

bool key = true;//用于调试
vector<int> write_result;//写入结果存放

/*
F,L,A 三段式指令
F段伪操作码
L段定义层
A段位移量（相对地址）
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

//栈式活动记录,采用嵌套层次显示表display
class Activation {
	/*activation record 结构
	*0 :动态链接DL
	 1 ：返回地址RA
	 2 :全局display：指示直接外层活动记录的display位置
	 ID个数：参数+变量个数
	 形参区 name+value
	 变量区 name+value
	 局部display
	 临时单元
	*/
	/*display 结构
	从现行层到最外层依次存放各活动记录基地址
	*/
public:
	string name = "";//当前过程名
	int layer = 0;//当前层
	int define_layer = 0;//定义层
	int top = 0;//栈顶指针
	int base = 0;//栈底指针
	vector<string> stack;//数据栈
	Activation() {}

	void init(SymbolTable& symTable) {
		stack.clear();
		top = 0;
		base = 0;
		layer = 0;
		define_layer = 0;
		symTable.current_layer_ = symTable.first_layer_;
		SymLayer* cur_layer = symTable.current_layer_;
		int id_num = cur_layer->var_offset_;
		int param_num = cur_layer->param_count_;
		push("0");//动态链接DL
		push("0");//返回地址RA
		push("0");//全局display
		push(to_string(id_num));//Id个数

		//插入形参
		Symbol* sym = cur_layer->sym_head_;
		while (sym != nullptr) {
			if (sym->getType() == SYMBOLTYPE::PARAM) {
				push(sym->getName()+":0");
			}
			sym = sym->getNext();
		}

		//插入变量
		sym = cur_layer->sym_head_;
		while (sym != nullptr) {
			if (sym->getType() == SYMBOLTYPE::VAR) {
				push(sym->getName()+":0");
			}
			sym = sym->getNext();
		}
		push("0");//局部display
	}

	void newSapce(int count) {
		for (int i = 0; i < count; i++) {
			stack.push_back("0");
		}
	}
	void deleteSpace(int count ,int place = -1) {
		if (place == -1) {
			for (int i = 0; i < count; i++) {
				stack.pop_back();
			}
		}
		else {
			for (int i = 0; i < count; i++) {
				stack.erase(stack.begin() + place);
			}
		}
	}

	string get(int offset) {
		string val = stack.at(base + offset);
		return val;
	}
	void set(int offset, const string& val) {
		stack.at(base + offset) = val;
	}

	void push(const string& val) {
		if (top >= stack.size()) {
			newSapce(1);
		}
		stack.at(top) = val;
		top++;
	}
	string pop() {
		if (top > 0) {
			string val = stack.at(top - 1);
			top--;
			return val;
		}
		cerr << "栈空，无法弹出" << endl;
	}

	string* getId(int L, int A) {
		int a1 = base + 4 + stoi(get(3)); 
		int baseL = stoi(stack.at(a1 +  L));
		return & stack[baseL + A];
	}
	int getIdVal(int L, int A) {
		//通过display获取变量值
		int a1 = base + 4 + stoi(get(3));
		int baseL = stoi(stack.at(a1 +  L));
		string s = stack.at(baseL + A);
		int pos = s.find(":");
		string val_str = s.substr(pos + 1);
		return stoi(val_str);
	}

	void newAc(SymLayer* symlayer) {
		int newbase = top;

		Symbol* proc_sym = symlayer->outer_->findInLayer(symlayer->getLayerName());
		int id_num = symlayer->var_offset_; 
		int param_num = symlayer->param_count_;
		name = symlayer->getLayerName();
		define_layer = symlayer->getLevel();
		File << "\nnewAc:" << name  << endl;
		push(to_string(base));//动态链接DL
		push("0");//返回地址RA

		int global_display_pos = base + 4 + stoi(get(3));
		push(to_string(global_display_pos));//全局display
		push(to_string(id_num));//Id个数
		//插入形参
		Symbol* sym = symlayer->sym_head_;
		while (sym != nullptr) {
			if (sym->getType() == SYMBOLTYPE::PARAM) {
				push(sym->getName() + ":0");
			}
			sym = sym->getNext();
		}
		//插入变量
		sym = symlayer->sym_head_;
		while (sym != nullptr) {
			if (sym->getType() == SYMBOLTYPE::VAR) {
				push(sym->getName() + ":0");
			}
			sym = sym->getNext();
		}

		//插入局部display
		int proc_level = symlayer->getLevel();
		for (int i = 0; i <= proc_level - 1; i++) {
			push(stack.at(global_display_pos+i)); 
			
		}
		push(to_string(newbase));//当前层display
	
		base = newbase;
		layer++;
		if (key) {
			cout << "\n进入新活动记录，当前层级：" << layer << endl;
			cout << "base=" << base << ",top=" << top << endl;
		}
	}
	void returnAc() {
		//恢复上一个活动记录
		int return_base = stoi(get(0));//动态链接DL
		int return_RA = stoi(get(1));//返回地址RA
		//删除当前活动记录
		top = base;
		base = return_base;
		layer--;
		//返回地址暂不处理，由解释器负责PC的修改
		if (key) {
			cout << "\n返回上一个活动记录，当前层级：" << layer << endl;
		}
		File << "\nback " << layer << endl;
	}

	void printStack() {
		//从栈顶到底打印
		cout << "\n当前活动记录栈内容：" << endl;
		for(int i = top - 1; i >= 0; i--) {
			cout << "[" << i << "]: " << stack.at(i) << endl;
			File << "[" << i << "]: " << stack.at(i) << endl;
		}
	}
};

class Pcode
{
private:
	vector<int> jumpStack; // 跳转地址栈（辅助回填）

public:
	int PC = 0; // 程序计数器（记录指令总数）
	vector<label> labels; // 标签表
	vector<Ins> code;     // Pcode代码存储区

	void emit(string op,int L,int A,int count){//插入前count个
		Ins instruction;
		instruction.op = op;
		instruction.L = L;
		instruction.A = A;
		if (count < 0 || count > code.size()) {
			cerr << "插入位置越界: " << PC-count << endl;
			return;
		}
		code.insert(code.begin() + (PC - count), instruction);
		PC++; // 程序计数器自增
	}
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
				code[it->place].A = A;
				//删除标签
				labels.erase((it + 1).base()); // 反向迭代器转换为正向迭代器
				
				return;
			}
		}
		// 未找到标签时给出提示
		cerr << "未找到标签: " << id << "，回填失败" << endl;
	}

	void printCode() {
		if (key) {
			cout << "\n生成的Pcode代码如下：\n" << endl;
			for (int i = 0; i < code.size(); i++) {
				cout << i << ": " << code[i].op << " " << code[i].L << " " << code[i].A << endl;
			}
		}
		for (int i = 0; i < code.size(); i++) {
			File<< i << ": " << code[i].op << " " << code[i].L << " " << code[i].A << endl;
		}
	}
	Ins& getInstruction(int index) { // 改用Ins&（私有结构体在类内部可正常返回）
		if (index < 0 || index >= code.size()) {
			throw runtime_error("指令索引越界: " + to_string(index));
		}
		return code[index];
	}

	// 解释执行Pcode（简易虚拟机）
	void interpret(SymbolTable& symTable) {
		int pc = 0;
		vector<int> returnStack; // 返回地址栈
		Activation Ac; // 活动记录（栈式）
		Ac.init(symTable); // 初始化活动记录栈
		vector<vector<int>> args; // 新活动记录的参数存储

		File.open("pcode_output.txt", ios::out);
		if(!File.is_open()) {
			cerr << "无法打开输出文件" << endl;
			return;
		}

		while (1) {
			Ins instr = getInstruction(pc);
			pc++;
			string op = instr.op;
			if(key)cout<<pc-1<<": " << op << " " << instr.L << " " << instr.A << endl;
			File << pc - 1 << ": " << op << " " << instr.L << " " << instr.A << endl;
			if(op == "LIT") {// 常量入栈
				int value = instr.A;
				Ac.push(to_string(value));
			}
			else if (op == "LOD") {// 变量/参数入栈
				
				int value = Ac.getIdVal(instr.L, instr.A + 4);
				Ac.push(to_string(value));
			}
			else if (op == "STO") {// 栈顶值存入变量/参数
				int val = stoi(Ac.pop());
				

				if (instr.L == -1) {// 新活动的变量存储
					vector<int> arg;
					arg.push_back(instr.L);
					arg.push_back(instr.A+4);
					arg.push_back(val);
					
					args.insert(args.begin(), arg);
				}
				else {
					string* s = Ac.getId(instr.L, instr.A + 4);
					
					int pos = s->find(":");
					string name = s->substr(0, pos);
					string val_str = s->substr(pos + 1);
					val_str = to_string(val);
					*s = name + string(":") + val_str;
				
				}
			}
			else if (op == "CAL") {// 过程调用
				returnStack.push_back(pc);
				pc = instr.A;

				SymLayer* symlayer = symTable.findProcByEntry(pc);
				
				// 初始化新活动记录
				Ac.newAc(symlayer);
				
				// 传递参数
				while (!args.empty()) {
					vector<int> arg = args.back();
					args.pop_back();
					int L = arg[0];
					L = Ac.define_layer;
						
					int offset = arg[1];
					int val = arg[2];
					string* s = Ac.getId(L, offset);
					int pos = s->find(":");
					string name = s->substr(0, pos);
					string val_str = s->substr(pos + 1);
					val_str = to_string(val);
					*s = name + ":" + val_str;
						

					
				}

			}
			else if (op == "INT") {// 开辟数据区
				Ac.newSapce(instr.A);

			}
			else if (op == "JMP") {// 无条件跳转
				pc = instr.A;

			}
			else if (op == "JPC") {// 条件跳转
				int cond = stoi(Ac.pop());
				if (cond == 0) {
					pc = instr.A;
				}

			}
			else if (op == "OPR") {// 运算/操作
				switch (instr.A)
				{
					case 0:// 过程返回
					{
						if (returnStack.empty()) {
							for (int i : write_result) {
								cout << "输出: " << i << endl;
								//File << "输出: " << i << endl;
							}
							cout << "程序结束" << endl;
							return;
						}
						pc = returnStack.back();
						returnStack.pop_back();

						Ac.returnAc();
						break;
					}
					case 1:// 取负
					{
						int val = stoi(Ac.pop());
						Ac.push(to_string(-val));
						break;
					}
					case 2:// 加法
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a + b));
						break;
					}
					case 3:// 减法
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a - b));
						break;
					}
					case 4:// 乘法
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a * b));
						break;
					}
					case 5:// 除法
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						if (b == 0) {
							cerr << "运行时错误：除以零" << endl;
							return;
						}
						Ac.push(to_string(a / b));
						break;
					}
					case 6:// 奇偶判断
					{
						int a = stoi(Ac.pop());
						Ac.push(to_string(a % 2));
						break;
					}
					case 7:// 相等
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a == b ? 1 : 0));
						break;
					}
					case 8:// 不等
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a != b ? 1 : 0));
						break;
					}
					case 9:// 小于
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a < b ? 1 : 0));
						break;
					}
					case 10:// 小于等于
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a <= b ? 1 : 0));
						break;
					}
					case 11:// 大于
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a > b ? 1 : 0));
						break;
					}
					case 12:// 大于等于
					{
						int b = stoi(Ac.pop());
						int a = stoi(Ac.pop());
						Ac.push(to_string(a >= b ? 1 : 0));
						break;
					}
				default:
					break;
				}
			}
			else if (op == "RED") {// 读入值入栈
				string input;
				cin >> input;
				Ac.push(input);


			}
			else if (op == "WRT") {// 栈顶值输出
				write_result.push_back(stoi(Ac.pop()));
			}

			else {
				cerr << "未知操作码: " << op << endl;
				break;
			}
			Ac.printStack();
		}

	}




};

