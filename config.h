#pragma once
#include<string>
#include<unordered_map>
#include<unordered_set>
using namespace std;
// 单词类型枚举,终结符
enum class TokenType {
	// 关键字（共15个，严格对应 BNF 中的保留字）
	PROGRAM, CONST, VAR, PROCEDURE, CALL,
	BEGIN, END, IF, THEN, ELSE,
	WHILE, DO, ODD, READ, WRITE,

	// 标识符和常量（带值的 Token）
	IDENTIFIER,  // 如 sum、max（<id>）
	INTEGER,     // 如 100、50（<integer>）

	// 运算符（3类，对应 BNF 的 <aop>、<mop>、<lop>）
	AOP,  // 加法运算符：+、-
	MOP,  // 乘法运算符：*、/
	LOP,  // 关系运算符：=、<>、<、<=、>、>=

	// 界符（6个，对应 BNF 中的分隔符号）
	SEMICOLON,  // ; （语句/说明结束）
	COMMA,      // , （多个标识符/常量分隔）
	LPAREN,     // ( （参数/表达式开头）
	RPAREN,     // ) （参数/表达式结尾）
	COLONEQUAL, // := （赋值符号，<statement> 中的赋值语句）

	// 特殊标记
	EOF_TOKEN,  // 文件结束标记
	ERROR       // 无效字符错误
};

struct Token {
	TokenType type;
	string value;
	int row, column;
	// 保留默认构造函数以便无参初始化
	Token() : type(TokenType::ERROR), value(""), row(0), column(0) {}
	// 与 tokenization.h 中一致的带参构造函数
	Token(TokenType type, const string& value, int row, int column)
		: type(type), value(value), row(row), column(column) {
	}

};


//类型映射表：键为字符串，值为对应 TokenType（快速匹配）
unordered_map<string, TokenType> typeMap = {
	{"PROGRAM" , TokenType::PROGRAM},
	{"CONST" , TokenType::CONST},
	{"VAR" , TokenType::VAR},
	{"PROCEDURE" , TokenType::PROCEDURE},
	{"CALL" , TokenType::CALL},
	{"BEGIN" , TokenType::BEGIN},
	{"END" , TokenType::END},
	{"IF" , TokenType::IF},
	{"THEN" , TokenType::THEN},
	{"ELSE" , TokenType::ELSE},
	{"WHILE" , TokenType::WHILE},
	{"DO" , TokenType::DO},
	{"ODD" , TokenType::ODD},
	{"READ" , TokenType::READ},
	{"WRITE" , TokenType::WRITE},
	{"AOP" , TokenType::AOP},
	{"MOP" , TokenType::MOP},
	{"LOP" , TokenType::LOP},
	{"IDENTIFIER" , TokenType::IDENTIFIER},
	{"INTEGER" , TokenType::INTEGER},
	{"SEMICOLON" , TokenType::SEMICOLON},
	{"COMMA" , TokenType::COMMA},
	{"LPAREN" , TokenType::LPAREN},
	{"RPAREN" , TokenType::RPAREN},
	{"COLONEQUAL" , TokenType::COLONEQUAL},
	{"EOF" , TokenType::EOF_TOKEN}

};

// 将 TokenType 转为可读名字（用于错误消息）
string tokenTypeName(TokenType t) {
	switch (t) {
	case TokenType::PROGRAM: return "PROGRAM";
	case TokenType::IDENTIFIER: return "IDENTIFIER";
	case TokenType::INTEGER: return "INTEGER";
	case TokenType::SEMICOLON: return "';'";
	case TokenType::COMMA: return "','";
	case TokenType::LPAREN: return "'('";
	case TokenType::RPAREN: return "')'";
	case TokenType::COLONEQUAL: return "':='";
	case TokenType::BEGIN: return "BEGIN";
	case TokenType::END: return "END";
	case TokenType::IF: return "IF";
	case TokenType::THEN: return "THEN";
	case TokenType::ELSE: return "ELSE";
	case TokenType::WHILE: return "WHILE";
	case TokenType::CALL: return "CALL";
	case TokenType::READ: return "READ";
	case TokenType::WRITE: return "WRITE";
	case TokenType::AOP: return "加/减号(+/-)";
	case TokenType::MOP: return "乘/除号(*//)";
	case TokenType::LOP: return "关系运算符(=, <>, <, <=, >, >=)";
	case TokenType::EOF_TOKEN: return "EOF";
	default: return "";
	}
}

