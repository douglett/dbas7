// ----------------------------------------
// Program runtime
// ----------------------------------------
#pragma once
#include <vector>
#include <map>
#include <stdexcept>
#include <cassert>
using namespace std;



struct Runtime {
	// structs
	struct MemPage { string type; vector<int32_t> mem; };
	struct MemPtr  { int32_t ptr, off; string v; };
	struct Var     { string type; int32_t v; };
	typedef  map<string, Var>  StackFrame;
	// state
	map<string, int32_t>           consts;
	map<int32_t, MemPage>          heap;
	StackFrame                     globals;
	vector<StackFrame>             fstack;
	vector<int32_t>                istack;  // expression stack
	vector<string>                 sstack;  // string expression stack
	int32_t memtop = 0;
	// program source
	Prog prog;



// --- Helpers ---

	int32_t getnum(const string& num) const {
		try                         { return stoi(num); }
		catch (invalid_argument& e) { return consts.at(num); }
	}
	int typeindex(const string& name) const {
		for (int i = 0; i < prog.types.size(); i++)
			if (prog.types[i].name == name)  return i;
		return -1;
	}
	const Prog::Type& gettype(const string& name) const {
		if (typeindex(name) == -1)  throw runtime_error("missing type: " + name);
		return prog.types[typeindex(name)];
	}
	int funcindex(const string& name) const {
		for (int i = 0; i < prog.functions.size(); i++)
			if (prog.functions[i].name == name)  return i;
		return -1;
	}
	const Prog::Function getfunc(const string& name) const {
		for (auto& fn : prog.functions)
			if (fn.name == name)  return fn;
		throw runtime_error("missing function: " + name);
	}



// --- Main memory ---

	StackFrame& ftop() {
		return fstack.at(fstack.size() - 1);
	}
	int32_t& get(string id) {
		return ftop().at(id).v;
	}
	int32_t& get_global(string id) {
		return globals.at(id).v;
	}
	int32_t& memget(int32_t ptr, int32_t off) {
		return heap.at(ptr).mem.at(off);
	}
	int32_t memsize(int32_t ptr) const {
		return heap.at(ptr).mem.size();
	}
	// void memresize(int32_t ptr, int32_t size) const {
	// 	heap.at(ptr).mem.resize()
	// }
	// void mempush(int32_t ptr, )

	// heap memory make
	int32_t memalloc(string type, int32_t size) {
		heap[++memtop] = { .type=type, .mem=vector<int32_t>(size, 0) };
		return memtop;
	}
	int32_t make(const string& type) {
		if      (type == "int")               return 0;
		else if (type == "string")            return memalloc("string", 0);
		else if (Tokens::is_arraytype(type))  return memalloc(type, 0);
		else if (typeindex(type) > -1) {
			auto& t = gettype(type);
			int32_t off = 0,  ptr = memalloc( type, t.members.size() );
			for (auto& m : t.members)
				memget(ptr, off++) = make(m.type);
			return ptr;
		}
		else    throw runtime_error("make: unknown type: " + type);
	}
	int32_t make_str(const string& val) {
		int ptr = memalloc("string", 0);
		heap.at(ptr).mem.insert( heap.at(ptr).mem.end(), val.begin(), val.end() );
		return ptr;
	}

	// heap memory clone
	int32_t clone(int32_t sptr) {
		int32_t dptr = memalloc(heap.at(sptr).type, 0);
		_clone(sptr, dptr);
		return dptr;
	}
	void cloneto(int32_t sptr, int32_t dptr) {
		unmake(dptr);
		_clone(sptr, dptr);
	}
	void clonestr(string s, int32_t dptr) {
		assert(heap.at(dptr).type == "string");
		heap.at(dptr).mem = {};
		heap.at(dptr).mem.insert( heap.at(dptr).mem.end(), s.begin(), s.end() );
	}
	void _clone(int32_t sptr, int32_t dptr) {
		// TODO: is this memory safe?
		auto& spage = heap.at(sptr);
		auto& dpage = heap.at(dptr);
		assert(dpage.mem.size() > 0);
		// linear memory
		if (spage.type == "string" || spage.type == "int[]")
			dpage.mem = spage.mem;
		// objects
		else if (typeindex(spage.type) > -1) {
			auto& t = gettype(spage.type);
			assert(spage.mem.size() == t.members.size());
			dpage.mem.resize(spage.mem.size(), 0);
			for (int i = 0; i < t.members.size(); i++)
				if    (t.members[i].type == "int")  dpage.mem[i] = spage.mem[i];
				else  dpage.mem[i] = clone(spage.mem[i]);
		}
		// arrays
		else if (Tokens::is_arraytype(spage.type)) {
			dpage.mem.resize(spage.mem.size(), 0);
			for (int i = 0; i < spage.mem.size(); i++)
				dpage.mem[i] = clone(spage.mem[i]);
		}
		else    throw runtime_error("clone: unknown type: " + spage.type);
	}

