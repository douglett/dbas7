// ----------------------------------------
// Useful debugging functions
// ----------------------------------------
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "dbas7.hpp"
using namespace std;


struct Progshow {
	const Prog& prog;

	Progshow(const Prog& _prog) : prog(_prog) { }

	void show() {
		// module name
		printf("<module>\n");
		printf("%s%s\n", ind(1), prog.module.c_str() );
		// all string literals
		printf("\n<literals>\n");
		for (auto& s : prog.literals)
			show_literal_index(s, 1);
		// types
		printf("\n<types>\n");
		for (auto& t : prog.types)
			show_type(t, 1);
		// globals
		printf("\n<globals>\n");
		for (auto& g : prog.globals)
			show_dim(g, 1);
		// functions
		printf("\n<functions>\n");
		for (auto& fn : prog.functions)
			printf("\n"), show_function(fn, 0);
	}

	void show_literal(const string& s, int id) {
		printf("%s\"%s\"\n", ind(id), s.c_str() );
	}
	void show_literal_index(const string& s, int id) {
		int index = &s - &prog.literals[0];
		printf("%s%02d \"%s\"\n", ind(id), index, s.c_str() );
	}

	void show_type(const Prog::Type& t, int id) {
		printf("%stype %s\n", ind(id), t.name.c_str() );
		for (auto& d : t.members)
			show_dim(d, id+1);
	}

	void show_dim(const Prog::Dim& d, int id) {
		printf("%s%s  %s\n", ind(id), d.type.c_str(), d.name.c_str());
	}

	void show_function(const Prog::Function& fn, int id) {
		printf("%sfunction %s\n", ind(id), fn.name.c_str() );
		printf("%sargs\n", ind(id+1) );
		for (auto& d : fn.args)
			show_dim(d, id+2);
		printf("%slocals\n", ind(id+1) );
		for (auto& d : fn.locals)
			show_dim(d, id+2);
		show_block(prog.blocks.at(fn.block), id+1);
	}

	void show_block(const Prog::Block& b, int id) {
		printf("%sblock\n", ind(id) );
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
		else    printf("%s??\n", ind(id) );
	}

	void show_let(const Prog::Let& l, int id) {
		printf("%slet\n", ind(id) );
			printf("%spath\n", ind(id+1) );
				show_varpath( prog.varpaths.at(l.varpath), id+2 );
			printf("%sexpr\n", ind(id+1) );
				show_expr   ( prog.exprs.at(l.expr), id+2 );
		// printf("%s-->\n", ind(id+1) );
	}

	void show_print(const Prog::Print& pr, int id) {
		printf("%sprint\n", ind(id) );
		for (auto& in : pr.instr) {
			printf("%s%s\n", ind(id+1), in.first.c_str() );
			if      (in.first == "literal")   show_literal( prog.literals.at(stoi(in.second)), id+2 );
			else if (in.first == "expr")      show_expr   ( prog.exprs.at(stoi(in.second)), id+2 );
			else if (in.first == "expr_str")  show_expr   ( prog.exprs.at(stoi(in.second)), id+2 );
			else    printf("%s?? (%s)\n", ind(id+2), in.first.c_str() );
		}
	}

	void show_varpath(const Prog::VarPath& vp, int id) {
		for (auto& in : vp.instr)
			if (in.cmd == "get" || in.cmd == "get_global" || in.cmd == "memget_prop")
				printf("%s%s %s\n", ind(id), in.cmd.c_str(), in.sarg.c_str() );
			else if (in.cmd == "memget_expr")
				printf("%s%s\n", ind(id), in.cmd.c_str() ),
				show_expr( prog.exprs.at(in.iarg), id+1 );
			else    printf("%s?? (%s)\n", ind(id), in.cmd.c_str() );
	}

	void show_expr(const Prog::Expr& ex, int id) {
		for (auto& in : ex.instr)
			if      (in.cmd == "i")    printf("%s%s %d\n", ind(id), in.cmd.c_str(), in.iarg );
			else if (in.cmd == "lit")  show_literal( prog.literals.at(in.iarg), id );
			else if (in.cmd == "varpath" || in.cmd == "varpath_str" || in.cmd == "varpath_ptr")
				show_varpath( prog.varpaths.at(in.iarg), id );
			else if (in.cmd == "call")
				show_call   ( prog.calls.at(in.iarg), id );
			else if (in.cmd == "add" || in.cmd == "sub" || in.cmd == "strcat")
				printf("%s%s\n", ind(id), in.cmd.c_str() );
			else    printf("%s?? (%s)\n", ind(id), in.cmd.c_str() );
	}

	void show_call(const Prog::Call& ca, int id) {
		printf("%scall %s\n", ind(id), ca.fname.c_str() );
		for (auto& arg : ca.args) {
			printf("%sexpr %s\n", ind(id+1), arg.type.c_str() );
			show_expr( prog.exprs.at(arg.expr), id+2 );
			// printf("    ---\n");
		}
	}

	const char* ind(int id) {
		static string s;
		return s = string(id*3, ' '),  s.c_str();
	}
};