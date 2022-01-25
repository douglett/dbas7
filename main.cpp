#include "dbas7.hpp"
#include "runtime.hpp"
#include "parser.hpp"
using namespace std;


void test4() {
	Parser p;
	p.load("scripts/scratch.bas");
	printf("-----\n");
	p.parse();
	p.show();
	printf("-----\n");
	
	Runtime r;
	r.prog = p.prog;
	r.run();
	r.show();
	int32_t v = r.varpath({
		"get_global a",
		"memget USRTYPE_tt_bbb"
	});
	printf("result: %d\n", v);
}


int main() {
	printf("hello world\n");

	// test1();
	// test2();
	// test3();
	test4();
}