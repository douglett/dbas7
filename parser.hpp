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
	// vector<Prog::Type>        types;
	// vector<Prog::Dim>         globals;
	// vector<Prog::Let>         lets;
	// vector<Prog::VarPath>     varpaths;
	// vector<Prog::Expr>        exprs;
	Prog prog;

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
		string type, name, tname = lastrule.at(0);
		prog.types.push_back({ tname });
		// type members
		while (!eof()) {
			if      (expect("@endl"))                                  { nextline();  continue; }
			else if (expect("dim @identifier @identifier @endl"))      type = lastrule.at(0), name = lastrule.at(1);
			else if (expect("dim @identifier [ ] @identifier @endl"))  type = lastrule.at(0) + "[]", name = lastrule.at(1);
			else if (expect("dim @identifier @endl"))                  type = "int", name = lastrule.at(0);
			else    break;
			// typecheck(type), namecollision(name);
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
			else    throw error("unexpected block statement", currenttoken());
	}

	void p_dim() {
		string type, name;
		if      (expect ("dim @identifier @identifier"))      type = lastrule.at(0), name = lastrule.at(1);
		else if (expect ("dim @identifier [ ] @identifier"))  type = lastrule.at(0) + "[]", name = lastrule.at(1);
		else if (require("dim @identifier"))                  type = "int", name = lastrule.at(0);
		// typecheck(type), namecollision(name);
		prog.globals.push_back({ name, type });
		// assign here
		require("@endl"), nextline();
	}

	int p_let() {
		require("let");
		int32_t vp = p_varpath();
		require("=");
		int32_t ex = p_expr();
		prog.lets.push_back({ vp, ex });
		return prog.lets.size() - 1;
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
		return vp;
	}

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


	// show results
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
		if   (st.type == "let")  show( prog.lets.at(st.loc) );
		else  printf("  ??\n");
	}
	void show(const Prog::Let& l) const {
		printf("  let\n");
		printf("   -varpath\n");
		show( prog.varpaths.at(l.varpath) );
		printf("   -expr\n");
		show( prog.exprs.at(l.expr) );
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