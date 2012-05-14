#include <iostream>
#include <cmath>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <map>
#include <string>
#include <fstream>
#include <stack>
#include <sstream>
#include "tools.h"

using namespace std;

const bool DEBUG = true;

ifstream fin;

stack< Object > stk; //object stack
map< string, Object > memory; //where all variables are stored
map< void*, int > pointerCt; //naive garbage collector (counts how many variables are pointing to that object)
map<string, streamoff> jumpLoc; //locations of each label in the source file
stack<streamoff> pastLoc; //stack of previous jump locations (for when using recursion or nested calls)

void handlePush(Tokenizer tk)
{
	string s = tk.nextToken();

	if (isValidIntConst(s)) //pushing an literal int
	{
		int * ptr = new int;
		*ptr = toNum(s);

		stk.push(Object(INTEGER, ptr));
	}
	else if (isValidVariableName(s)) //pushing a variable
	{
		if (memory.find(s) == memory.end())
			error("Unidentified variable name :: " + s);

		stk.push(memory[s]);
	}
}

void handleLoad(Tokenizer tk)
{
	string name = tk.nextToken();
	if (!isValidVariableName(name))
		error("Invalid variable name - " + name);

	if (stk.empty())
		error("Can't load with an empty stack - " + name);

	if (memory.find(name) != memory.end()) //if this variable already exists (recycle it)
	{
		void* old = memory[name].ptr;
		if (pointerCt[old] > 0)
		{
			pointerCt[ old ]--;
			if (pointerCt[ old ] == 0)
			{	
				pointerCt.erase(old);
				delete old;
			}
		}
	}

	memory[name] = stk.top();
	pointerCt[ stk.top().ptr ]++;
}

void handlePop()
{
	if (stk.empty())
		error("Can't pop an empty stack!");
	else 
	{
		void * ptr = stk.top().ptr;
		if (ptr != NULL && pointerCt[ptr] == 0) //if not NULL and no other variables are pointing to it, free it
		{
			pointerCt.erase(ptr);
			delete ptr;
		}

		stk.pop();
	}
}

void handleRead(Tokenizer tk)
{
	string type = tk.nextToken();

	if (type == "INT")
	{
		int * ptr = new int;
		cin >> *ptr;
		stk.push(Object(INTEGER, ptr));
	}
	else error("Read does not support that type");
}

void handleOperation(int op)
{
	if (stk.size() < 2) error("Not enough elements in the stack to perform operation " + toStr(op));
	Object b = stk.top(); stk.pop();
	Object a = stk.top(); stk.pop();

	if (a.type != b.type) error("operation requires two objects of the same type\n");
	if (a.ptr == NULL || b.ptr == NULL) error("Null reference while performing operation");
	
	if (a.type == INTEGER)
	{
		int * pval = new int;

		if (op == ADD)
			*pval = *(int*)a.ptr + *(int*)b.ptr;
		else if (op == SUB)
			*pval = *(int*)a.ptr - *(int*)b.ptr;
		else if (op == MULT)
			*pval = *(int*)a.ptr * *(int*)b.ptr;
		else if (op == DIV)
		{
			if (*(int*)b.ptr == 0) error("Division by ZERO");
			*pval = *(int*)a.ptr / *(int*)b.ptr;
		}
		else if (op == MOD)
		{
			if (*(int*)b.ptr == 0) error("Modulo by ZERO");
			*pval = *(int*)a.ptr % *(int*)b.ptr;
		}
		else if (op == LESS_THAN)
			*pval = *(int*)a.ptr < *(int*)b.ptr;
		else if (op == LESS_THAN_OR_EQUAL)
			*pval = *(int*)a.ptr <= *(int*)b.ptr;
		else if (op == EQUAL)
			*pval = *(int*)a.ptr == *(int*)b.ptr;
		else if (op == GREATER_THAN_OR_EQUAL)
			*pval = *(int*)a.ptr >= *(int*)b.ptr;
		else if (op == GREATER_THAN)
			*pval = *(int*)a.ptr > *(int*)b.ptr;
		else error("int type does not support this operation");

		if (pointerCt[a.ptr] == 0) delete a.ptr;
		if (pointerCt[b.ptr] == 0) delete b.ptr;

		stk.push(Object(INTEGER, pval));
	}
	else error("Unidentified type while performing an operation" + toStr(a.type));
}

