#include "tools.h"
using namespace std;

const bool DEBUG = false;

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
	else if (isValidDoubleConst(s))
	{
		double * ptr = new double;
		*ptr = toDouble(s);
		stk.push(Object(DOUBLE, ptr));
	}
	else if (s[0] == '"')
	{
		string str = s;
		for (int i=tk.inx; i<(int)tk.str.length(); i++)
			str += tk.str[i];

		if (!isValidString(str))
			error("Push failed, what the hell is -- " + str);

		string * ptr = new string;
		*ptr = str.substr(1, str.length()-2);
		stk.push(Object(STRING, ptr));
	}
	else error("Push failed, what the hell is - " + s);
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

	if (type == "int")
	{
		int * ptr = new int;
		cin >> *ptr;
		stk.push(Object(INTEGER, ptr));
	}
	else if (type == "double")
	{
		double * ptr = new double;
		cin >> *ptr;
		stk.push(Object(DOUBLE, ptr));
	}
	else if (type == "string")
	{
		string * ptr = new string;
		getline(cin, *ptr);
		stk.push(Object(STRING, ptr));
	}
	else error("Read does not support that type");
}

void handleOperation(int op, bool isCond = false)
{
	if (stk.size() < 2) error("Not enough elements in the stack to perform operation " + toStr(op));
	Object b = stk.top(); stk.pop();
	Object a = stk.top(); stk.pop();

	if (a.type != b.type) //error("operation requires two objects of the same type\n");
	{
		if (a.type == INTEGER && b.type == DOUBLE) //upcast a into a double
		{
			int * old = (int*)a.ptr;
			double * val = new double;
			*val = (*(int*)old)*1.0;
			a = Object(DOUBLE, val);
			delete old;
		}
		else if (a.type == DOUBLE && b.type == INTEGER) //upcast b into a double
		{
			int * old = (int*)b.ptr;
			double * val = new double;
			*val = (*(int*)old)*1.0;
			b = Object(DOUBLE, val);
			delete old;
		}
		else if (a.type == STRING) //upcast b into a string
		{
			string str;
			void * old;
			if (b.type == DOUBLE)
			{
				old = (double*)b.ptr;
				str = toStr(*(double*)b.ptr);
			}
			else
			{
				old = (int*)b.ptr;
				str = toStr(*(int*)b.ptr);
			}

			string * val = new string;
			*val = str;

			b = Object(STRING, val);
			delete old;
		}
		else if (b.type == STRING)
		{
			string str;
			void * old;
			if (a.type == DOUBLE)
			{
				old = (double*)a.ptr;
				str = toStr(*(double*)a.ptr);
			}
			else
			{
				old = (int*)a.ptr;
				str = toStr(*(int*)a.ptr);
			}

			string * val = new string;
			*val = str;

			a = Object(STRING, val);
			delete old;
		}
	}

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
		else if (op == NOT_EQUAL)
			*pval = *(int*)a.ptr != *(int*)b.ptr;
		else error("int type does not support this operation");

		if (pointerCt[a.ptr] == 0) delete a.ptr;
		if (pointerCt[b.ptr] == 0) delete b.ptr;

		stk.push(Object(INTEGER, pval));
	}
	else if (a.type == DOUBLE)
	{
		void * pval;
		if (isCond)
			pval = new int;
		else pval = new double;

		if (op == ADD)
			*(double*)pval = *(double*)a.ptr + *(double*)b.ptr;
		else if (op == SUB)
			*(double*)pval = *(double*)a.ptr - *(double*)b.ptr;
		else if (op == MULT)
			*(double*)pval = *(double*)a.ptr * *(double*)b.ptr;
		else if (op == DIV)
		{
			if (*(double*)b.ptr == 0) error("Division by ZERO");
			*(double*)pval = *(double*)a.ptr / *(double*)b.ptr;
		}
		else if (op == MOD)
		{
			if (*(double*)b.ptr == 0) error("Modulo by ZERO");
			*(double*)pval = *(double*)a.ptr - floor(*(double*)a.ptr / *(double*)b.ptr) * *(double*)b.ptr; // (a - a/b*b) == (a = a%b)
		}
		else if (op == LESS_THAN)
			*(int*)pval = *(double*)a.ptr < *(double*)b.ptr;
		else if (op == LESS_THAN_OR_EQUAL)
			*(int*)pval = *(double*)a.ptr <= *(double*)b.ptr;
		else if (op == EQUAL)
			*(int*)pval = *(double*)a.ptr == *(double*)b.ptr;
		else if (op == GREATER_THAN_OR_EQUAL)
			*(int*)pval = *(double*)a.ptr >= *(double*)b.ptr;
		else if (op == GREATER_THAN)
			*(int*)pval = *(double*)a.ptr > *(double*)b.ptr;
		else if (op == NOT_EQUAL)
			*(int*)pval = *(double*)a.ptr != *(double*)b.ptr;
		else error("Double type does not support this operation");

		if (pointerCt[a.ptr] == 0) delete a.ptr;
		if (pointerCt[b.ptr] == 0) delete b.ptr;

		if (isCond) stk.push(Object(INTEGER, pval)); //condition operations return INTEGERS {0, 1}
		else stk.push(Object(DOUBLE, pval)); //arithmetic operations return doubles
	}
	else if (a.type == STRING)
	{
		void * pval;
		if (isCond)
			pval = new int;
		else pval = new string;

		if (op == ADD)
			*(string*)pval = *(string*)a.ptr + *(string*)b.ptr;
		else if (op == LESS_THAN)
			*(int*)pval = *(string*)a.ptr < *(string*)b.ptr;
		else if (op == LESS_THAN_OR_EQUAL)
			*(int*)pval = *(string*)a.ptr <= *(string*)b.ptr;
		else if (op == EQUAL)
			*(int*)pval = *(string*)a.ptr == *(string*)b.ptr;
		else if (op == GREATER_THAN_OR_EQUAL)
			*(int*)pval = *(string*)a.ptr >= *(string*)b.ptr;
		else if (op == GREATER_THAN)
			*(int*)pval = *(string*)a.ptr > *(string*)b.ptr;
		else if (op == NOT_EQUAL)
			*(int*)pval = *(string*)a.ptr != *(string*)b.ptr;
		else error("STRING type does not support this operation");

		if (pointerCt[a.ptr] == 0) delete a.ptr;
		if (pointerCt[b.ptr] == 0) delete b.ptr;

		if (isCond)
			stk.push(Object(INTEGER, pval));
		else stk.push(Object(STRING, pval));
	}
	else error("Unidentified type while performing an operation" + toStr(a.type));
}

