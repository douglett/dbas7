// ----------------------------------------
// Program runtime
// ----------------------------------------
#pragma once
#include <vector>
#include <map>
using namespace std;


// enum MEMTYPE {
// 	MEMTYPE_NIL,
// 	MEMTYPE_INT,
// 	MEMTYPE_STRING,
// 	MEMTYPE_ARRAY,
// 	MEMTYPE_OBJECT,
// };


struct Runtime {
	// structs
	struct MemPage { vector<int32_t> mem; };
	struct MemPtr  { int32_t ptr, off; string v; };
	typedef  map<string, int32_t>  StackFrame;
	// state
	map<string, int32_t>           consts;
	map<int32_t, MemPage>          heap;
	StackFrame                     globals;
	vector<StackFrame>             fstack;
	vector<int32_t>                istack;  // expression stack
	vector<string>                 sstack;  // string expression stack
	int32_t memtop = 0;
	// prog
	Prog prog;


	// helpers
	int32_t getnum(const string& num) const {
		try                        { return stoi(num); }
		catch (invalid_argument e) { return consts.at(num); }
	}
	int typeindex(const string& name) const {
		for (int i = 0; i < prog.types.size(); i++)
			if (prog.types[i].name == name)  return i;
		return -1;
	}

	// memory
	int32_t memalloc(int size) {
		heap[++memtop] = { .mem=vector<int32_t>(size, 0) };
		return memtop;
	}
	int32_t& get(string id, int isglobal=0) {
		if (isglobal)  return globals.at(id);
		else           return fstack.at(fstack.size() - 1).at(id);
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
	int32_t make(const string& type) {
		if      (type == "int")               return 0;
		else if (type == "string")            return memalloc(0);
		else if (Tokens::is_arraytype(type))  return memalloc(0);
		else if (typeindex(type) > -1) {
			auto& t = prog.types.at(typeindex(type));
			int32_t ptr = memalloc( t.members.size() ), off = 0;
			for (auto& m : t.members)
				memget(ptr, off++) = make(m.type);
			return ptr;
		}
		// else if (Tokens::is_array(type)) {
		// 	return memalloc(0);
		// }
		else    throw runtime_error("unknown type: " + type);
	}


	// main run function
	void run() {
		init();
		// run blocks in order
		for (auto& bl : prog.blocks)
			block(bl);
	}


	// init
	void init() {
		for (auto& t : prog.types)    init_type(t);
		for (auto& d : prog.globals)  init_dim(d);
	}
	void init_type(const Prog::Type& t) {
		for (int i = 0; i < t.members.size(); i++)
			consts["USRTYPE_" + t.name + "_" + t.members[i].name] = i;
	}
	void init_dim(const Prog::Dim& d) {
		globals[d.name] = make(d.type);
	}


	// run statements
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
		if      (l.type == "int")  vp = ex;
		else if (l.type == "string") {
			auto& mem = heap.at(vp).mem;
			auto s = spop();
			mem = {};
			mem.insert(mem.end(), s.begin(), s.end());
		}
		else if (vp != ex) {
			throw runtime_error("TODO: assignment clone");
		}
		// else    throw runtime_error("let assignment error");
	}
	void print(int pr) { return print(prog.prints.at(pr)); }
	void print(const Prog::Print& pr) {
		for (auto& in : pr.instr)
			if      (in.first == "literal")  printf("%s ", Strings::deliteral(in.second).c_str() );
			else if (in.first == "expr")     printf("%d ", expr(getnum(in.second)) );
			else if (in.first == "varpath")  printf("%d ", varpath(getnum(in.second)) );
			else if (in.first == "varpath_string")  printf("%s ", varpath_str(getnum(in.second)).c_str() );
			else    throw runtime_error("unknown print: " + in.first);
		printf("\n");
	}
	void call(int ptr) { return call(prog.calls.at(ptr)); }
	void call(const Prog::Call& ca) {
		if (ca.fname == "push") {
			int32_t arrptr = expr(ca.args.at(0).expr);
			int32_t ex = expr(ca.args.at(1).expr);
			heap.at(arrptr).mem.push_back(ex);
		}
		else  throw runtime_error("unknown function: " + ca.fname);
	}


	// variable path parsing
	int32_t& varpath(int vp) { return varpath(prog.varpaths.at(vp)); }
	int32_t& varpath(const Prog::VarPath& vp) {
		int32_t* ptr = NULL;
		for (auto& in : vp.instr) {
			auto cmd = Strings::split(in);
			if      (cmd.at(0) == "get")          ptr = &get(cmd.at(1));
			else if (cmd.at(0) == "get_global")   ptr = &get(cmd.at(1), true);
			else if (ptr == NULL)                 goto err;
			// else if (cmd.at(0) == "memget")       ptr = &memget(*ptr, getnum(cmd.at(1)) );
			else if (cmd.at(0) == "memget_expr")  ptr = &memget(*ptr, expr(getnum(cmd.at(1))) );
			else    throw runtime_error("unknown varpath: " + cmd.at(0));
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
		for (auto& in : ex.instr) {
			auto cmd = Strings::split(in);
			// integers
			if      (cmd.at(0) == "i")    t = getnum(cmd.at(1)),  ipush(t);
			else if (cmd.at(0) == "varpath")  t = getnum(cmd.at(1)),  ipush(varpath(t));
			else if (cmd.at(0) == "add")  t = ipop(),  ipeek() += t;
			else if (cmd.at(0) == "sub")  t = ipop(),  ipeek() -= t;
			// strings
			else if (cmd.at(0) == "lit")  t = getnum(cmd.at(1)),  spush(t);
			else if (cmd.at(0) == "varpath_str")  t = getnum(cmd.at(1)),  spush(varpath_str(t));
			else if (cmd.at(0) == "strcat")  s = spop(),  speek() += s;
			// other
			else if (cmd.at(0) == "varpath_ptr")  t = getnum(cmd.at(1)),  ipush(varpath(t));
			else    throw runtime_error("unknown expr: " + cmd.at(0));
		}
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


	// show state
	void show() {
		// printf("  heap:  %d\n", heap.size() );
		// printf("  stack:  i.%d  s.%d\n", istack.size(), sstack.size() );
		printf("  heap %d | istack %d | sstack %d\n", heap.size(), istack.size(), sstack.size() );
		printf("  consts:\n");
		for (auto& c : consts)
			printf("    %s  %d\n", c.first.c_str(), c.second );
		printf("  globals:\n");
		for (auto& g : globals)
			printf("    %-10s  %d\n", g.first.c_str(), g.second );
	}
};