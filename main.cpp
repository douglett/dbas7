#include "dbas7.hpp"
#include "runtime.hpp"
#include "parser.hpp"
using namespace std;


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


void test3() {
	printf(":: test 3\n");
	Parser p;
	// p.loadstring(
	// 	"type tt\n"
	// 	"	dim a\n"
	// 	"	dim bbb\n"
	// 	"end type\n"
	// 	"dim a\n"
	// );
	p.load("scripts/scratch.bas");
	p.p_type();
	p.p_dim();
	p.p_let();
	p.show();
	printf("-----\n");

	Runtime r;
	r.prog = p.prog;
	r.init();
	r.let(p.prog.lets.at(0));
	r.show();

	int32_t v = r.varpath({
		{ "get_global", "a" },
		{ "memget", "USRTYPE_tt_a" }
	});
	printf("result: %d\n", v);
}


void test4() {
	Parser p;
	p.load("scripts/scratch.bas");
	p.parse();
	p.show();
	printf("-----\n");
	
	// Runtime r;
	// r.prog = p.prog;
	// r.run();
	// r.show();
}


int main() {
	printf("hello world\n");

	// test1();
	// test2();
	// test3();
	test4();
}