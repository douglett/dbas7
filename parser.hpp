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
	int flag_mod = 0, flag_func = -1;



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
	int is_local(const string& name) const {
		if (flag_func <= -1)  return 0;
		auto& fn = prog.functions.at(flag_func);
		// TODO: arguments
		for (int i = 0; i < fn.locals.size(); i++)
			if (fn.locals[i].name == name)  return 1;
		return 0;
	}
	int is_member(const string& type, const string& member) const {
		for (auto& t : prog.types)
			for (auto& m : t.members)
				if (t.name == type && m.name == member)  return 1;
		return 0;
	}
	int is_func(const string& fname) {
		static vector<string> fn_internal = { "push", "pop", "len", "default" };
		for (auto& n : fn_internal)
			if (fname == n)  return 1;
		for (auto& fn : prog.functions)
			if (fn.name == fname)  return 1;
		return 0;
	}



// --- Main parsing functions ---

	void parse() {
		prog.module = "default";
		p_section("module");
		p_section("type");
		p_section("dim");
		p_section("function");
		if (!eof())  throw error("unexpected command", currenttoken());
		p_callcheck_all();
	}

	void p_section(const string& section) {
		while (!eof())
			if      (expect("@endl"))  nextline();
			else if (section == "module"   && peek("module"))    p_module();
			else if (section == "type"     && peek("type"))      p_type();
			else if (section == "dim"      && peek("dim"))       p_dim_global();
			else if (section == "function" && peek("function"))  p_function();
			else    break;
	}

	void p_module() {
		require("module @identifier @endl");
		if (flag_mod)  throw error("module name redefinition");
		prog.module = lastrule.at(0),  flag_mod = 1;
		if (Tokens::is_keyword(prog.module))  throw error("module name collision", prog.module);
	}

	void p_type() {
		require("type @identifier @endl");
		string type, btype, name, ctype = lastrule.at(0);
		if (Tokens::is_keyword(ctype) || is_type(ctype) || is_global(ctype))
			throw error("type name collision", ctype);
		prog.types.push_back({ ctype });
		// type members
		while (!eof()) {
			if      (expect("@endl"))                                  { nextline();  continue; }
			else if (expect("dim @identifier @identifier @endl"))      type = btype = lastrule.at(0),  name = lastrule.at(1);
			else if (expect("dim @identifier [ ] @identifier @endl"))  btype = lastrule.at(0),  type = btype + "[]",  name = lastrule.at(1);
			else if (expect("dim @identifier @endl"))                  type = btype = "int",  name = lastrule.at(0);
			else    break;
			if (Tokens::is_keyword(name) || is_type(name) || !is_type(btype) || is_member(ctype, name))
				throw error("type member collision", type + ":" + name);
			prog.types.back().members.push_back({ name, type });  // save type member
			nextline();
		}
		require("end type @endl"), nextline();
	}

	Prog::Dim p_dim() {
		string type, btype, name;
		if      (expect ("dim @identifier @identifier"))      type = btype = lastrule.at(0),  name = lastrule.at(1);
		else if (expect ("dim @identifier [ ] @identifier"))  btype = lastrule.at(0),  type = btype + "[]",  name = lastrule.at(1);
		else if (require("dim @identifier"))                  type = btype = "int",  name = lastrule.at(0);
		if (Tokens::is_keyword(name) || is_type(name) || !is_type(btype))
			throw error("dim collision", type + ":" + name);
		// prog.globals.push_back({ name, type });
		return { name, type };
	}

	void p_dim_global() {
		auto dim = p_dim();
		if (is_global(dim.name))
			throw error("global redefined", dim.name);
		prog.globals.push_back(dim);
		// TODO: assign here
		require("@endl"), nextline();
	}

	void p_dim_local(int fn) {
		auto dim = p_dim();
		for (auto& l : prog.functions.at(fn).locals)
			if (l.name == dim.name)
				throw error("local redefined", dim.name);
		prog.functions.at(fn).locals.push_back(dim);
		// TODO: assign here
		require("@endl"), nextline();
	}

	void p_function() {
		require("function @identifier ( ) @endl");
		auto fname = lastrule.at(0);
		if (Tokens::is_keyword(fname) || is_global(fname) || is_func(fname))
			throw error("function name collision", fname);
		prog.functions.push_back({ fname });
		int fn = prog.functions.size() - 1;
		flag_func = fn;
		prog.functions.at(fn).dsym = lno + 1;
		// locals
		while (!eof())
			if      (expect("@endl"))  nextline();
			else if (peek("dim"))      p_dim_local(fn);
			else    break;
		// parse block
		prog.functions.at(fn).block = p_block();
		// end
		require("end function @endl"), nextline();
		flag_func = -1;
	}

	Prog::Block& bptr(int ptr) { return prog.blocks.at(ptr); }
	// void p_block0() {
	// 	require("block @endl");
	// 	p_block();
	// 	require("end block @endl"), nextline();
	// }
	int p_block() {
		prog.blocks.push_back({});
		int bl = prog.blocks.size() - 1;
		while (!eof())
			if      (expect("@endl"))         nextline();
			else if (peek("end"))             break;
			else if (peek("print"))           bptr(bl).statements.push_back({ "print",  p_print() });
			else if (peek("let"))             bptr(bl).statements.push_back({ "let",    p_let() });
			else if (peek("call"))            bptr(bl).statements.push_back({ "call",   p_call_stmt() });
			else if (peek("@identifier ("))   bptr(bl).statements.push_back({ "call",   p_call_stmt() });
			else if (peek("@identifier"))     bptr(bl).statements.push_back({ "let",    p_let() });
			else    throw error("unexpected block statement", currenttoken());
		return bl;
	}

	int p_let() {
		// require("let");
		expect("let");  // optional
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
		int pr = prog.prints.size() - 1;
		while (!eol()) {
			int ex = p_expr();
			if      (eptr(ex).type == "int")     prog.prints.at(pr).instr.push_back({ "expr", to_string(ex) });
			else if (eptr(ex).type == "string")  prog.prints.at(pr).instr.push_back({ "expr_str", to_string(ex) });
			else    throw error("unexpected type in print", eptr(ex).type);
		}
		require("@endl"), nextline();
		return pr;
	}



