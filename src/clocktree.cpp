//////////////////////////////////////////////////////////////
//
// Source File
//
// File name: clocktree.cpp
// Author: Ting-Wei Chang
// Date: 2017-07
//
//////////////////////////////////////////////////////////////

#include "clocktree.h"
#include <iterator>
#include <fstream>
#include <queue>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Skip the line when parsing the timing report
//
/////////////////////////////////////////////////////////////////////
bool ClockTree::ifSkipLine(string line)
{
	if(line.empty())
		return true;
	else if(line.find("(net)") != string::npos)
		return true;
	else if(line.find("-----") != string::npos)
		return true;
	else if(line.find("Path Group:") != string::npos)
		return true;
	else if(line.find("Path Type:") != string::npos)
		return true;
	else if(line.find("Point") != string::npos)
		return true;
	else if(line.find("clock reconvergence pessimism") != string::npos)
		return true;
	else if(line.find("clock network delay (propagated)") != string::npos)
		return true;
	else
		return false;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Count each type of critical paths and disable unused critical path
//
/////////////////////////////////////////////////////////////////////
void ClockTree::pathTypeChecking(void)
{
	switch(this->_pathlist.back()->getPathType())
	{
		case PItoFF:
			this->_pitoffnum++;
			if(this->_pathselect == 2)
				this->_pathlist.back()->setPathType(NONE);
			else
				this->_pathusednum++;
			break;
		case FFtoPO:
			this->_fftoponum++;
			if((this->_pathselect == 2) || (this->_pathselect == 1))
				this->_pathlist.back()->setPathType(NONE);
			else
				this->_pathusednum++;
			break;
		case FFtoFF:
			this->_fftoffnum++;
			this->_pathusednum++;
			break;
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Record the clock path of the startpoint/endpoint
// Input parameter:
// who: 's' => startpoint
//      'e' => endpoint
//
/////////////////////////////////////////////////////////////////////
void ClockTree::recordClockPath(char who)
{
	CriticalPath *path = this->_pathlist.back();
	ClockTreeNode *node = nullptr;
	switch(who)
	{
		// startpoint
		case 's':
			node = path->getStartPonitClkPath().at(0);
			while(node != nullptr)
			{
				node = node->getParent();
				path->getStartPonitClkPath().resize(path->getStartPonitClkPath().size()+1);
				path->getStartPonitClkPath().back() = node;
			}
			path->getStartPonitClkPath().pop_back();
			reverse(path->getStartPonitClkPath().begin(), path->getStartPonitClkPath().end());
			path->getStartPonitClkPath().shrink_to_fit();
			break;
		// endpoint
		case 'e':
			node = path->getEndPonitClkPath().at(0);
			while(node != nullptr)
			{
				node = node->getParent();
				path->getEndPonitClkPath().resize(path->getEndPonitClkPath().size()+1);
				path->getEndPonitClkPath().back() = node;
			}
			path->getEndPonitClkPath().pop_back();
			reverse(path->getEndPonitClkPath().begin(), path->getEndPonitClkPath().end());
			path->getEndPonitClkPath().shrink_to_fit();
			break;
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Record the last common node of the clock tree, i.e., the first 
// node has children from clock source
//
/////////////////////////////////////////////////////////////////////
void ClockTree::checkFirstChildrenFormRoot(void)
{
	ClockTreeNode *findnode = this->_clktreeroot;
	while(1)
	{
		if(findnode->getChildren().size() == 1)
			findnode = findnode->getChildren().at(0);
		else
		{
			long count = 0;
			ClockTreeNode *recordnode = nullptr;
			for(long loop = 0;loop < findnode->getChildren().size();loop++)
			{
				if(findnode->getChildren().at(loop)->ifUsed() == 1)
				{
					count++;
					recordnode = findnode->getChildren().at(loop);
				}
			}
			if(count == 1)
				findnode = recordnode;
			else
				break;
		}
	}
	this->_firstchildrennode = findnode;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Initial the upper and lower bound of Tc using in the Binary search
// The higher boundary for non-aging and the lower boundary for aging
//
/////////////////////////////////////////////////////////////////////
void ClockTree::initTcBound(void)
{
	this->_tcupbound  = ceilNPrecision( this->_tc * ((this->_aging) ? getAgingRateByDutyCycle(0.5) : 1.4), 1 );
	this->_tclowbound = floorNPrecision(this->_tc * 2 - this->_tcupbound, 1 );
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Generate all DCC constraint clauses according to every combinations
// Input parameter:
// comblist: combinations (2 dimension array)
//
/////////////////////////////////////////////////////////////////////
void ClockTree::genDccConstraintClause( vector<vector<long> > *comblist )
{
	for( long loop1 = 0; loop1 < comblist->size(); loop1++ )
	{
		if( comblist->at(loop1).size() == 2 )
		{
			// Generate 4 clauses based on two clock nodes
			string clause1, clause2, clause3, clause4;
			long nodenum1 = comblist->at(loop1).at(0), nodenum2 = comblist->at(loop1).at(1);
			clause1 = to_string((nodenum1 * -1)) + " " + to_string((nodenum2 * -1)) + " 0";
			clause2 = to_string((nodenum1 * -1)) + " " + to_string(((nodenum2 + 1) * -1)) + " 0";
			clause3 = to_string(((nodenum1 + 1) * -1)) + " " + to_string((nodenum2 * -1)) + " 0";
			clause4 = to_string(((nodenum1 + 1) * -1)) + " " + to_string(((nodenum2 + 1) * -1)) + " 0";
			if(this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-4))
			{
				this->_dccconstraintlist.insert(clause1);
				this->_dccconstraintlist.insert(clause2);
				this->_dccconstraintlist.insert(clause3);
				this->_dccconstraintlist.insert(clause4);
			}
			else
			{
				cerr << "\033[32m[Info]: DCC Constraint List Full!\033[0m\n";
				return;
			}
		}
		// Deal with the combination containing the number of nodes greater than two
		// (reduce to two nodes)
		else
		{
			vector<long> gencomb, simplify = comblist->at(loop1);
			vector<vector<long> > simplifylist;
			for(long loop2 = 0;loop2 <= (simplify.size()-2);loop2++)
				combination(loop2+1, simplify.size(), 1, gencomb, &simplifylist);
			updateCombinationList(&simplify, &simplifylist);
			this->genDccConstraintClause(&simplifylist);
		}
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Update the clause according to the type of DCC
// None => 00
// 20% DCC => 01
// 40% DCC => 10
// 80% DCC => 11
// Input parameter:
// dcctype: 0  => None
//          20 => 20% DCC
//          40 => 40% DCC
//          80 => 80% DCC
//
/////////////////////////////////////////////////////////////////////
void ClockTree::writeClauseByDccType( ClockTreeNode *node, string *clause, int dcctype )
{
	if((node == nullptr) || (clause == nullptr) || !node->ifPlacedDcc())
		return ;
    
	long nodenum = node->getNodeNumber();
	switch(dcctype)
	{
		case 0:
			*clause += to_string(nodenum) + " " + to_string(nodenum + 1) + " ";
			break;
		case 20:
			*clause += to_string(nodenum) + " " + to_string((nodenum + 1) * -1) + " ";
			break;
		case 40:
			*clause += to_string(nodenum * -1) + " " + to_string(nodenum + 1) + " ";
			break;
		case 80:
			*clause += to_string(nodenum * -1) + " " + to_string((nodenum + 1) * -1) + " ";
			break;
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Update timing information of the critical path in a given DCC
// deployment
// Input parameter:
// update: 0 => do not update
//         1 => update
//
/////////////////////////////////////////////////////////////////////
double ClockTree::updatePathTimingWithDcc(CriticalPath *path, bool update)
{
	if((this->_besttc == 0) || (path == nullptr))
		return -1;
	if(update)
		this->_tc = this->_besttc;
	ClockTreeNode *sdccnode = nullptr, *edccnode = nullptr;
	vector<ClockTreeNode *> tmp;
	double slack = 9999;
	switch(path->getPathType())
	{
		// Path type: input to FF
		case PItoFF:
			edccnode = path->findDccInClockPath('e');
			if(edccnode == nullptr)
				slack = this->assessTimingWithoutDcc(path, 0, update);
			else
			{
				vector<double> cjtime = this->calculateClockLatencyWithDcc(path->getEndPonitClkPath(), edccnode);
				slack = this->assessTimingWithDcc(path, 0, cjtime.front(), 0 ,0, nullptr, nullptr, tmp, tmp, 0, update);
			}
			break;
		// Path type: FF to output
		case FFtoPO:
			sdccnode = path->findDccInClockPath('s');
			if(sdccnode == nullptr)
				slack = this->assessTimingWithoutDcc(path, 0, update);
			else
			{
				vector<double> citime = this->calculateClockLatencyWithDcc(path->getStartPonitClkPath(), sdccnode);
				slack = this->assessTimingWithDcc(path, citime.front(), 0, 0 ,0, nullptr, nullptr, tmp, tmp, 0, update);
			}
			break;
		// Path type: FF to FF
		case FFtoFF:
			sdccnode = path->findDccInClockPath('s');
			edccnode = path->findDccInClockPath('e');
			if((sdccnode == nullptr) && (edccnode == nullptr))
				slack = this->assessTimingWithoutDcc(path, 0, update);
			else
			{
				vector<double> citime = this->calculateClockLatencyWithDcc(path->getStartPonitClkPath(), sdccnode);
				vector<double> cjtime = this->calculateClockLatencyWithDcc(path->getEndPonitClkPath(), edccnode);
				slack = this->assessTimingWithDcc(path, citime.front(), cjtime.front(), 0 ,0, nullptr, nullptr, tmp, tmp, 0, update);
			}
			break;
		default:
			break;
	}
	return slack;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Delete the whole clock tree for releasing memory
//
/////////////////////////////////////////////////////////////////////
void ClockTree::deleteClockTree(void)
{
	if(this->_clktreeroot == nullptr)
		return;
	queue<ClockTreeNode *> nodequeue;
	ClockTreeNode *deletenode = nullptr;
	nodequeue.push(this->_clktreeroot);
	// BFS
	while(!nodequeue.empty())
	{
		if(!nodequeue.front()->getChildren().empty())
		{
			for(auto const &nodeptr : nodequeue.front()->getChildren())
				nodequeue.push(nodeptr);
		}
		deletenode = nodequeue.front();
		nodequeue.pop();
		delete deletenode;
	}
	this->_clktreeroot = nullptr;
	this->_firstchildrennode = nullptr;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Dump all DCC information (location and type) to a file
// Reperent in: buffer_name DCC_type
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dumpDccListToFile(void)
{
	if(!this->_dumpdcc)
		return;
	fstream dccfile;
	if(this->_dumpdcc && !isDirectoryExist(this->_outputdir))
		mkdir(this->_outputdir.c_str(), 0775);
	string filename = this->_outputdir + this->_timingreportdesign + ".dcc";
	dccfile.open(filename, ios::out | fstream::trunc);
	if(!dccfile.is_open())
	{
		cerr << "\033[31m[Error]: Cannot open " << filename << "\033[0m\n";
		dccfile.close();
		return;
	}
	if(!this->_dumpdcc || this->_dcclist.empty())
		dccfile << "\n";
	else
		for(auto const& node: this->_dcclist)
			dccfile << node.first << " " << node.second->getDccType() << "\n";
	dccfile.close();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Dump all clock gating cell information (location and gating 
// probability) to a file
// Reperent in: buffer_name gating_probability
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dumpClockGatingListToFile(void)
{
	if(!this->_dumpcg)
		return;
	fstream cgfile;
	if(this->_dumpcg && !isDirectoryExist(this->_outputdir))
		mkdir(this->_outputdir.c_str(), 0775);
	string filename = this->_outputdir + this->_timingreportdesign + ".cg";
	cgfile.open(filename, ios::out | fstream::trunc);
	if(!cgfile.is_open())
	{
		cerr << "\033[31m[Error]: Cannot open " << filename << "\033[0m\n";
		cgfile.close();
		return;
	}
	if(!this->_dumpcg || this->_cglist.empty())
		cgfile << "\n";
	else
		for(auto const& node: this->_cglist)
			cgfile << node.first << " " << node.second->getGatingProbability() << "\n";
	cgfile.close();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Private Method
// Dump all inserted buffer information (location and buffer delay) 
// to a file
// Reperent in: buffer_name buffer_delay
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dumpBufferInsertionToFile(void)
{
	if(!this->_dumpbufins || (this->_insertbufnum == 0))
		return;
	fstream bufinsfile;
	if(this->_dumpbufins && !isDirectoryExist(this->_outputdir))
		mkdir(this->_outputdir.c_str(), 0775);
	string filename = this->_outputdir + this->_timingreportdesign + ".bufins";
	bufinsfile.open(filename, ios::out | fstream::trunc);
	if(!bufinsfile.is_open())
	{
		cerr << "\033[31m[Error]: Cannot open " << filename << "\033[0m\n";
		bufinsfile.close();
		return;
	}
	if(!this->_dumpbufins || (this->_insertbufnum == 0))
		bufinsfile << "\n";
	else
	{
		for(auto const& node: this->_buflist)
			if(node.second->ifInsertBuffer())
				bufinsfile << node.first << " " << node.second->getInsertBufferDelay() << "\n";
		for(auto const& node: this->_ffsink)
			if(node.second->ifInsertBuffer())
				bufinsfile << node.first << " " << node.second->getInsertBufferDelay() << "\n";
	}
	bufinsfile.close();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Destructor
//
/////////////////////////////////////////////////////////////////////
ClockTree::~ClockTree(void)
{
	this->deleteClockTree();
	for(long loop = 0;loop < this->_pathlist.size();loop++)
		this->_pathlist.at(loop)->~CriticalPath();
	this->_pathlist.clear();
	this->_pathlist.shrink_to_fit();
	this->_ffsink.clear();
	this->_buflist.clear();
	this->_cglist.clear();
	this->_dcclist.clear();
	this->_dccconstraintlist.clear();
	this->_timingconstraintlist.clear();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Check the commands from input
// Input parameter:
// message: error message
//
/////////////////////////////////////////////////////////////////////
int ClockTree::checkParameter(int argc, char **argv, string *message)
{
	if( argc == 1 )
	{
		*message = "\033[31m[ERROR]: Missing operand!!\033[0m\n";
		*message += "Try \"--help\" for more information.\n";
		return -1;
	}
	
	for(int loop = 1;loop < argc;loop++)
	{
		if(( strcmp(argv[loop], "-h") == 0 ) || ( strcmp(argv[loop], "--help") == 0) )
		{
			message->clear();
			return -1;
		}
		else if(strcmp(argv[loop], "-nondcc") == 0)
			this->_placedcc = 0;								// Do not insert DCC
		else if(strcmp(argv[loop], "-nonaging") == 0)
			this->_aging = 0;									// Non-aging
		else if(strcmp(argv[loop], "-mindcc") == 0)
			this->_mindccplace = 1;								// Minimize DCC deployment
		else if(strcmp(argv[loop], "-tc_recheck") == 0)
			this->_tcrecheck = 1;								// Recheck Tc
        else if(strcmp(argv[loop], "-print=Clause") == 0)
            this->_printClause = 1;                                // Recheck Tc
		else if(strcmp(argv[loop], "-mask_leng") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stod(string(argv[loop+1])) < 0) || (stod(string(argv[loop+1])) > 1))
			{
				*message = "\033[31m[ERROR]: Wrong length of mask!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_maskleng = stod(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-mask_level") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stol(string(argv[loop+1])) < 0))
			{
				*message = "\033[31m[ERROR]: Wrong level of mask!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_masklevel = stol(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-agingrate_tcq") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stod(string(argv[loop+1])) <= 0))
			{
				*message = "\033[31m[ERROR]: Wrong aging rate of tcq!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_agingtcq = stod(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-agingrate_dij") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stod(string(argv[loop+1])) <= 0))
			{
				*message = "\033[31m[ERROR]: Wrong aging rate of dij!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_agingdij = stod(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-agingrate_tsu") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stod(string(argv[loop+1])) <= 0))
			{
				*message = "\033[31m[ERROR]: Wrong aging rate of tsu!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_agingtsu = stod(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-cg_percent") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stod(string(argv[loop+1])) < 0) || (stod(string(argv[loop+1])) > 1))
			{
				*message = "\033[31m[ERROR]: Wrong percent of clock gating cells replacement!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_cgpercent = stod(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-gp_upbound") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stoi(string(argv[loop+1])) < 0) || (stoi(string(argv[loop+1])) > 100))
			{
				*message = "\033[31m[ERROR]: Wrong upperbound probability of clock gating!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_gpupbound = stoi(string(argv[loop+1]));
			loop++;
		}
		else if(strcmp(argv[loop], "-gp_lowbound") == 0)
		{
			if(!isRealNumber(string(argv[loop+1])) || (stoi(string(argv[loop+1])) < 0) || (stoi(string(argv[loop+1])) > 100))
			{
				*message = "\033[31m[ERROR]: Wrong lowerbound probability of clock gating!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_gplowbound = stoi(string(argv[loop+1]));
			loop++;
		}
		else if(strncmp(argv[loop], "-path=", 6) == 0)
		{
			vector<string> strspl = stringSplit(string(argv[loop]), "=");
			if(strspl.back().compare("all") == 0)
				this->_pathselect = 0;							// PItoFF, FFtoPO, and FFtoFF
			else if(strspl.back().compare("pi_ff") == 0)
				this->_pathselect = 1;							// PItoFF and FFtoFF
			else if(strspl.back().compare("onlyff") == 0)
				this->_pathselect = 2;							// FFtoFF
			else
			{
				*message = "\033[31m[ERROR]: Wrong path selection!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
		}
		else if(strncmp(argv[loop], "-clockgating=", 13) == 0)
		{
			vector<string> strspl = stringSplit(string(argv[loop]), "=");
			if(strspl.back().compare("no") == 0)
				this->_clkgating = 0;							// Do not replace buffers
			else if(strspl.back().compare("yes") == 0)
				this->_clkgating = 1;							// Replace buffers
			else
			{
				*message = "\033[31m[ERROR]: Wrong clock gating setting!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
		}
		else if(strncmp(argv[loop], "-gatingfile=", 12) == 0)
		{
			vector<string> strspl = stringSplit(string(argv[loop]), "=");
			if(!isFileExist(strspl.back()))
			{
				*message = "\033[31m[ERROR]: Gating file not found!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			this->_cgfilename.assign(strspl.back());
		}
		else if(strncmp(argv[loop], "-bufinsert=", 11) == 0)
		{
			vector<string> strspl = stringSplit(string(argv[loop]), "=");
			if(strspl.back().compare("insert") == 0)
				this->_bufinsert = 1;							// Insert buffers
			else if(strspl.back().compare("min_insert") == 0)
				this->_bufinsert = 2;							// Insert buffers and minimize buffer insertion
			else
			{
				*message = "\033[31m[ERROR]: Wrong Buffer insertion setting!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
		}
		else if(strncmp(argv[loop], "-dump=", 6) == 0)
		{
			vector<string> strspl = stringSplit(string(argv[loop]), "=");
			if(strspl.back().compare("dcc") == 0)
				this->_dumpdcc = 1;								// Dump DCC list
			else if(strspl.back().compare("cg") == 0)
				this->_dumpcg = 1;								// Dump clock gating cell list
			else if(strspl.back().compare("buf_ins") == 0)
				this->_dumpbufins = 1;							// Dump inserted buffer list
			else if(strspl.back().compare("all") == 0)
			{
				this->_dumpdcc = 1;
				this->_dumpcg = 1;
				this->_dumpbufins = 1;
			}
		}
		else if(isFileExist(string(argv[loop])))
		{
			// Check if the timing report exist or not
			if(!this->_timingreportfilename.empty())
			{
				*message = "\033[31m[ERROR]: Too many timing reports!!\033[0m\n";
				*message += "Try \"--help\" for more information.\n";
				return -1;
			}
			char path[100] = {'\0'}, *pathptr = nullptr;
			this->_timingreport.assign(argv[loop]);
			vector<string> strspl = stringSplit(this->_timingreport, "/");
			this->_timingreportfilename.assign(strspl.back());
			realpath(argv[loop], path);
			pathptr = strrchr(path, '/');
			path[pathptr - path + 1] = '\0';
			this->_timingreportloc.assign(path);
		}
		else
		{
			*message = "\033[31m[ERROR]: Missing operand!!\033[0m\n";
			*message += "Try \"--help\" for more information.\n";
			return -1;
		}
	}
	if(_timingreportfilename.empty())
	{
		*message = "\033[31m[ERROR]: Missing timing report!!\033[0m\n";
		*message += "Try \"--help\" for more information.\n";
		return -1;
	}
	if(this->_gplowbound > this->_gpupbound)
	{
		*message = "\033[31m[ERROR]: Lower bound probability can't greater than upper bound!!\033[0m\n";
		*message += "Try \"--help\" for more information.\n";
		return -1;
	}
	if(!this->_aging)
	{
		// Non-aging for Tcq, Dij, and Tsu
		this->_agingtcq = 1, this->_agingdij = 1, this->_agingtsu = 1;
	}
	
	return 0;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Parse the timing report
//
/////////////////////////////////////////////////////////////////////
void ClockTree::parseTimingReport(void)
{
	fstream tim_max;
	long pathnum = -1, maxlevel = -1;
	bool startscratchfile = false, pathstart = false, firclkedge = false;
	double clksourlate = 0, clktime = 0;
	string line;
	ClockTreeNode *parentnode = nullptr;
	
	tim_max.open(this->_timingreport, ios::in);
	if(!tim_max.is_open())
	{
		cerr << "\033[31m[Error]: Cannot open " << this->_timingreport << "\033[0m\n";
		tim_max.close();
		abort();
	}
	
	while(!tim_max.eof())
	{
		getline(tim_max, line);
		if(line.find("Design :") != string::npos)
		{
			vector<string> strspl;
			strspl = stringSplit(line, " ");
			this->_timingreportdesign = strspl.at(2);
		}
		if((startscratchfile == false) && (line.find("Startpoint") != string::npos))
		{
			startscratchfile = true;
			cout << "\033[32m[Info]: Bilding clock tree...\033[0m\n";
		}
		if(startscratchfile)
		{
			// End of the timing report
			if((line.length() == 1) && (line.compare("1") == 0))
			{
				startscratchfile = false;
				cout << "\033[32m[Info]: Clock tree complete.\033[0m\n";
				break;
			}
			
			if(this->ifSkipLine(line))
				continue;
			else
			{
				vector<string> strspl;
				strspl = stringSplit(line, " ");
				if(strspl.empty())
					continue;
				if(line.find("Startpoint") != string::npos)
				{
					int type = NONE;
					if(strspl.size() > 2)
					{
						if(line.find("input port") != string::npos)
							type = PItoPO;
						else if(line.find("flip-flop") != string::npos)
							type = FFtoPO;
					}
					else
					{
						getline(tim_max, line);
						if(line.find("input port") != string::npos)
							type = PItoPO;
						else if(line.find("flip-flop") != string::npos)
							type = FFtoPO;
					}
					pathnum++;
					CriticalPath *path = new CriticalPath(strspl.at(1), type, pathnum);
					this->_pathlist.resize(this->_pathlist.size()+1);
					this->_pathlist.back() = path;
				}
				else if(line.find("Endpoint") != string::npos)
				{
					CriticalPath *path = this->_pathlist.back();
					path->setEndPointName(strspl.at(1));
					if(strspl.size() > 2)
					{
						if(line.find("flip-flop") != string::npos)
							path->setPathType(path->getPathType()+1);
					}
					else
					{
						getline(tim_max, line);
						if(line.find("flip-flop") != string::npos)
							path->setPathType(path->getPathType()+1);
					}
					this->pathTypeChecking();
				}
				else if(line.find("(rise edge)") != string::npos)
				{
					if(this->_clktreeroot == nullptr)
					{
						ClockTreeNode *node = new ClockTreeNode(nullptr, this->_totalnodenum, 0, 1);
						node->getGateData()->setGateName(strspl.at(1));
						node->getGateData()->setWireTime(stod(strspl.at(4)));
						node->getGateData()->setGateTime(stod(strspl.at(4)));
						// Assign two numbers to a node
						this->_totalnodenum += 2;
						this->_clktreeroot = node;
						//this->_buflist.insert(pair<string, ClockTreeNode *> (strspl.at(1), node));
					}
					if(firclkedge)
					{
						firclkedge = false;
						this->_origintc = stod(strspl.at(5)) - clktime;
					}
					else
					{
						firclkedge = true;
						clktime = stod(strspl.at(5));
					}
				}
				else if(line.find("clock source latency") != string::npos)
					clksourlate = stod(strspl.at(3));
				else if(line.find("data arrival time") != string::npos)
					this->_pathlist.back()->setArrivalTime(stod(strspl.at(3)));
				else if(line.find("data required time") != string::npos)
					this->_pathlist.back()->setRequiredTime(stod(strspl.at(3)) + abs(this->_pathlist.back()->getClockUncertainty()));
				else if(line.find("input external delay") != string::npos)
				{
					if((this->_pathlist.back()->getPathType() == PItoPO) || (this->_pathlist.back()->getPathType() == PItoFF))
						this->_pathlist.back()->setTinDelay(stod(strspl.at(3)));
				}
				else if(line.find("output external delay") != string::npos)
				{
					// Assign to Tsu
					if((this->_pathlist.back()->getPathType() == PItoPO) || (this->_pathlist.back()->getPathType() == FFtoPO))
						this->_pathlist.back()->setTsu(stod(strspl.at(3)));
				}
				else if(line.find("clock uncertainty") != string::npos)
					this->_pathlist.back()->setClockUncertainty(stod(strspl.at(2)));
				else if(line.find("library setup time") != string::npos)
					this->_pathlist.back()->setTsu(stod(strspl.at(3)));
				// Clock source
				else if(line.find("(in)") != string::npos)
				{
					if(strspl.at(0).compare(this->_clktreeroot->getGateData()->getGateName()) == 0)	// clock input
						parentnode = this->_clktreeroot;
					else if(strspl.at(0).compare(this->_pathlist.back()->getStartPointName()) == 0)		// input port
					{
						pathstart = true;
						GateData *pathnode = new GateData(this->_pathlist.back()->getStartPointName(), stod(strspl.at(3)), 0);
						this->_pathlist.back()->getGateList().resize(this->_pathlist.back()->getGateList().size()+1);
						this->_pathlist.back()->getGateList().back() = pathnode;
					}
				}
				else if(line.find("slack (") != string::npos)
				{
					this->_pathlist.back()->setSlack(stod(strspl.at(2)) + abs(this->_pathlist.back()->getClockUncertainty()));
					switch(this->_pathlist.back()->getPathType())
					{
						case PItoFF:
							this->recordClockPath('e');
							break;
						case FFtoPO:
							this->recordClockPath('s');
							break;
						case FFtoFF:
							this->recordClockPath('s');
							this->recordClockPath('e');
							break;
						default:
							break;
					}
					this->_pathlist.back()->getGateList().shrink_to_fit();
					pathstart = false, firclkedge = false, parentnode = nullptr;
				}
				else
				{
					CriticalPath *path = this->_pathlist.back();
					vector<string> namespl;
					bool scratchnextline = false;
					namespl = stringSplit(strspl.at(0), "/");
					if(line.find("/") != string::npos)
						scratchnextline = true;
					// Deal with combinational logic nodes
					if(pathstart)
					{
						GateData *pathnode = new GateData(namespl.at(0), stod(strspl.at(3)), 0);
						if(namespl.at(0).compare(path->getEndPointName()) == 0)
						{
							pathstart = false;
							path->setDij(stod(strspl.at(5)) - path->getCi() - path->getTcq() - path->getTinDelay());
						}
						if(scratchnextline && pathstart)
						{
							getline(tim_max, line);
							strspl = stringSplit(line, " ");
							pathnode->setGateTime(stod(strspl.at(3)));
						}
						path->getGateList().resize(path->getGateList().size()+1);
						path->getGateList().back() = pathnode;
					}
					// Deal with clock tree buffers
					else
					{
						ClockTreeNode *findnode = nullptr;
						bool sameinsameout = false;
						if((this->_pathlist.back()->getStartPointName().compare(path->getEndPointName()) == 0) && (path->getStartPonitClkPath().size() > 0))
							sameinsameout = true;
						// Meet the startpoint FF/input
						if((namespl.at(0).compare(path->getStartPointName()) == 0) && (sameinsameout == false))
						{
							pathstart = true;
							findnode = parentnode->searchChildren(namespl.at(0));
							path->setCi(stod(strspl.at(5)));
							if(findnode == nullptr)
							{
								ClockTreeNode *node = new ClockTreeNode(parentnode, this->_totalnodenum, parentnode->getDepth()+1);
								node->getGateData()->setGateName(namespl.at(0));
								node->getGateData()->setWireTime(stod(strspl.at(3)));
								this->_ffsink.insert(pair<string, ClockTreeNode *> (namespl.at(0), node));
								if((path->getPathType() == PItoPO) || (path->getPathType() == NONE))
									node->setIfUsed(0);
								parentnode->getChildren().resize(parentnode->getChildren().size()+1);
								parentnode->getChildren().back() = node;
								if(node->getDepth() > maxlevel)
									maxlevel = node->getDepth();
								// Assign two numbers to a node
								this->_totalnodenum += 2;
								findnode = node, parentnode = nullptr;
							}
							if(scratchnextline)
							{
								getline(tim_max, line);
								strspl = stringSplit(line, " ");
								findnode->getGateData()->setGateTime(stod(strspl.at(3)));
							}
							GateData *pathnode = new GateData(path->getStartPointName(), findnode->getGateData()->getWireTime(), findnode->getGateData()->getGateTime());
							path->getGateList().resize(path->getGateList().size()+1);
							path->getGateList().back() = pathnode;
							path->getStartPonitClkPath().resize(path->getStartPonitClkPath().size()+1);
							path->getStartPonitClkPath().back() = findnode;
							if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoPO) || (path->getPathType() == FFtoFF))
							{
								if(findnode->ifUsed() != 1)
									this->_ffusednum++;
								findnode->setIfUsed(1);
							}
							if((path->getPathType() == FFtoPO) || (path->getPathType() == FFtoFF))
								path->setTcq(findnode->getGateData()->getGateTime());
						}
						// Meet the endpoint FF/output
						else if(!firclkedge && (namespl.at(0).compare(path->getEndPointName()) == 0))
						{
							findnode = parentnode->searchChildren(namespl.at(0));
							if(findnode == nullptr)
							{
								ClockTreeNode *node = new ClockTreeNode(parentnode, this->_totalnodenum, parentnode->getDepth()+1);
								node->getGateData()->setGateName(namespl.at(0));
								node->getGateData()->setWireTime(stod(strspl.at(3)));
								this->_ffsink.insert(pair<string, ClockTreeNode *> (namespl.at(0), node));
								if((path->getPathType() == PItoPO) || (path->getPathType() == NONE))
									node->setIfUsed(0);
								parentnode->getChildren().resize(parentnode->getChildren().size()+1);
								parentnode->getChildren().back() = node;
								if(node->getDepth() > maxlevel)
									maxlevel = node->getDepth();
								// Assign two numbers to a node
								this->_totalnodenum += 2;
								findnode = node, parentnode = nullptr;
							}
							path->getEndPonitClkPath().resize(path->getEndPonitClkPath().size()+1);
							path->getEndPonitClkPath().back() = findnode;
							path->setCj(stod(strspl.at(5)) - this->_origintc);
							if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoPO) || (path->getPathType() == FFtoFF))
							{
								if(findnode->ifUsed() != 1)
									this->_ffusednum++;
								findnode->setIfUsed(1);
							}
						}
						// Meet clock buffers
						else
						{
							findnode = parentnode->searchChildren(namespl.at(0));
							if(findnode == nullptr)
							{
								ClockTreeNode *node = new ClockTreeNode(parentnode, this->_totalnodenum, parentnode->getDepth()+1);
								node->getGateData()->setGateName(namespl.at(0));
								node->getGateData()->setWireTime(stod(strspl.at(3)));
								this->_buflist.insert(pair<string, ClockTreeNode *> (namespl.at(0), node));
								if((path->getPathType() == PItoPO) || (path->getPathType() == NONE))
									node->setIfUsed(0);
								parentnode->getChildren().resize(parentnode->getChildren().size()+1);
								parentnode->getChildren().back() = node;
								// Assign two numbers to a node
								this->_totalnodenum += 2;
								if(scratchnextline)
								{
									getline(tim_max, line);
									strspl = stringSplit(line, " ");
									node->getGateData()->setGateTime(stod(strspl.at(3)));
								}
								findnode = node, parentnode = node;
							}
							else
							{
								parentnode = findnode;
								if(scratchnextline)
									getline(tim_max, line);
							}
							if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoPO) || (path->getPathType() == FFtoFF))
							{
								if(findnode->ifUsed() != 1)
									this->_bufferusednum++;
								findnode->setIfUsed(1);
							}
						}
					}
				}
			}
		}
	}
	
	tim_max.close();
	this->_totalnodenum /= 2;
	this->_maxlevel = maxlevel;
	this->checkFirstChildrenFormRoot();
	this->_outputdir = "./" + this->_timingreportdesign + "_output/";
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Replace buffers to clock gating cells
//
/////////////////////////////////////////////////////////////////////
void ClockTree::clockGatingCellReplacement(void)
{
	if( !this->_clkgating )
		return;
	// Get the replacement from cg file if the cg file exist
	if(!this->_cgfilename.empty())
	{
		fstream cgfile;
		string line;
		cout << "\033[32m[Info]: Open the setting file of clock gating cells.\033[0m\n";
		cgfile.open(this->_cgfilename, ios::in);
		if(!cgfile.is_open())
		{
			cerr << "\033[31m[Error]: Cannot open " << this->_cgfilename << "\033[0m\n";
			cgfile.close();
			return;
		}
		cout << "\033[32m[Info]: Replacing some buffers to clock gating cells...\033[0m\n";
		while(!cgfile.eof())
		{
			getline(cgfile, line);
			if(line.empty())
				continue;
			vector<string> strspl = stringSplit(line, " ");
			map<string, ClockTreeNode *>::iterator findptr = this->_buflist.find(strspl.at(0));
			if(findptr != this->_buflist.end())
			{
				if((strspl.size() == 2) && isRealNumber(strspl.at(1)))
					findptr->second->setGatingProbability(stod(strspl.at(1)));
				else
					findptr->second->setGatingProbability(genRandomNum("float", this->_gpupbound, this->_gplowbound, 2));
				findptr->second->setIfClockGating(1);
				this->_cglist.insert(pair<string, ClockTreeNode *> (findptr->second->getGateData()->getGateName(), findptr->second));
			}
		}
		cgfile.close();
	}
	// Replace buffers randomly to clock gating cells
	else
	{
		cout << "\033[32m[Info]: Replacing some buffers to clock gating cells...\033[0m\n";
		map<string, ClockTreeNode *> buflist(this->_buflist);
		map<string, ClockTreeNode *>::iterator nodeitptr = buflist.begin();
		for(long loop1 = 0;loop1 < (long)(this->_bufferusednum * this->_cgpercent);loop1++)
		{
			if(buflist.empty() || ((long)(this->_bufferusednum * this->_cgpercent) == 0))
				break;
			long rannum = (long)genRandomNum("integer", 0, buflist.size()-1);
			nodeitptr = buflist.begin(), advance(nodeitptr, rannum);
			ClockTreeNode *picknode = nodeitptr->second;
			if((picknode->ifUsed() == 1) && (picknode->ifClockGating() == 0))
			{
				bool breakflag = true;
				for(auto const &nodeptr : picknode->getChildren())
				{
					if(!nodeptr->getChildren().empty())
					{
						breakflag = false;
						break;
					}
				}
				if(breakflag)
				{
					loop1--;
					buflist.erase(nodeitptr);
					continue;
				}
				map<string, ClockTreeNode *>::iterator findptr;
				// Replace the buffer if it has brothers/sisters
				if((picknode->getParent()->getChildren().size() > 1) || (picknode->getParent() == this->_clktreeroot))
				{
					picknode->setIfClockGating(1);
					picknode->setGatingProbability(genRandomNum("float", this->_gplowbound, this->_gpupbound, 2));
					this->_cglist.insert(pair<string, ClockTreeNode *> (picknode->getGateData()->getGateName(), picknode));
				}
				// Deal with the buffer if it does not has brothers/sisters
				// Replace the parent buffer of the buffer
				else
				{
					findptr = buflist.find(picknode->getParent()->getGateData()->getGateName());
					if(findptr != buflist.end())
					{
						ClockTreeNode *nodeptr = picknode->getParent();
						while((nodeptr->getParent()->getChildren().size() == 1) && (nodeptr->getParent() != this->_clktreeroot))
							nodeptr = nodeptr->getParent();
						nodeptr->setIfClockGating(1);
						nodeptr->setGatingProbability(genRandomNum("float", this->_gplowbound, this->_gpupbound, 2));
						this->_cglist.insert(pair<string, ClockTreeNode *> (nodeptr->getGateData()->getGateName(), nodeptr));
						while(nodeptr != picknode)
						{
							findptr = buflist.find(nodeptr->getGateData()->getGateName());
							if(findptr != buflist.end())
								buflist.erase(findptr);
							nodeptr = nodeptr->getChildren().front();
						}
					}
					else
						loop1--;
				}
				buflist.erase(picknode->getGateData()->getGateName());
				if(picknode->getChildren().size() == 1)
				{
					findptr = buflist.find(picknode->getChildren().front()->getGateData()->getGateName());
					if(findptr != buflist.end())
						buflist.erase(findptr);
				}
			}
			else
			{
				loop1--;
				buflist.erase(nodeitptr);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Adjust the primitive Tc in the timing report to a suitable Tc
// using in the start of Binary search
//
/////////////////////////////////////////////////////////////////////
void ClockTree::adjustOriginTc(void)
{
	double minslack = 999, tcdiff = 0;
	CriticalPath *minslackpath = nullptr;
	// Find the critical path dominating Tc
	for( auto const &pathptr : this->_pathlist )
	{
		if(( pathptr->getPathType() == NONE) || pathptr->getPathType() == PItoPO )
			continue;
		if( min(pathptr->getSlack(), minslack ) != minslack )
		{
			minslack = min(pathptr->getSlack(), minslack);
			minslackpath = pathptr;
		}
	}
	if( minslack < 0 )
		tcdiff = abs(minslackpath->getSlack());
	else if(minslack > (this->_origintc * (roundNPrecision(getAgingRateByDutyCycle(0.5), PRECISION) - 1)))
		tcdiff = (this->_origintc - floorNPrecision(((this->_origintc - minslackpath->getSlack()) * 1.2), PRECISION)) * -1;
	if( tcdiff != 0 )
	{
		// Update the required time and slack of each critical path
		for(auto const &pathptr : this->_pathlist )
		{
			if( (pathptr->getPathType() == NONE) || (pathptr->getPathType() == PItoPO) )
				continue;
			pathptr->setRequiredTime(pathptr->getRequiredTime() + tcdiff);
			pathptr->setSlack(pathptr->getSlack() + tcdiff);
		}
	}
	// Adjust Tc
	this->_tc = this->_origintc + tcdiff;
	// Initial the boundary of Tc using in Binary search
	this->initTcBound();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Prohibit DCCs inserting below the mask
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dccPlacementByMasked(void)
{
	if( !this->_placedcc )//-nondcc
		return ;
	long pathcount = 0;
	for( auto const &pathptr : this->_pathlist )
	{
		//if(pathcount > (long)(this->_pathusednum * PATHMASKPERCENT))
		//	break;
		bool dealstart = 0, dealend = 0 ;
		ClockTreeNode *sameparent = this->_firstchildrennode;
		switch( pathptr->getPathType() )
		{
			// Path type: input to FF
			case PItoFF:
				dealend = 1   ;
				break;
			// Path type: FF to output
			case FFtoPO:
				dealstart = 1 ;
				break;
			// Path type: FF to FF
			case FFtoFF:
				if( pathptr->isEndPointSameAsStartPoint() )
				{
					pathcount++  ;
					continue     ;
				}
				sameparent = pathptr->findLastSameParentNode();
				dealstart = 1 ;
				dealend   = 1 ;
				break            ;
			default:
				continue;
		}
		pathcount++;
		// Deal with the clock path of the startpoint
		if( dealstart )
		{
			vector< ClockTreeNode * > starttemp = pathptr->getStartPonitClkPath();//return by reference
			starttemp.pop_back() ;
            //front = root, back = FF
			reverse( starttemp.begin(), starttemp.end() );
            //front = FF, back = root
			// Ignore the common nodes
			while( 1 )
			{
				if( starttemp.back() != sameparent )
					starttemp.pop_back();
				else if(starttemp.back() == sameparent)
				{
					starttemp.pop_back();
					reverse(starttemp.begin(), starttemp.end());
                    
					break;
				}
			}
			long clkpathlen = (long)(starttemp.size() * this->_maskleng);
			// Mask by length
			for( long loop = 0;loop < clkpathlen;loop++ )//when loop=0, it mean same parent-1
				starttemp.pop_back() ;
			// Mask by level
			for( auto const &nodeptr : starttemp )
				if( nodeptr->getDepth() <= (this->_maxlevel - this->_masklevel) )
					nodeptr->setIfPlaceDcc(1);
		}
		// Deal with the clock path of the endpoint
		if( dealend )
		{
			vector<ClockTreeNode *> endtemp = pathptr->getEndPonitClkPath();//return by reference
			endtemp.pop_back();//delete FF?
            //endtemp[0]=root, endtemp[tail]=FF+1
			reverse(endtemp.begin(), endtemp.end());
            //endtemp[0]=FF+1, endtemp[tail]=root
			// Ignore the common nodes
			while(1)
			{
				if( endtemp.back() != sameparent )
					endtemp.pop_back();
				else if( endtemp.back() == sameparent )
				{
					endtemp.pop_back();
					reverse(endtemp.begin(), endtemp.end());
                    //endtemp[0]=sameparent-1, endtemp[tail]=FF+1
					break;
				}
			}
			long clkpathlen = (long)(endtemp.size() * this->_maskleng);
			// Mask by length
			for( long loop = 0;loop < clkpathlen;loop++ )
				endtemp.pop_back();
			// Mask by level
			for( auto const &nodeptr : endtemp )
				if(nodeptr->getDepth() <= (this->_maxlevel - this->_masklevel))
					nodeptr->setIfPlaceDcc(1);
		}
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// DCC constraint and generate clauses
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dccConstraint(void)
{
	if( !this->_placedcc )
	{
		this->_nonplacedccbufnum = this->_buflist.size();
		return;
	}
	// Generate two clauses for the clock tree root (clock source)
	if( this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-2) )
	{
		string clause1 = to_string(this->_clktreeroot->getNodeNumber() * -1) + " 0";
		string clause2 = to_string((this->_clktreeroot->getNodeNumber() + 1) * -1) + " 0";
		this->_dccconstraintlist.insert(clause1);
		this->_dccconstraintlist.insert(clause2);
	}
	else
	{
		cerr << "\033[32m[Info]: DCC Constraint List Full!\033[0m\n";
		return;
	}
	// Generate two clauses for each buffer can not insert DCC
	for( auto const& node: this->_buflist )//_buflist = map< string, clknode * >
	{
		if( !node.second->ifPlacedDcc() )
		{
			string clause1, clause2;
			clause1 = to_string(node.second->getNodeNumber() * -1) + " 0";
			clause2 = to_string((node.second->getNodeNumber() + 1) * -1) + " 0";
			if(this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-2))
			{
				this->_dccconstraintlist.insert(clause1);
				this->_dccconstraintlist.insert(clause2);
			}
			else
			{
				cerr << "\033[32m[Info]: DCC Constraint List Full!\033[0m\n";
				return;
			}
		}
	}
	this->_nonplacedccbufnum = (this->_dccconstraintlist.size() / 2) - 1;
	// Generate clauses based on DCC constraint in each clock path
	for( auto const& node: this->_ffsink )
	{
        //-- Don't Put DCC ahead of FF
		string clause1, clause2;
		clause1 = to_string(node.second->getNodeNumber() * -1) + " 0";
		clause2 = to_string((node.second->getNodeNumber() + 1) * -1) + " 0";
		if(this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-2))
		{
			this->_dccconstraintlist.insert(clause1);
			this->_dccconstraintlist.insert(clause2);
		}
		else
		{
			cerr << "\033[32m[Info]: DCC Constraint List Full!\033[0m\n";
			return;
		}
		ClockTreeNode *nodeptr = node.second->getParent() ;//FF's parent
		vector<long>          path, gencomb;
		vector<vector<long> > comblist;
		while( nodeptr != this->_firstchildrennode )
		{
			if( nodeptr->ifPlacedDcc() )
				path.push_back( nodeptr->getNodeNumber() );
			nodeptr = nodeptr->getParent();
		}
		path.shrink_to_fit();
		reverse(path.begin(), path.end());
		// Combination of DCC constraint
        // Can't Put more than 2 DCC along the same clock path
		for( long loop = 0; loop <= ((long)path.size()) - 2; loop++ )
			combination( loop+1, (int)(path.size()), 1, gencomb, &comblist );
		updateCombinationList( &path, &comblist );
		// Generate clauses
		this->genDccConstraintClause(&comblist);
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Generate all kinds of DCC deployment (location of DCC excluding 
// the types of DCC) in each critical path
//
/////////////////////////////////////////////////////////////////////
void ClockTree::genDccPlacementCandidate(void)
{
	if( !this->_placedcc )
		return ;
	for( auto const& path: this->_pathlist )
		if( (path->getPathType() != NONE) && (path->getPathType() != PItoPO) )
			path->setDccPlacementCandidate();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Timing constraint and generate clauses based on each DCC
// deployment
//
/////////////////////////////////////////////////////////////////////
double ClockTree::timingConstraint(void)
{
    printf( YELLOW"\t[Timing Constraint] " RESET"Tc range: %f - %f ...\033[0m\n", this->_tclowbound, this->_tcupbound ) ;
	double slack    = 0   ;
    double minslack = 100 ;//Eric
	this->_timingconstraintlist.clear();//set<string>
	for( auto const& path: this->_pathlist )
	{
		if( (path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF) )
			continue;
		// Assess if the critical path occurs timing violation without inserting any DCC
		slack = this->assessTimingWithoutDcc(path);
		// Assess if the critical path occurs timing violation when inserting DCCs
		if( this->_placedcc )//-nodcc
			this->assessTimingWithDcc(path);
        /*Senior
		if( (slack < 0) && (!this->_placedcc) )
			return slack;
         */
        minslack = (slack<minslack)?(slack):(minslack);
	}
	return minslack;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Assess if the critical path occurs timing violation without
// inserting any DCC (violating the setup time requirement)
// Input parameter:
// genclause: 0           => Do not generate a clause
//            1 (default) => Generate clauses (default)
// update: 0 (default) => Do not update the timing information of critical path
//         1           => Update the timing information of critical path
/////////////////////////////////////////////////////////////////////
double ClockTree::assessTimingWithoutDcc( CriticalPath *path, bool genclause, bool update )
{
    /* Note:
     writeClauseByDccType( nodeptr, &clause, 0 ), the last arg is 0, because as title imply
     we access timing without Dcc
    */
    
	if( path == nullptr )
		return -1 ;
    //------ Declare ------------------------------------------------------------------
    double newslack    = 0 ;
    double dataarrtime = 0 ;
    double datareqtime = 0 ;
	
    //------- Ci & Cj ------------------------------------------------------------------
	this->calculateClockLatencyWithoutDcc( path, &datareqtime, &dataarrtime );
	if( update )
    {
        path->setCi(dataarrtime) ;
        path->setCj(datareqtime) ;
    }
	//------- Require time --------------------------------------------------------------
	datareqtime += (path->getTsu() * this->_agingtsu) + this->_tc;
    
	//------- Arrival time --------------------------------------------------------------
	dataarrtime += path->getTinDelay() + (path->getTcq() * this->_agingtcq) + (path->getDij() * this->_agingdij);
	newslack = datareqtime - dataarrtime  ;

	if( update )
    {
        path->setArrivalTime(dataarrtime) ;
        path->setRequiredTime(datareqtime);
        path->setSlack(newslack)          ;
    }
		
	//-------- Timing Violation ---------------------------------------------------------
	if( (newslack < 0) && this->_placedcc && genclause)
	{
		string clause;
		//---- PItoFF or FFtoPO --------------------------------------------------------
		if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoPO))
		{
			vector<ClockTreeNode *> clkpath = ((path->getPathType() == PItoFF) ? path->getEndPonitClkPath() : path->getStartPonitClkPath());
			//- Generate Clause --------------------------------------------------------
			for( auto const& nodeptr: clkpath )
            {
				if( nodeptr->ifPlacedDcc() )    this->writeClauseByDccType( nodeptr, &clause, 0 ) ;
            }
		}
		else if( path->getPathType() == FFtoFF )//-- FFtoFF -----------------------------
		{
			ClockTreeNode *sameparent = path->findLastSameParentNode();
			//- Generate Clause/Left ----------------------------------------------------
			long sameparentloc = path->nodeLocationInClockPath('s', sameparent);
			for( auto const& nodeptr: path->getStartPonitClkPath() )
				if( nodeptr->ifPlacedDcc() )
					this->writeClauseByDccType( nodeptr, &clause, 0 ) ;
			//- Generate Clause/Right ---------------------------------------------------
			for( long loop = (sameparentloc + 1);loop < path->getEndPonitClkPath().size(); loop++ )
				if( path->getEndPonitClkPath().at(loop)->ifPlacedDcc() )
					this->writeClauseByDccType( path->getEndPonitClkPath().at(loop), &clause, 0 );
		}
		clause += "0";
		if( this->_timingconstraintlist.size() < (this->_timingconstraintlist.max_size()-1) )
			this->_timingconstraintlist.insert(clause) ;
		else
			cerr << "\033[32m[Info]: Timing Constraint List Full!\033[0m\n";
	}
	return newslack;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// According to each DCC deployment, calculate the Ci and Cj and 
// assess if the critical path occurs timing violation when inserting
// DCCs (violating the setup time requirement)
//
/////////////////////////////////////////////////////////////////////
void ClockTree::assessTimingWithDcc(CriticalPath *path)
{
	if(path == nullptr)
		return;
	vector<vector<ClockTreeNode *> > dcccandi = path->getDccPlacementCandi();
	// Consider every DCC deployment of this critical path
	for( long loop = 0; loop < dcccandi.size(); loop++ )
	{
		vector<double> citime, cjtime;
		// Path type: input to FF
		if( path->getPathType() == PItoFF )
		{
			// Calculate the Cj
			cjtime = this->calculateClockLatencyWithDcc(path->getEndPonitClkPath(), dcccandi.at(loop).front());//return a vector
			// Assess if the critical path occurs timing violation when inserting 20%/40%/80% DCC//back()?
            //Error here (maybe), argc is not consistent
			this->assessTimingWithDcc(path, 0, cjtime.at(0), 20, -1, dcccandi.at(loop).front(), nullptr, path->getEndPonitClkPath());
			this->assessTimingWithDcc(path, 0, cjtime.at(1), 40, -1, dcccandi.at(loop).front(), nullptr, path->getEndPonitClkPath());
			this->assessTimingWithDcc(path, 0, cjtime.at(2), 80, -1, dcccandi.at(loop).front(), nullptr, path->getEndPonitClkPath());
		}
		// Path type: FF to output
		else if( path->getPathType() == FFtoPO )
		{
			// Calculate the Ci
			citime = this->calculateClockLatencyWithDcc(path->getStartPonitClkPath(), dcccandi.at(loop).front());
			// Assess if the critical path occurs timing violation when inserting 20%/40%/80% DCC
			this->assessTimingWithDcc(path, citime.at(0), 0, 20, -1, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath());
			this->assessTimingWithDcc(path, citime.at(1), 0, 40, -1, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath());
			this->assessTimingWithDcc(path, citime.at(2), 0, 80, -1, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath());
		}
		// Path type: FF to FF
		else if( path->getPathType() == FFtoFF )
		{
			long candilocleft  = path->nodeLocationInClockPath('s', dcccandi.at(loop).back() /*Clk node*/ );//location id, 's' mean start clk path
			long candilocright = path->nodeLocationInClockPath('e', dcccandi.at(loop).back() /*Clk node*/ );//location id, 'e' mean end   clk path
			long sameparentloc = path->nodeLocationInClockPath('s', path->findLastSameParentNode());
			// Insert one DCC
			if( dcccandi.at(loop).size() == 1 )
			{
				// Insert DCC on common node
				if((candilocleft != -1) && (candilocleft <= sameparentloc))
				{
					// Calculate the Ci and Cj
					citime = this->calculateClockLatencyWithDcc(path->getStartPonitClkPath(), dcccandi.at(loop).front());
					cjtime = this->calculateClockLatencyWithDcc(path->getEndPonitClkPath(), dcccandi.at(loop).front());
                    //citime.at(0), put 20% DCC on the location of dcccandi.at(loop).front()
                    //citime.at(1), put 40% DCC on the location of dcccandi.at(loop).front()
                    //citime.at(2), put 80% DCC on the location of dcccandi.at(loop).front()
					// Assess if the critical path occurs timing violation when inserting 20%/40%/80% DCC
					this->assessTimingWithDcc(path, citime.at(0), cjtime.at(0), 20, 20, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath(), path->getEndPonitClkPath());
					this->assessTimingWithDcc(path, citime.at(1), cjtime.at(1), 40, 40, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath(), path->getEndPonitClkPath());
					this->assessTimingWithDcc(path, citime.at(2), cjtime.at(2), 80, 80, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath(), path->getEndPonitClkPath());
				}
                // Insert DCC on the branch part of right clk path:OK
				else if( candilocleft < candilocright )
				{
					double newcitime = 0;
					// Calculate the Ci and Cj
					this->calculateClockLatencyWithoutDcc( path, nullptr, &newcitime );
					cjtime = this->calculateClockLatencyWithDcc(path->getEndPonitClkPath(), dcccandi.at(loop).front());
					// Assess if the critical path occurs timing violation when inserting 20%/40%/80% DCC
                    // Generate clause here:
					this->assessTimingWithDcc(path, newcitime, cjtime.at(0), 20, 0, dcccandi.at(loop).front(), nullptr, path->getEndPonitClkPath(), path->getStartPonitClkPath());
					this->assessTimingWithDcc(path, newcitime, cjtime.at(1), 40, 0, dcccandi.at(loop).front(), nullptr, path->getEndPonitClkPath(), path->getStartPonitClkPath());
					this->assessTimingWithDcc(path, newcitime, cjtime.at(2), 80, 0, dcccandi.at(loop).front(), nullptr, path->getEndPonitClkPath(), path->getStartPonitClkPath());
				}
                // Insert DCC on the branch part of left clk path:OK
				else if(candilocleft > candilocright)
				{
					double newcjtime = 0;
					// Calculate the Ci and Cj
					this->calculateClockLatencyWithoutDcc(path, &newcjtime, nullptr);
					citime = this->calculateClockLatencyWithDcc(path->getStartPonitClkPath(), dcccandi.at(loop).front());
					// Assess if the critical path occurs timing violation when inserting 20%/40%/80% DCC
					this->assessTimingWithDcc(path, citime.at(0), newcjtime, 20, 0, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath(), path->getEndPonitClkPath());
					this->assessTimingWithDcc(path, citime.at(1), newcjtime, 40, 0, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath(), path->getEndPonitClkPath());
					this->assessTimingWithDcc(path, citime.at(2), newcjtime, 80, 0, dcccandi.at(loop).front(), nullptr, path->getStartPonitClkPath(), path->getEndPonitClkPath());
				}
			}
            // Insert two DCCs:OK
			else if( dcccandi.at(loop).size() == 2 )
			{
				// Calculate the Ci and Cj
				citime = this->calculateClockLatencyWithDcc(path->getStartPonitClkPath(), dcccandi.at(loop).front());
				cjtime = this->calculateClockLatencyWithDcc(path->getEndPonitClkPath(), dcccandi.at(loop).back());
				// Assess if the critical path occurs timing violation when inserting 20%/40%/80% DCC and 20%/40%/80% DCC
				// Total 9 combinations
				this->assessTimingWithDcc(path, citime.at(0), cjtime.at(0), 20, 20, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(0), cjtime.at(1), 20, 40, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(0), cjtime.at(2), 20, 80, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(1), cjtime.at(0), 40, 20, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(1), cjtime.at(1), 40, 40, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(1), cjtime.at(2), 40, 80, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(2), cjtime.at(0), 80, 20, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(2), cjtime.at(1), 80, 40, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
				this->assessTimingWithDcc(path, citime.at(2), cjtime.at(2), 80, 80, dcccandi.at(loop).front(), dcccandi.at(loop).back(), path->getStartPonitClkPath(), path->getEndPonitClkPath());
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Assess if the critical path occurs timing violation when inserting
// DCCs (violating the setup time requirement)
// Input parameter:
// citime: clock latency of startpoint
// cjtime: clock latency of endpoint
// dcctype1: type of DCC 1
// dcctype2: type of DCC 2
// candinode1: pointer of DCC 1 (location)
// candinode2: pointer of DCC 2 (location)
// clkpath1: clock path 1 containing DCC 1
// clkpath2: clock path 2 containing DCC 2
// genclause: 0          => do not generate a clause
//            1 (default)=> generate a clause
// update: 0 (default) => do not update the timing information of critical path
//         1           => update the timing information of critical path
//
/////////////////////////////////////////////////////////////////////
double ClockTree::assessTimingWithDcc( CriticalPath *path, double citime, double cjtime, int dcctype1, int dcctype2,
									   ClockTreeNode *candinode1, ClockTreeNode *candinode2,
									   vector<ClockTreeNode *> clkpath1, vector<ClockTreeNode *> clkpath2,
									   bool genclause, bool update )
{
	double newslack = 0, dataarrtime = 0, datareqtime = 0;
	// Require time
	datareqtime = cjtime + (path->getTsu() * this->_agingtsu) + this->_tc;
	// Arrival time
	dataarrtime = citime + path->getTinDelay() + (path->getTcq() * this->_agingtcq) + (path->getDij() * this->_agingdij);
	newslack = datareqtime - dataarrtime;
	if( update )
	{
		path->setCi(citime), path->setCj(cjtime);
        path->setArrivalTime(dataarrtime)       ;
        path->setRequiredTime(datareqtime)      ;
        path->setSlack(newslack)                ;
	}
	//---- Timing violationprint -----------
	if( (newslack < 0) && genclause )
	{
		string clause;
		// Deal with two type of critical path (PItoFF and FFtoPO)
		if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoPO))
		{
			// Generate the clause
			for( auto const& nodeptr: clkpath1 )
			{
				if((nodeptr != candinode1) && nodeptr->ifPlacedDcc())
					this->writeClauseByDccType(nodeptr, &clause, 0);
				else if((nodeptr == candinode1) && nodeptr->ifPlacedDcc())
					this->writeClauseByDccType(nodeptr, &clause, dcctype1);
			}
		}
		// Deal with the FF to FF critical path
		else if( path->getPathType() == FFtoFF )
		{
			ClockTreeNode *sameparent = path->findLastSameParentNode();
			long sameparentloc = path->nodeLocationInClockPath('s', sameparent);
			// Generate the clause based on the clock path 1
			for( auto const& nodeptr: clkpath1 )
			{
				if((nodeptr != candinode1) && nodeptr->ifPlacedDcc())
					this->writeClauseByDccType(nodeptr, &clause, 0);
				else if((nodeptr == candinode1) && nodeptr->ifPlacedDcc())
					this->writeClauseByDccType(nodeptr, &clause, dcctype1);
			}
			// Generate the clause based on the branch clock path 2
			for(long loop = (sameparentloc + 1);loop < clkpath2.size();loop++)
			{
				if(candinode2 == nullptr)
					this->writeClauseByDccType(clkpath2.at(loop), &clause, 0);
				else
				{
					if((clkpath2.at(loop) != candinode2) && (clkpath2.at(loop)->ifPlacedDcc()))
						this->writeClauseByDccType(clkpath2.at(loop), &clause, 0);
					else if((clkpath2.at(loop) == candinode2) && (clkpath2.at(loop)->ifPlacedDcc()))
						this->writeClauseByDccType(clkpath2.at(loop), &clause, dcctype2);
				}
			}
		}
		clause += "0";
		if(this->_timingconstraintlist.size() < (this->_timingconstraintlist.max_size()-1))
			this->_timingconstraintlist.insert(clause);
		else
			cerr << "\033[32m[Info]: Timing Constraint List Full!\033[0m\n";
	}
	return newslack;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Calculate the clock latency of the startpoint/endpoint without
// any inserted DCCs
// Input parameter:
// datareqtime: clock latency of the endpoint
// dataarrtime: clock latency of the startpoint
//
/////////////////////////////////////////////////////////////////////
void ClockTree::calculateClockLatencyWithoutDcc(CriticalPath *path, double *datareqtime, double *dataarrtime)
{
	if(path == nullptr)
		return;
	bool dealstart = 0, dealend = 0;
	double agingrate = 1;
	switch(path->getPathType())
	{
		// Path type: input to FF
		case PItoFF:
			dealend = 1;
			break;
		// Path type: FF to output
		case FFtoPO:
			dealstart = 1;
			break;
		// Path type: FF to FF
		case FFtoFF:
			dealstart = 1;
			dealend = 1;
			break;
		default:
			break;
	}
	// Calculate the clock latency of the startpoint
	if(dealstart && (dataarrtime != nullptr))
	{
		double dutycycle = 0.5;
		vector<ClockTreeNode *> sclkpath = path->getStartPonitClkPath();
		if(this->_aging)
			agingrate = getAgingRateByDutyCycle(dutycycle);
		for(long loop = 0;loop < (sclkpath.size() - 1);loop++)
		{
			*dataarrtime += (sclkpath.at(loop)->getGateData()->getWireTime() + sclkpath.at(loop)->getGateData()->getGateTime()) * agingrate;
			// Meet the clock gating cells
			if(sclkpath.at(loop)->ifClockGating() && this->_aging)
			{
				dutycycle = dutycycle * (1 - sclkpath.at(loop)->getGatingProbability());
				agingrate = getAgingRateByDutyCycle(dutycycle);
			}
		}
		*dataarrtime += sclkpath.back()->getGateData()->getWireTime() * agingrate;
	}
	// Calculate the clock latency of the endpoint
	if(dealend && (datareqtime != nullptr))
	{
		double dutycycle = 0.5;
		vector<ClockTreeNode *> eclkpath = path->getEndPonitClkPath();
		if(this->_aging)
			agingrate = getAgingRateByDutyCycle(dutycycle);
		for(long loop = 0;loop < (eclkpath.size() - 1);loop++)
		{
			*datareqtime += (eclkpath.at(loop)->getGateData()->getWireTime() + eclkpath.at(loop)->getGateData()->getGateTime()) * agingrate;
			// Meet the clock gating cells
			if(eclkpath.at(loop)->ifClockGating() && this->_aging)
			{
				dutycycle = dutycycle * (1 - eclkpath.at(loop)->getGatingProbability());
				agingrate = getAgingRateByDutyCycle(dutycycle);
			}
		}
		*datareqtime += eclkpath.back()->getGateData()->getWireTime() * agingrate;
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Calculate the clock latency of the startpoint/endpoint based on
// specific dcc deployment
// Input parameter:
// clkpath: clock path of the critical path
// candinode: pointer of DCC (location)
//
/////////////////////////////////////////////////////////////////////
vector<double> ClockTree::calculateClockLatencyWithDcc( vector<ClockTreeNode *> clkpath, ClockTreeNode *candinode )
{
    vector<double> clklatency(3, 0)   ;//Calculate 3 possible latency of 3 DCC
    vector<double> oneclklatency(1, 0);
	bool   isfront        = 1   ;
    double minbufdelay    = 999 ;
    double dutycycle20    = 0.5, dutycycle40 = 0.5, dutycycle80 = 0.5, dutycycleondcc = 0.5;
	double agingrate20dcc = getAgingRateByDutyCycle(dutycycle20);
    double agingrate40dcc = agingrate20dcc ;
    double agingrate80dcc = agingrate20dcc ;
	for( long loop = 0; loop < (clkpath.size() - 1); loop++ )
	{
		ClockTreeNode *node = clkpath.at(loop);
        //wire_delay + gate_delay
		double buftime = node->getGateData()->getWireTime() + node->getGateData()->getGateTime();
		// Find the minimization delay of buffer in the clock path
		if( node != this->_clktreeroot )
			minbufdelay = min( buftime, minbufdelay );
        // The node before the DCC (dir: root->FF)
		if( isfront )
		{
			if((node != candinode) && (node->ifClockGating()))
				dutycycleondcc = dutycycleondcc * (1 - node->getGatingProbability());
			// Meet the DCC
			else if(node == candinode)
			{
				isfront = 0;
                dutycycle20 = 0.2 ; dutycycle40 = 0.4 ; dutycycle80 = 0.8 ;
				agingrate20dcc = getAgingRateByDutyCycle(dutycycle20);
				agingrate40dcc = getAgingRateByDutyCycle(dutycycle40);
				agingrate80dcc = getAgingRateByDutyCycle(dutycycle80);
				
			}
		}
		clklatency.at(0) += buftime * agingrate20dcc;
		clklatency.at(1) += buftime * agingrate40dcc;
		clklatency.at(2) += buftime * agingrate80dcc;
		// Meet the clock gating cells
		if(node->ifClockGating())
		{
			dutycycle20 = dutycycle20 * (1 - node->getGatingProbability());
			dutycycle40 = dutycycle40 * (1 - node->getGatingProbability());
			dutycycle80 = dutycycle80 * (1 - node->getGatingProbability());
			agingrate20dcc = getAgingRateByDutyCycle(dutycycle20);
			agingrate40dcc = getAgingRateByDutyCycle(dutycycle40);
			agingrate80dcc = getAgingRateByDutyCycle(dutycycle80);
		}
	}
	clklatency.at(0) += clkpath.back()->getGateData()->getWireTime() * agingrate20dcc;
	clklatency.at(1) += clkpath.back()->getGateData()->getWireTime() * agingrate40dcc;
	clklatency.at(2) += clkpath.back()->getGateData()->getWireTime() * agingrate80dcc;
	if( candinode != nullptr )
	{
		double dccagingrate = getAgingRateByDutyCycle(dutycycleondcc);
		clklatency.at(0) += minbufdelay * dccagingrate * DCCDELAY20PA;
		clklatency.at(1) += minbufdelay * dccagingrate * DCCDELAY40PA;
		clklatency.at(2) += minbufdelay * dccagingrate * DCCDELAY80PA;
		if(candinode->getDccType() == 20)
			oneclklatency.front() = clklatency.front();
		else if(candinode->getDccType() == 40)
			oneclklatency.front() = clklatency.at(1);
		else if(candinode->getDccType() == 80)
			oneclklatency.front() = clklatency.back();
	}
	if(oneclklatency.front() != 0)
		return oneclklatency;
	else
		return clklatency;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Dump all clauses (DCC constraints and timing constraints) to a
// file
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dumpClauseToCnfFile(void)
{
	if( !this->_placedcc )  return ;
	fstream cnffile ;
	string cnfinput = this->_outputdir + "cnfinput_" + to_string(this->_tc);
	if( !isDirectoryExist(this->_outputdir) )
		mkdir(this->_outputdir.c_str(), 0775);
	if( !isFileExist(cnfinput) )
	{
        printf( YELLOW"\t[CNF] " RESET"Encoded as %s...\033[0m\n", cnfinput.c_str());
		cnffile.open(cnfinput, ios::out | fstream::trunc);
		if( !cnffile.is_open() )
		{
			cerr << RED"\t[Error]: Cannot open " << cnfinput << "\033[0m\n";
			cnffile.close();
			return;
		}
		//--- DCC constraint ---------------------------------
        for( auto const& clause: this->_dccconstraintlist )     {    cnffile << clause << "\n" ; }
		//--- Timing constraint -------------------------------
        for( auto const& clause: this->_timingconstraintlist )  {    cnffile << clause << "\n" ; }
		
        cnffile.close();
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Call MiniSat
//
/////////////////////////////////////////////////////////////////////
void ClockTree::execMinisat(void)
{
    if(!this->_placedcc)
        return;
    // MiniSat input file
    string cnfinput = this->_outputdir + "cnfinput_" + to_string(this->_tc);
    // MiniSat output file
    string cnfoutput = this->_outputdir + "cnfoutput_" + to_string(this->_tc);
    if(!isDirectoryExist(this->_outputdir))
        mkdir(this->_outputdir.c_str(), 0775);
    if(isFileExist(cnfinput))
    {
        this->_minisatexecnum++;
        //string execmd = "minisat " + cnfinput + " " + cnfoutput;
        //system(execmd.c_str());
        // Fork a process
        pid_t childpid = fork();
        if(childpid == -1)
            cerr << RED"[Error]: Cannot fork child process!\033[0m\n";
        else if(childpid == 0)
        {
            // Child process
            string minisatfile = this->_outputdir + "minisat_output";
            int fp = open(minisatfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
            if(fp == -1)
                cerr << RED"[Error]: Cannot dump the executive output of minisat!\033[0m\n";
            else
            {
                dup2(fp, STDOUT_FILENO);
                dup2(fp, STDERR_FILENO);
                close(fp);
            }
            //this->~ClockTree();
            // Call MiniSat
            if(execlp("./minisat", "./minisat", cnfinput.c_str(), cnfoutput.c_str(), (char *)0) == -1)
            {
                cerr << RED"[Error]: Cannot execute minisat!\033[0m\n";
                this->_minisatexecnum--;
            }
            exit(0);
        }
        else
        {
            // Parent process
            int exitstatus;
            waitpid(childpid, &exitstatus, 0);
        }
    }
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Change the boundary of Binary search and search for the optimal Tc
//
/////////////////////////////////////////////////////////////////////
void ClockTree::tcBinarySearch(double slack)
{
    // Place DCCs
    if( this->_placedcc )
    {
        fstream cnffile;
        string line, cnfoutput = this->_outputdir + "cnfoutput_" + to_string(this->_tc);//Minisat results
        if( !isFileExist(cnfoutput) )
            return;
        cnffile.open(cnfoutput, ios::in);
        if( !cnffile.is_open() )
        {
            cerr << RED"\t[Error]: Cannot open " << cnfoutput << "\033[0m\n";
            cnffile.close();
            return;
        }
        getline(cnffile, line);
        // Change the lower boundary
        if((line.size() == 5) && (line.find("UNSAT") != string::npos))
        {
            this->_tclowbound = this->_tc;
            this->_tc = ceilNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
            printf( YELLOW"\t[Minisat] " RESET "Return: " RED"UNSAT \033[0m\n" ) ;
            printf( YELLOW"\t[Binary Search] " RESET"Next Tc range: %f - %f \033[0m\n", _tclowbound, _tcupbound ) ;
        }
        // Change the upper boundary
        else if((line.size() == 3) && (line.find("SAT") != string::npos))
        {
            this->_besttc = this->_tc;
            this->_tcupbound = this->_tc;
            this->_tc = floorNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
            printf( YELLOW"\t[Minisat] " RESET"Return: " GREEN"SAT \033[0m\n" ) ;
            printf( YELLOW"\t[Binary Search] " RESET"Next Tc range: %f - %f \033[0m\n", _tclowbound, _tcupbound ) ;
        }
        cnffile.close();
    }
    // Do not place DCCs
    else if(!this->_placedcc)
    {
        // Change the lower boundary
        if(slack <= 0)
        {
            this->_tclowbound = this->_tc;
            this->_tc = ceilNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
        }
        // Change the upper boundary
        else if(slack > 0)
        {
            this->_besttc = this->_tc;
            this->_tcupbound = this->_tc;
            this->_tc = floorNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
        }
    }
}


/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Update timing information of all critical path based on the
// optimal Tc
//
/////////////////////////////////////////////////////////////////////
void ClockTree::updateAllPathTiming(void)
{
    if( this->_besttc == 0 )
		return;
	double minslack = 9999;
	this->_tc = this->_besttc;
	// Place DCCs
	if( this->_placedcc )
	{
		for( auto const& node: this->_buflist )
			node.second->setIfPlaceDcc(0) ;
		fstream cnffile;
        string line = "" ;
        string lastsatfile = this->_outputdir + "cnfoutput_" + to_string(this->_tc);
		if( !isFileExist(lastsatfile) )
		{
			cerr << "\033[31m[Error]: File \"" << lastsatfile << "\" does not found!\033[0m\n";
			return;
		}
		cnffile.open(lastsatfile, ios::in);
		if(!cnffile.is_open())
		{
			cerr << "\033[31m[Error]: Cannot open " << lastsatfile << "\033[0m\n";
			cnffile.close();
			return;
		}
		// Get the DCC deployment from the MiniSat output file
		getline(cnffile, line);
		if( (line.size() == 3) && (line.find("SAT") != string::npos) )
		{
			getline( cnffile, line ) ;
			vector<string> strspl = stringSplit(line, " ");
			for( long loop = 0; ; loop += 2 )
			{
				if( stoi( strspl[loop] ) == 0 )
					break ;
				if( (stoi(strspl.at(loop)) > 0) || (stoi(strspl.at(loop + 1)) > 0) )
				{
					ClockTreeNode *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
					if( findnode != nullptr )
					{
						findnode->setIfPlaceDcc(1);
						findnode->setDccType(stoi(strspl.at(loop)), stoi(strspl.at(loop + 1)));
						this->_dcclist.insert(pair<string, ClockTreeNode *> (findnode->getGateData()->getGateName(), findnode));
					}
                    
				}
			}
		}
		else
		{
			cerr << "\033[31m[Error]: File \"" << lastsatfile << "\" is not SAT!\033[0m\n";
			return;
		}
		cnffile.close();
		for( auto const& path: this->_pathlist )
		{
			if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
				continue;
			// Update timing information
			double slack = this->updatePathTimingWithDcc(path, 1);
            
			if( slack < minslack )
			{
				this->_mostcriticalpath = path;
				minslack = min( slack, minslack );
			}
		}
	}
	// Do not place DCCs
	else
	{
		for(auto const& path: this->_pathlist)
		{
			if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
				continue;
			// Update timing information
			double slack = this->assessTimingWithoutDcc(path, 0, 1);
			if( slack < minslack )
			{
				this->_mostcriticalpath = path;
				minslack = min(slack, minslack);
			}
		}
	}
	// Count the DCCs inserting at final buffer
	for(auto const& node: this->_dcclist)
		if(node.second->ifPlacedDcc() && node.second->isFinalBuffer())
			this->_dccatlastbufnum++;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Minimize the DCC deployment based on the optimal Tc
//
/////////////////////////////////////////////////////////////////////
void ClockTree::minimizeDccPlacement(void)
{
	if(!this->_mindccplace || !this->_placedcc || (this->_besttc == 0))
		return;
	cout << "\033[32m[Info]: Minimizing DCC Placement...\033[0m\n";
	cout << "\033[32m    Before DCC Placement Minimization\033[0m\n";
	this->printDccList();
	map<string, ClockTreeNode *> dcclist = this->_dcclist;
	map<string, ClockTreeNode *>::iterator finddccptr;
	// Reserve the DCCs locate before the critical path dominating the optimal Tc
	for(auto const& path: this->_pathlist)
	{
		ClockTreeNode *sdccnode = nullptr, *edccnode = nullptr;
		// If DCC locate in the clock path of startpoint
		sdccnode = path->findDccInClockPath('s');
		// If DCC locate in the clock path of endpoint
		edccnode = path->findDccInClockPath('e');
		if(sdccnode != nullptr)
		{
			finddccptr = dcclist.find(sdccnode->getGateData()->getGateName());
			if(finddccptr != dcclist.end())
				dcclist.erase(finddccptr);
		}
		if(edccnode != nullptr)
		{
			finddccptr = dcclist.find(edccnode->getGateData()->getGateName());
			if(finddccptr != dcclist.end())
				dcclist.erase(finddccptr);
		}
		if( path == this->_mostcriticalpath)
			break;
	}
	for(auto const& node: dcclist)
		node.second->setIfPlaceDcc(0);
	this->_tc = this->_besttc;
	// Greedy minimization
	while(1)
	{
		if(dcclist.empty())
			break;
		bool endflag = 1, findstartpath = 1;
		// Reserve one of the rest of DCCs above if one of critical paths occurs timing violation
		for(auto const& path: this->_pathlist)
		{
			if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
				continue;
			// Assess if the critical path occurs timing violation in the DCC deployment
			double slack = this->updatePathTimingWithDcc(path, 0);
			if(slack < 0)
			{
				endflag = 0;
				for(auto const& node: dcclist)
					if(node.second->ifPlacedDcc())
						dcclist.erase(node.first);
				// Reserve the DCC locate in the clock path of endpoint
				for(auto const& node: path->getEndPonitClkPath())
				{
					finddccptr = dcclist.find(node->getGateData()->getGateName());
					if((finddccptr != dcclist.end()) && !finddccptr->second->ifPlacedDcc())
					{
						finddccptr->second->setIfPlaceDcc(1);
						findstartpath = 0;
					}
				}
				// Reserve the DCC locate in the clock path of startpoint
				if(findstartpath)
				{
					for(auto const& node: path->getStartPonitClkPath())
					{
						finddccptr = dcclist.find(node->getGateData()->getGateName());
						if((finddccptr != dcclist.end()) && !finddccptr->second->ifPlacedDcc())
							finddccptr->second->setIfPlaceDcc(1);
					}
				}
				break;
			}
		}
		if(endflag)
			break;
	}
	for(auto const& node: this->_dcclist)
	{
		if(!node.second->ifPlacedDcc())
		{
			node.second->setDccType(0);
			this->_dcclist.erase(node.first);
		}
	}
	this->_dccatlastbufnum = 0;
	// Count the DCCs inserting at final buffer
	for(auto const& node: this->_dcclist)
		if(node.second->ifPlacedDcc() && node.second->isFinalBuffer())
			this->_dccatlastbufnum++;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Recheck the optimal Tc if it is minimum
//
/////////////////////////////////////////////////////////////////////
void ClockTree::tcRecheck(void)
{
	if(!this->_tcrecheck || (this->_besttc == 0))
		return;
	cout << "\033[32m[Info]: Rechecking Tc...\033[0m\n";
	cout << "\033[32m    Before Tc Rechecked\033[0m\n";
	cout << "\t*** Optimal tc                     : \033[36m" << this->_besttc << "\033[0m\n";
	if(this->_mostcriticalpath->getSlack() < 0)
		this->_besttc += ceilNPrecision(abs(this->_mostcriticalpath->getSlack()), PRECISION);
	double oribesttc = this->_besttc;
	while(1)
	{
		bool endflag = 0;
		// Decrease the optimal Tc
		this->_tc = this->_besttc - (1 / pow(10, PRECISION));
		if(this->_tc < 0)
			break;
		// Assess if the critical path occurs timing violation
		for(auto const& path: this->_pathlist)
		{
			if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
				continue;
			double slack = 0;
			if(this->_placedcc)
				slack = this->updatePathTimingWithDcc(path, 0);
			else
				slack = this->assessTimingWithoutDcc(path, 0, 0);
			if(slack < 0)
			{
				endflag = 1;
				break;
			}
		}
		if(endflag)
			break;
		this->_besttc = this->_tc;
	}
	if(oribesttc == this->_besttc)
		return;
	this->_tc = this->_besttc;
	// Update timing information of all critical path based on the new optimal Tc
	for(auto const& path: this->_pathlist)
	{
		if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
			continue;
		if(this->_placedcc)
			this->updatePathTimingWithDcc(path, 1);
		else
			this->assessTimingWithoutDcc(path, 0, 1);
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Insert buffers based on the optimal Tc determined by DCC insertion
//
/////////////////////////////////////////////////////////////////////
void ClockTree::bufferInsertion(void)
{
	if((this->_bufinsert < 1) || (this->_bufinsert > 2))
		return;
	cout << "\033[32m[Info]: Inserting Buffer (Tc = " << this->_besttc << " )...\033[0m\n";
	this->_tc = this->_besttc;
	for(int counter = 1;;counter++)
	{
		bool insertflag = 0;
		for(auto const& path : this->_pathlist)
		{
			if((path->getPathType() == NONE) || (path->getPathType() == PItoPO))
				continue;
			double dataarrtime = 0, datareqtime = 0;
			// Calculate the Ci and Cj
			this->calculateClockLatencyWithoutDcc(path, &datareqtime, &dataarrtime);
			// Require time
			datareqtime += (path->getTsu() * this->_agingtsu) + this->_tc;
			// Arrival time
			dataarrtime += path->getTinDelay() + (path->getTcq() * this->_agingtcq) + (path->getDij() * this->_agingdij);
			if((path->getPathType() == FFtoPO) || (path->getPathType() == FFtoFF))
				if(!path->getStartPonitClkPath().empty() && path->getStartPonitClkPath().back()->ifInsertBuffer())
					dataarrtime += path->getStartPonitClkPath().back()->getInsertBufferDelay();
			if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoFF))
				if(!path->getEndPonitClkPath().empty() && path->getEndPonitClkPath().back()->ifInsertBuffer())
					datareqtime += path->getEndPonitClkPath().back()->getInsertBufferDelay();
			// Timing violation
			if((datareqtime - dataarrtime) < 0)
			{
				if(path->getPathType() == FFtoPO)
				{
					this->_insertbufnum = 0;
					return;
				}
				else
				{
					// Insert buffer
					insertflag = 1;
					if(!path->getEndPonitClkPath().back()->ifInsertBuffer())
						this->_insertbufnum++;
					path->getEndPonitClkPath().back()->setIfInsertBuffer(1);
					double delay = path->getEndPonitClkPath().back()->getInsertBufferDelay();
					delay = max(delay, (dataarrtime - datareqtime));
					path->getEndPonitClkPath().back()->setInsertBufferDelay(delay);
				}
			}
		}
		if(!insertflag)
			break;
		// Check 3 times
		if((counter == 3) && insertflag)
		{
			this->_insertbufnum = 0;
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Minimize the inserted buffers
//
/////////////////////////////////////////////////////////////////////
void ClockTree::minimizeBufferInsertion(void)
{
	if((this->_bufinsert != 2) || (this->_insertbufnum < 2))
		return;
	cout << "\033[32m[Info]: Minimizing Buffer Insertion...\033[0m\n";
	cout << "\033[32m    Before Buffer Insertion Minimization\033[0m\n";
	this->printBufferInsertedList();
	queue<ClockTreeNode *> nodequeue;
	nodequeue.push(this->_firstchildrennode);
	// BFS
	while(!nodequeue.empty())
	{
		if(!nodequeue.front()->getChildren().empty())
			for(auto const &nodeptr : nodequeue.front()->getChildren())
				nodequeue.push(nodeptr);
		if(nodequeue.front()->isFFSink())
		{
			nodequeue.pop();
			continue;
		}
		vector<ClockTreeNode *> ffchildren = this->getFFChildren(nodequeue.front());
		set<double, greater<double>> bufdelaylist;
		bool continueflag = 0, insertflag = 1;
		double adddelay = 0;
		// Do not reduce the buffers
		for(auto const& nodeptr : ffchildren)
		{
			if(nodeptr->ifInsertBuffer())
				bufdelaylist.insert(nodeptr->getInsertBufferDelay());
			else
			{
				continueflag = 1;
				break;
			}
		}
		if(continueflag || bufdelaylist.empty())
		{
			nodequeue.pop();
			continue;
		}
		// Assess if the buffers can be reduced
		for(auto const& bufdelay : bufdelaylist)
		{
			for(auto const& nodeptr : ffchildren)
			{
				// Get the critical path by startpoint
				vector<CriticalPath *> findpath = this->searchCriticalPath('s', nodeptr->getGateData()->getGateName());
				for(auto const& path : findpath)
				{
					double dataarrtime = 0, datareqtime = 0;
					// Calculate the Ci and Cj
					this->calculateClockLatencyWithoutDcc(path, &datareqtime, &dataarrtime);
					// Require time
					datareqtime += (path->getTsu() * this->_agingtsu) + this->_tc;
					// Arrival time
					dataarrtime += path->getTinDelay() + (path->getTcq() * this->_agingtcq) + (path->getDij() * this->_agingdij);
					for(auto const& clknodeptr : path->getEndPonitClkPath())
					{
						if((clknodeptr == nodequeue.front()) && (bufdelay >= path->getEndPonitClkPath().back()->getInsertBufferDelay()))
							datareqtime += (bufdelay - path->getEndPonitClkPath().back()->getInsertBufferDelay());
						if(clknodeptr->ifInsertBuffer())
							datareqtime += clknodeptr->getInsertBufferDelay();
					}
					for(auto const& clknodeptr : path->getStartPonitClkPath())
						if(clknodeptr->ifInsertBuffer())
							dataarrtime += clknodeptr->getInsertBufferDelay();
					if(bufdelay >= nodeptr->getInsertBufferDelay())
						dataarrtime += (bufdelay - nodeptr->getInsertBufferDelay());
					// Timing violation
					if((datareqtime - dataarrtime) < 0)
					{
						insertflag = 0;
						break;
					}
				}
				findpath.clear();
				// Get the critical path by endpoint
				findpath = this->searchCriticalPath('e', nodeptr->getGateData()->getGateName());
				for(auto const& path : findpath)
				{
					double dataarrtime = 0, datareqtime = 0;
					// Calculate the Ci and Cj
					this->calculateClockLatencyWithoutDcc(path, &datareqtime, &dataarrtime);
					// Require time
					datareqtime += (path->getTsu() * this->_agingtsu) + this->_tc;
					// Arrival time
					dataarrtime += path->getTinDelay() + (path->getTcq() * this->_agingtcq) + (path->getDij() * this->_agingdij);
					for(auto const& clknodeptr : path->getStartPonitClkPath())
					{
						if((clknodeptr == nodequeue.front()) && (bufdelay >= path->getStartPonitClkPath().back()->getInsertBufferDelay()))
							datareqtime += (bufdelay - path->getStartPonitClkPath().back()->getInsertBufferDelay());
						if(clknodeptr->ifInsertBuffer())
							datareqtime += clknodeptr->getInsertBufferDelay();
					}
					for(auto const& clknodeptr : path->getEndPonitClkPath())
						if(clknodeptr->ifInsertBuffer())
							dataarrtime += clknodeptr->getInsertBufferDelay();
					if(bufdelay >= nodeptr->getInsertBufferDelay())
						dataarrtime += (bufdelay - nodeptr->getInsertBufferDelay());
					// Timing violation
					if((datareqtime - dataarrtime) < 0)
					{
						insertflag = 0;
						break;
					}
				}
				if(!insertflag)
					break;
			}
			if(insertflag)
			{
				adddelay = bufdelay;
				break;
			}
		}
		// Reduce the buffers
		if(insertflag && (adddelay != 0))
		{
			nodequeue.front()->setIfInsertBuffer(1);
			nodequeue.front()->setInsertBufferDelay(adddelay);
			this->_insertbufnum++;
			for(auto const& nodeptr : ffchildren)
			{
				if(nodeptr->ifInsertBuffer())
				{
					double delay = nodeptr->getInsertBufferDelay() - adddelay;
					nodeptr->setInsertBufferDelay(delay);
					if(delay <= 0)
					{
						this->_insertbufnum--;
						nodeptr->setIfInsertBuffer(0);
						nodeptr->setInsertBufferDelay(0);
					}
				}
			}
		}
		nodequeue.pop();
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Search the node in clock tree with the specific gate name
// Input parameter:
// gatename: specific buffer/FF name
//
/////////////////////////////////////////////////////////////////////
ClockTreeNode *ClockTree::searchClockTreeNode(string gatename)
{
	if(gatename.compare(this->_clktreeroot->getGateData()->getGateName()) == 0)
		return this->_clktreeroot;
	// Search in buffer list
	map<string, ClockTreeNode *>::iterator findnode = this->_buflist.find(gatename);
	if(findnode != this->_buflist.end())
		return findnode->second;
	// Search in FF list
	findnode = this->_ffsink.find(gatename);
	if( findnode != this->_buflist.end() )
		return findnode->second;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Search the node in clock tree with the specific node number
// Input parameter:
// nodenum: specific clock tree node number
//
/////////////////////////////////////////////////////////////////////
ClockTreeNode *ClockTree::searchClockTreeNode(long nodenum)
{
	if(nodenum < 0)
		return nullptr;
	ClockTreeNode *findnode = nullptr;
	// Search in buffer list
	for(auto const& node: this->_buflist)
	{
		if(node.second->getNodeNumber() == nodenum)
		{
			findnode = node.second;
			break;
		}
	}
	if(findnode != nullptr)
		return findnode;
	// Search in FF list
	for(auto const& node: this->_ffsink)
	{
		if(node.second->getNodeNumber() == nodenum)
		{
			findnode = node.second;
			break;
		}
	}
	return findnode;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Search the critical path with the specific startpoint/endpoint
// Input parameter:
// selection: 's' => startpoint
//            'e' => endpoint
// pointname: specific name of startpoint/endpoint
//
/////////////////////////////////////////////////////////////////////
vector<CriticalPath *> ClockTree::searchCriticalPath(char selection, string pointname)
{
	vector<CriticalPath *> findpathlist;
	if((selection != 's') || (selection != 'e'))
		return findpathlist;
	for(auto const &pathptr : this->_pathlist)
	{
		if((pathptr->getPathType() == NONE) || (pathptr->getPathType() == PItoPO))
			continue;
		string name = "";
		switch(selection)
		{
			// startpoint
			case 's':
				name = pathptr->getStartPointName();
				break;
			// endpoint
			case 'e':
				name = pathptr->getEndPointName();
				break;
			default:
				break;
		}
		if(pointname.compare(name) == 0)
		{
			findpathlist.resize(findpathlist.size() + 1);
			findpathlist.back() = pathptr;
		}
	}
	return findpathlist;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Report all FFs under the specific clock tree node
//
/////////////////////////////////////////////////////////////////////
vector<ClockTreeNode *> ClockTree::getFFChildren(ClockTreeNode *node)
{
	vector<ClockTreeNode *> ffchildren;
	if((node == nullptr) || node->isFFSink())
		return ffchildren;
	queue<ClockTreeNode *> nodequeue;
	nodequeue.push(node);
	// BFS
	while(!nodequeue.empty())
	{
		if(!nodequeue.front()->getChildren().empty())
			for(auto const &nodeptr : nodequeue.front()->getChildren())
				nodequeue.push(nodeptr);
		if(nodequeue.front()->isFFSink() && nodequeue.front()->ifUsed())
		{
			ffchildren.resize(ffchildren.size() + 1);
			ffchildren.back() = nodequeue.front();
		}
		nodequeue.pop();
	}
	return ffchildren;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Dump to files
// 1. DCC
// 2. Clock gating cell
// 3. Insserted buffer
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dumpToFile(void)
{
	this->dumpDccListToFile();
	this->dumpClockGatingListToFile();
	this->dumpBufferInsertionToFile();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print whole clock tree presented in depth
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printClockTree(void)
{
	cout << "\033[34m----- [   Clock Tree   ] -----\033[0m\n";
	if(this->_clktreeroot == nullptr)
	{
		cout << "\t[Info]: Empty clock tree.\n";
		cout << "\033[34m----- [ Clock Tree End ] -----\033[0m\n";
		return;
	}
	long bufnum = 0, ffnum = 0;
	queue<ClockTreeNode *> nodequeue;
	nodequeue.push(this->_clktreeroot);
	cout << "Depth: Node_name(Node_num, if_used, is_cg" << ((this->_placedcc) ? ", place_dcc)\n" : ")\n");
	cout << "Depth: (buffer_num, ff_num)\n";
	cout << this->_clktreeroot->getDepth() << ": ";
	// BFS
	while(!nodequeue.empty())
	{
		long nowdepth = nodequeue.front()->getDepth();
		cout << nodequeue.front()->getGateData()->getGateName() << "(" << nodequeue.front()->getNodeNumber() << ", ";
		cout << nodequeue.front()->ifUsed() << ", " << nodequeue.front()->ifClockGating();
		(this->_placedcc) ? cout << ", " << nodequeue.front()->ifPlacedDcc() << ")" : cout << ")";
		if((nodequeue.front() != this->_clktreeroot) && nodequeue.front()->ifUsed())
			(nodequeue.front()->isFFSink()) ? ffnum++ : bufnum++;
		if(!nodequeue.front()->getChildren().empty())
			for(auto const &nodeptr : nodequeue.front()->getChildren())
				nodequeue.push(nodeptr);
		nodequeue.pop();
		if(!nodequeue.empty())
		{
			if(nowdepth == nodequeue.front()->getDepth())
				cout << ", ";
			else
			{
				cout << "\n" << nowdepth << ": (" << bufnum << ", " << ffnum << ")\n";
				bufnum = 0, ffnum = 0;
				cout << nodequeue.front()->getDepth() << ": ";
			}
		}
	}
	cout << "\n" << this->_maxlevel << ": (" << bufnum << ", " << ffnum << ")\n";
	cout << "\033[34m----- [ Clock Tree End ] -----\033[0m\n";
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print one of critical path with given specific critical path
// number
// Input parameter:
// pathnum: specific critical path number
// verbose: 0 => briefness
//          1 => detail
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printSingleCriticalPath(long pathnum, bool verbose)
{
	cout << "\033[34m----- [   Single Critical Path   ] -----\033[0m\n";
	if((pathnum < 0) || (pathnum > this->_pathlist.size()))
		cout << "\033[34m[Info]: Path NO." << pathnum << " doesn't exist.\033[0m\n";
	else
		this->_pathlist.at(pathnum)->printCriticalPath(verbose);
	cout << "\033[34m----- [ Single Critical Path end ] -----\033[0m\n";
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print one of critical path with given a specific
// startpoint/endpoint
// Input parameter:
// selection: 's' => startpoint
//            'e' => endpoint
// pointname: specific name of startpoint/endpoint
// verbose: 0 => briefness
//          1 => detail
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printSingleCriticalPath(char selection, string pointname, bool verbose)
{
	cout << "\033[34m----- [   Single Critical Path   ] -----\033[0m\n";
	vector<CriticalPath *> findpathlist = this->searchCriticalPath(selection, pointname);
	if(findpathlist.empty())
		cout << "\033[34m[Info]: Ponit name " << pointname << " of path doesn't exist.\033[0m\n";
	else
		for(auto const& path : findpathlist)
			path->printCriticalPath(verbose);
	cout << "\033[34m----- [ Single Critical Path end ] -----\033[0m\n";
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print all critical path information
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printAllCriticalPath(void)
{
	cout << "\033[34m----- [ All Critical Paths List ] -----\033[0m\n";
	if(this->_pathlist.empty())
	{
		cout << "\t\033[34m[Info]: Non critical path.\033[0m\n";
		cout << "\033[34m----- [ Critical Paths List End ] -----\033[0m\n";
		return;
	}
	for(auto const& pathptr : this->_pathlist)
		pathptr->printCriticalPath(0);
	cout << "\033[34m----- [ Critical Path List End ] -----\033[0m\n";
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print all DCC information (location and type)
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printDccList(void)
{
	if( !this->_placedcc)
		return;
	cout << "\t*** # of inserted DCCs             : " << this->_dcclist.size() << "\n";
	cout << "\t*** # of DCCs placed at last buffer: " << this->_dccatlastbufnum << "\n";
	cout << "\t*** Inserted DCCs List             : " ;
	if( !this->_placedcc || this->_dcclist.empty() )
		cout << "N/A\n";
    else{
		for( auto const& node: this->_dcclist )
			cout << node.first << "(" << node.second->getNodeNumber() << ","<< node.second->getDccType() << ((node.second != this->_dcclist.rbegin()->second) ? "), " : ")\n");
        
        cout << endl ;
        for( auto const& node: this->_dcclist )
            printf("%ld -1 %f\n", (node.second->getNodeNumber()-1)/2*3+1, (double)node.second->getDccType()/100 );
    }
	
    cout << "\t*** DCCs Placed at Last Buffer     : ";
	if(!this->_placedcc || (this->_dccatlastbufnum == 0))
		cout << "N/A\n";
	else
	{
		bool firstprint = true;
		for(auto const& node: this->_dcclist)
		{
			if(node.second->isFinalBuffer())
			{
                if( firstprint ) firstprint = false ;
                else    cout << "), " ;
				cout << node.first << "(" << node.second->getNodeNumber() << ","<< node.second->getDccType();
			}
		}
		cout << ")\n";
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print all clock gating cell information (location and gating 
// probability)
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printClockGatingList(void)
{
	cout << "\t*** Clock Gating Cells List        : ";
	if(!this->_clkgating || this->_cglist.empty())
		cout << "N/A\n";
	else
		for(auto const& node: this->_cglist)
			cout << node.first << "(" << node.second->getGatingProbability() << ((node.second != this->_cglist.rbegin()->second) ? "), " : ")\n");
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Print all inserted buffer information (location and buffer delay)
//
/////////////////////////////////////////////////////////////////////
void ClockTree::printBufferInsertedList(void)
{
	if((this->_bufinsert < 1) || (this->_bufinsert > 2))
		return;
	cout << "\t*** # of inserted buffers          : " << this->_insertbufnum << "\n";
	cout << "\t*** Inserted Buffers List          : ";
	if((this->_bufinsert < 1) || (this->_bufinsert > 2) || (this->_insertbufnum == 0))
		cout << "N/A\n";
	else
	{
		long counter = 0;
		// Buffer list
		for(auto const& node: this->_buflist)
		{
			if(node.second->ifInsertBuffer())
			{
				counter++;
				cout << node.first << "(" << node.second->getInsertBufferDelay();
				cout << ((counter != this->_insertbufnum) ? "), " : ")\n");
			}
		}
		// FF list
		for(auto const& node: this->_ffsink)
		{
			if(node.second->ifInsertBuffer())
			{
				counter++;
				cout << node.first << "(" << node.second->getInsertBufferDelay();
				cout << ((counter != this->_insertbufnum) ? "), " : ")\n");
			}
		}
	}
}
