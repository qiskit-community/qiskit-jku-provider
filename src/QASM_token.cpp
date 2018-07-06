/*
 * QASM_token.cpp
 *
 *  Created on: Jul 5, 2018
 *      Author: zulehner
 */


#include <QASM_token.hpp>

std::map<Token::Kind, std::string> Token::KindNames = {
			{Kind::none,"none"},
			{Kind::include,"include"},
			{Kind::identifier,"identifier"},
			{Kind::number,"number"},
			{Kind::plus,"plus"},
			{Kind::semicolon,"semicolon"},
			{Kind::eof,"eof"},
			{Kind::lpar,"lpar"},
			{Kind::rpar,"rpar"},
			{Kind::lbrack,"lbrack"},
			{Kind::rbrack,"rbrack"},
			{Kind::lbrace,"lbrace"},
			{Kind::rbrace,"rbrace"},
			{Kind::comma,"comma"},
			{Kind::minus,"minus"},
			{Kind::times,"times"},
			{Kind::nninteger,"nninteger"},
			{Kind::real,"real"},
			{Kind::qreg,"qreg"},
			{Kind::creg,"creg"},
			{Kind::ugate,"ugate"},
			{Kind::cxgate,"cxgate"},
			{Kind::gate,"gate"},
			{Kind::pi,"pi"},
			{Kind::measure,"measure"},
			{Kind::openqasm,"openqasm"},
			{Kind::probabilities,"probabilities"},
			{Kind::measureall,"measureall"},
			{Kind::sin,"sin"},
			{Kind::cos,"cos"},
			{Kind::tan,"tan"},
			{Kind::exp,"exp"},
			{Kind::ln,"ln"},
			{Kind::sqrt,"sqrt"},
			{Kind::div,"div"},
			{Kind::power,"power"},
			{Kind::string,"string"},
			{Kind::gt,"gt"}
	};