	// heap memory erase
	void destroy(int32_t ptr) {
		unmake(ptr);
		heap.erase(ptr);
	}
	void unmake(int32_t ptr) {
		auto& page = heap.at(ptr);
		// printf("unmaking %s\n", page.type.c_str() );
		if (page.type == "int[]" || page.type == "string") ;
		else if (typeindex(page.type) > -1) {
			auto& t = gettype(page.type);
			for (int i = 0; i < t.members.size(); i++)
				if (t.members[i].type != "int")
					destroy(page.mem.at(i));
		}
		else if (Tokens::is_arraytype(page.type))
			for (auto p : page.mem)
				destroy(p);
		else  throw runtime_error("unmake: unknown type: " + page.type);
		page.mem = {};
	}
	void unmake_default(int ptr) {
		unmake(ptr);
		int p = make(heap.at(ptr).type);  // new default object
		heap.at(ptr).mem = heap.at(p).mem;  // copy default values
		heap.erase(p);  // remove old object
	}



// --- Main runtime ---

	int32_t run() {
		init();
		// run blocks in order
		// for (auto& bl : prog.blocks)
		// 	block(bl);
		// return function("main");

		// TODO: internal call
		return call({ "main" });
	}
	void init() {
		for (auto& t : prog.types)    init_type(t);
		for (auto& d : prog.globals)  init_dim(d);
	}
	void init_type(const Prog::Type& t) {
		for (int i = 0; i < t.members.size(); i++)
			consts["USRTYPE_" + t.name + "_" + t.members[i].name] = i;
	}
	void init_dim(const Prog::Dim& d) {
		globals[d.name] = { d.type, make(d.type) };
	}


	// run statements
	void block(int bl) { return block(prog.blocks.at(bl)); }
	void block(const Prog::Block& bl) {
		for (auto& st : bl.statements)
			if      (st.type == "let")    let(st.loc);
			else if (st.type == "print")  print(st.loc);
			else if (st.type == "call")   call(st.loc);
			else    throw runtime_error("unknown statement: " + st.type);
	}
	void let(int l) { return let(prog.lets.at(l)); }
	void let(const Prog::Let& l) {
		int32_t  ex = expr(l.expr);
		int32_t& vp = varpath(l.varpath);
		if      (l.type == "int")     vp = ex;
		else if (l.type == "string")  clonestr(spop(), vp);
		else if (vp != ex)            cloneto(ex, vp);
	}
	void print(int pr) { return print(prog.prints.at(pr)); }
	void print(const Prog::Print& pr) {
		for (auto& in : pr.instr)
			if      (in.first == "literal")   printf("%s ", Strings::deliteral(in.second).c_str() );
			else if (in.first == "expr")      printf("%d ", expr(getnum(in.second)) );
			else if (in.first == "expr_str")  expr(getnum(in.second)),  printf("%s ", spop().c_str() );
			else    throw runtime_error("unknown print: " + in.first);
		printf("\n");
	}


	// function calls
	int32_t call(int ptr) { return call(prog.calls.at(ptr)); }
	int32_t call(const Prog::Call& ca) {
		// if not user function, run internal function
		if (funcindex(ca.fname) == -1)
			return call_system(ca);
		// run user function
		auto& fn = getfunc(ca.fname);
		// calculate arguments in current frame context
		StackFrame newframe;
		assert(fn.args.size() == ca.args.size());
		for (int i = 0; i < fn.args.size(); i++) {
			assert(fn.args[i].type == ca.args[i].type);  // basic errors
			if (fn.args[i].type == "string")
				printf("WARNING: problem with string arguments\n");
			newframe[fn.args[i].name] = { fn.args[i].type, expr(ca.args[i].expr) };  // run argument expression
		}
		// push new frame and calculate locals
		fstack.push_back(newframe);  
		for (auto& d : fn.locals)
			ftop()[d.name] = { d.type, make(d.type) };
		// run main block
		block(fn.block);
		// destroy local variables only in frame
		for (auto& d : fn.locals)
			if (d.type != "int")  destroy( get(d.name) );
		fstack.pop_back();
		return 0;
	}
	int32_t call_system(const Prog::Call& ca) {
		// push array
		if (ca.fname == "push") {
			int32_t t = 0,  arrptr = expr(ca.args.at(0).expr),  val = expr(ca.args.at(1).expr);
			auto&   av     = ca.args.at(1);
			if      (av.type == "int")     heap.at(arrptr).mem.push_back(val);
			else if (av.type == "string")  t = make_str(spop()),  heap.at(arrptr).mem.push_back(t);
			else    t = clone(val),  heap.at(arrptr).mem.push_back(t);
			return 0;
		}
		// pop array
		else if (ca.fname == "pop") {
			int32_t arrptr = expr(ca.args.at(0).expr);
			auto&   mem    = heap.at(arrptr).mem;
			int32_t val    = mem.at(mem.size() - 1);  // save the value we're popping
			if (ca.args.at(0).type != "int[]")  destroy(val);
			mem.pop_back();
			return val;  // return it
		}
		// array length
		else if (ca.fname == "len") {
			int32_t arrptr = expr(ca.args.at(0).expr);
			if (ca.args.at(0).type == "string")  return spop().size();
			else  return heap.at(arrptr).mem.size();
		}
		// reset memory to default
		else if (ca.fname == "default") {
			int32_t ptr = expr(ca.args.at(0).expr);
			unmake_default(ptr);
			return 0;
		}
		else  throw runtime_error("unknown function: " + ca.fname);
	}


