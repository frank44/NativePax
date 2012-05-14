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
using namespace std;

//All of the types that I currently support
enum Types { INTEGER };
enum Operations { ADD, SUB, MULT, DIV, MOD, LESS_THAN, LESS_THAN_OR_EQUAL, EQUAL, GREATER_THAN_OR_EQUAL, GREATER_THAN};

void error(string s)
{
	cerr << "ERROR :: " << s << endl;
	exit(0);
}

struct Object
{
	int type; //type of object
	void * ptr; //location in memory
	
	Object() {}
	Object(int a, void * b)
	{ type = a; ptr = b; }

	/*~Object()
	{ 
		if (ptr != NULL)
			delete ptr; 
	}*/

	string valToStr()
	{
		stringstream ss;
		if (type == INTEGER)
			ss << *(int*)ptr;
		else error("Unidentified Object type, cannot print it.");

		return ss.str();
	}
};

bool isValidIntConst(string a) //checks if the string represents a valid literal int
{
	int inx = 0;
	if (a[0] == '-') inx++;

	for (int i=inx; i<(int)a.length(); i++)
		if (!isdigit(a[i])) return false;

	return true;
}

bool isValidVariableName(string a) //valid regex = [_A-Za-z][_0-9A-Za-z]*
{
	bool ok = a.length()>0 && (a[0]=='_' || isalpha(a[0]));
	for (int i=1; i<(int)a.length() && ok; i++)
		ok &= isalpha(a[i]) || isdigit(a[i]) || a[i]=='_';

	return ok;
}

int toNum(string s) //string -> integer
{
	stringstream ss(s);
	int ret;
	ss >> ret;
	return ret;
}

template<class T>
string toStr(T n)
{
	stringstream ss;
	ss << n;
	return ss.str();
}

bool isOperator(char a)
{
	return (a=='+' ||
			a=='-' ||
			a=='*' ||
			a=='/' ||
			a=='%' ||
			a=='<' ||
			a=='=' ||
			a=='>' ||
			a=='$');
}

struct Tokenizer
{
	string str;
	int inx;

	Tokenizer(string s) { str = s; inx=0; }

	void update()
	{
		while (inx<(int)str.length() && isspace(str[inx])) inx++; //next non-whitespace character
	}

	bool hasGroup()
	{ 
		update();
		return str[inx] == '(';
	}

	string nextGroup()
	{
		update(); //sanity check
		
		string ret = "";
		int c = 0;
		do
		{
			if (str[inx] == '(') c++;
			else if (str[inx] == ')') c--;
			
			ret += str[inx];
			inx++;
		}
		while (inx < (int)str.length() && c!=0);

		return ret.substr(1, ret.length()-2)+"$";
	}

	int nextInt()
	{
		update();
		int d = 0;
		if (isdigit(str[inx]) || str[inx] == '-')
		{
			int f = 1; //controls the sign
			if (str[inx] == '-') 
			{
				f = -1;
				inx++;
			}

			d = 0;
			while (isdigit(str[inx]))
				d = 10*d + (str[inx++]-'0');

			return f*d;
		}
		else error("Integer expected, instead found :: " + str[inx]); 

		return -666; //should never get here
	}

	string nextOp()
	{
		update();

		if (isOperator(str[inx]))
		{
			if ((str[inx] == '<' || str[inx] == '>') && str[inx+1]=='=')
			{
				inx += 2;
				return str.substr(inx-2,2);
			}
			else return string(1, str[inx++]);
		}
		else
		{
			cout << "ERROR :: Operator expected, found '" << str[inx] << "' instead\n";
			exit(0);
		}

		return "should never get here";
	}

	bool hasNext()
	{
		update();
		return inx != str.length();
	}

	string nextToken()
	{
		update();
		string ret = "";
		while (inx < (int)str.length() && !isspace(str[inx]))
			ret += str[inx++];

		return ret;
	}

	string getAll() //get whatever is left to read
	{
		update();
		return str.substr(inx);
	}

	bool hasNextInt()
	{
		return (isdigit(str[inx]) || str[inx] == '-');
	}

	string nextVar()
	{
		update();

		string name = "";
		while (inx < (int)str.length()-1 && !isspace(str[inx]) && !isOperator(str[inx]))
			name += str[inx++];

		return name;
	}

	bool isEmpty()
	{
		update();
		return inx == str.length();
	}

	char peek()
	{
		update();
		return str[inx];
	}

	string nextLiteralString()
	{
		string ret = "";
		if (peek() != '"')
		{
			cout << "ERROR :: String literal must beging with '\"', not ' " << peek() << "'\n";
			exit(0);
		}

		inx++;
		while (inx<(int)str.length() && !(str[inx-1] != '\\' && str[inx] == '"') )
			ret += str[inx++];

		inx++; //eat last quote
		return ret;
	}
};

bool isLabel(string & s) //sanitizes the string in the process
{
	Tokenizer tk(s);
	string label = tk.nextToken();

	if (label.size() >= 2 && label[0] == '.' && tk.isEmpty())
	{
		s = label;
		return true;
	}

	return false;
}
