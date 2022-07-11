#pragma once
#include <vector>
#include <list>
#include <string>
#include <stdexcept>
using namespace std;


// TODO: this is a bad idea in general
template <typename T>
class dlist : public list<T> {
public:
	using list<T>::list;
	T&       at        (int i)       { return (T&)find(i); }
	T&       operator[](int i)       { return (T&)find(i); }
	const T& at        (int i) const { return find(i); }
	const T& operator[](int i) const { return find(i); }
	const T& find(int i) const {
		if (i < 0 || i >= this->size())
			throw out_of_range(string("dlist out of range: ") + to_string(i));
		auto it = this->begin();
		advance(it, i);
		return *it;
	}
};


// template <typename T>
// class dvec {
// private:
// 	vector<T*> v;
// 	void destroyall() { for (T* el : v) delete el; }
// public:
// 	dvec(const dvec& d) {
// 		destroyall();
// 		v.resize(d.size(), nullptr);
// 		for (int i = 0; i < d.size(); i++)
// 			v[i] = new T(*d.v[i]);
// 	}
// 	~dvec() { destroyall(); }
// 	int size() { return v.size(); }
// 	int length() { return v.size(); }
// };


// enum class Cmd {
// 	// integers
// 	i = 1,
// 	varpath,
// 	add,
// 	sub,
// 	mul,
// 	div,
// 	and_,
// 	or_,
// 	eq,
// 	neq,
// 	lt,
// 	gt,
// 	lte,
// 	gte,
// 	// strings
// 	lit = 20,
// 	varpath_str,
// 	strcat,
// 	eq_str,
// 	neq_str,
// 	// memory
// 	get = 30,
// 	get_global,
// 	memget_expr,
// 	memget_prop,
// 	// other
// 	varpath_ptr = 40,
// 	call,
// };


struct Prog {
	struct Dsym         { int lno, fno; };
	struct Dim          { string name, type; int expr; Dsym dsym; };
	struct Type         { string name; vector<Dim> members; };
	struct Function     { string name; int block; vector<Dim> args, locals; Dsym dsym; };
	struct Statement    { string type; int loc; };
	struct Block        { vector<Statement> statements; };
	struct Instruction  { string cmd; int32_t iarg; string sarg; };
	struct Print        { vector<Instruction> instr; };
	struct Input        { string prompt; int varpath; };
	struct Condition    { int expr; int block; };
	struct If           { vector<Condition> conds; };
	struct While        { int expr; int block; };
	struct For          { int varpath; int start_expr; int end_expr; int32_t step; int block; };
	struct Let          { string type; int varpath, expr; };
	struct VarPath      { string type; vector<Instruction> instr; };
	struct Expr         { string type; vector<Instruction> instr; };
	struct Argument     { string type; int expr; };
	struct Call         { string fname; vector<Argument> args; Dsym dsym; };

	string                   module;
	vector<string>           files;
	dlist<Prog::Type>        types;
	dlist<Prog::Dim>         globals;
	dlist<Prog::Function>    functions;
	dlist<string>            literals;  // temp?
	dlist<Prog::Block>       blocks;
	dlist<Prog::Print>       prints;
	dlist<Prog::Input>       inputs;
	dlist<Prog::If>          ifs;
	dlist<Prog::While>       whiles;
	dlist<Prog::For>         fors;
	dlist<Prog::Let>         lets;
	dlist<Prog::VarPath>     varpaths;
	dlist<Prog::Expr>        exprs;
	dlist<Prog::Call>        calls;
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
	int is_arraytype(const string& s) {
		return s.size() >= 3 && s[s.length()-2] == '[' && s[s.length()-1] == ']';
	}
	string basetype(const string& s) {
		return is_arraytype(s) ? s.substr(0, s.length()-2) : s;
	}
	int is_keyword(const string& s) {
		static const vector<string> KEYWORDS = {
			// "int", "string",
			"type", "function", "end", "if", "else", "while", "for", "break", "continue", "to", "step",
			"dim", "redim", "let", "call", "print", "input" };
		for (auto& k : KEYWORDS)  if (k == s)  return 1;
		return 0;
	}
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
	string join(const vector<string>& vs, const string& glue=" ") {
		string str;
		for (int i = 0; i < vs.size(); i++)
			str += (i > 0 ? glue : "") + vs[i];
		return str;
	}
	// string chomp(const string& s) {
	// 	int i = 0, j = s.length() - 1;
	// 	for ( ; i < s.length(); i++)
	// 		if (!isspace(s[i]))  break;
	// 	for ( ; j >= 0; j--)
	// 		if (!isspace(s[j]))  { j++; break; }
	// 	return s.substr(i, j-i);
	// }
	string deliteral(const string& s) {
		return Tokens::is_literal(s) ? s.substr(1, s.length()-2) : s;
	}
};