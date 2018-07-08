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
			{Kind::identifier,"<identifier>"},
			{Kind::number,"<number>"},
			{Kind::plus,"+"},
			{Kind::semicolon,";"},
			{Kind::eof,"EOF"},
			{Kind::lpar,"("},
			{Kind::rpar,")"},
			{Kind::lbrack,"["},
			{Kind::rbrack,"]"},
			{Kind::lbrace,"{"},
			{Kind::rbrace,"}"},
			{Kind::comma,","},
			{Kind::minus,"-"},
			{Kind::times,"*"},
			{Kind::nninteger,"<nninteger>"},
			{Kind::real,"<real>"},
			{Kind::qreg,"qreg"},
			{Kind::creg,"creg"},
			{Kind::ugate,"U"},
			{Kind::cxgate,"CX"},
			{Kind::gate,"gate"},
			{Kind::pi,"pi"},
			{Kind::measure,"measure"},
			{Kind::openqasm,"openqasm"},
			{Kind::probabilities,"probabilities"},
			{Kind::opaque,"opaque"},
			{Kind::sin,"sin"},
			{Kind::cos,"cos"},
			{Kind::tan,"tan"},
			{Kind::exp,"exp"},
			{Kind::ln,"ln"},
			{Kind::sqrt,"sqrt"},
			{Kind::div,"/"},
			{Kind::power,"^"},
			{Kind::string,"string"},
			{Kind::gt,">"},
			{Kind::barrier,"barrier"},
			{Kind::_if,"if"},
			{Kind::eq,"=="},
			{Kind::reset,"reset"}
	};