void handlePrint(Tokenizer tk)
{
	string s = tk.nextToken();

	if (s == "") //print top of stack
		cout << stk.top().valToStr() << endl;
	else if (isValidVariableName(s))
	{
		if (memory.find(s) == memory.end())
			error("Cannot find variable named - " + s);

		cout << memory[s].valToStr() << endl;
	}
	else if (isValidIntConst(s))
		cout << s << endl;
	else error("Cannot print - " + s);
}

void handleJump(Tokenizer tk)
{
	string label = tk.nextToken();
	if (jumpLoc.find(label) == jumpLoc.end())
		error("Cannot JMP to unknown location - " + label);

	pastLoc.push(fin.tellg());
	fin.clear();
	fin.seekg(jumpLoc[label], ios::beg);
}

void handleRet()
{
	if (pastLoc.empty())
		error("Cannot RET with a JMP");

	fin.clear();
	fin.seekg(pastLoc.top(), ios::beg);
	pastLoc.pop();
}

void printTrace()
{
	cerr << "Done\n\nContents of Memory\n-------------------------\n";

	for (map<string, Object>::iterator it = memory.begin(); it!=memory.end(); it++)
		cerr << it->first << " = " << (it->second).valToStr() << endl;
	
	cerr << "-------------------------\n";

	cerr << "\nContents of Stack\n-------------------------\n";
	while (!stk.empty())
	{
		cerr << stk.top().type << " - " << stk.top().valToStr() << endl;
		stk.pop();
	}
	cerr << "-------------------------\n";

	cerr << "\nContents of Garbage Tracker\n-------------------------\n";
	for (map<void*, int>::iterator it = pointerCt.begin(); it!=pointerCt.end(); it++)
		cerr << it->first << " = " << it->second  << " == " << *(int*)it->first << endl;

	cerr << "-------------------------\n";
}

int main (int argc, char *argv[])
{
	if (argc == 1) fin = ifstream("inst.txt");
	else if (argc == 2) fin = ifstream(argv[1]);
	else error("Invalid console arguments");

	string str;
	while ( getline(fin, str) ) //one scan to memoize all jump locations/functions
	{
		if (isLabel(str)) //must be on its own line starting with a '.'
		{
			if (jumpLoc.find(str) != jumpLoc.end()) //repeated a jump label
				error("Repeated a jump label - " + str);

			jumpLoc[str] = fin.tellg();
			if (DEBUG)
				cerr << "Memoized label :: " << str << " - " << jumpLoc[str] << endl;
		}
	}

	if (jumpLoc.find(".START") == jumpLoc.end()) //no .START found!
		error("Could not find .START");

	fin.clear(); //reset fstream flags
	fin.seekg( jumpLoc[".START"], ios::beg); //jump to .START function

	while (!fin.eof())
	{
		getline(fin, str);
		Tokenizer tk(str);
		string cmd = tk.nextToken();

		if (DEBUG) cerr << "At :: " << cmd << endl;

		if (cmd == "") continue; //blank line, whoops
		else if (cmd == "EXIT") break;  
		else if (cmd == "PUSH") handlePush(tk);
		else if (cmd == "LOAD") handleLoad(tk);
		else if (cmd == "POP") handlePop();
		else if (cmd == "READ") handleRead(tk);
		else if (cmd == "ADD") handleOperation(ADD);
		else if (cmd == "SUB") handleOperation(SUB);
		else if (cmd == "MULT") handleOperation(MULT);
		else if (cmd == "DIV") handleOperation(DIV);
		else if (cmd == "MOD") handleOperation(MOD);
		else if (cmd == "OP<") handleOperation(LESS_THAN);
		else if (cmd == "OP<=") handleOperation(LESS_THAN_OR_EQUAL);
		else if (cmd == "OP=") handleOperation(EQUAL);
		else if (cmd == "OP>=") handleOperation(GREATER_THAN_OR_EQUAL);
		else if (cmd == "OP>") handleOperation(GREATER_THAN);
		else if (cmd == "PRINT") handlePrint(tk);
		else if (cmd == "JMP") handleJump(tk);
		else if (cmd == "RET") handleRet();
		else error("Do not recognize cmd - " + cmd);
	}

	if (DEBUG) printTrace();
	
	return 0;
}