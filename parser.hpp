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
	int flag_mod = 0, flag_func = -1, flag_loop = 0;



// --- State checking ---

	int is_type(const string& type) const {
		if (type == "int" || type == "string")  return 1;
		for (auto& t : prog.types)
			if (t.name == type)  return 1;
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
		for (int i = 0; i < fn.args.size(); i++)
			if (fn.args[i].name == name)  return 1;
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
		static vector<string> fn_system = { "push", "pop", "len", "default" };
		for (auto& n : fn_system)
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
		string ctype = lastrule.at(0);
		if (Tokens::is_keyword(ctype) || is_type(ctype) || is_global(ctype))
			throw error("type name collision", ctype);
		prog.types.push_back({ ctype });
		// type members
		while (!eof()) {	
			if      (expect("@endl"))  { nextline();  continue; }
			else if (!peek("dim"))     { break; }
			Prog::Dim d = p_dim_start();
			if (is_member(ctype, d.name))
				throw error("type member collision", d.type + ":" + d.name);
			prog.types.back().members.push_back(d);  // save type member
			require("@endl"), nextline();  // next member
		}
		require("end type @endl"), nextline();
	}

	Prog::Dim p_dim_start() {
		string type, btype, name;
		if      (expect ("dim @identifier @identifier"))      type = btype = lastrule.at(0),  name = lastrule.at(1);
		else if (expect ("dim @identifier [ ] @identifier"))  btype = lastrule.at(0),  type = btype + "[]",  name = lastrule.at(1);
		else if (require("dim @identifier"))                  type = btype = "int",  name = lastrule.at(0);
		if (Tokens::is_keyword(name) || is_type(name) || !is_type(btype))
			throw error("dim collision", type + ":" + name);
		return { name, type };
	}

	void p_dim_global() {
		auto dim = p_dim_start();
		if (is_global(dim.name))
			throw error("global redefined", dim.name);
		prog.globals.push_back(dim);
		// TODO: assign here
		require("@endl"), nextline();
	}

	void p_dim_local() {
		auto dim = p_dim_start();
		if (is_local(dim.name))
			throw error("local redefined", dim.name);
		prog.functions.at(flag_func).locals.push_back(dim);
		// TODO: assign here
		require("@endl"), nextline();
	}

	void p_function() {
		// function header start
		require("function @identifier (");
		auto fname = lastrule.at(0);
		if (Tokens::is_keyword(fname) || is_global(fname) || is_func(fname))
			throw error("function name collision", fname);
		prog.functions.push_back({ fname });
		int fn = prog.functions.size() - 1;
		prog.functions.at(fn).dsym = lineno();
		flag_func = fn;
		// function arguments
		while (!eol()) {
			if (!expect("@identifier @identifier"))  break;  // expect short dim format
			prog.functions.at(fn).args.push_back({ lastrule.at(1), lastrule.at(0) });  // save argument
			if (!expect(","))  break;  // comma seperated arguments
		}
		// function header end
		require(") @endl"), nextline();
		// locals
		while (!eof())
			if      (expect("@endl"))  nextline();
			else if (peek("dim"))      p_dim_local();
			else    break;
		// parse main block
		prog.functions.at(fn).block = p_block();
		// end
		require("end function @endl"), nextline();
		flag_func = -1;
	}



// --- Block parsing ---

	int p_block() {
		prog.blocks.push_back({});
		int bl = prog.blocks.size() - 1;
		while (!eof())
			if      (expect("@endl"))         nextline();
			else if (peek("end"))             break;  // end all control blocks
			else if (peek("else"))            break;  // end if-sub-block
			// I/O
			else if (peek("print"))           add_stmt(bl, { "print",      p_print() });
			else if (peek("input"))           add_stmt(bl, { "input",      p_input() });
			// control blocks
			else if (peek("if"))              add_stmt(bl, { "if",         p_if() });
			else if (peek("while"))           add_stmt(bl, { "while",      p_while() });
			// else if (peek("for"))        p_for();
			// control
			else if (peek("return"))          add_stmt(bl, { "return",     p_return() });
			else if (peek("break"))           add_stmt(bl, { "break",      p_break() });
			// else if (peek("continue"))        add_stmt(bl, { "continue",   p_continue();
			// expressions
			else if (peek("let"))             add_stmt(bl, { "let",        p_let() });
			else if (peek("call"))            add_stmt(bl, { "call",       p_call_stmt() });
			else if (peek("@identifier ("))   add_stmt(bl, { "call",       p_call_stmt() });
			else if (peek("@identifier"))     add_stmt(bl, { "let",        p_let() });
			else    throw error("unexpected block statement", currenttoken());
		return bl;
	}
	// block helpers
	void add_stmt(int ptr, const Prog::Statement& st) { prog.blocks.at(ptr).statements.push_back(st); }
	// Prog::Block& bptr(int ptr) { return prog.blocks.at(ptr); }
	// void p_block0() {
	// 	require("block @endl");
	// 	p_block();
	// 	require("end block @endl"), nextline();
	// }



