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

#include <QASM_token.hpp>

class QASM_scanner {


public:
    QASM_scanner(std::istream& in_stream);
    Token next();

private:
  	std::istream& in;
  	char ch;
  	std::map<std::string, Token::Kind> keywords;
  	int line;
    int col;
    void nextCh();
    void readName(Token& t);
    void readNumber(Token& t);
};

#endif /* QASM_SCANNER_HPP_ */
