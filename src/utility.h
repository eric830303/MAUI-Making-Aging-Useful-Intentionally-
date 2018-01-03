//////////////////////////////////////////////////////////////
//
// Header File
//
// File name: utility.h
// Author: Ting-Wei Chang
// Date: 2017-07
//
//////////////////////////////////////////////////////////////

#ifndef UTILITY_H
#define UTILITY_H

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <chrono>
//#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>

#ifndef DEBUG
	#define assertFunc(expr, message) _assertFunc(expr, #expr, __FILE__, __func__, __LINE__, message)
#else
	#define assertFunc(expr, message) ((void)0)
#endif

using namespace std;

inline bool isFileExist(string filename)
	{ return (access(filename.c_str(), F_OK) != -1); }
inline bool isDirectoryExist(string directory)
	{ struct stat dir = {0}; return ((stat(directory.c_str(), &dir) == 0) && S_ISDIR(dir.st_mode)); }

// Round/Ceil/Floor a number with N precision
inline double roundNPrecision(double num, int precision)
	{ return (precision >= 0) ? (round(num * pow(10, precision)) / (pow(10, precision) * 1.0)) : (num); }
inline double ceilNPrecision(double num, int precision)
	{ return (precision >= 0) ? (ceil(num * pow(10, precision)) / (pow(10, precision) * 1.0)) : (num); }
inline double floorNPrecision(double num, int precision)
	{ return (precision >= 0) ? (floor(num * pow(10, precision)) / (pow(10, precision) * 1.0)) : (num); }

void _assertFunc(bool expr, const char *exprstr, const char *file, const char *function, int line, const char *message = nullptr);
vector<string> stringSplit(string, const char *);
vector<string> stringSplitByPattern(string, string);
double genRandomNum(const char *, long, long, unsigned int precision = 0, bool updeateseed = 1);
bool isRealNumber(string);
void combination(long, int, int, vector<long>, vector<vector<long> > *);
void updateCombinationList(vector<long> *, vector<vector<long> > *);

#endif	// UTILITY_H