// --- Variable path handling ---

	int p_varpath() {
		require("@identifier");
		prog.varpaths.push_back({ "<NULL>" });
		int32_t vp = prog.varpaths.size() - 1,  ex = -1;
		string  type,  prop,  varname = lastrule.at(0);
		if (is_local(varname))
			type = getlocaltype(varname),
			prog.varpaths.at(vp).instr.push_back({ "get", 0, varname });
		else
			type = getglobaltype(varname),
			prog.varpaths.at(vp).instr.push_back({ "get_global", 0, varname });
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
	string getlocaltype(const string& name) const {
		auto& fn = prog.functions.at(flag_func);
		for (auto& d : fn.locals)
			if (d.name == name)  return d.type;
		throw error("undefined local", name);
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
		prog.calls.at(ca).dsym = lno + 1;
		// arguments
		while (!eol() && !peek(")")) {
			int ex = p_expr();
			prog.calls.at(ca).args.push_back({ prog.exprs.at(ex).type, ex });
			peek(")") || require(",");
		}
		require(")");
		// type checking
		// if (!p_call_internal( prog.calls.at(ca) ))
		// 	throw error("unknown function", fname);
		return ca;
	}
	
	int p_call_stmt() {
		// require("call");
		expect("call");  // optional
		int ca = p_call();
		require("@endl"), nextline();
		return ca;
	}

	int p_callcheck_all() const {
		for (auto& ca : prog.calls)
			p_callcheck(ca);
		return 1;
	}

	int p_callcheck(const Prog::Call& ca) const {
		if (p_callcheck_internal(ca))  return 1;
		for (auto& fn : prog.functions)
			if (fn.name == ca.fname) {
				if (ca.args.size() != 0)  throw error("incorrect argument count, line " + to_string(ca.dsym));
				return 1;
			}
		throw error("function undefined, line " + to_string(ca.dsym), ca.fname);
	}

	int p_callcheck_internal(const Prog::Call& ca) const {
		using Tokens::is_arraytype;
		using Tokens::basetype;
		if (ca.fname == "push") {
			if (ca.args.size() == 2 && is_arraytype(ca.args[0].type) && basetype(ca.args[0].type) == ca.args[1].type)  return 1;
			throw error("incorrect arguments");
		}
		else if (ca.fname == "pop") {
			if (ca.args.size() == 1 && is_arraytype(ca.args[0].type))  return 1;
			throw error("incorrect argument");
		}
		else if (ca.fname == "len") {
			if (ca.args.size() == 1 && (is_arraytype(ca.args[0].type) || ca.args[0].type == "string"))  return 1;
			throw error("incorrect argument");
		}
		else if (ca.fname == "default") {
			if (ca.args.size() == 1 && ca.args[0].type != "int")  return 1;
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
		else if (peek("@literal"))
			t = p_literal(),
			eptr(ex).instr.push_back({ "lit", t }),
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

	int p_literal() {
		require("@literal");
		auto lit = Strings::deliteral( lastrule.at(0) );
		// de-duplicate
		for (int i = 0; i < prog.literals.size(); i++)
			if (prog.literals[i] == lit)  return i;
		prog.literals.push_back(lit);
		return prog.literals.size() - 1;
	}



// --- Show results ---

	void show() const {
		printf("<module>\n");
		printf("%s%s\n", ind(1), prog.module.c_str() );
		printf("\n");
		printf("<literals>\n");
		for (int i = 0; i < prog.literals.size(); i++)  show(prog.literals.at(i), i, 1);
		printf("\n");
		printf("<types>\n");
		for (auto& t : prog.types)  show(t, 1);
		printf("\n");
		printf("<globals>\n");
		for (auto& g : prog.globals)  show(g, 1);
		// printf("\n");
		// printf("<blocks>\n");
		// for (int i = 0; i < prog.blocks.size(); i++)  show(prog.blocks.at(i), i, 0);
		printf("\n");
		printf("<functions>\n");
		for (auto& fn : prog.functions)  show(fn, 0);
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

	void show(const Prog::Function& fn, int id) const {
		printf("%sfunction %s\n", ind(id), fn.name.c_str() );
		show(prog.blocks.at(fn.block), fn.block, id+1);
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
			printf("%sexpr\n", ind(id+1) );
				show( prog.exprs.at(l.expr), id+2 );
		// printf("%s-->\n", ind(id+1) );
	}

	void show(const Prog::Print& pr, int id) const {
		printf("%sprint\n", ind(id) );
		for (auto& in : pr.instr) {
			printf("%s%s\n", ind(id+1), in.first.c_str() );
			if      (in.first == "literal")   show( prog.literals.at(stoi(in.second)), id+2 );
			else if (in.first == "expr")      show( prog.exprs.at(stoi(in.second)), id+2 );
			else if (in.first == "expr_str")  show( prog.exprs.at(stoi(in.second)), id+2 );
			else    printf("%s?? (%s)\n", ind(id+2), in.first.c_str() );
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
		printf("%scall %s\n", ind(id), ca.fname.c_str() );
		for (auto& arg : ca.args) {
			printf("%sexpr %s\n", ind(id+1), arg.type.c_str() );
			show( prog.exprs.at(arg.expr), id+2 );
			// printf("    ---\n");
		}
	}

	const char* ind(int id) const {
		static string s;
		return s = string(id*3, ' '),  s.c_str();
	}
};