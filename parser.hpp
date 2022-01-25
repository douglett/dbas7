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
	vector<Prog::Dim>    vdim;
	vector<Prog::Type>   vtype;
	string ctypename;

	void p_type() {
		// dsym
		require("type"), requirep("identifier"), ctypename = lasttok, requirep("endl");
		// namecollision(ctypename);
		vtype.push_back({ ctypename });
		nextline();
		// type members
		while (!eof())
			if      (expectp("endl"))  { nextline(); continue; }
			else if (!peek("dim"))     break;
			else {
				string type = "int", name;
				require("dim"), requirep("identifier"), name = lasttok;
				if (expectp("endl")) ;   // dim {identifier} -> as-int
				else {                   // dim {type?[]} {identifier} -> as-type
					type = name;
					if (expect("["))  require("]"), type += "[]";
					requirep("identifier"), name = lasttok, requirep("endl");
				}
				// typecheck(type), namecollision(name);  // name collision check
				vtype.back().members.push_back({ name, type });  // save member
				nextline();
			}
		// end type
		ctypename = "";
		require("end"), require("type"), requirep("endl"), nextline();
	}
};