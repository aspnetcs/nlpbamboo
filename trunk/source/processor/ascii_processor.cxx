#include <ctype.h>
#include <iostream>

#include "utf8.hxx"
#include "lex_token.hxx"
#include "ascii_processor.hxx"

AsciiProcessor::AsciiProcessor(IConfig *config)
{
	config->get_value("chinese_number_end", _chinese_number_end);
	config->get_value("chinese_punctuation", _chinese_punctuation);
	config->get_value("chinese_number", _chinese_number);
}

void AsciiProcessor::_process(LexToken *token, std::vector<LexToken *> &out)
{
	const char *s = token->get_token();
	char first[8], *subtoken, *subtoken_ptr;
	size_t len;
	enum {
		state_unknow = 0,
		state_alpha,
		state_number,
		state_punctuation,
		state_end
	} state, last;

	if (*s == '\0') return;
	subtoken = new char [strlen(s) + 1];
	subtoken[0] = '\0';
	subtoken_ptr = subtoken;
	state = last = state_unknow;
	while (true) {
		len = utf8::first(s, first);
		if (isalpha(*first)) state = state_alpha;
		else if (isdigit(*first)) state = state_number;
		/* use utf8::strstr to replace */
		else if (utf8::strstr(_chinese_punctuation, first) || ispunct(*first)) state = state_punctuation;
		else if (*first == '.' && last == state_number) state = state_number;
		else if (utf8::strstr(_chinese_number, first)) state = state_number;
		else if (utf8::strstr(_chinese_number_end, first) && last == state_number) state = state_number;
		else if (*first == '\0') state = state_end;
		else state = state_unknow;
		if (last != state && subtoken[0]) {
			LexToken::attr_t attr = LexToken::attr_unknow;
			if (last == state_alpha) attr = LexToken::attr_alpha;
			else if (last == state_number) attr = LexToken::attr_number;
			*subtoken_ptr = '\0';
			out.push_back(new LexToken(subtoken, attr));
			subtoken_ptr = subtoken;
			subtoken[0] = '\0';
			if (state == state_end) break;
		}
		memcpy(subtoken_ptr, first, len);
		subtoken_ptr += len;
		s += len;
		last = state;
	}
	delete []subtoken;
}
