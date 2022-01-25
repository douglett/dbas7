#pragma once
#include <vector>
#include <string>
using namespace std;


struct Prog {
	struct Dim  { string name, type; /* int locality; */ };
	struct Type { string name; vector<Dim> members; };
	// struct Func { string name; map<string, Dim> args, locals; };
};