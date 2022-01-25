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
	int lno = 0, pos = 0;


	// --- Helpers ---
	// info
	int eol(int off=0)    const { return pos + off >= tokens.size(); }
	int eof()             const { return lno >= lines.size(); }
	string peekline()     const { return lno < lines.size() ? lines[lno] : "<EOF>"; }
	string currenttoken() const { return eol() ? "<EOL>" : tokens.at(pos); }
	// mutators
	int nextline() { return lno++, tokenizeline(); }


	// --- Load ---
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


	// --- Do work ---
	runtime_error error(const string& err, const string& val="") {
		string s = err
			+ (val.length() ? " [" + val + "]" : "")
			+ " . line " + to_string(lno + 1);
		return runtime_error(s);
	}

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


	// --- Token access ---
	int peek(const string& tok, int off=0) {
		if (!eol(off) && tokens[pos + off] == tok)
			return lasttok = tok, 1;
		return 0;
	}
	int expect(const string& tok) {
		return peek(tok) ? ++pos, 1 : 0;
	}
	int require(const string& tok) {
		if (!expect(tok))  throw error("expected token", tok);
		return 1;
	}
	int peekp(const string& pat, int off=0) {
		string tok = eof() ? "<EOF>" : eol(off) ? "<EOL>" : tokens[pos + off];
		int res = 0;
		printf("here  %s \n", tok.c_str() );
		if      (pat == "eof")         res = eof();
		else if (pat == "eol")         res = eol(off);
		else if (pat == "endl")        res = eof() || eol(off) || is_comment(tok);
		else if (pat == "integer")     res = is_integer(tok);
		else if (pat == "identifier")  res = is_identifier(tok);
		else if (pat == "literal")     res = is_literal(tok);
		else  throw error("unknown pattern", pat);
		return lasttok = tok, res;
	}
	int expectp(const string& pat) {
		return peekp(pat) ? ++pos, 1 : 0;
	}
	int requirep(const string& pat) {
		if (!expectp(pat))  throw error("expected pattern", pat);
		return 1;
	}


	// --- Useful functions ---
	int is_identifier(const string& s) {
		if (s.length() == 0)  return 0;
		for (int i = 0; i < s.length(); i++)
			if      (i == 0 && !isalpha(s[i]) && s[i] != '_')  return 0;
			else if (i  > 0 && !isalnum(s[i]) && s[i] != '_')  return 0;
		return 1;
	}
	int is_integer(const string& s) {
		if (s.length() == 0)  return 0;
		for (int i = 0; i < s.length(); i++)
			if (!isdigit(s[i]))  return 0;
		return 1;
	}
	int is_literal(const string& s) {
		return s.size() >= 2 && s[0] == '"' && s.back() == '"';
	}
	int is_comment(const string& s) {
		return s.size() >= 1 && s[0] == '#';
	}
};