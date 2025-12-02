/*
LL(1) 语法分析器头文件
*/


#pragma once
#include<fstream>
#include<string>
#include<iostream>
#include<vector>
#include<unordered_map>
#include<unordered_set>
#include"config.h"

using namespace std;





class Parser
{
private:
	fstream srcFile;//中间文件，存放词法分析结果
	vector<string> symbols;//符号栈，非终结符
	Token currentToken;

	
	// 打印终结符详细错误
	void syntaxErrorDetail(const string& expected, const string& hint = "") {
		cerr << "语法错误: 在(" << currentToken.row << "," << currentToken.column << ")处，期望 "
			<< expected << "，但遇到 '" << currentToken.value << "'(" << tokenTypeName(currentToken.type) << ")";
		if (!hint.empty()) cerr << "。建议: " << hint<<endl;
		exit(1);
	}

	// 终结符匹配，供 match 调用
	bool expectTerminal(const string& name, TokenType t, const string& hint = "") {
		if (currentToken.type == t) {
			symbols.erase(symbols.begin());
			currentToken = getNextToken();
			return true;
		}
		// 报错并尝试恢复：先提示，再跳到下一个同步点继续解析
		syntaxErrorDetail(name, hint);
		return false;
	}



public:
	Parser(const string& srcPath) {
		srcFile.open(srcPath);
		if (!srcFile.is_open()) {
			cerr << "源文件" << srcPath << "打开失败" << endl;
			exit(1);
		}
		symbols.push_back(string("<prog>")); //开始符号
	}
	~Parser() {
		if (srcFile.is_open()) {
			srcFile.close();
		}
	}

	Token getNextToken() {// 从 srcFile 中获取下一个 Token 的逻辑
		std::string line;
		// 读取一行，若读不到则返回 EOF token
		if (!std::getline(srcFile, line)) {
			return Token{ TokenType::EOF_TOKEN, "EOF", 0, 0 };
		}

		// 跳过空行
		while (line.empty() && std::getline(srcFile, line)) {}
		if (line.empty()) {
			return Token{ TokenType::EOF_TOKEN, "EOF", 0, 0 };
		}

		// 期望格式: TYPE(value)(row,column)
		auto p1 = line.find('(');
		if (p1 == std::string::npos) {
			cerr << "无效的 token 行: " << line << endl;
			return Token{ TokenType::ERROR, line, 0, 0 };
		}
		std::string typeStr = line.substr(0, p1);

		auto p2 = line.find(')', p1 + 1);
		if (p2 == std::string::npos) {
			cerr << "无效的 token 行(缺少 )): " << line << endl;
			return Token{ TokenType::ERROR, line, 0, 0 };
		}
		std::string valueStr = line.substr(p1 + 1, p2 - p1 - 1);

		// 查找行列对：在 p2 之后寻找下一个 '(' 和对应的 ')'
		auto p3 = line.find('(', p2 + 1);
		auto p4 = (p3 == std::string::npos) ? std::string::npos : line.find(')', p3 + 1);
		int row = 0, column = 0;
		if (p3 != std::string::npos && p4 != std::string::npos) {
			std::string pos = line.substr(p3 + 1, p4 - p3 - 1);
			auto comma = pos.find(',');
			if (comma != std::string::npos) {
				try {
					row = std::stoi(pos.substr(0, comma));
					column = std::stoi(pos.substr(comma + 1));
				}
				catch (...) {
					row = 0; column = 0;
				}
			}
		}

		Token token;
		auto it = typeMap.find(typeStr);
		if (it != typeMap.end()) {
			token.type = it->second;
		}
		else {
			cerr << "未知Token类型: " << typeStr << " 行: " << line << endl;
			token.type = TokenType::ERROR;
		}

		token.value = valueStr;
		token.row = row;
		token.column = column;
		return token;
	}


