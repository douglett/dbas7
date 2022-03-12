#include "dbas7.hpp"
#include "debug.hpp"
#include "parser.hpp"
#include "runtime.hpp"
using namespace std;


void scratch() {
	// parse
	Parser p;
	// p.load("scripts/scratch.bas");
	p.load("scripts/advent2.bas");
	printf("-----\n");
	p.parse();
	Progshow(p.prog).tofile("bin/prog.tree");
	printf("-----\n");
	// run
	Runtime r;
	r.prog = p.prog;
	r.run();
	printf("-----\n");
	r.show();
}


int main() {
	printf("hello world\n");

	scratch();
}