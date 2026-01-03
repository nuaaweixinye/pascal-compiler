
/*
词法分析器（Tokenizer/Lexical Analyzer）
*/

#pragma once
#include <fstream>
#include <string>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <limits>
#include <cctype>
#include "config.h"
using namespace std;

class tokenizationer
{
private:
	ifstream srcFile;
	ofstream outFile;
	int currentChar; // 使用 int 以正确表示 EOF
	int row, column;

	// 关键字映射表：键为小写关键字，值为对应 TokenType（快速匹配）
	unordered_map<string, TokenType> keywordMap = {
		{"program", TokenType::PROGRAM},
		{"const", TokenType::CONST},
		{"var", TokenType::VAR},
		{"procedure", TokenType::PROCEDURE},
		{"call", TokenType::CALL},
		{"begin", TokenType::BEGIN},
		{"end", TokenType::END},
		{"if", TokenType::IF},
		{"then", TokenType::THEN},
		{"else", TokenType::ELSE},
		{"while", TokenType::WHILE},
		{"do", TokenType::DO},
		{"odd", TokenType::ODD},
		{"read", TokenType::READ},
		{"write", TokenType::WRITE}
	};

public:
	tokenizationer(const string& srcPath, const string& outPath) : currentChar(0), row(1), column(0)
	{
		srcFile.open(srcPath);
		outFile.open(outPath);

		if (!srcFile.is_open()) {
			cerr << "源文件" << srcPath << "打开失败" << endl;
			exit(1);
		}
		if (!outFile.is_open()) {
			cerr << "输出文件" << outPath << "打开失败" << endl;
			exit(1);
		}

		nextchar();
	}
	~tokenizationer() {
		if (srcFile.is_open()) {
			srcFile.close();
		}
		if (outFile.is_open()) {
			outFile.close();
		}
	}

	void nextchar() {
		if (srcFile.eof()) {
			currentChar = EOF;
			return;
		}

		int c = srcFile.get();
		if (c == EOF) {
			currentChar = EOF;
			return;
		}
		currentChar = c;

		// 更新位置： column 表示 currentChar 的列（从1开始）
		if (currentChar == '\n') {
			row++;
			column = 0; // 刚读到换行，下一次 nextchar() 会把 column 设为 1
		}
		else {
			column++;
		}
	}

	void skipspace() {
		while (currentChar != EOF && std::isspace(static_cast<unsigned char>(currentChar))) {
			nextchar();
		}
	}

