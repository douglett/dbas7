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

	// helpers
	int32_t getnum(const string& num) {
		try                        { return stoi(num); }
		catch (invalid_argument e) { return consts.at(num); }
	}

	// memory
	int32_t memalloc() {
		heap[++memtop] = {};
		return memtop;
	}
	int32_t& get(string id, int isglobal=0) {
		if (isglobal)  return globals.at(id);
		else           return fstack.at(fstack.size() - 1).at(id);
	}
	int32_t& memget(int32_t ptr, int32_t off) {
		return heap.at(ptr).mem.at(off);
	}

	// variable path parsing
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

	// dim
	void dim(const string& type, const string& name) {
		int32_t p = 0;
		if      (globals.count(name))  throw runtime_error("already defined: " + name);
		else if (type == "int")        globals[name] = 0;
		else if (type == "string")     p = memalloc(),  globals[name] = p;
		else    throw runtime_error("unknown type: " + type);
	}
};