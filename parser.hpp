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
		for (auto& g : prog.globals)
			if (g.name == name)  return 1;
		return 0;
	}
	int is_local(const string& name) const {
		if (flag_func <= -1)  return 0;
		auto& fn = prog.functions.at(flag_func);
		for (auto& a : fn.args)
			if (a.name == name)  return 1;
		for (auto& l : fn.locals)
			if (l.name == name)  return 1;
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
		return { name, type, .expr=-1, .dsym=lineno() };
	}

	void p_dim_global() {
		auto dim = p_dim_start();
		while (true) {
			if (is_global(dim.name))
				throw error("global redefined", dim.name);
			if (expect("="))
				dim.expr = p_expr(dim.type);
			prog.globals.push_back(dim);
			if (!expect(","))  // comma seperated defines
				break;
			require("@identifier");
			dim.name = lastrule.at(0),  dim.expr = -1;
		}
		require("@endl"), nextline();
	}

	void p_dim_local() {
		auto dim = p_dim_start();
		while (true) {
			if (is_local(dim.name))
				throw error("local redefined", dim.name);
			if (expect("="))
				dim.expr = p_expr(dim.type);
			prog.functions.at(flag_func).locals.push_back(dim);
			if (!expect(","))  // comma seperated defines
				break;
			require("@identifier");
			dim.name = lastrule.at(0),  dim.expr = -1;
		}
		require("@endl"), nextline();
	}

	void p_function() {
		// function header start
		require("function @identifier (");
		auto fname = lastrule.at(0);
		if (Tokens::is_keyword(fname) || is_global(fname) || is_func(fname))
			throw error("function name collision", fname);
		prog.functions.push_back({ fname });
		flag_func = prog.functions.size() - 1;
		auto& fn  = prog.functions.back();
		fn.dsym   = lineno();
		// function arguments
		while (!eol()) {
			if (!expect("@identifier @identifier"))  break;  // expect short dim format
			fn.args.push_back({ lastrule.at(1), lastrule.at(0) });  // save argument
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
		fn.block = p_block();
		// end
		require("end function @endl"), nextline();
		flag_func = -1;
	}



// --- Block parsing ---

	int p_block() {
		prog.blocks.push_back({ });
		int   blp = prog.blocks.size() - 1;
		auto& stm = prog.blocks.back().statements;
		while (!eof())
			if      (expect("@endl"))         nextline();
			else if (peek("end"))             break;  // end all control blocks
			else if (peek("else"))            break;  // end if-sub-block
			// I/O
			else if (peek("print"))           stm.push_back({ "print",      p_print() });
			else if (peek("input"))           stm.push_back({ "input",      p_input() });
			// control blocks
			else if (peek("if"))              stm.push_back({ "if",         p_if() });
			else if (peek("while"))           stm.push_back({ "while",      p_while() });
			else if (peek("for"))             stm.push_back({ "for",        p_for() });
			// control
			else if (peek("return"))          stm.push_back({ "return",     p_return() });
			else if (peek("break"))           stm.push_back({ "break",      p_break() });
			else if (peek("continue"))        stm.push_back({ "continue",   p_continue() });
			// expressions
			else if (peek("let"))             stm.push_back({ "let",        p_let() });
			else if (peek("call"))            stm.push_back({ "call",       p_call_stmt() });
			else if (peek("@identifier ("))   stm.push_back({ "call",       p_call_stmt() });
			else if (peek("@identifier"))     stm.push_back({ "let",        p_let() });
			else    throw error("unexpected block statement", currenttoken());
		return blp;
	}
	// block helpers
	// void add_stmt(int ptr, const Prog::Statement& st) { prog.blocks.at(ptr).statements.push_back(st); }
	// Prog::Block& bptr(int ptr) { return prog.blocks.at(ptr); }
	// void p_block0() {
	// 	require("block @endl");
	// 	p_block();
	// 	require("end block @endl"), nextline();
	// }



// --- Block statements ---

	int p_print() {
		require("print");
		prog.prints.push_back({ });
		int   prp = prog.prints.size() - 1;
		auto& pr  = prog.prints.back();
		while (!eol()) {
			int   exp = p_expr_any();
			auto& ex  = prog.exprs.at(exp);
			if      (ex.type == "int")     pr.instr.push_back({ "expr",     exp });
			else if (ex.type == "string")  pr.instr.push_back({ "expr_str", exp });
			else    throw error("unexpected type in print", ex.type);
			// whitespace seperators
			if      (expect(","))  pr.instr.push_back({ "literal", p_addliteral(" ")  });
			// else if (expect(";"))  pr.instr.push_back({ "literal", p_addliteral("\t") });
		}
		require("@endl"), nextline();
		return prp;
	}

	int p_input() {
		require("input");
		prog.inputs.push_back({ "> " });
		int   inp = prog.inputs.size() - 1;
		auto& in  = prog.inputs.back();
		// TODO: string expression in prompt
		if (expect("@literal"))
			in.prompt = Strings::deliteral( lastrule.at(0) ),
			require(",");
		in.varpath = p_varpath("string");
		require("@endl"), nextline();
		return inp;
	}

	int p_if() {
		require("if");
		prog.ifs.push_back({ });
		int   iip = prog.ifs.size() - 1;
		auto& ii  = prog.ifs.back();
		// if condition
		ii.conds.push_back({ }),
			ii.conds.back().expr = p_expr("int"),
			require("@endl"), nextline(),
			ii.conds.back().block = p_block();
		// else-if condition
		while (expect("else if"))
			ii.conds.push_back({ }),
			ii.conds.back().expr = p_expr("int"),
			require("@endl"), nextline(),
			ii.conds.back().block = p_block();
		// else condition
		if (expect("else"))
			require("@endl"), nextline(),
			ii.conds.push_back({ }),
			ii.conds.back().expr  = -1,  // empty condition - always run
			ii.conds.back().block = p_block();
		// end if
		require("end if @endl"), nextline();
		return iip;
	}
	// if helpers
	// void             add_cond (int ptr) { prog.ifs.at(ptr).conds.push_back({ -1, -1 }); }
	// Prog::Condition& last_cond(int ptr) { auto& c = prog.ifs.at(ptr).conds;  return c.at(c.size() - 1); }

	int p_while() {
		require("while");
		prog.whiles.push_back({ });
		int   whp = prog.whiles.size() - 1;
		auto& wh  = prog.whiles.back();
		flag_loop++;                              // increment loop control flag (for tracking valid break / continue)
		wh.expr = p_expr("int");                  // while-true condition
		require("@endl"), nextline();
		wh.block = p_block();                     // main block
		require("end while @endl"), nextline();   // block end
		flag_loop--;                              // decrement loop control flag
		return whp;
	}

	int p_for() {
		require("for");
		prog.fors.push_back({ });
		int   fop = prog.fors.size() - 1;
		auto& fo  = prog.fors.back();
		flag_loop++;
		// for condition
		fo.varpath    = p_varpath("int");
		require("=");
		fo.start_expr = p_expr("int");
		require("to");
		fo.end_expr   = p_expr("int");
		// step (optional - fixed value)
		fo.step = 1;  // default step
		if (expect("step"))
			fo.step = p_integer();
		require("@endl"), nextline();
		// block
		fo.block      = p_block();
		// block end
		require("end for @endl"), nextline();
		flag_loop--;
		return fop;
	}

	int p_return() {
		require("return");
		int ex = -1;  // default: no expression
		if (!peek("@endl"))  ex = p_expr("int");
		require("@endl"), nextline();
		return ex;
	}

	int p_break   () { require("break");     return p_break_level(); }
	int p_continue() { require("continue");  return p_break_level(); }
	int p_break_level() {
		if (!flag_loop)
			throw error("break/continue outside of loop");
		int level = 1;
		if (expect("@integer")) {
			level = stoi(lastrule.at(0));
			if (level < 1 || level > flag_loop)
				throw error("break/continue level out-of-bounds", lastrule.at(0));
		}
		require("@endl"), nextline();
		return level;
	}

	int p_let() {
		expect("let");                                          // keyword is optional
		prog.lets.push_back({ "<NULL>", -1, -1 });
		int   letp  = prog.lets.size() - 1;
		auto& let   = prog.lets.back();
		let.varpath = p_varpath_any();                          // parse dest varpath
		let.type    = prog.varpaths.at(let.varpath).type;       // get varpath type
		require("=");
		let.expr    = p_expr(let.type);                         // parse src varpath as type
		require("@endl"), nextline();
		return letp;
	}



// --- Variable path handling ---

	int p_varpath(const string& type) {
		int vpp = p_varpath_any();
		const auto& t = prog.varpaths.at(vpp).type;
		if (type != t)
			throw error("unexpected type in varpath (expected " + type + ")", t);
		return vpp;
	}

	int p_varpath_any() {
		require("@identifier");
		prog.varpaths.push_back({ "<NULL>" });
		int   vpp  = prog.varpaths.size() - 1;
		auto& vp   = prog.varpaths.back();
		auto& inst = prog.varpaths.back().instr;
		string  type,  prop,  varname = lastrule.at(0);
		if (is_local(varname))
			type = getlocaltype(varname),
			inst.push_back({ "get", 0, varname });
		else
			type = getglobaltype(varname),
			inst.push_back({ "get_global", 0, varname });
		// path chain
		while (!eol())
			if (expect("[")) {
				inst.push_back({ "memget_expr", p_expr("int") });
				require("]");
				if (!Tokens::is_arraytype(type))
					throw error("expected array type", type);
				type = Tokens::basetype(type);
			}
			else if (expect(".")) {
				require("@identifier"),  prop = lastrule.at(0);
				inst.push_back({ "memget_prop", 0, "USRTYPE_" + type + "_" + prop });
				type = getproptype(type, prop);
			}
			else
				break;
		vp.type = type;  // save type
		return vpp;
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

	int p_call_stmt() {
		expect("call");  // optional keyword
		int cap = p_call();
		require("@endl"), nextline();
		return cap;
	}

	int p_call() {
		require("@identifier (");
		string fname = lastrule.at(0);
		prog.calls.push_back({ fname });
		int   cap = prog.calls.size() - 1;
		auto& ca  = prog.calls.back();
		ca.dsym   = lineno();
		// arguments
		while (!eol() && !peek(")")) {
			int ex = p_expr_any();
			ca.args.push_back({ prog.exprs.at(ex).type, ex });
			peek(")") || require(",");
		}
		require(")");
		return cap;
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

	int p_expr(const string& type) {
		int exp = p_expr_any();
		const auto& t = prog.exprs.at(exp).type;
		if (type != t)
			throw error("unexpected type in expression (expected " + type + ")", t);
		return exp;
	}

	int p_expr_any() {
		prog.exprs.push_back({ "<NULL>" });
		int   exp = prog.exprs.size() - 1;
		auto& ex  = prog.exprs.back();
		p_expr_compare(ex);
		return exp;
	}
	
	void p_expr_compare(Prog::Expr& ex) {
		p_expr_add(ex);
		auto type = ex.type;  // cache expression type
		if (expect("`= `=") || expect("`! `=")) {
			if (type != "int" && type != "string")    throw error("cannot compare type", type);
			string opcode, op = Strings::join(lastrule, "");
			// figure out the proper opcode using type + operator
			if      (type == "int" && op == "==")  opcode = "eq";
			else if (type == "int" && op == "!=")  opcode = "neq";
			else  throw error("cannot do comparison on type", type + " : " + op);
			// printf("opcode: %s  %s\n", op.c_str(), opcode.c_str() );
			p_expr_add(ex);
			if (type != ex.type)                      throw error("compare type mismatch");
			ex.instr.push_back({ opcode });
		}
	}

	void p_expr_add(Prog::Expr& ex) {
		p_expr_mul(ex);
		auto type = ex.type;  // cache expression type
		while (expect("+") || expect("-")) {
			if (type != "int" && type != "string")    throw error("cannot add type", type);
			string op = lasttok;
			p_expr_mul(ex);
			if      (type != ex.type)                 throw error("add type mismatch");
			else if (type == "int"    && op == "+")   ex.instr.push_back({ "add" });
			else if (type == "int"    && op == "-")   ex.instr.push_back({ "sub" });
			else if (type == "string" && op == "+")   ex.instr.push_back({ "strcat" });
			else if (type == "string" && op == "-")   throw error("cannot subtract strings");
			else    throw error("unexpected add expression");
		}
	}

	void p_expr_mul(Prog::Expr& ex) {
		p_expr_atom(ex);
		auto type = ex.type;  // cache expression type
		while (expect("*") || expect("/")) {
			if (type != "int")                        throw error("multiply expected int", type);
			string op = lasttok;
			p_expr_atom(ex);
			if      (ex.type != "int")                throw error("multiply expected int", type);
			else if (op == "*")                       ex.instr.push_back({ "mul" });
			else if (op == "/")                       ex.instr.push_back({ "div" });
		}
	}

	void p_expr_atom(Prog::Expr& ex) {
		if (peek("@sign") || peek("@integer"))
			ex.instr.push_back({ "i",     p_integer() }),
			ex.type = "int";
		else if (expect("true") || expect("false"))
			ex.instr.push_back({ "i",     lasttok == "true" }),
			ex.type = "int";
		else if (peek("@literal"))
			ex.instr.push_back({ "lit",   p_literal() }),
			ex.type = "string";
		else if (peek("@identifier ("))
			ex.instr.push_back({ "call",  p_call() }),
			ex.type = "int";
		else if (peek("@identifier")) {
			int vpp = p_varpath_any();
			ex.type = prog.varpaths.at(vpp).type;
			if      (ex.type == "int")     ex.instr.push_back({ "varpath",     vpp });
			else if (ex.type == "string")  ex.instr.push_back({ "varpath_str", vpp });
			else    ex.instr.push_back({ "varpath_ptr", vpp });
		}
		else if (expect("("))
			p_expr_compare(ex),
			require(")");
		else
			throw error("expected atom");
	}

	int32_t p_integer() {
		expect("@sign @integer") || require("@integer");
		return stoi( Strings::join(lastrule, "") );
	}

	// double p_realnum() {
	// 	expect("@sign @integer . @integer")
	// 		|| expect("@integer . @integer")
	// 		|| expect("@sign @integer")
	// 		|| require("@integer");
	// 	return stod( Strings::join(lastrule, "") );
	// }

	int p_literal() {
		require("@literal");
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