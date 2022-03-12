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
	typedef  int32_t  pos_t;
	// errors
	// struct DBRunError : runtime_error {};
	struct ctrl_exception : exception      { int32_t val = 0;  ctrl_exception(int32_t _val) : val(_val) {} };
	struct ctrl_return    : ctrl_exception { using ctrl_exception::ctrl_exception; };
	struct ctrl_break     : ctrl_exception { using ctrl_exception::ctrl_exception; };
	struct ctrl_continue  : ctrl_exception { using ctrl_exception::ctrl_exception; };
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
	pos_t typeindex(const string& name) const {
		for (size_t i = 0; i < prog.types.size(); i++)
			if (prog.types[i].name == name)  return i;
		return -1;
	}
	const Prog::Type& gettype(const string& name) const {
		if (typeindex(name) == -1)  throw runtime_error("missing type: " + name);
		return prog.types[typeindex(name)];
	}
	pos_t funcindex(const string& name) const {
		for (size_t i = 0; i < prog.functions.size(); i++)
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
		int32_t ptr = memalloc("string", 0);
		heap.at(ptr).mem.insert( heap.at(ptr).mem.end(), val.begin(), val.end() );
		return ptr;
	}

	// heap memory clone
	// TODO: clean up multiple clone functions
	int32_t clone2(const string& type, int32_t sptr, int32_t dptr=0) {
		assert(!(type == "int" && dptr != 0));
		if      (type == "int")  return sptr;  // raw int
		else if (dptr == 0)      dptr = memalloc(heap.at(sptr).type, 0);  // cloning to empty memory
		else                     unmake(dptr);  // cloning to existing memory
		_clone(sptr, dptr);  // perform clone
		return dptr;
	}

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
		// assert(dpage.mem.size() > 0);  // TODO: why?
		assert(spage.type == dpage.type);
		// linear memory
		if (spage.type == "string" || spage.type == "int[]")
			dpage.mem = spage.mem;
		// objects
		else if (typeindex(spage.type) > -1) {
			auto& t = gettype(spage.type);
			assert(spage.mem.size() == t.members.size());
			dpage.mem.resize(spage.mem.size(), 0);
			for (size_t i = 0; i < t.members.size(); i++)
				if    (t.members[i].type == "int")  dpage.mem[i] = spage.mem[i];
				else  dpage.mem[i] = clone(spage.mem[i]);
		}
		// arrays
		else if (Tokens::is_arraytype(spage.type)) {
			dpage.mem.resize(spage.mem.size(), 0);
			for (size_t i = 0; i < spage.mem.size(); i++)
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
			for (size_t i = 0; i < t.members.size(); i++)
				if (t.members[i].type != "int")
					destroy(page.mem.at(i));
		}
		else if (Tokens::is_arraytype(page.type))
			for (auto p : page.mem)
				destroy(p);
		else  throw runtime_error("unmake: unknown type: " + page.type);
		page.mem = {};
	}
	void unmake_default(int32_t ptr) {
		unmake(ptr);
		int32_t p = make(heap.at(ptr).type);  // new default object
		heap.at(ptr).mem = heap.at(p).mem;  // copy default values
		heap.erase(p);  // remove old object
	}