// --- Block statements ---

	int p_print() {
		require("print");
		prog.prints.push_back({});
		int pr = prog.prints.size() - 1;
		while (!eol()) {
			int ex = p_expr();
			if      (eptr(ex).type == "int")     prog.prints.at(pr).instr.push_back({ "expr",     ex });
			else if (eptr(ex).type == "string")  prog.prints.at(pr).instr.push_back({ "expr_str", ex });
			else    throw error("unexpected type in print", eptr(ex).type);
			// whitespace seperators
			if      (expect(","))  prog.prints.at(pr).instr.push_back({ "literal", p_addliteral(" ")  });
			// else if (expect(";"))  prog.prints.at(pr).instr.push_back({ "literal", p_addliteral("\t") });
		}
		require("@endl"), nextline();
		return pr;
	}

	int p_input() {
		require("input");
		prog.inputs.push_back({ "> " });
		int in = prog.inputs.size()-1;
		// TODO: string expression in prompt
		if (expect("@literal"))
			prog.inputs.at(in).prompt = Strings::deliteral( lastrule.at(0) ),
			require(",");
		prog.inputs.at(in).varpath = p_varpath();
		require("@endl"), nextline();
		return in;
	}

	int p_if() {
		require("if");
		prog.ifs.push_back({ });
		int i = prog.ifs.size() - 1;
		// if condition
		add_cond(i),
			last_cond(i).expr = p_expr(),
			require("@endl"), nextline(),
			last_cond(i).block = p_block();
		// else-if condition
		while (expect("else if"))
			add_cond(i),
			last_cond(i).expr = p_expr(),
			require("@endl"), nextline(),
			last_cond(i).block = p_block();
		// else condition
		if (expect("else"))
			require("@endl"), nextline(),
			add_cond(i),
			last_cond(i).expr  = -1,  // empty condition - always run
			last_cond(i).block = p_block();
		// end if
		require("end if @endl"), nextline();
		return i;
	}
	// if helpers
	void             add_cond (int ptr) { prog.ifs.at(ptr).conds.push_back({ -1, -1 }); }
	Prog::Condition& last_cond(int ptr) { auto& c = prog.ifs.at(ptr).conds;  return c.at(c.size() - 1); }

	int p_while() {
		require("while");
		prog.whiles.push_back({ });
		int w = prog.whiles.size() - 1;
		flag_loop++;                              // increment loop control flag (for tracking valid break / continue)
		prog.whiles.at(w).expr = p_expr();        // while-true condition
		require("@endl"), nextline();
		prog.whiles.at(w).block = p_block();      // main block
		require("end while @endl"), nextline();   // block end
		flag_loop--;                              // decrement loop control flag
		return w;
	}

	int p_return() {
		require("return");
		int ex = -1;  // default: no expression
		if (!peek("@endl"))  ex = p_expr();
		require("@endl"), nextline();
		return ex;
	}

	int p_break() {
		require("break");
		if (!flag_loop)
			throw error("break outside of loop");
		int level = 1;
		if (expect("@integer")) {
			level = stoi(lastrule.at(0));
			if (level > flag_loop)
				throw error("break level out-of-bounds", lastrule.at(0));
		}
		require("@endl"), nextline();
		return level;
	}

	int p_let() {
		expect("let");                                          // keyword is optional
		prog.lets.push_back({ "<NULL>", -1, -1 });
		int let = prog.lets.size() - 1;
		int vp = prog.lets.at(let).varpath = p_varpath();       // dest varpath
		string dest_type = prog.lets.at(let).type = prog.varpaths.at(vp).type;
		require("=");
		prog.lets.at(let).expr = p_expr( dest_type );           // src varpath
		require("@endl"), nextline();
		return let;
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
		for (auto& d : fn.args)
			if (d.name == name)  return d.type;
		for (auto& d : fn.locals)
			if (d.name == name)  return d.type;
		throw error("undefined argument or local", name);
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
		prog.calls.at(ca).dsym = lineno();
		// arguments
		while (!eol() && !peek(")")) {
			int ex = p_expr();
			prog.calls.at(ca).args.push_back({ prog.exprs.at(ex).type, ex });
			peek(")") || require(",");
		}
		require(")");
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
		// check system call arguments (if system)
		if (p_callcheck_system(ca))  return 1;
		// check user function exists
		if (getfuncindex(ca.fname) == -1)
			throw error("function undefined, line " + to_string(ca.dsym), ca.fname);
		// check user call arguments
		auto& fn = prog.functions.at(getfuncindex(ca.fname));
		if (ca.args.size() != fn.args.size())
			throw error("incorrect argument count, line " + to_string(ca.dsym));
		for (int i = 0; i < ca.args.size(); i++)
			if (ca.args[i].type != fn.args[i].type)
				throw error(string("incorrect argument type.")
					+ " expected " + fn.args[i].type + "(" + to_string(i+1) + ")"
					+ ", got " + ca.args[i].type
					+ ", line " + to_string(ca.dsym));
		// OK 
		return 1;
	}
	int getfuncindex(const string& fname) const {
		for (int i = 0; i < prog.functions.size(); i++)
			if (prog.functions[i].name == fname)  return i;
		return -1;
	}

	int p_callcheck_system(const Prog::Call& ca) const {
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
		p_expr_compare(ex);
		return ex;
	}
	
	void p_expr_compare(int ex) {
		p_expr_add(ex);
		auto type = eptr(ex).type;
		if (expect("`= `=") || expect("`! `=")) {
			if (type != "int" && type != "string")    throw error("cannot compare type", type);
			string opcode, op = Strings::join(lastrule, "");
			// figure out thr proper opcode using type + operator
			if      (type == "int" && op == "==")  opcode = "eq";
			else if (type == "int" && op == "!=")  opcode = "neq";
			else  throw error("cannot do comparison on type", type + " : " + op);
			// printf("opcode: %s  %s\n", op.c_str(), opcode.c_str() );
			p_expr_add(ex);
			if (type != eptr(ex).type)                throw error("compare type mismatch");
			eptr(ex).instr.push_back({ opcode });
		}
	}

	void p_expr_add(int ex) {
		p_expr_atom(ex);
		auto type = eptr(ex).type;
		while (expect("+") || expect("-")) {
			if (type != "int" && type != "string")    throw error("cannot add type", type);
			string op = lasttok;
			p_expr_atom(ex);
			if      (type != eptr(ex).type)           throw error("add type mismatch");
			else if (type == "int"    && op == "+")   eptr(ex).instr.push_back({ "add" });
			else if (type == "int"    && op == "-")   eptr(ex).instr.push_back({ "sub" });
			else if (type == "string" && op == "+")   eptr(ex).instr.push_back({ "strcat" });
			else if (type == "string" && op == "-")   throw error("cannot subtract strings");
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
		// auto lit = Strings::deliteral( lastrule.at(0) );
		// // de-duplicate
		// for (int i = 0; i < prog.literals.size(); i++)
		// 	if (prog.literals[i] == lit)  return i;
		// prog.literals.push_back(lit);
		// return prog.literals.size() - 1;
		return p_addliteral( lastrule.at(0) );
	}

	// TODO: better name for this?
	int p_addliteral(const string& s) {
		auto lit = Strings::deliteral( s );
		// de-duplicate
		for (int i = 0; i < prog.literals.size(); i++)
			if (prog.literals[i] == lit)  return i;
		prog.literals.push_back(lit);
		return prog.literals.size() - 1;
	}

};  // end Parser