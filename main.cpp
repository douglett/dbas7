#include "dbas7.hpp"
#include "debug.hpp"
#include "parser.hpp"
#include "runtime.hpp"
using namespace std;


void runscript(const string& fname) {
	// parse
	Parser p;
	p.load("scripts/" + fname + ".bas");
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

	runscript("scratch");
	// runscript("advent2");
}