	// variable path parsing
	int32_t& varpath(int vp) { return varpath(prog.varpaths.at(vp)); }
	int32_t& varpath(const Prog::VarPath& vp) {
		int32_t* ptr = NULL;
		for (auto& in : vp.instr) {
			// auto cmd = Strings::split(in);
			if      (in.cmd == "get")          ptr = &get(in.sarg);
			else if (in.cmd == "get_global")   ptr = &get_global(in.sarg);
			else if (ptr == NULL)              goto err;
			// else if (cmd.at(0) == "memget")       ptr = &memget(*ptr, getnum(cmd.at(1)) );
			else if (in.cmd == "memget_expr")  ptr = &memget(*ptr, expr(in.iarg) );
			else if (in.cmd == "memget_prop")  ptr = &memget(*ptr, getnum(in.sarg) );
			else    throw runtime_error("unknown varpath: " + in.cmd);
		}
		if (ptr == NULL)  goto err;
		return *ptr;
		err:  throw out_of_range("memget ptr is null");
	}
	string varpath_str(int vp) {
		const auto& mem = heap.at( varpath(vp) ).mem;
		return string(mem.begin(), mem.end());
	}


	// expression parsing
	int32_t expr(int ex) { return expr(prog.exprs.at(ex)); }
	int32_t expr(const Prog::Expr& ex) {
		// istack = {}, sstack = {};
		int32_t t = 0;
		string s;
		for (auto& in : ex.instr)
			// integers
			if      (in.cmd == "i")            ipush(in.iarg);
			else if (in.cmd == "varpath")      ipush( varpath(in.iarg) );
			else if (in.cmd == "add")          t = ipop(),  ipeek() += t;
			else if (in.cmd == "sub")          t = ipop(),  ipeek() -= t;
			// strings
			else if (in.cmd == "lit")          spush(in.iarg);
			else if (in.cmd == "varpath_str")  spush(varpath_str(in.iarg));
			else if (in.cmd == "strcat")       s = spop(),  speek() += s;
			// other
			else if (in.cmd == "varpath_ptr")  ipush(varpath(in.iarg));
			else if (in.cmd == "call")         ipush(call(in.iarg));
			else    throw runtime_error("unknown expr: " + in.cmd);
		// sanity check
		if (istack.size() + sstack.size() != 1)  printf("WARNING: odd expression results\n");
		return istack.size() ? ipop() : 0;
	}
	// string expr_str(int ex) {
	// 	expr(ex);
	// 	return spop();
	// }
	// expression stack operations
	int32_t& ipeek() { return istack.at(istack.size() - 1); }
	int32_t  ipop () { auto t = istack.at(istack.size() - 1);  istack.pop_back();  return t; }
	void     ipush(int32_t t) { istack.push_back(t); }
	string&  speek() { return sstack.at(sstack.size() - 1); }
	string   spop () { auto t = sstack.at(sstack.size() - 1);  sstack.pop_back();  return t; }
	void     spush(const string& t) { sstack.push_back(t); }
	void     spush(int loc) { sstack.push_back( prog.literals.at(loc) ); }



// --- Show state ---

	void show() {
		// printf("  heap:  %d\n", heap.size() );
		// printf("  stack:  i.%d  s.%d\n", istack.size(), sstack.size() );
		printf("  heap %d | istack %d | sstack %d\n", (int)heap.size(), (int)istack.size(), (int)sstack.size() );
		printf("  consts:\n");
		for (auto& c : consts)
			printf("    %s  %d\n", c.first.c_str(), c.second );
		printf("  globals:\n");
		for (auto& g : globals)
			printf("    %-10s  %d\n", g.first.c_str(), g.second.v );
	}
};