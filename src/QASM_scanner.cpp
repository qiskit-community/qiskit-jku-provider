/*
 * qasm_scanner.cpp
 *
 *  Created on: Jun 15, 2018
 *      Author: zulehner
 */

/**
 * The Scanner for the MicroJava Compiler.
 */

#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <istream>
#include <map>
#include <wctype.h>
#include <ctype.h>
#include <sstream>

#include <QASM_scanner.hpp>

QASM_scanner::QASM_scanner(std::istream& in_stream) : in(in_stream) {
        // initialize error handling support
        keywords["qreg"] = Token::Kind::qreg;
        keywords["creg"] = Token::Kind::creg;
        keywords["gate"] = Token::Kind::gate;
        keywords["measure"] = Token::Kind::measure;
        keywords["U"] = Token::Kind::ugate;
        keywords["CX"] = Token::Kind::cxgate;
        keywords["pi"] = Token::Kind::pi;
        keywords["OPENQASM"] = Token::Kind::openqasm;
        keywords["show_probabilities"] = Token::Kind::probabilities;
        keywords["measure_all"] = Token::Kind::measureall;
        line = 1;
        col = 0;
        ch = 0;
        nextCh();
}

void QASM_scanner::nextCh() {
    if(!in.eof()) {
    	col++;
        in.get(ch);
    } else {
    	ch = (char) -1;
    }
    if(ch == '\n') {
    	col = 0;
    	line++;
    }
}

Token QASM_scanner::next() {
	while(iswspace(ch)) {
		nextCh();
    }

	Token t = Token(Token::Kind::none, line, col);

	switch(ch) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
            readName(t);break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            readNumber(t);break;
        case ';': t.kind = Token::Kind::semicolon; nextCh(); break;
        case (char) -1: t.kind = Token::Kind::eof; break;
        case '(': t.kind = Token::Kind::lpar; nextCh(); break;
        case ')': t.kind = Token::Kind::rpar; nextCh(); break;
        case '[': t.kind = Token::Kind::lbrack; nextCh(); break;
        case ']': t.kind = Token::Kind::rbrack; nextCh(); break;
        case '{': t.kind = Token::Kind::lbrace; nextCh(); break;
        case '}': t.kind = Token::Kind::rbrace; nextCh(); break;
        case ',': t.kind = Token::Kind::comma; nextCh(); break;
        case '+': nextCh(); t.kind = Token::Kind::plus; break;
        case '-': nextCh(); t.kind = Token::Kind::minus; break;
        case '*': nextCh(); t.kind = Token::Kind::times; break;
        default:
            std::cerr << "ERROR: UNEXPECTED CHARACTER: '" << ch << "'! " << std::endl;
            nextCh();
        }

        return t;
    }

void QASM_scanner::readName(Token& t) {
    	std::stringstream ss;
        while(isdigit(ch) || ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_') {
            ss << ch;
            nextCh();
        }
        t.str = ss.str();
        std::map<std::string, Token::Kind>::iterator it = keywords.find(t.str);
        if(it != keywords.end()) {
            t.kind = it->second;
        } else {
            t.kind = Token::Kind::identifier;
        }
    }

void QASM_scanner::readNumber(Token& t) {
        std::stringstream ss;
        while(isdigit(ch)) {
            ss << ch;
            nextCh();
        }
        t.kind = Token::Kind::nninteger;
        t.str = ss.str();
        if(ch != '.') {
        	ss >> t.val;
        	return;
        }
        t.kind = Token::Kind::real;
        ss << ch;
        nextCh();
        while(isdigit(ch)) {
        	ss << ch;
        	nextCh();
        }
        if(ch != 'e' && ch != 'E') {
        	ss >> t.val_real;
        	return;
        }
        ss << ch;
        nextCh();
        if(ch == '-' || ch == '+') {
        	ss << ch;
        	nextCh();
        }
        while(isdigit(ch)) {
        	ss << ch;
        	nextCh();
        }
        ss >> t.val_real;
    }



