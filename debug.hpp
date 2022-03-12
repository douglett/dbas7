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


	// === api ===

	Progshow(const Prog& _prog) : prog(_prog) { }

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


	// === helpers ===

	ostream& outp() { return fs.is_open() ? fs : cout; }
	const char* ind(int id) {
		static string s;
		// return s = string(id*3, ' '),  s.c_str();
		return s = string(id, '\t'),  s.c_str();
	}
	string numfmt(int num, int w) {
		string s = to_string(num);
		if (s.length() < w)  s = string(w - s.length(), '0') + s;
		return s;
	}
	void output(const string& s, int id) {
		outp() << ind(id) << s << endl;
	}



	// === show ===

	void show() {
		// module name
		output("<module>\n", 0);
		output(prog.module, 0);
		// all string literals
		output("", 0);
		output("<literals>", 0);
		for (int i = 0; i < prog.literals.size(); i++)
			show_literal_index(i, 1);
		// types
		output("", 0);
		output("<types>", 0);
		for (const auto& t : prog.types)
			show_type(t, 1);
		// globals
		output("", 0);
		output("<globals>", 0);
		for (const auto& g : prog.globals)
			show_dim(g, 1);
		// functions
		output("", 0);
		output("<functions>", 0);
		for (const auto& fn : prog.functions)
			output("", 0),  show_function(fn, 0);
	}

	void show_literal      (int sp, int id) { output("\"" + prog.literals.at(sp) + "\"", id); }
	void show_literal_head (int sp, int id) { output("lit \"" + prog.literals.at(sp) + "\"", id); }
	void show_literal_index(int sp, int id) { output(numfmt(sp, 2) + " \"" + prog.literals.at(sp) + "\"", id); }

	void show_dim(const Prog::Dim& d, int id) {
		output(d.type + "  " + d.name, id);
		if (d.expr > -1)
			show_expr_head(d.expr, id+1);
	}

	void show_type(const Prog::Type& t, int id) {
		output("type " + t.name, id);
		for (auto& d : t.members)
			show_dim(d, id+1);
	}

	void show_function(const Prog::Function& fn, int id) {
		output("function " + fn.name, id);
		output("args", id+1);
		for (auto& d : fn.args)
			show_dim(d, id+2);
		output("locals", id+1);
		for (auto& d : fn.locals)
			show_dim(d, id+2);
		show_block(fn.block, id+1);
	}

	void show_block(int blp, int id) {
		const auto& bl = prog.blocks.at(blp);
		output("block", id);
		for (auto& st : bl.statements)
			if      (st.type == "print")     show_print (st.loc, id);
			else if (st.type == "if")        show_if    (st.loc, id);
			else if (st.type == "while")     show_while (st.loc, id);
			else if (st.type == "return")    show_return(st.loc, id);
			else if (st.type == "break")     output("break " + to_string(st.loc), id);
			else if (st.type == "continue")  output("continue " + to_string(st.loc), id);
			else if (st.type == "let")       show_let   (st.loc, id);
			else if (st.type == "call")      show_call  (st.loc, id);
			else    output("?? (" + st.type + ")", id);
	}

	void show_print(int prp, int id) {
		const auto& pr = prog.prints.at(prp);
		output("print", id);
		for (auto& in : pr.instr) {
			output(in.cmd, id+1);
			if      (in.cmd == "literal")   show_literal(in.iarg, id+2);
			else if (in.cmd == "expr")      show_expr   (in.iarg, id+2);
			else if (in.cmd == "expr_str")  show_expr   (in.iarg, id+2);
			else    output("?? (" + in.cmd + ")", id+2);
		}
	}

	void show_if(int ifp, int id) {
		const auto& ii = prog.ifs.at(ifp);
		output("if", id);
		for (auto& cond : ii.conds)
			if (cond.expr > -1)
				output    ("condition", id+1),
				show_expr (cond.expr, id+2),
				show_block(cond.block, id+2);
			else
				show_block(cond.block, id+1);
	}

	void show_while(int whp, int id) {
		const auto& wh = prog.whiles.at(whp);
		output        ("while", id);
		show_expr_head(wh.expr, id+1);
		show_block    (wh.block, id+1);
	}

	void show_return(int exp, int id) {
		output("return", id);
		if (exp > -1)  show_expr(exp, id+1);
	}

	void show_let(int lp, int id) {
		const auto& l = prog.lets.at(lp);
		output           ("let", id);
		show_varpath_head(l.varpath, id+1);
		show_expr_head   (l.expr, id+1);
	}

	void show_call(int cap, int id) {
		const auto& ca = prog.calls.at(cap);
		output("call " + ca.fname, id);
		for (auto& arg : ca.args)
			show_expr_head(arg.expr, id+1);
	}

	void show_varpath_head(int vpp, int id) {
		const auto& vp = prog.varpaths.at(vpp);
		output("varpath (" + vp.type + ")", id);
		show_varpath(vpp, id+1);
	}
	void show_varpath(int vpp, int id) {
		const auto& vp = prog.varpaths.at(vpp);
		for (auto& in : vp.instr)
			if (in.cmd == "get" || in.cmd == "get_global" || in.cmd == "memget_prop")
				output(in.cmd + " " + in.sarg, id);
			else if (in.cmd == "memget_expr")
				output   (in.cmd, id),
				show_expr(in.iarg, id+1);
			else
				output("?? (" + in.cmd + ")", id);
	}

	void show_expr_head(int exp, int id) {
		const auto& ex = prog.exprs.at(exp);
		output("expr (" + ex.type + ")", id);
		show_expr(exp, id+1);
	}
	void show_expr(int exp, int id) {
		const auto& ex = prog.exprs.at(exp);
		for (auto& in : ex.instr)
			if      (in.cmd == "i")
				output("i " + to_string(in.iarg), id);
			else if (in.cmd == "lit")
				show_literal_head(in.iarg, id);
			else if (in.cmd == "varpath" || in.cmd == "varpath_str")
				output(in.cmd, id),  show_varpath(in.iarg, id);
			else if (in.cmd == "varpath_ptr")
				show_varpath_head(in.iarg, id);
			else if (in.cmd == "call")
				show_call(in.iarg, id);
			else if (in.cmd == "add" || in.cmd == "sub" || in.cmd == "eq" || in.cmd == "neq" || in.cmd == "strcat")
				output(in.cmd, id);
			else
				output("?? (" + in.cmd + ")", id);
	}

};