void handlePrint(Tokenizer tk)
{
	string s = tk.nextToken();

	if (s == "") //print top of stack
	{
		if (stk.empty())
			error("Cannot print top of an empty stack");

		string str = stk.top().valToStr();
		if (stk.top().type == STRING)
			cout << str.substr(1, str.length()-2) << endl; //don't print the quotations in a string
		else cout << str << endl;
	}
	else if (isValidVariableName(s))
	{
		if (memory.find(s) == memory.end())
			error("Cannot find variable named - " + s);

		string str = memory[s].valToStr();

		if (memory[s].type == STRING)
			cout << str.substr(1, str.length()-2) << endl; //don't print the quotations
		else cout << str << endl;
	}
	else if (isValidIntConst(s) || isValidDoubleConst(s))
		cout << s << endl;
	else //only other choice left is literal strings
	{
		string str = s;
		for (int i=tk.inx; i<(int)tk.str.length(); i++)
			str += tk.str[i];

		if (isValidString(str))
			cout << str.substr(1, str.length()-2) << endl; //don't print the quotations
		else 
			error("Cannot print - " + s);
	}
}

void handleJump(Tokenizer tk, bool isZero = false)
{
	string label = tk.nextToken();
	
	if (jumpLoc.find(label) == jumpLoc.end())
		error("Cannot JMP to unknown location - " + label);

	if (isZero && stk.empty())
		error("Cannot JZ if the system stack is empty - " + label);

	if (!isZero || isZero && *(int*)(stk.top().ptr) == 0) //jump if doing a JMP, or if JZ and top = 0
	{	
		pastLoc.push(fin.tellg());
		fin.clear();
		fin.seekg(jumpLoc[label], ios::beg);

		if (isZero) stk.pop(); //remove condition from the stack, so you can treat the top of the stack as paramenters to a function
	}
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
		cerr << it->first << " = " << it->second  << " == " << *(int*)it->first << " == " << *(double*)it->first << endl;

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
		else if (cmd == "OP<") handleOperation(LESS_THAN, true);
		else if (cmd == "OP<=") handleOperation(LESS_THAN_OR_EQUAL, true);
		else if (cmd == "OP=") handleOperation(EQUAL, true);
		else if (cmd == "OP>=") handleOperation(GREATER_THAN_OR_EQUAL, true);
		else if (cmd == "OP>") handleOperation(GREATER_THAN, true);
		else if (cmd == "OP!=") handleOperation(NOT_EQUAL, true);
		else if (cmd == "PRINT") handlePrint(tk);
		else if (cmd == "JMP") handleJump(tk);
		else if (cmd == "JZ") handleJump(tk, true);
		else if (cmd == "RET") handleRet();
		else error("Do not recognize cmd - " + cmd);
	}

	if (DEBUG) printTrace();
	
	return 0;
}