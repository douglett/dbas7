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
			else if (peek("let"))      prog.blocks.at(bl).statements.push_back({ "let", p_let() });
			else if (peek("print"))    prog.blocks.at(bl).statements.push_back({ "print", p_print() });
			else    throw error("unexpected block statement", currenttoken());
	}

	void p_dim() {
		string type, name;
		if      (expect ("dim @identifier @identifier"))      type = lastrule.at(0), name = lastrule.at(1);
		else if (expect ("dim @identifier [ ] @identifier"))  type = lastrule.at(0) + "[]", name = lastrule.at(1);
		else if (require("dim @identifier"))                  type = "int", name = lastrule.at(0);
		if (Tokens::is_keyword(name) || is_type(name) || is_global(name) || Tokens::is_keyword(type) || !is_type(type))
			throw error("dim collision", type + ":" + name);
		prog.globals.push_back({ name, type });
		// TODO: assign here
		require("@endl"), nextline();
	}

	int p_let() {
		require("let");
		prog.lets.push_back({ "<NULL>", -1, -1 });
		int let = prog.lets.size() - 1;
		// dest varpath
		int vp = prog.lets.at(let).varpath = p_varpath();
		require("=");
		string dest_type = prog.lets.at(let).type = prog.varpaths.at(vp).type;
		
		// get src based on dest_type
		if (dest_type == "int") {
			prog.lets.at(let).expr = p_expr();
		}
		else if (dest_type == "string") {
			require("@literal");
			prog.literals.push_back( Strings::deliteral(lastrule.at(0)) );
			prog.lets.at(let).expr = prog.literals.size() - 1;
		}
		else
			throw error("let unimplemented", dest_type);

		// string src_type = prog.exprs.at(ex).type;
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

	int p_varpath() {
		prog.varpaths.push_back({ "<NULL>" });
		// int32_t ex = -1, 
		int32_t vp = prog.varpaths.size() - 1;
		require("@identifier");
		string global = lastrule.at(0), type = getglobaltype(global), prop;
		prog.varpaths.at(vp).instr.push_back("get_global " + lastrule.at(0));
		// path chain
		while (!eol())
			if (expect("["))
				// ex = p_expr(),
				// prog.varpaths.at(vp).instr.push_back("memget_expr " + to_string(ex)),
				// require("]");
				throw error("not yet...");
			else if (expect(".")) {
				require("@identifier"), prop = lastrule.at(0);
				prog.varpaths.at(vp).instr.push_back("memget USRTYPE_" + type + "_" + prop);
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

	int p_expr() {
		prog.exprs.push_back({ "<NULL>" });
		int ex = prog.exprs.size() - 1;
		require("@integer");
		prog.exprs.at(ex).instr.push_back("i " + lastrule.at(0));
		prog.exprs.at(ex).type = "int";
		return ex;
	}



	// --- Show results ---

	void show() const {
		printf("<types>\n");
		for (auto& t : prog.types)  show(t);
		printf("<globals>\n");
		for (auto& g : prog.globals)  show(g);
		printf("<blocks>\n");
		for (auto& b : prog.blocks)  show(b);
	}
	void show(const Prog::Type& t) const {
		printf("type %s\n", t.name.c_str() );
		for (auto& d : t.members)  show(d);
	}
	void show(const Prog::Dim& d) const {
		printf("   %s  %s\n", d.type.c_str(), d.name.c_str());
	}
	void show(const Prog::Block& b) const {
		printf("block {\n");
		for (auto& st : b.statements)  show(st);
		printf("}\n");
	}
	void show(const Prog::Statement& st) const {
		if      (st.type == "let")    show( prog.lets.at(st.loc) );
		else if (st.type == "print")  show( prog.prints.at(st.loc) );
		else    printf("  ??\n");
	}
	void show(const Prog::Let& l) const {
		printf("  let\n");
		show( prog.varpaths.at(l.varpath) );
		printf("    ---\n");
		show( prog.exprs.at(l.expr) );
	}
	void show(const Prog::Print& pr) const {
		printf("  print\n");
		for (auto& in : pr.instr)
			if      (in.first == "literal")  printf("    %s\n", in.second.c_str() );
			else if (in.first == "varpath")  printf("    ---\n"),  show( prog.varpaths.at(stoi(in.second)) );
			else if (in.first == "expr")     printf("    ---\n"),  show( prog.exprs.at(stoi(in.second)) );
			else    printf("    ??\n");
	}
	void show(const Prog::VarPath& vp) const {
		for (auto& in : vp.instr)
			printf("    %s\n", in.c_str() );
	}
	void show(const Prog::Expr& ex) const {
		for (auto& in : ex.instr)
			printf("    %s\n", in.c_str() );
	}
};