#include <iostream>
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

// vector<string> split(const string& str) {
// 	vector<string> vs;
// 	string s;
// 	for (auto c : str)
// 		if (isspace(c))  s.length() ? vs.push_back(s), s = "" : NULL;
// 		else             s += c;  
// 	if (s.length())  vs.push_back(s);
// 	return vs;
// }

struct Prog {
	struct Dim  { string name, type; /* int locality; */ };
	struct Type { string name; vector<Dim> members; };
	// struct Func { string name; map<string, Dim> args, locals; };
};

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


void test1() {
	// i[0].a[1]
	vector<vector<string>> vs = {
		{ "get", "i" },
		{ "memget", "0" },
		{ "memget", "USROBJ_A_a" },
		{ "memget", "1" },
	};

	Runtime r;

	int32_t p1, p2, p3, v;
	p1 = r.memalloc();
	r.heap.at(p1).mem = { 0, 101 };        // a[0, 1]
	r.consts["USROBJ_A_a"] = 0;
	p2 = r.memalloc();
	r.heap.at(p2).mem = { p1 };            // i[0]
	p3 = r.memalloc();
	r.heap.at(p3).mem = { p2 };            // i
	r.fstack.push_back({ { "i", p3 } });   // ptr(i)

	Prog::Type t = { .name="B", .members={
		{"a", "int[]"}
	}};

	v = r.varpath(vs);
	printf("result: %d\n", v );
}


void test2() {
	Runtime r;
	r.dim("int", "i");
	r.dim("string", "s");

	// TODO: make user type
	// TODO: assign string
	// TODO: push

	int32_t v = r.varpath({ {"get_global", "i"} });
	printf("result: %d\n", v );
}


int main() {
	printf("hello world\n");

	// test1();
	test2();
}