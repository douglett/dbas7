// ----------------------------------------
// Input file parsing
// ----------------------------------------
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "dbas7.hpp"
#include "inputfile.hpp"
using namespace std;


struct Parser : InputFile {
	// parser results
	Prog prog;
	// parser state
	// string ctype;



// --- State checking ---

	int is_type(const string& type) const {
		if (type == "int" || type == "string")  return 1;
		for (int i = 0; i < prog.types.size(); i++)
			if (prog.types[i].name == type)  return 1;
		return 0;
	}
	int is_global(const string& name) const {
		for (int i = 0; i < prog.globals.size(); i++)
			if (prog.globals[i].name == name)  return 1;
		return 0;
	}
	int is_member(const string& type, const string& member) const {
		for (auto& t : prog.types)
			for (auto& m : t.members)
				if (t.name == type && m.name == member)  return 1;
		return 0;
	}



// --- Main parsing functions ---

	void parse() {
		while (!eof())
			if      (expect("@endl"))  { nextline();  continue; }
			else if (peek("type"))     p_type();
			else if (peek("dim"))      p_dim();
			else if (peek("block"))    p_block0();
			else    throw error("unexpected command", currenttoken());
	}

	void p_type() {
		require("type @identifier @endl");
		string type, name, ctype = lastrule.at(0);
		if (Tokens::is_keyword(ctype) || is_type(ctype) || is_global(ctype))
			throw error("type name collision", ctype);
		prog.types.push_back({ ctype });
		// type members
		while (!eof()) {
			if      (expect("@endl"))                                  { nextline();  continue; }
			else if (expect("dim @identifier @identifier @endl"))      type = lastrule.at(0), name = lastrule.at(1);
			else if (expect("dim @identifier [ ] @identifier @endl"))  type = lastrule.at(0) + "[]", name = lastrule.at(1);
			else if (expect("dim @identifier @endl"))                  type = "int", name = lastrule.at(0);
			else    break;
			if (Tokens::is_keyword(name) || is_type(name) || Tokens::is_keyword(type) || !is_type(type) || is_member(ctype, name))
				throw error("type member collision", type + ":" + name);
			prog.types.back().members.push_back({ name, type });  // save type member
			nextline();
		}
		require("end type @endl"), nextline();
	}

	void p_dim() {
		string type, btype, name;
		if      (expect ("dim @identifier @identifier"))      type = btype = lastrule.at(0), name = lastrule.at(1);
		else if (expect ("dim @identifier [ ] @identifier"))  btype = lastrule.at(0), type = btype + "[]", name = lastrule.at(1);
		else if (require("dim @identifier"))                  type = btype = "int", name = lastrule.at(0);
		if (Tokens::is_keyword(name) || is_type(name) || is_global(name) || Tokens::is_keyword(btype) || !is_type(btype))
			throw error("dim collision", type + ":" + name);
		prog.globals.push_back({ name, type });
		// TODO: assign here
		require("@endl"), nextline();
	}

	void p_block0() {
		require("block @endl");
		p_block();
		require("end block @endl"), nextline();
	}
	void p_block() {
		prog.blocks.push_back({});
		int bl = prog.blocks.size() - 1;
		while (!eof())
			if      (expect("@endl"))  { nextline();  continue; }
			else if (peek("end"))      break;
			else if (peek("let"))      prog.blocks.at(bl).statements.push_back({ "let",    p_let() });
			else if (peek("print"))    prog.blocks.at(bl).statements.push_back({ "print",  p_print() });
			else if (peek("call"))     prog.blocks.at(bl).statements.push_back({ "call",   p_call_stmt() });
			else    throw error("unexpected block statement", currenttoken());
	}

	int p_let() {
		require("let");
		prog.lets.push_back({ "<NULL>", -1, -1 });
		int let = prog.lets.size() - 1;
		// dest varpath
		int vp = prog.lets.at(let).varpath = p_varpath();
		string dest_type = prog.lets.at(let).type = prog.varpaths.at(vp).type;
		require("=");
		// src varpath
		prog.lets.at(let).expr = p_expr( dest_type );
		require("@endl"), nextline();
		return let;
	}

	int p_print() {
		require("print");
		prog.prints.push_back({});
		int pr = prog.prints.size() - 1, loc = 0;

		while (!eol())
			if      (expect("@literal"))   prog.prints.at(pr).instr.push_back({ "literal", lastrule.at(0) });
			else if (peek("@integer"))     loc = p_expr(),  prog.prints.at(pr).instr.push_back({ "expr", to_string(loc) });
			else if (peek("@identifier")) {
				// TODO: this is messy
				loc = p_varpath();
				string cmd, vptype = prog.varpaths.at(loc).type;
				if      (vptype == "int")     cmd = "varpath";
				else if (vptype == "string")  cmd = "varpath_string";
				else    throw error("invalid varpath type", vptype);
				prog.prints.at(pr).instr.push_back({ cmd, to_string(loc) });
			}
			else    throw error("unexpected in print", currenttoken());

		require("@endl"), nextline();
		return pr;
	}



// --- Variable path handling ---