	Token gettoken() {
		skipspace();
		if (currentChar == EOF) {
			return Token(TokenType::EOF_TOKEN, "EOF", row, column);
		}

		string token = "";
		if (std::isalpha(static_cast<unsigned char>(currentChar))) { // 关键字或标识符
			// 记录起始位置
			int startrow = row;
			int startcolumn = column;

			while (currentChar != EOF && std::isalnum(static_cast<unsigned char>(currentChar))) {
				token.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(currentChar))));
				nextchar();
			}

			auto it = keywordMap.find(token);
			if (it != keywordMap.end()) { // 关键字
				return Token(it->second, token, startrow, startcolumn);
			}
			else { // 标识符
				return Token(TokenType::IDENTIFIER, token, startrow, startcolumn);
			}
		}
		else if (std::isdigit(static_cast<unsigned char>(currentChar))) { // 数字
			int startrow = row;
			int startcolumn = column;

			while (currentChar != EOF && !std::isspace(static_cast<unsigned char>(currentChar))) {
				if (std::isalpha(static_cast<unsigned char>(currentChar))) {
					// 包含字母，错误（例如 12a）
					token.push_back(static_cast<char>(currentChar));
					nextchar();
					return Token(TokenType::ERROR, token, startrow, startcolumn);
				}
				else if (std::isdigit(static_cast<unsigned char>(currentChar))) {
					token.push_back(static_cast<char>(currentChar));
					nextchar();
				}
				else {
					break;
				}
			}
			return Token(TokenType::INTEGER, token, startrow, startcolumn);
		}
		else {
			// 记录起始位置
			int startrow = row;
			int startcolumn = column;
			string lexeme = "";

			switch (currentChar) {
				// 界符. , ; (  ) 
			case ',':
				lexeme = ",";
				nextchar();
				return Token(TokenType::COMMA, lexeme, startrow, startcolumn);
			case ';':
				lexeme = ";";
				nextchar();
				return Token(TokenType::SEMICOLON, lexeme, startrow, startcolumn);
			case '(':
				lexeme = "(";
				nextchar();
				return Token(TokenType::LPAREN, lexeme, startrow, startcolumn);
			case ')':
				lexeme = ")";
				nextchar();
				return Token(TokenType::RPAREN, lexeme, startrow, startcolumn);

				// 运算符 aop
			case '+':
			case '-':
				lexeme = static_cast<char>(currentChar);
				nextchar();
				return Token(TokenType::AOP, lexeme, startrow, startcolumn);

				// mop
			case '*':
			case '/':
				lexeme = static_cast<char>(currentChar);
				nextchar();
				return Token(TokenType::MOP, lexeme, startrow, startcolumn);

				// 关系运算符 lop  =、<>、<、<=、>、>=
			case '=':
				lexeme = "=";
				nextchar();
				return Token(TokenType::LOP, lexeme, startrow, startcolumn);
			case '<':
				lexeme = "<";
				nextchar();
				if (currentChar == '=') {
					lexeme.push_back('=');
					nextchar();
				}
				else if (currentChar == '>') {
					lexeme.push_back('>');
					nextchar();
				}
				return Token(TokenType::LOP, lexeme, startrow, startcolumn);
			case '>':
				lexeme = ">";
				nextchar();
				if (currentChar == '=') {
					lexeme.push_back('=');
					nextchar();
				}
				return Token(TokenType::LOP, lexeme, startrow, startcolumn);

				// 赋值界符 :=
			case ':':
				lexeme = ":";
				nextchar();
				if (currentChar == '=') {
					lexeme.push_back('=');
					nextchar();
					return Token(TokenType::COLONEQUAL, lexeme, startrow, startcolumn);
				}
				else {
					return Token(TokenType::ERROR, lexeme, startrow, startcolumn);
				}

			default:
				lexeme = static_cast<char>(currentChar);
				nextchar();
				return Token(TokenType::ERROR, lexeme, startrow, startcolumn);
			}
		}
	}

	void printtoken(Token token) {
		// 输出统一格式：TYPE(value)(row,column) 
		// 先输出 TYPE（大写枚举名对应字符串）
		switch (token.type)
		{
		case TokenType::PROGRAM:    outFile << "PROGRAM"; break;
		case TokenType::CONST:      outFile << "CONST"; break;
		case TokenType::VAR:        outFile << "VAR"; break;
		case TokenType::PROCEDURE:  outFile << "PROCEDURE"; break;
		case TokenType::CALL:       outFile << "CALL"; break;
		case TokenType::BEGIN:      outFile << "BEGIN"; break;
		case TokenType::END:        outFile << "END"; break;
		case TokenType::IF:         outFile << "IF"; break;
		case TokenType::THEN:       outFile << "THEN"; break;
		case TokenType::ELSE:       outFile << "ELSE"; break;
		case TokenType::WHILE:      outFile << "WHILE"; break;
		case TokenType::DO:         outFile << "DO"; break;
		case TokenType::ODD:        outFile << "ODD"; break;
		case TokenType::READ:       outFile << "READ"; break;
		case TokenType::WRITE:      outFile << "WRITE"; break;
		case TokenType::IDENTIFIER: outFile << "IDENTIFIER"; break;
		case TokenType::INTEGER:    outFile << "INTEGER"; break;
		case TokenType::AOP:        outFile << "AOP"; break;
		case TokenType::MOP:        outFile << "MOP"; break;
		case TokenType::LOP:        outFile << "LOP"; break;
		case TokenType::SEMICOLON:  outFile << "SEMICOLON"; break;
		case TokenType::COMMA:      outFile << "COMMA"; break;
		case TokenType::LPAREN:     outFile << "LPAREN"; break;
		case TokenType::RPAREN:     outFile << "RPAREN"; break;
		case TokenType::COLONEQUAL: outFile << "COLONEQUAL"; break;
		case TokenType::EOF_TOKEN:  outFile << "EOF"; break;
		case TokenType::ERROR:      outFile << "ERROR"; break;
		default:
			outFile << "UNKNOWN";
			break;
		}

		// 然后输出 (value)
		outFile << "(" << token.value << ")";

		// 最后输出 (row,column)
		outFile << "(" << token.row << "," << token.column << ")" << endl;
	}

	void tokenize() {

		Token token = gettoken();
		cout << "开始词法分析... " << endl;
		while (token.type != TokenType::EOF_TOKEN) {
			if (token.type == TokenType::ERROR) {
				cerr << "ERROR(无效字符: '" << token.value << "')"
					<< "(" << token.row << "," << token.column << ")" << endl;
				exit(1);
			}
			printtoken(token);
			token = gettoken();
		}
		cout << "\n词法分析完成！Token 结果已保存到中间文件。" << endl;


	}

	// 从当前输入流连续读取 token 到 tokens（解析器可多次调用，流位置会继续）
	// 约定：count > 0 => 读取最多 count 个；count <= 0 => 读取直到 EOF
	void getTokens(int count, vector<Token>& tokens) {//获取count个token,存到tokens里
		tokens.clear();

		bool toEOF = (count <= 0);
		int remaining = count > 0 ? count : numeric_limits<int>::max();

		if (count > 0) tokens.reserve(count);

		int read = 0;
		while (read < remaining) {
			Token t = gettoken();
			tokens.push_back(t);
			read++;

			if (t.type == TokenType::ERROR) {
				// 打印错误，交由调用方决定后续处理（不在此 exit）
				cerr << "ERROR(无效字符: '" << t.value << "')"
					<< "(" << t.row << "," << t.column << ")" << endl;
				break;
			}
			if (t.type == TokenType::EOF_TOKEN) {
				break;
			}
			if (!toEOF && read >= remaining) {
				break;
			}
		}
	}
};