	bool match(string symbol) {
		/*
		  - [] ：可选成分（可出现0次或1次）
		 - {} ：重复成分（可出现0次或多次）
		  - |  ：选择关系（多个选项中选一个）
		  - "" ：终结符（关键字、符号、字面量等）
		  - <> ：非终结符（语法单元）
		 */
		if (symbol == "INTEGER") {
			return expectTerminal("整数常量", TokenType::INTEGER, "需要整数常量");
		}
		if (symbol == "ID") {
			return expectTerminal("标识符", TokenType::IDENTIFIER, "需要标识符");
		}
		if (symbol == "END") {
			return expectTerminal("END关键字", TokenType::END, "需要END关键字（程序/块结束标记）");
		}
		if (symbol == "THEN") {
			return expectTerminal("THEN关键字", TokenType::THEN, "IF语句后需要THEN关键字");
		}
		if (symbol == "DO") {
			return expectTerminal("DO关键字", TokenType::DO, "WHILE语句后需要DO关键字");
		}
		if (symbol == "LOP") {
			// LOP → "=" | "<>" | "<" | "<=" | ">" | ">="
			return expectTerminal("关系运算符", TokenType::LOP, "需要关系运算符（=、<>、<、<=、>、>=）");
		}
		if (symbol == "MOP") {
			// MOP → "*" | "/"
			return expectTerminal("乘法/除法运算符", TokenType::MOP, "需要乘法或除法运算符（*、/）");
		}
		if (symbol == "AOP") {
			// AOP → "+" | "-"
			return expectTerminal("加法/减法运算符", TokenType::AOP, "需要加法或减法运算符（+、-）");
		}
		if (symbol == "WRITE") {
			return expectTerminal("WRITE关键字", TokenType::WRITE, "需要WRITE关键字（输出语句标记）");
		}
		if (symbol == "READ") {
			return expectTerminal("READ关键字", TokenType::READ, "需要READ关键字（输入语句标记）");
		}
		if (symbol == "CALL") {
			return expectTerminal("CALL关键字", TokenType::CALL, "需要CALL关键字（过程调用标记）");
		}
		if (symbol == "IF") {
			return expectTerminal("IF关键字", TokenType::IF, "需要IF关键字（条件语句标记）");
		}
		if (symbol == "WHILE") {
			return expectTerminal("WHILE关键字", TokenType::WHILE, "需要WHILE关键字（循环语句标记）");
		}
		if (symbol == "ELSE") {
			return expectTerminal("ELSE关键字", TokenType::ELSE, "IF-THEN语句后需要ELSE关键字（可选分支）");
		}
		if (symbol == "ODD") {
			return expectTerminal("ODD关键字", TokenType::ODD, "需要ODD关键字（奇偶判断运算符）");
		}
		if (symbol == "VAR") {
			return expectTerminal("VAR关键字", TokenType::VAR, "需要VAR关键字（变量声明标记）");
		}
		if (symbol == "CONST") {
			return expectTerminal("CONST关键字", TokenType::CONST, "需要CONST关键字（常量声明标记）");
		}
		if (symbol == "SEMICOLON") {
			return expectTerminal("分号 ';'", TokenType::SEMICOLON, "语句结束需要分号 ';'");
		}
		if (symbol == "PROCEDURE") {
			return expectTerminal("PROCEDURE关键字", TokenType::PROCEDURE, "需要PROCEDURE关键字（过程声明标记）");
		}
		if (symbol == "BEGIN") {
			return expectTerminal("BEGIN关键字", TokenType::BEGIN, "需要BEGIN关键字（程序/块开始标记）");
		}
		if (symbol == "COMMA") {
			return expectTerminal("逗号 ','", TokenType::COMMA, "可能缺少逗号（分隔多个标识符/常量）");
		}
		if (symbol == "LPAREN") {
			return expectTerminal("左括号 '('", TokenType::LPAREN, "可能缺少左括号 '('（表达式/参数列表开始）");
		}
		if (symbol == "RPAREN") {
			return expectTerminal("右括号 ')'", TokenType::RPAREN, "可能缺少右括号 ')'（表达式/参数列表结束）");
		}
		if (symbol == "COLONEQUAL") {
			return expectTerminal("赋值运算符 ':='", TokenType::COLONEQUAL, "赋值语句需要赋值运算符 ':='");
		}


		
		if (symbol == "<prog>") {
			//产生式 <prog> → "program" ID ";" <block>
			if (currentToken.type == TokenType::PROGRAM) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<block>");
				symbols.insert(symbols.begin(), "SEMICOLON");
				symbols.insert(symbols.begin(), "ID");

				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}

		if (symbol == "<block>") {
			//<block> → <condecl_opt> <vardecl_opt> <proc_opt> <body>
			//first:  "const", "var", "procedure", "begin"

			if (currentToken.type == TokenType::CONST ||
				currentToken.type == TokenType::VAR ||
				currentToken.type == TokenType::PROCEDURE ||
				currentToken.type == TokenType::BEGIN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<body>");
				symbols.insert(symbols.begin(), "<proc_opt>");
				symbols.insert(symbols.begin(), "<vardecl_opt>");
				symbols.insert(symbols.begin(), "<condecl_opt>");
				return true;
			}
			else {
				return false;
			}

		}
		if (symbol == "<condecl_opt>") {
			//<condecl_opt> → <condecl> | ε 
			//first: "const", ε
			if (currentToken.type == TokenType::CONST) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<condecl>");
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<condecl>") {
			//<condecl> → "const" < const_list > ";"
			if (currentToken.type == TokenType::CONST) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "SEMICOLON");
				symbols.insert(symbols.begin(), "<const_list>");

				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}

		}
		if (symbol == "<const_list>") {
			//<const_list> → <const> <const_list_tail>
			//first : ID
			if (currentToken.type == TokenType::IDENTIFIER) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<const_list_tail>");
				symbols.insert(symbols.begin(), "<const>");
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<const>") {
			//<const>→ ID ":=" < integer >
			if (currentToken.type == TokenType::IDENTIFIER) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "INTEGER");
				symbols.insert(symbols.begin(), "COLONEQUAL");

				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<const_list_tail>") {
			//<const_list_tail> → "," <const> <const_list_tail> | ε 
			//first: ",", ε
			if (currentToken.type == TokenType::COMMA) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<const_list_tail>");
				symbols.insert(symbols.begin(), "<const>");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}

		if (symbol == "<vardecl_opt>") {
			//<vardecl_opt> → <vardecl> | ε 
			//first ; "var", ε
			if (currentToken.type == TokenType::VAR) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<vardecl>");
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<vardecl>") {
			//<vardecl> → "var" < id_list > ";"
			if (currentToken.type == TokenType::VAR) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "SEMICOLON");
				symbols.insert(symbols.begin(), "<id_list>");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}

		if (symbol == "<proc_opt>") {
			//<proc_opt>  → <proc> | ε 
			//first: "procedure", ε
			if (currentToken.type == TokenType::PROCEDURE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<proc>");
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<proc>") {
			//<proc> → "procedure" ID <param_list_opt> ";" <block> <proc_tail> 
			if (currentToken.type == TokenType::PROCEDURE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<proc_tail>");
				symbols.insert(symbols.begin(), "<block>");
				symbols.insert(symbols.begin(), "SEMICOLON");
				symbols.insert(symbols.begin(), "<param_list_opt>");
				symbols.insert(symbols.begin(), "ID");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<param_list_opt>") {
			//<param_list_opt> → "(" < id_list_opt > ")"
			if (currentToken.type == TokenType::LPAREN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "RPAREN");
				symbols.insert(symbols.begin(), "<id_list_opt>");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<id_list_opt>") {
			//<id_list_opt> → <id_list> | ε
			//first: ID, ε
			if (currentToken.type == TokenType::IDENTIFIER) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<id_list>");
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<proc_tail>") {
			//<proc_tail> → ";" < proc > | ε
			//first: ";", ε
			if (currentToken.type == TokenType::SEMICOLON) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<proc>");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		
		if (symbol == "<body>") {
			//<body>→ "begin" <statement_list> "end" 
			if (currentToken.type == TokenType::BEGIN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "END");
				symbols.insert(symbols.begin(), "<statement_list>");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<statement_list>") {
			//<statement_list> → <statement> <statement_tail>
			//first: ID, "if", "while", "call", "begin", "read", "write"
			if (currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::IF ||
				currentToken.type == TokenType::WHILE ||
				currentToken.type == TokenType::CALL ||
				currentToken.type == TokenType::BEGIN ||
				currentToken.type == TokenType::READ ||
				currentToken.type == TokenType::WRITE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<statement_tail>");
				symbols.insert(symbols.begin(), "<statement>");
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<statement_tail>") {
			//<statement_tail> → ";" <statement> <statement_tail> | ε 
			//first: ";", ε
			if (currentToken.type == TokenType::SEMICOLON) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<statement_tail>");
				symbols.insert(symbols.begin(), "<statement>");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}

		if (symbol == "<statement>") {
			/*<statement>   →  ID ":=" <exp>  // 赋值语句
			| "if" < lexp > "then" < statement > <else_opt>       // 条件语句
			| <while_stmt>     // 循环语句（当型）
			| <call_stmt>      // 过程调用语句
			| <body>           // 嵌套复合语句
			| <read_stmt>      // 输入语句
			| <write_stmt>     // 输出语句
			*/
			//first: ID, "if", "while", "call", "begin", "read", "write"
			if (currentToken.type == TokenType::IDENTIFIER) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp>");
				symbols.insert(symbols.begin(), "COLONEQUAL");

				currentToken = getNextToken();
				return true;
			}
			else if (currentToken.type == TokenType::IF) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<else_opt>");
				symbols.insert(symbols.begin(), "<statement>");
				symbols.insert(symbols.begin(), "THEN");
				symbols.insert(symbols.begin(), "<lexp>");

				currentToken = getNextToken();
				return true;
			}
			else if (currentToken.type == TokenType::WHILE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<while_stmt>");
				return true;
			}
			else if (currentToken.type == TokenType::CALL) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<call_stmt>");
				return true;
			}
			else if (currentToken.type == TokenType::BEGIN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<body>");
				return true;
			}
			else if (currentToken.type == TokenType::READ) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<read_stmt>");
				return true;
			}
			else if (currentToken.type == TokenType::WRITE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<write_stmt>");
				return true;
			}
			else {
				return false;
			}

		}
		if (symbol == "<else_opt>") {
			//<else_opt> → "else" < statement > | ε 
			//first: "else", ε
			if (currentToken.type == TokenType::ELSE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<statement>");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}

		if (symbol == "<while_stmt>") {
			//<while_stmt>  → "while" <lexp> "do" <statement>
			if (currentToken.type == TokenType::WHILE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<statement>");
				symbols.insert(symbols.begin(), "DO");
				symbols.insert(symbols.begin(), "<lexp>");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}

		}
		
		if (symbol == "<lexp>") {
			//<lexp>→ <odd_lexp> | <cmp_lexp>  
			//first: "odd" , ID, <integer>, "(", "+", "-"
			if (currentToken.type == TokenType::ODD) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<odd_lexp>");
				return true;
			}
			else if (currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::INTEGER ||
				currentToken.type == TokenType::LPAREN ||
				currentToken.type == TokenType::AOP) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<cmp_lexp>");
				return true;
			}
			else {
				return false;
			}
		}

		if (symbol == "<odd_lexp>") {
			//<odd_lexp> → "odd" < exp >  
			if (currentToken.type == TokenType::ODD) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp>");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<exp>"){
			//<exp>→ <sign_opt> <term> <exp_tail>
			//first: AOP, ID, <integer>, "("
			if (currentToken.type == TokenType::AOP ||
				currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::INTEGER ||
				currentToken.type == TokenType::LPAREN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp_tail>");
				symbols.insert(symbols.begin(), "<term>");
				symbols.insert(symbols.begin(), "<sign_opt>");
				return true;
			}
			else {
				return false;
			}
				

		}
		if (symbol == "<sign_opt>") {
			//<sign_opt> → AOP | ε 
			//first: AOP, ε
			if (currentToken.type == TokenType::AOP) {
				symbols.erase(symbols.begin());
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<term>") {
			//<term> → <factor> <term_tail>
			//first: ID, <integer>, "("
			if (currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::INTEGER ||
				currentToken.type == TokenType::LPAREN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<term_tail>");
				symbols.insert(symbols.begin(), "<factor>");
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<factor>") {
			//<factor> → ID | <integer> | "(" <exp> ")"
			//first: ID, <integer>, "("
			if (currentToken.type == TokenType::IDENTIFIER) {
				symbols.erase(symbols.begin());
				currentToken = getNextToken();
				return true;
			}
			else if (currentToken.type == TokenType::INTEGER) {
				symbols.erase(symbols.begin());
				currentToken = getNextToken();
				return true;
			}
			else if (currentToken.type == TokenType::LPAREN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "RPAREN");
				symbols.insert(symbols.begin(), "<exp>");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}

		if (symbol ==  "<exp_tail>") {
			//<exp_tail> → AOP <term> <exp_tail> | ε 
			//first: AOP, ε
			if (currentToken.type == TokenType::AOP) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp_tail>");
				symbols.insert(symbols.begin(), "<term>");

				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<term_tail>") {
			//<term_tail> → MOP <factor> <term_tail> | ε   
			//first: MOP, ε
			if (currentToken.type == TokenType::MOP) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<term_tail>");
				symbols.insert(symbols.begin(), "<factor>");

				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}

		if (symbol == "<cmp_lexp>") {
			//<cmp_lexp> → <exp> LOP <exp>
			//first: ID, <integer>, "(", AOP
			if (currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::INTEGER ||
				currentToken.type == TokenType::LPAREN ||
				currentToken.type == TokenType::AOP) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp>");
				symbols.insert(symbols.begin(), "LOP");
				symbols.insert(symbols.begin(), "<exp>");
				return true;
			}
			else {
				return false;
			}
		}
		

		if (symbol == "<call_stmt>") {
			//<call_stmt> → "call" ID <arg_list_opt>
			if (currentToken.type == TokenType::CALL) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<arg_list_opt>");
				symbols.insert(symbols.begin(), "ID");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<arg_list_opt>") {
			//<arg_list_opt> → "(" <exp_list_opt> ")" | ε
			if (currentToken.type == TokenType::LPAREN) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "RPAREN");
				symbols.insert(symbols.begin(), "<exp_list_opt>");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<exp_list_opt>") {
			//<exp_list_opt> → <exp_list> | ε
			//first: ID, <integer>, "(", AOP, ε
			if (currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::INTEGER ||
				currentToken.type == TokenType::LPAREN ||
				currentToken.type == TokenType::AOP) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp_list>");
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}
		if (symbol == "<exp_list>") {
			//<exp_list> → <exp> <exp_list_tail>
			//first: ID, <integer>, "(", AOP
			if (currentToken.type == TokenType::IDENTIFIER ||
				currentToken.type == TokenType::INTEGER ||
				currentToken.type == TokenType::LPAREN ||
				currentToken.type == TokenType::AOP) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp_list_tail>");
				symbols.insert(symbols.begin(), "<exp>");
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<exp_list_tail>") {
			//<exp_list_tail> → "," <exp> <exp_list_tail> | ε
			//first: ",", ε
			if (currentToken.type == TokenType::COMMA) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<exp_list_tail>");
				symbols.insert(symbols.begin(), "<exp>");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}


		if (symbol ==  "<read_stmt>") {
			//<read_stmt>   → "read" "(" < id_list > ")"
			if (currentToken.type == TokenType::READ) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "RPAREN");
				symbols.insert(symbols.begin(), "<id_list>");
				symbols.insert(symbols.begin(), "LPAREN");

				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<id_list>") {
			//<id_list> → ID <id_list_tail>
			if (currentToken.type == TokenType::IDENTIFIER) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<id_list_tail>");
				symbols.insert(symbols.begin(), "ID");
				return true;
			}
			else {
				return false;
			}
		}
		if (symbol == "<id_list_tail>") {
			//<id_list_tail> → "," ID <id_list_tail> | ε
			if (currentToken.type == TokenType::COMMA) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "<id_list_tail>");
				symbols.insert(symbols.begin(), "ID");
				currentToken = getNextToken();
				return true;
			}
			else {
				//ε产生式
				symbols.erase(symbols.begin());
				return true;
			}
		}


		if (symbol == "<write_stmt>") {
			//<write_stmt>  → "write" "(" < exp_list > ")"
			if (currentToken.type == TokenType::WRITE) {
				symbols.erase(symbols.begin());
				symbols.insert(symbols.begin(), "RPAREN");
				symbols.insert(symbols.begin(), "<exp_list>");
				symbols.insert(symbols.begin(), "LPAREN");
				currentToken = getNextToken();
				return true;
			}
			else {
				return false;
			}
		}

		else {
			cerr << "未知符号: " << symbol << endl;
			return false;
		}
	}

	void parse() {
		cout << "\n开始语法分析... " << endl;

		currentToken = getNextToken();

		while (!symbols.empty()) {
			string symbol = symbols.front();
			//cout << symbol << endl;
			if(!match(symbol)){
				FirstSet fs;
				cerr << "\n语法错误\t也许你期望："<<symbol;
				fs.printFirstSet(symbol);
				cerr << "\t 在(" << currentToken.row << ","
					<< currentToken.column << ")处，遇到无效符号 '" << currentToken.value << "'" << endl;
				exit(1);
			}
		}

		cout << "\n语法分析成功，源程序符合语法规则！" << endl;
		
	}

};