	int p_varpath() {
		require("@identifier");
		prog.varpaths.push_back({ "<NULL>" });
		int32_t vp = prog.varpaths.size() - 1,  ex = -1;
		string global = lastrule.at(0), type = getglobaltype(global), prop;
		prog.varpaths.at(vp).instr.push_back({ "get_global", 0, lastrule.at(0) });
		// path chain
		while (!eol())
			if (expect("[")) {
				ex = p_expr();
				prog.varpaths.at(vp).instr.push_back({ "memget_expr", ex });
				require("]");
				if (!Tokens::is_arraytype(type))
					throw error("expected array type", type);
				type = Tokens::basetype(type);
			}
			else if (expect(".")) {
				require("@identifier"), prop = lastrule.at(0);
				prog.varpaths.at(vp).instr.push_back({ "memget_prop", 0, "USRTYPE_" + type + "_" + prop });
				type = getproptype(type, prop);
			}
			else
				break;
		prog.varpaths.at(vp).type = type;  // save type
		return vp;
	}

	// p_varpath helpers
	string getglobaltype(const string& name) const {
		for (auto& g : prog.globals)
			if (g.name == name)  return g.type;
		throw error("undefined global", name);
	}
	string getproptype(const string& type, const string& prop) const {
		for (auto& t : prog.types)
			if (t.name == type)
				for (auto& m : t.members)
					if (m.name == prop)  return m.type;
		throw error("type or property not defined", type + "." + prop);
	}



// --- Function calls ---

	int p_call() {
		require("@identifier (");
		string fname = lastrule.at(0);
		prog.calls.push_back({ fname });
		int ca = prog.calls.size() - 1;

		while (!eol() && !peek(")")) {
			int ex = p_expr();
			prog.calls.at(ca).args.push_back({ prog.exprs.at(ex).type, ex });
			peek(")") || require(",");
		}
		require(")");
		// type checking
		if (!p_call_internal( prog.calls.at(ca) ))
			throw error("unknown function", fname);
		return ca;
	}
	
	int p_call_stmt() {
		require("call");
		int ca = p_call();
		require("@endl"), nextline();
		return ca;
	}

	int p_call_internal(const Prog::Call& ca) {
		if (ca.fname == "push") {
			// if (ca.args.size() != 2 || ca.args[0].type != "int[]" || ca.args[1].type != "int")
			if (ca.args.size() == 2 && Tokens::is_arraytype(ca.args[0].type) 
				&& Tokens::basetype(ca.args[0].type) == ca.args[1].type)  return 1;
			throw error("incorrect arguments");
		}
		else if (ca.fname == "pop") {
			if (ca.args.size() == 1 && Tokens::is_arraytype(ca.args[0].type))  return 1;
			throw error("incorrect argument");
		}
		else if (ca.fname == "len") {
			if (ca.args.size() == 1 && (Tokens::is_arraytype(ca.args[0].type) || ca.args[0].type == "string"))  return 1;
			throw error("incorrect argument");
		}
		return 0;
	}



// --- Expressions ---

	Prog::Expr& eptr(int ptr) { return prog.exprs.at(ptr); }

	int p_expr(const string& type) {
		int ex = p_expr();
		const auto& t = prog.exprs.at(ex).type;
		if (type != t)
			throw error("unexpected type in expression (expected " + type + ")", t);
		return ex;
	}
	int p_expr() {
		prog.exprs.push_back({ "<NULL>" });
		int ex = prog.exprs.size() - 1;
		p_expr_add(ex);
		return ex;
	}

	void p_expr_add(int ex) {
		p_expr_atom(ex);
		auto type = eptr(ex).type;
		if (type != "int" && type != "string")  return;
		while (peek("+") || peek("-")) {
			string op = currenttoken();
			expect("+") || expect("-");
			p_expr_atom(ex);
			if      (type != eptr(ex).type)                throw error("add type mismatch");
			else if (type == "int"    && op == "+")  eptr(ex).instr.push_back({ "add" });
			else if (type == "int"    && op == "-")  eptr(ex).instr.push_back({ "sub" });
			else if (type == "string" && op == "+")  eptr(ex).instr.push_back({ "strcat" });
			else if (type == "string" && op == "-")  throw error("cannot subtract strings");
			else    throw error("unexpected add expression");
		}
	}

