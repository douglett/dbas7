#include "dbas7.hpp"
#include "debug.hpp"
#include "parser.hpp"
#include "runtime.hpp"
using namespace std;


void test4() {
	Parser p;
	p.load("scripts/test4.bas");
	printf("-----\n");
	p.parse();
	// p.show();
	Progshow(p.prog).show();
	printf("-----\n");
	
	Runtime r;
	r.prog = p.prog;
	r.run();
	printf("-----\n");
	
	r.show();
	// int32_t v = r.varpath({
	// 	"get_global a",
	// 	"memget USRTYPE_tt_bbb"
	// });
	// printf("result: %d\n", v);
}


void test5() {
	Parser p;
	p.load("scripts/scratch.bas");
	printf("-----\n");
	p.parse();
	Progshow(p.prog).show();
	printf("-----\n");

	Runtime r;
	r.prog = p.prog;
	r.run();
	printf("-----\n");
	r.show();
	
	// r.expr({
	// 	"varpath 1",
	// 	"strpush",
	// 	"varpath 2",
	// 	"strpush",
	// 	"strcat",
	// });
}


int main() {
	printf("hello world\n");

	// test4();
	test5();
}