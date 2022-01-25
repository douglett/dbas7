// ----------------------------------------
// Input file parsing
// ----------------------------------------
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "dbas7.hpp"
#include "inputfile.hpp"
using namespace std;


struct Parser : InputFile {
	vector<Prog::Dim>    dims;
	vector<Prog::Type>   types;

	void p_type() {
		require("type @identifier @endl");
		string type, name, tname = lastrule.at(0);
		types.push_back({ tname });
		// type members
		while (!eof()) {
			if      (expect("@endl"))                                  { nextline();  continue; }
			else if (expect("dim @identifier @identifier @endl"))      type = lastrule.at(0), name = lastrule.at(1);
			else if (expect("dim @identifier [ ] @identifier @endl"))  type = lastrule.at(0) + "[]", name = lastrule.at(1);
			else if (expect("dim @identifier @endl"))                  type = "int", name = lastrule.at(0);
			else    break;
			// typecheck(type), namecollision(name);
			types.back().members.push_back({ name, type });  // save type member
			nextline();
		}
		require("end type @endl"), nextline();
	}

	// show results
	void show() const {
		for (auto& t : types)
			show(t);
	}
	void show(const Prog::Type& t) const {
		printf("type: %s\n", t.name.c_str() );
		for (auto& d : t.members)
			show(d);
	}
	void show(const Prog::Dim& d) const {
		printf("   %s  %s\n", d.type.c_str(), d.name.c_str());
	}
};