	void p_expr_atom(int ex) {
		int t = 0;
		if (expect("@integer"))
			eptr(ex).instr.push_back({ "i", stoi(lastrule.at(0)) }),
			eptr(ex).type = "int";
		else if (expect("@literal"))
			prog.literals.push_back( Strings::deliteral(lastrule.at(0)) ),
			eptr(ex).instr.push_back({ "lit", int32_t(prog.literals.size()-1) }),
			eptr(ex).type = "string";
		else if (peek("@identifier ("))
			t = p_call(),
			eptr(ex).instr.push_back({ "call", t }),
			eptr(ex).type = "int";
		else if (peek("@identifier")) {
			t = p_varpath();
			eptr(ex).type = prog.varpaths.at(t).type;
			if      (eptr(ex).type == "int")     eptr(ex).instr.push_back({ "varpath", t });
			else if (eptr(ex).type == "string")  eptr(ex).instr.push_back({ "varpath_str", t });
			else    eptr(ex).instr.push_back({ "varpath_ptr", t });
		}
		else
			throw error("expected atom");
	}



// --- Show results ---

	void show() const {
		printf("<literals>\n");
		for (int i = 0; i < prog.literals.size(); i++)  show(prog.literals.at(i), i, 1);
		printf("<types>\n");
		for (auto& t : prog.types)  show(t, 1);
		printf("<globals>\n");
		for (auto& g : prog.globals)  show(g, 1);
		printf("<blocks>\n");
		for (int i = 0; i < prog.blocks.size(); i++)  show(prog.blocks.at(i), i, 0);
	}
	void show(const string& s, int index, int id) const {
		printf("%s%02d \"%s\"\n", ind(id), index, s.c_str() );
	}
	void show(const string& s, int id) const {
		printf("%s\"%s\"\n", ind(id), s.c_str() );
	}
	void show(const Prog::Type& t, int id) const {
		printf("%stype %s\n", ind(id), t.name.c_str() );
		for (auto& d : t.members)  show(d, id+1);
	}
	void show(const Prog::Dim& d, int id) const {
		printf("%s%s  %s\n", ind(id), d.type.c_str(), d.name.c_str());
	}
	void show(const Prog::Block& b, int index, int id) const {
		printf("%sblock %d\n", ind(id), index );
		for (auto& st : b.statements)  show(st, id+1);
	}
	void show(const Prog::Statement& st, int id) const {
		if      (st.type == "let")    show( prog.lets.at(st.loc), id );
		else if (st.type == "print")  show( prog.prints.at(st.loc), id );
		else if (st.type == "call")   show( prog.calls.at(st.loc), id );
		else    printf("%s??\n", ind(id) );
	}
	void show(const Prog::Let& l, int id) const {
		printf("%slet\n", ind(id) );
			printf("%spath\n", ind(id+1) );
				show( prog.varpaths.at(l.varpath), id+2 );
			printf("%sto\n", ind(id+1) );
				show( prog.exprs.at(l.expr), id+2 );
		// printf("%s-->\n", ind(id+1) );
	}
	void show(const Prog::Print& pr, int id) const {
		printf("%sprint\n", ind(id) );
		for (auto& in : pr.instr) {
			if      (in.first == "literal")  printf("%s%s\n", ind(id+1), in.second.c_str() );
			else if (in.first == "varpath")  show( prog.varpaths.at(stoi(in.second)), id+1 );
			else if (in.first == "expr")     show( prog.exprs.at(stoi(in.second)), id+1 );
			else    printf("%s?? (%s)\n", ind(id+1), in.first.c_str() );
			// printf("    ---\n");
		}
	}
	void show(const Prog::VarPath& vp, int id) const {
		for (auto& in : vp.instr)
			if (in.cmd == "get" || in.cmd == "get_global" || in.cmd == "memget_prop")
				printf("%s%s %s\n", ind(id), in.cmd.c_str(), in.sarg.c_str() );
			else if (in.cmd == "memget_expr")
				printf("%s%s\n", ind(id), in.cmd.c_str() ),
				show( prog.exprs.at(in.iarg), id+1 );
			else    printf("%s?? (%s)\n", ind(id), in.cmd.c_str() );
	}
	void show(const Prog::Expr& ex, int id) const {
		for (auto& in : ex.instr)
			if      (in.cmd == "i")    printf("%s%s %d\n", ind(id), in.cmd.c_str(), in.iarg );
			else if (in.cmd == "lit")  show( prog.literals.at(in.iarg), id );
			else if (in.cmd == "varpath" || in.cmd == "varpath_str" || in.cmd == "varpath_ptr")
				show( prog.varpaths.at(in.iarg), id );
			else if (in.cmd == "call")
				show( prog.calls.at(in.iarg), id );
			else if (in.cmd == "add" || in.cmd == "sub" || in.cmd == "strcat")
				printf("%s%s\n", ind(id), in.cmd.c_str() );
			else    printf("%s?? (%s)\n", ind(id), in.cmd.c_str() );
	}
	void show(const Prog::Call& ca, int id) const {
		printf("%scall\n", ind(id) );
		for (auto& arg : ca.args) {
			printf("%s%s\n", ind(id), arg.type.c_str() );
			show( prog.exprs.at(arg.expr), id+1 );
			// printf("    ---\n");
		}
	}
	const char* ind(int id) const {
		static string s;
		return s = string(id*3, ' '),  s.c_str();
	}
};