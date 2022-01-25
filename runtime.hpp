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
	int32_t memalloc(int size=0) {
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
	int32_t make(const string& type) {
		if      (type == "int")         return 0;
		else if (type == "string")      return memalloc();
		else if (typeindex(type) > -1) {
			auto& t = prog.types.at(typeindex(type));
			int32_t ptr = memalloc( t.members.size() ), off = 0;
			for (auto& m : t.members)
				memget(ptr, off++) = make(m.type);
			return ptr;
		}
		else    throw runtime_error("unknown type: " + type);
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
	void init_dim(const Prog::Dim& t) {
		dim(t.type, t.name);
	}


	// runtime actions
	void dim(const string& type, const string& name) {
		if (globals.count(name))   throw runtime_error("already defined: " + name);
		globals[name] = make(type);
	}
	void let(const Prog::Let& l) {
		int32_t  ex = expr(l.expr);
		int32_t& vp = varpath(l.varpath);
		vp = ex;
		// printf("let %d\n", ex);
	} 


	// variable path parsing
	int32_t& varpath(int vp) {
		vector<vector<string>> path;
		for (auto& instr : prog.varpaths.at(vp).instr)
			path.push_back( Strings::split(instr) );
		return varpath(path);
	}
	int32_t& varpath(const vector<vector<string>>& path) {
		int32_t* ptr = NULL;
		for (auto& cmd : path)
			if      (cmd.at(0) == "get")         ptr = &get(cmd.at(1));
			else if (cmd.at(0) == "get_global")  ptr = &get(cmd.at(1), true);
			else if (ptr == NULL)                goto err;
			else if (cmd.at(0) == "memget")      ptr = &memget(*ptr, getnum(cmd.at(1)));
		if (ptr == NULL)  goto err;
		return *ptr;
		err:  throw out_of_range("memget ptr is null");
	}


	// expression parsing
	int32_t expr(int ex) {
		return expr( prog.exprs.at(ex) );
	}
	int32_t expr(const Prog::Expr& ex) {
		int res = 0;
		for (auto& in : ex.instr) {
			auto cmd = Strings::split(in);
			if    (cmd.at(0) == "i")  res = getnum(cmd.at(1));
			else  throw runtime_error("unknown expression command: " + cmd.at(0));
		}
		return res;
	}


	// show state
	void show() {
		printf("  heap:  %d\n", heap.size());
		printf("  consts:\n");
		for (auto& c : consts)
			printf("    %s  %d\n", c.first.c_str(), c.second );
		printf("  globals:\n");
		for (auto& g : globals)
			printf("    %s  %d\n", g.first.c_str(), g.second );
	}
};