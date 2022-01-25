#pragma once
#include <vector>
#include <string>
using namespace std;


struct Prog {
	struct Dim  { string name, type; };
	struct Type { string name; vector<Dim> members; };
	// struct Func { string name; map<string, Dim> args, locals; };
	struct Statement  { string type; int32_t loc; };
	struct Block      { vector<Statement> statements; };
	struct Let        { int32_t varpath, expr; };
	struct VarPath    { string type; vector<string> instr; };
	struct Expr       { string type; vector<string> instr; };

	vector<Prog::Type>        types;
	vector<Prog::Dim>         globals;
	vector<Prog::Let>         lets;
	vector<Prog::VarPath>     varpaths;
	vector<Prog::Expr>        exprs;
	vector<Prog::Block>       blocks;
};


namespace Tokens {
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
	// int is_arraytype(const string& s) {
	// 	return s.size() >= 3 && s[s.length()-2] == '[' && s[s.length()-1] == ']';
	// }
	// string basetype(const string& s) {
	// 	return is_arraytype(s) ? s.substr(0, s.length()-2) : s;
	// }
};


namespace Strings {
	vector<string> split(const string& str) {
		vector<string> vs;
		string s;
		for (auto c : str)
			if   (isspace(c)) { if (s.length())  vs.push_back(s), s = ""; }
			else              { s += c; }
		if (s.length())  vs.push_back(s);
		return vs;
	}
	// string join(const vector<string>& vs, const string& glue=" ") {
	// 	string str;
	// 	for (int i = 0; i < vs.size(); i++)
	// 		str += (i > 0 ? glue : "") + vs[i];
	// 	return str;
	// }
	// string chomp(const string& s) {
	// 	int i = 0, j = s.length() - 1;
	// 	for ( ; i < s.length(); i++)
	// 		if (!isspace(s[i]))  break;
	// 	for ( ; j >= 0; j--)
	// 		if (!isspace(s[j]))  { j++; break; }
	// 	return s.substr(i, j-i);
	// }
	// string deliteral(const string& s) {
	// 	return is_strliteral(s) ? s.substr(1, s.length()-2) : s;
	// }
};