// 定义空串常量表示 EPSILON（空产生式）
const string EPSILON = "";

class FirstSet {
private:
	// 正确初始化 firstSets 映射（使用统一的初始化列表语法）
	unordered_map<std::string, std::unordered_set<std::string>> firstSets = {
		// 程序起始符号
		{"<prog>", {"PROGRAM"}},

		// 代码块
		{"<block>", {"CONST", "VAR", "PROCEDURE", "BEGIN"}},

		// 常量声明相关
		{"<condecl_opt>", {"CONST", EPSILON}},
		{"<condecl>", {"CONST"}},
		{"<const_list>", {"ID"}},
		{"<const>", {"ID"}},
		{"<const_list_tail>", {"COMMA", EPSILON}},

		// 变量声明相关
		{"<vardecl_opt>", {"VAR", EPSILON}},
		{"<vardecl>", {"VAR"}},

		// 过程声明相关
		{"<proc_opt>", {"PROCEDURE", EPSILON}},
		{"<proc>", {"PROCEDURE"}},
		{"<param_list_opt>", {"LPAREN", EPSILON}},  // 可选参数列表
		{"<id_list_opt>", {"ID", EPSILON}},
		{"<proc_tail>", {"SEMICOLON", EPSILON}},

		// 程序体
		{"<body>", {"BEGIN"}},

		// 语句相关
		{"<statement_list>", {"ID", "IF", "WHILE", "CALL", "BEGIN", "READ", "WRITE"}},
		{"<statement_tail>", {"SEMICOLON", EPSILON}},
		{"<statement>", {"ID", "IF", "WHILE", "CALL", "BEGIN", "READ", "WRITE"}},
		{"<else_opt>", {"ELSE", EPSILON}},

		// 循环语句
		{"<while_stmt>", {"WHILE"}},

		// 逻辑表达式
		{"<lexp>", {"ODD", "ID", "INTEGER", "LPAREN", "AOP"}},
		{"<odd_lexp>", {"ODD"}},
		{"<cmp_lexp>", {"ID", "INTEGER", "LPAREN", "AOP"}},

		// 算术表达式
		{"<exp>", {"AOP", "ID", "INTEGER", "LPAREN"}},
		{"<sign_opt>", {"AOP", EPSILON}},
		{"<term>", {"ID", "INTEGER", "LPAREN"}},
		{"<factor>", {"ID", "INTEGER", "LPAREN"}},
		{"<exp_tail>", {"AOP", EPSILON}},
		{"<term_tail>", {"MOP", EPSILON}},

		// 过程调用语句
		{"<call_stmt>", {"CALL"}},
		{"<arg_list_opt>", {"LPAREN", EPSILON}},

		// 表达式列表（参数/输出用）
		{"<exp_list_opt>", {"ID", "INTEGER", "LPAREN", "AOP", EPSILON}},
		{"<exp_list>", {"ID", "INTEGER", "LPAREN", "AOP"}},
		{"<exp_list_tail>", {"COMMA", EPSILON}},

		// 输入输出语句
		{"<read_stmt>", {"READ"}},
		{"<id_list>", {"ID"}},
		{"<id_list_tail>", {"COMMA", EPSILON}},
		{"<write_stmt>", {"WRITE"}}
	};

public:
	// 可选：提供获取 First 集的接口（根据需要添加）
	unordered_set<std::string> getFirstSet(const std::string& nonTerminal) const {
		auto iter = firstSets.find(nonTerminal);
		if (iter != firstSets.end()) {
			return iter->second;
		}
		// 若未找到，返回空集（或抛出异常，根据需求调整）
		return {};
	}
	//输出对应非终结符的First集
	void printFirstSet(const string& nonTerminal) {
		auto iter = firstSets.find(nonTerminal);
		if (iter != firstSets.end()) {
			cout << "{ ";
			for (const auto& symbol : iter->second) {
				cout << symbol << " ";
			}
			cout << "}";
		}
		else {
			cout << "Non-terminal " << nonTerminal << " not found in First sets." << endl;
		}
	}
};


