/*
 * QASM_scanner.hpp
 *
 *  Created on: Jun 15, 2018
 *      Author: zulehner
 */

#ifndef QASM_SCANNER_HPP_
#define QASM_SCANNER_HPP_

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <istream>
#include <map>
#include <wctype.h>
#include <ctype.h>
#include <sstream>
#include <stack>

#include <QASM_token.hpp>

class QASM_scanner {


public:
    QASM_scanner(std::istream& in_stream);
    Token next();
    void addFileInput(std::string fname);

private:
  	std::istream& in;
  	std::stack<std::istream*> streams;
  	char ch;
  	std::map<std::string, Token::Kind> keywords;
  	int line;
    int col;
    void nextCh();
    void readName(Token& t);
    void readNumber(Token& t);
    void readString(Token& t);
    void skipComment();

    class LineInfo {
    public:
    	char ch;
    	int line, col;

    	LineInfo(char ch, int line, int col) {
    		this->ch = ch;
    		this->line = line;
    		this->col = col;
    	}
    };

  	std::stack<LineInfo> lines;

};

#endif /* QASM_SCANNER_HPP_ */