// --- Main runtime ---

	int32_t run() {
		init();
		// TODO: internal call
		return call({ "main" });
	}
	void init() {
		for (auto& t : prog.types)    init_type(t);
		for (auto& d : prog.globals)  init_dim(d);
	}
	void init_type(const Prog::Type& t) {
		for (size_t i = 0; i < t.members.size(); i++)
			consts["USRTYPE_" + t.name + "_" + t.members[i].name] = i;
	}
	void init_dim(const Prog::Dim& d) {
		auto& cframe = fstack.size() ? fstack.back() : globals;
		cframe[d.name] = { d.type, 0 };
		if (d.expr > -1 && d.type == "string")
			expr(d.expr),
			cframe[d.name].v = make_str( spop() );
		else if (d.expr > -1)
			cframe[d.name].v = clone2( d.type, expr(d.expr) );
		else
			cframe[d.name].v = make(d.type);
	}


	// run block
	void block(pos_t bptr) {
		const Prog::Block& bl = prog.blocks.at(bptr);
		for (auto& st : bl.statements)
			// I/O
			if      (st.type == "print")        r_print(st.loc);
			else if (st.type == "input")        r_input(st.loc);
			// control blocks
			else if (st.type == "if")           r_if(st.loc);
			else if (st.type == "while")        r_while(st.loc);
			else if (st.type == "for")          r_for(st.loc);
			// control
			else if (st.type == "return")       throw ctrl_return( st.loc > -1 ? expr(st.loc) : 0 );  // return (rval: expr OR default(0))
			else if (st.type == "break")        throw ctrl_break(st.loc);     // break loop (arg: break-level)
			else if (st.type == "continue")     throw ctrl_continue(st.loc);  // continue loop (arg: break-level)
			// expressions
			else if (st.type == "let")          let(st.loc);
			else if (st.type == "call")         call(st.loc);
			else    throw runtime_error("unknown statement: " + st.type);
	}


	// run block statements
	void r_print(pos_t ptr) {
		const Prog::Print& pr = prog.prints.at(ptr);
		for (auto& in : pr.instr)
			if      (in.cmd == "literal")   printf("%s", prog.literals.at(in.iarg).c_str() );
			else if (in.cmd == "expr")      printf("%d", expr(in.iarg) );
			else if (in.cmd == "expr_str")  expr(in.iarg),  printf("%s", spop().c_str() );
			else    throw runtime_error("unknown print: " + in.cmd);
		printf("\n");
	}
	void r_input(pos_t ptr) {
		const Prog::Input& in = prog.inputs.at(ptr);
		printf("%s", in.prompt.c_str() );
		string s;
		getline(cin, s);
		clonestr( s, varpath(in.varpath) );
	}
	void r_if(pos_t ptr) {
		const auto& ip = prog.ifs.at(ptr);
		for (auto& cond : ip.conds)
			// run block on empty OR truthy condition
			if (cond.expr == -1 || expr(cond.expr)) {
				block(cond.block);
				break;
			}
	}
	void r_while(pos_t ptr) {
		const auto& wh = prog.whiles.at(ptr);
		while ( expr(wh.expr) )
			try                        { block(wh.block); }
			catch (ctrl_continue& con) { if (--con.val > 0) throw con;  continue; }
			catch (ctrl_break&    brk) { if (--brk.val > 0) throw brk;  break; }
	}
	void r_for(pos_t ptr) {
		const auto& fo = prog.fors.at(ptr);
		varpath(fo.varpath) = expr(fo.start_expr);
		while (true) {
			if      (fo.step >= 0 && varpath(fo.varpath) > expr(fo.end_expr))  break;  // forward loop
			else if (fo.step <  0 && varpath(fo.varpath) < expr(fo.end_expr))  break;  // reverse loop
			try                        { block(fo.block); }
			catch (ctrl_continue& con) { if (--con.val > 0) throw con; }
			catch (ctrl_break&    brk) { if (--brk.val > 0) throw brk;  break; }
			varpath(fo.varpath) += fo.step;  // step
		}
	}
	void let(pos_t ptr) {
		const auto& l = prog.lets.at(ptr);
		int32_t& vp = varpath(l.varpath);  // TODO: execution order could cause bugs with arrays potentially
		int32_t  ex = expr(l.expr);
		if      (l.type == "int")     vp = ex;
		else if (l.type == "string")  clonestr(spop(), vp);
		else if (vp != ex)            cloneto(ex, vp);
	}


	// function calls
	int32_t call(pos_t ptr) { return call(prog.calls.at(ptr)); }
	int32_t call(const Prog::Call& ca) {
		// if not user function, run internal function
		if (funcindex(ca.fname) == -1)
			return call_system(ca);
		// calculate arguments in current frame context
		auto& fn = getfunc(ca.fname);                                   // get user function def
		StackFrame newframe;                                            // new stack frame
		assert( fn.args.size() == ca.args.size() );                     // basic arguments error
		for (pos_t i = 0; i < fn.args.size(); i++) {
			assert( fn.args[i].type == ca.args[i].type );               // basic argument error
			int32_t ex = expr(ca.args[i].expr);                         // run argument expression
			if (fn.args[i].type == "string")  ex = make_str(spop());    // new string by value
			newframe[fn.args[i].name] = { fn.args[i].type, ex };        // push to stack
		}
		// push new frame and calculate locals
		fstack.push_back(newframe);  
		for (auto& d : fn.locals)
			// ftop()[d.name] = { d.type, make(d.type) };
			init_dim(d);
		// run main block
		int32_t rval = 0;
		try { block(fn.block); }
		catch (ctrl_return& r) { rval = r.val; }
		// cleanup
		for (auto& d : fn.locals)
			if (d.type != "int")  destroy( get(d.name) );      // destroy local variables only in frame
		for (auto& d : fn.args)
			if (d.type == "string")  destroy( get(d.name) );   // destroy argument strings (pass-by-value)
		fstack.pop_back();                                     // destroy stack frame
		return rval;
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
	int32_t& varpath(pos_t vptr) {
		const Prog::VarPath& vp = prog.varpaths.at(vptr);
		int32_t* ptr = NULL;
		for (auto& in : vp.instr) {
			if      (in.cmd == "get")          ptr = &get(in.sarg);
			else if (in.cmd == "get_global")   ptr = &get_global(in.sarg);
			else if (ptr == NULL)              goto err;
			else if (in.cmd == "memget_expr")  ptr = &memget(*ptr, expr(in.iarg) );
			else if (in.cmd == "memget_prop")  ptr = &memget(*ptr, getnum(in.sarg) );
			else    throw runtime_error("unknown varpath: " + in.cmd);
		}
		if (ptr == NULL)  goto err;
		return *ptr;
		err:  throw out_of_range("memget ptr is null");
	}
	string varpath_str(pos_t vptr) {
		const auto& mem = heap.at( varpath(vptr) ).mem;
		return string(mem.begin(), mem.end());
	}


	// expression parsing
	int32_t expr(pos_t eptr) {
		const Prog::Expr& ex = prog.exprs.at(eptr);
		pos_t istack_start = istack.size(), sstack_start = sstack.size();  // remember stack pos, for sanity
		int32_t t = 0, u = 0;
		string s, q;
		for (auto& in : ex.instr)
			// integers
			if      (in.cmd == "i")            ipush(in.iarg);
			else if (in.cmd == "varpath")      ipush( varpath(in.iarg) );
			else if (in.cmd == "add")          t = ipop(),  ipeek() += t;
			else if (in.cmd == "sub")          t = ipop(),  ipeek() -= t;
			else if (in.cmd == "mul")          t = ipop(),  ipeek() *= t;
			else if (in.cmd == "div")          t = ipop(),  ipeek() /= t;
			else if (in.cmd == "and")          t = ipop(),  ipeek()  = ipeek() && t;
			else if (in.cmd == "or")           t = ipop(),  ipeek()  = ipeek() || t;
			else if (in.cmd == "eq")           t = ipop(),  u = ipop(),  ipush(u == t);
			else if (in.cmd == "neq")          t = ipop(),  u = ipop(),  ipush(u != t);
			else if (in.cmd == "lt")           t = ipop(),  u = ipop(),  ipush(u <  t);
			else if (in.cmd == "gt")           t = ipop(),  u = ipop(),  ipush(u >  t);
			else if (in.cmd == "lte")          t = ipop(),  u = ipop(),  ipush(u <= t);
			else if (in.cmd == "gte")          t = ipop(),  u = ipop(),  ipush(u >= t);
			// strings
			else if (in.cmd == "lit")          spush(in.iarg);
			else if (in.cmd == "varpath_str")  spush(varpath_str(in.iarg));
			else if (in.cmd == "strcat")       s = spop(),  speek() += s;
			else if (in.cmd == "eq_str")       s = spop(),  q = spop(),  ipush(q == s);
			else if (in.cmd == "neq_str")      s = spop(),  q = spop(),  ipush(q != s);
			// other
			else if (in.cmd == "varpath_ptr")  ipush(varpath(in.iarg));
			else if (in.cmd == "call")         ipush(call(in.iarg));
			else    throw runtime_error("unknown expr: " + in.cmd);
		// sanity check
		pos_t istack_end = istack.size() - istack_start, sstack_end = sstack.size() - sstack_start;
		if (istack_end + sstack_end != 1) {
			printf("WARNING: odd expression results  i %d  s %d\n", istack_end, sstack_end );
			for (pos_t i = 0; i < istack.size(); i++)
				printf("  %02di  %d\n", i, istack[i] );
			for (pos_t i = 0; i < sstack.size(); i++)
				printf("  %02ds  %s\n", i, sstack[i].c_str() );
		}
		// result
		return istack.size() ? ipop() : 0;
	}
	// string expr_str(pos_t ex) {
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
	void     spush(pos_t loc) { sstack.push_back( prog.literals.at(loc) ); }



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