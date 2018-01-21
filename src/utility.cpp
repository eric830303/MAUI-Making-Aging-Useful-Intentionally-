//////////////////////////////////////////////////////////////
//
// Source File
//
// File name: utility.cpp
// Author: Ting-Wei Chang
// Date: 2017-07
//
//////////////////////////////////////////////////////////////

#include "utility.h"
#include <cmath>
#include <cctype>
//#include <ctime>
#include <random>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

// Declare the random engine with seed.
default_random_engine genrandom(chrono::system_clock::now().time_since_epoch().count());

/////////////////////////////////////////////////////////////////////
//
// Assertion Function
//
/////////////////////////////////////////////////////////////////////
void _assertFunc(bool expr, const char *exprstr, const char *file, const char *function, int line, const char *message)
{
	if(!expr)
	{
		cerr << "[ERROR]: " << ((message != nullptr) ? message : "Error occur!!") << "\n";
		cerr << "\tAssertion failed at " << file << ":" << line << " inside " << function << "\n";
		cerr << "\tExpression: " << exprstr << "\n";
	}
}

/////////////////////////////////////////////////////////////////////
//
// Split a string with specific delimiters, i.e., characters
//
/////////////////////////////////////////////////////////////////////
vector<string> stringSplit(string input, const char *delimiters)
{
	if(delimiters == NULL)
		return {};                              // Same as "return vector<string>();"
	
	vector<string> result;                      // Result list after splitting
	char inputarray[input.length()], *ptr;      // Copy of input string & pointer of split string
	
	strcpy(inputarray, input.c_str());
	ptr = strtok(inputarray, delimiters);
	// Split string by delimiters
	while(ptr != NULL)
	{
		result.push_back(ptr);
		ptr = strtok(NULL, delimiters);
	}
	return result;
}

/////////////////////////////////////////////////////////////////////
//
// Split a string with specific string pattern, i.e., a word
//
/////////////////////////////////////////////////////////////////////
vector<string> stringSplitByPattern(string input, string pattern)
{
	if(pattern.empty())
		return {};                      // Same as "return vector<string>();"
	
	string::size_type pos;              // Record the position of pattern in input string
	input += pattern;                   // Set the terminal to input string
	long size = input.size();           // Record the size of input string
	vector<string> result;              // Result list after splitting

	for(long loop = 0;loop < size;loop++)
	{
		// Find the position of pattern from loop position
		pos = input.find(pattern, loop);
		// Determine if splitting need to be terminated
		if(pos < size)
		{
			string s = input.substr(loop, pos - loop);
			result.push_back(s);
			loop = pos + pattern.size() - 1;
		}
	}
	return result;
}

/////////////////////////////////////////////////////////////////////
//
// Generate a random number with a boundary in N precision
//
/////////////////////////////////////////////////////////////////////
double genRandomNum(const char *type, long lowerbound, long upperbound, unsigned int precision, bool updeateseed)
{
	// Update the seed.
	if(updeateseed)
		genrandom.seed(chrono::system_clock::now().time_since_epoch().count());
	// Declare the distribution of random (range of random)
	uniform_int_distribution<long> distribution(lowerbound, upperbound);
	// Generate the random number in long type
	long randomnum = distribution(genrandom);
	// Convert random number to floating type if needed
	if((strcmp(type, "float") == 0) && (precision >= 0))
		return ((double)randomnum / pow(10, precision));
	else if(strcmp(type, "integer") == 0)
		return randomnum;
	else
		return -1;
}

/////////////////////////////////////////////////////////////////////
//
// Judge if the number in string type is a real number
//
/////////////////////////////////////////////////////////////////////
bool isRealNumber(string str)
{
	// Judge if the first character is not a digit and '-'/'+'
	if(str.empty() || (!isdigit(str.at(0)) && (str.at(0) != '-') && (str.at(0) != '+')))
		return false;
	// Judge if "str" is '-'/'+' or include "-."/"+."
	if((str.at(0) == '-') || (str.at(0) == '+'))
		if((str.size() == 1) || ((str.size() > 1) && (str.at(1) == '.')))
			return false;
	// Judge if "str" include two '.' or '.' at the back
	if((str.find_first_of(".") != str.find_last_of(".")) || (str.back() == '.'))
		return false;
	// Judge if each character is a digit or not
    return find_if(str.begin()+1, str.end(), [](char ch) { return !isdigit(ch) && ch != '.'; }) == str.end();
}

/////////////////////////////////////////////////////////////////////
//
// Combination Function, C(total, catchn)
// All combinations are stored in "comblist"
//
/////////////////////////////////////////////////////////////////////
void combination(long loc, long int total, int catchn, vector<long> gencomb, vector<vector<long> > *comblist)
{
	gencomb.push_back(loc-1);
	for( long loop = loc;loop <= (total-catchn) && (catchn > 0);loop++ )
		combination(loop+1, total, catchn-1, gencomb, comblist);
	if( catchn == 0 )
	{
		comblist->resize(comblist->size()+1);
		comblist->at(comblist->size()-1) = gencomb;
	}
}

/////////////////////////////////////////////////////////////////////
//
// Update all value in the list generated by "combination" function
//
/////////////////////////////////////////////////////////////////////
void updateCombinationList(vector<long> *path, vector<vector<long> > *comblist)
{
	for(long loop1 = 0;loop1 < comblist->size();loop1++)
		for(long  loop2 = 0;loop2 < comblist->at(loop1).size();loop2++)
			comblist->at(loop1).at(loop2) = path->at(comblist->at(loop1).at(loop2));
}
