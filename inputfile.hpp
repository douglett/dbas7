// ----------------------------------------
// Input File handler
// ----------------------------------------
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>


struct InputFile {
	// typedef  InputPattern::Results  Results;
	vector<string> lines, tokens;
	string lasttok;
	vector<string> lastrule;
	int lno = 0, pos = 0;

	// state info
	int eol(int off=0)    const { return pos + off >= tokens.size(); }
	int eof()             const { return lno >= lines.size(); }
	string peekline()     const { return lno < lines.size() ? lines[lno] : "<EOF>"; }
	string currenttoken() const { return eol() ? "<EOL>" : tokens.at(pos); }

	// mutators
	int nextline() { return lno++, tokenizeline(); }

	// errors
	runtime_error error(const string& err, const string& val="") {
		return runtime_error(
			err + (val.length() ? " [" + val + "]" : "") + " . line " + to_string(lno + 1) );
	}

	// loading
	int load(const string& fname) {
		// reset
		lines = tokens = {};
		lno = pos = 0;
		// load
		fstream fs(fname, ios::in);
		if (!fs.is_open())
			return fprintf(stderr, "error loading file %s\n", fname.c_str()), 1;
		string s;
		while (getline(fs, s))
			lines.push_back(s);
		printf("loaded file: %s (%d)\n", fname.c_str(), (int)lines.size());
		tokenizeline();
		return 0;
	}
	int loadstring(const string& program) {
		// reset
		lines = tokens = {};
		lno = pos = 0;
		// load
		stringstream ss(program);
		string s;
		while (getline(ss, s))
			lines.push_back(s);
		printf("loaded program string.\n");
		tokenizeline();
		return 0;
	}

	// main parsing function
	int tokenizeline() {
		// reset & check
		tokens = {};
		pos = 0;
		if (lno < 0 || lno >= lines.size())  return 0;
		// split
		auto& line = lines[lno];
		string s;
		for (int i = 0; i < line.length(); i++) {
			char c = line[i];
			if (isalnum(c) || c == '_')  s += c;  // id / number
			else {
				if (s.length())  tokens.push_back(s), s = "";  // next token
				if      (isspace(c)) { }  // whitespace (ignore)
				else if (c == '#')   { tokens.push_back(line.substr(i));  break; }  // line comment
				else if (c == '"')   {    // string literal
					s += c;
					for (i++; i < line.length(); i++) {
						c = line[i];
						if (c == '\\') {  // escape character
							if    (i < line.length()-1 && line[i+1] == '"')  s += c,  i++;
							else  throw error("bad string escape sequence");
						}
						else {            // normal character
							s += c;
							if (c == '"')  break;
						}
					}
					if (s.length() < 2 || s.back() != '"')
						throw error("unterminated string");
					tokens.push_back(s),  s = "";
				}
				else                 { tokens.push_back(string() + c); }  // punctuation
			}
		}
		if (s.length())  tokens.push_back(s);
		return 1;
	}

	// token access :: (simplified version of dbas6)
	int peekt(const string& tok, int off=0) {
		if (!eol(off) && tokens[pos + off] == tok)
			return lasttok = tok, 1;
		return 0;
	}
	int expectt(const string& tok) {
		return peekt(tok) ? ++pos, 1 : 0;
	}
	int requiret(const string& tok) {
		if (!expectt(tok))  throw error("expected token", tok);
		return 1;
	}
	int peekp(const string& pat, int off=0) {
		string tok = eof() ? "<EOF>" : eol(off) ? "<EOL>" : tokens[pos + off];
		int res = 0;
		// printf("rulecheck  %s \n", tok.c_str() );
		if      (pat == "eof")         res = eof();
		else if (pat == "eol")         res = eol(off);
		else if (pat == "endl")        res = eof() || eol(off) || Tokens::is_comment(tok);
		else if (pat == "integer")     res = Tokens::is_integer(tok);
		else if (pat == "identifier")  res = Tokens::is_identifier(tok);
		else if (pat == "literal")     res = Tokens::is_literal(tok);
		else  throw runtime_error("unknown pattern: " + pat);
		return lasttok = tok, res;
	}
	int expectp(const string& pat) {
		return peekp(pat) ? ++pos, 1 : 0;
	}
	int requirep(const string& pat) {
		if (!expectp(pat))  throw error("expected pattern", pat);
		return 1;
	}

	// full ruleset matching :: (much reduced from dbas 4 -> 6)
	int peek(const string& ruleset) {
		int off = 0;
		vector<string> vs;
		for (const auto& rule : Strings::split(ruleset))
			if      (rule.at(0) == '@' && peekp(rule.substr(1), off))  off++, vs.push_back(lasttok);
			else if (rule.at(0) != '@' && peekt(rule, off))  off++;
			else    return 0;
		lastrule = vs;
		return off;
	}
	int expect(const string& ruleset) {
		int matchc = peek(ruleset);
		return matchc ? pos += matchc, matchc : 0;
	}
	int require(const string& ruleset) {
		if (!expect(ruleset))  throw error("syntax error near", currenttoken());
		return 1;
	}
};