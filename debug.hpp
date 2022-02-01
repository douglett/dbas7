// ----------------------------------------
// Useful debugging functions
// ----------------------------------------
#pragma once
#include <iostream>
#include <string>
#include <vector>
// #include <iomanip>
#include <fstream>
#include "dbas7.hpp"
using namespace std;


struct Progshow {
	const Prog& prog;
	fstream fs;

	Progshow(const Prog& _prog) : prog(_prog) { }

	ostream& outp() { return fs.is_open() ? fs : cout; }
	const char* ind(int id) {
		static string s;
		return s = string(id*3, ' '),  s.c_str();
	}
	string numfmt(int num, int w) {
		string s = to_string(num);
		if (s.length() < w)  s = string(w - s.length(), '0') + s;
		return s;
	}
	void output(const string& s, int id) {
		outp() << ind(id) << s << endl;
	}


	int tofile(const string& fname) {
		fs.open(fname, ios::out);
		if (!fs.is_open()) {
			fprintf(stderr, "could not open file: %s\n", fname.c_str());
			cout.flush(), cerr.flush();
			return 1;
		}
		show();
		fs.close();
		printf("wrote program output to: %s\n", fname.c_str());
		return 0;
	}

	void show() {
		// module name
		output("<module>\n", 0);
		output(prog.module, 0);
		// all string literals
		output("", 0);
		output("<literals>", 0);
		for (auto& s : prog.literals)
			show_literal_index(s, 1);
		// types
		output("", 0);
		output("<types>", 0);
		for (auto& t : prog.types)
			show_type(t, 1);
		// globals
		output("", 0);
		output("<globals>", 0);
		for (auto& g : prog.globals)
			show_dim(g, 1);
		// functions
		output("", 0);
		output("<functions>", 0);
		for (auto& fn : prog.functions)
			output("", 0),  show_function(fn, 0);
	}

	void show_literal(const string& s, int id) {
		output("\"" + s + "\"", id);
	}
	void show_literal_index(const string& s, int id) {
		int index = &s - &prog.literals[0];
		output(numfmt(index, 2) + " \"" + s + "\"", id);
	}

	void show_type(const Prog::Type& t, int id) {
		outp() << ind(id) << "type " << t.name << endl;
		for (auto& d : t.members)
			show_dim(d, id+1);
	}

	void show_dim(const Prog::Dim& d, int id) {
		outp() << ind(id) << d.type << "  " << d.name << endl;
	}

	void show_function(const Prog::Function& fn, int id) {
		outp() << ind(id) << "function " << fn.name << endl;
		outp() << ind(id+1) << "args\n";
		for (auto& d : fn.args)
			show_dim(d, id+2);
		outp() << ind(id+1) << "locals\n";
		for (auto& d : fn.locals)
			show_dim(d, id+2);
		show_block(prog.blocks.at(fn.block), id+1);
	}

	void show_block(const Prog::Block& b, int id) {
		outp() << ind(id) << "block\n";
		for (auto& st : b.statements)
			show_statement(st, id+1);
	}
	// void show_block_index(const Prog::Block& b, int id) {
	// 	int index = &b - &prog.blocks[0];
	// 	printf("%sblock %d\n", ind(id), index );
	// 	for (auto& st : b.statements)
	// 		show_statement(st, id+1);
	// }

	void show_statement(const Prog::Statement& st, int id) {
		if      (st.type == "let")    show_let  ( prog.lets.at(st.loc), id );
		else if (st.type == "print")  show_print( prog.prints.at(st.loc), id );
		else if (st.type == "call")   show_call ( prog.calls.at(st.loc), id );
		else    outp() << ind(id) << "??\n";
	}

	void show_let(const Prog::Let& l, int id) {
		outp() << ind(id) << "let\n";
			// outp() << ind(id+1) << "path\n";
			show_varpath_head( prog.varpaths.at(l.varpath), id+1 );
			// outp() << ind(id+1) << "expr\n";
			show_expr_head   ( prog.exprs.at(l.expr), id+1 );
	}

	void show_print(const Prog::Print& pr, int id) {
		outp() << ind(id) << "print\n";
		for (auto& in : pr.instr) {
			outp() << ind(id+1) << in.first << endl;
			if      (in.first == "literal")   show_literal( prog.literals.at(stoi(in.second)), id+2 );
			else if (in.first == "expr")      show_expr   ( prog.exprs.at(stoi(in.second)), id+2 );
			else if (in.first == "expr_str")  show_expr   ( prog.exprs.at(stoi(in.second)), id+2 );
			else    outp() << ind(id) << "?? (" << in.first << ")\n";
		}
	}

	void show_varpath_head(const Prog::VarPath& vp, int id) {
		output("varpath (" + vp.type + ")", id);
		show_varpath(vp, id+1);
	}
	void show_varpath(const Prog::VarPath& vp, int id) {
		for (auto& in : vp.instr)
			if (in.cmd == "get" || in.cmd == "get_global" || in.cmd == "memget_prop")
				outp() << ind(id) << in.cmd << " " << in.sarg << endl;
			else if (in.cmd == "memget_expr")
				output(in.cmd, id),  show_expr( prog.exprs.at(in.iarg), id+1 );
			else    outp() << ind(id) << "?? (" << in.cmd << ")\n";
	}

	void show_expr_head(const Prog::Expr& ex, int id) {
		output("expr (" + ex.type + ")", id);
		show_expr(ex, id+1);
	}
	void show_expr(const Prog::Expr& ex, int id) {
		for (auto& in : ex.instr)
			if      (in.cmd == "i")    outp() << ind(id) << in.cmd << " " << in.iarg << endl;
			// else if (in.cmd == "lit")  show_literal( prog.literals.at(in.iarg), id );
			else if (in.cmd == "lit")  outp() << ind(id) << "lit \"" << prog.literals.at(in.iarg) << "\"\n";
			else if (in.cmd == "varpath" || in.cmd == "varpath_str")
				output(in.cmd, id),  show_varpath( prog.varpaths.at(in.iarg), id+1 );
			else if (in.cmd == "varpath_ptr")
				show_varpath_head( prog.varpaths.at(in.iarg), id+1 );
			else if (in.cmd == "call")
				show_call   ( prog.calls.at(in.iarg), id );
			else if (in.cmd == "add" || in.cmd == "sub" || in.cmd == "strcat")
				outp() << ind(id) << in.cmd << endl;
			else    outp() << ind(id) << "?? (" << in.cmd << ")\n";
	}

	void show_call(const Prog::Call& ca, int id) {
		outp() << ind(id) << "call " << ca.fname << endl;
		for (auto& arg : ca.args)
			show_expr_head( prog.exprs.at(arg.expr), id+1 );
	}
};