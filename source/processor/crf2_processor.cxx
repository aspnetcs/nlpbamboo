#include "lexicon_factory.hxx"
#include "crf2_processor.hxx"
#include "utf8.hxx"
#include <cassert>
#include <cstdio>
#include <stdexcept>

CRF2Processor::CRF2Processor(IConfig *config) {
	const char *model;

	config->get_value("crf_ending_tags", _ending_tags);
	config->get_value("crf2_model", model);
	_tagger = CRFPP::createTagger(model);
	_token = new char[8];
}

CRF2Processor::~CRF2Processor()
{
	delete []_token;
	delete _tagger;
}

const char *CRF2Processor::_get_crf2_tag(int attr) {
	switch(attr) {
	case LexToken::attr_number:
	case LexToken::attr_alpha:
		return "ASCII";
	case LexToken::attr_punct:
		return "PUNC";
	case LexToken::attr_cword:
		return "CN";
	default:
		return "CN";
	}
}

void CRF2Processor::process(std::vector<LexToken *> &in, std::vector<LexToken *> &out) {
	size_t i, j, length;
	std::vector<LexToken *> atom_tok;

	if (in.empty()) return;
	length = in.size();
	atom_tok.reserve(length);

	for (i = 0; i < length; i++) {
		if(atom_tok.size() == atom_tok.capacity())
			atom_tok.reserve(atom_tok.size()+length);

		LexToken *cur_token = in[i];
		int tok_attr = cur_token->get_attr();
		const wchar_t *s = cur_token->get_token();
		size_t len = cur_token->get_length();

		if(tok_attr==LexToken::attr_alpha
		|| tok_attr==LexToken::attr_number
		|| tok_attr==LexToken::attr_punct) {
			atom_tok.push_back(cur_token);
			continue;
		}

		wchar_t tmp_token[2];
		for(j = 0; j < len; ++j) {
			wcsncpy(tmp_token, s+j, 1);
			if(iswspace(*tmp_token)) continue;
			
			LexToken *t = new LexToken(tmp_token, LexToken::attr_cword);
			atom_tok.push_back(t);
		}
		delete in[i];

		/*
		 * if punct_cnt > threshold
		 * _crf2_tagger(atom_tok, out);
		 * atom_tok.clear();
		 */
	}
	_crf2_tagger(atom_tok, out);
}

void CRF2Processor::_crf2_tagger(std::vector<LexToken *> &in, std::vector<LexToken *> &out) {
	size_t i, size = in.size();

	_tagger->clear();

	for (i = 0; i < size; ++i) {
		LexToken *cur_tok = in[i];
		size_t tok_size = cur_tok->get_length();
		size_t max_tok_buf_size = tok_size*4;
		size_t max_buf_size = max_tok_buf_size+12;

		char *buf = new char[max_buf_size+1];
		char *tokbuf = new char[max_tok_buf_size+1];
		wcstombs(tokbuf, cur_tok->get_token(), max_tok_buf_size);
		snprintf(buf, max_buf_size, "%s %s", tok_buf, _get_crf2_tag(cur_tok->get_attr()));
		_tagger->add(buf);
		delete [] buf;
		delete [] tokbuf;
	}
	
	if (!_tagger->parse()) throw std::runtime_error("crf parse failed!");

	_result.clear();
	for (i = 0; i < _tagger->size(); ++i) {
		_result.append(_tagger->x(i, 0));
		const char * tag = _tagger->y2(i);
		int attr = in[i]->get_attr();
		if(attr==LexToken::attr_alpha || attr==LexToken::attr_number || attr==LexToken::attr_punct)	tag = "S";
		if (strstr(_ending_tags, _tagger->y2(i))) {
			wchar_t wcsbuf = new wchar_t[_result.size()+1];
			mbstowcs(wcsbuf, _result.c_str(), _result.size());
			out.push_back(new LexToken(wcsbuf, LexToken::attr_cword));
			_result.clear();
			delete [] wcsbuf;
		}
		delete in[i];
	}
}
