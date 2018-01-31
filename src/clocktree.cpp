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
#include <sstream>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define GRN     "\x1b[32m"
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
	this->_tcupbound  = ceilNPrecision( this->_tc * ((this->_aging) ? getAgingRate_givDC_givVth( 0.5, -1) : 1.4), 1 );
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
                
                if( _printClause )
                {
                    auto find = this->getDCCSet().find( pair<int,int>(comblist->at(loop1).at(0),comblist->at(loop1).at(1)) );
                    if( find == this->getDCCSet().end() ){
                        fprintf( fptr,"DCC (%ld with %ld): %s, %s, %s, %s \n", nodenum1, nodenum2, clause1.c_str(), clause2.c_str(), clause3.c_str(), clause4.c_str() );
                        this->getDCCSet().insert(pair<int,int>(comblist->at(loop1).at(0),comblist->at(loop1).at(1)) );
                    }
                }
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
void ClockTree::genClauseByDccVTA( ClockTreeNode *node, string *clause, double dcctype, int LibIndex )
{
	if((node == nullptr) || (clause == nullptr) )
		return ;
    
	long nodenum = node->getNodeNumber();
    
    if( !node->ifPlacedDcc() || node->ifMasked() )
    {
        *clause += to_string(nodenum) + " " + to_string(nodenum + 1) + " ";
        //-- Put Header ------
        if( ifdoVTA() )
        {
            if( LibIndex == 0 )
                *clause += to_string( ( nodenum + 2 )*(-1) ) + " " ;
            //-- No Put Header ----
            else if( LibIndex == -1 )
                *clause += to_string( nodenum + 2  ) + " " ;

            return ;
        }
    }else
    {
        if( dcctype == 0.5 || dcctype == -1 || dcctype == 0 )
            *clause += to_string(nodenum) + " " + to_string(nodenum + 1) + " ";
        else if( dcctype == 0.2 )
            *clause += to_string(nodenum) + " " + to_string((nodenum + 1) * -1) + " ";
        else if( dcctype == 0.4 )
            *clause += to_string(nodenum * -1) + " " + to_string(nodenum + 1) + " ";
        else if( dcctype == 0.8 )
            *clause += to_string(nodenum * -1) + " " + to_string((nodenum + 1) * -1) + " ";
        else//Don't care
            return ;
        if( ifdoVTA() )
        {
            if( LibIndex == 0 )
                *clause += to_string( ( nodenum + 2 )*(-1) ) + " " ;
            //-- No Put Header ----
            else if( LibIndex == -1 )
                *clause += to_string( nodenum + 2  ) + " " ;
            
            return ;
        }
    }
	
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
/*
void ClockTree::dumpDccVTALeaderToFile(void)
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
*/
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
/*---------------------------------------------------------------------------
 FuncName:
    readParameter(), calConvergentVth(), calSv()
 Where defined:
    ClockTree Class - Public Method
 Introduction:
    read parameters from text file "parameter.txt"
 Creator:
    Tien-Hung Tseng
 ----------------------------------------------------------------------------*/
double ClockTree::calConvergentVth( double dc, double exp )
{
    return 0.0039/2*( pow( dc*(this->getFinYear())*(31536000), exp )) ;
}
double ClockTree::calSv( double dc, double VthOffset, double VthFin  )
{
    if( VthOffset == 0 ) return 0 ;
    //The func refers to "hi_n_low_buffer.py"
    double Right   = VthFin - VthOffset                     ;
    double Time    = dc*( this->getFinYear() )*( 31536000 ) ;
    double C       = 0.0039/2*( pow( Time, this->getExp() ) )      ;
    return -( Right/(C) - 1 )/(VthOffset)                ;
}
void ClockTree::readParameter()
{
    fstream file    ;
    string  line    ;
    file.open("./setting/Parameter.txt");
    while( getline(file, line) )
    {
        if( line.find("#")                      != string::npos ) continue ;//Comment->Ignore
        if( line.find("FIN_CONVERGENT_YEAR")    != string::npos ) this->setFinYear( atoi(line.c_str() + 19 ))    ;
        if( line.find("BASE_VTH")               != string::npos ) this->setBaseVthOffset( atof(line.c_str() + 8 ))    ;
        if( line.find("EXP")               != string::npos )
        {
            this->setExp( atof(line.c_str() + 3 ))    ;
        }
        if( line.find("LIB_VTH_COUNT")          != string::npos )
        {
            this->setLibCount( atoi(line.c_str()+ 13 ))    ;
            if( this->getLibCount() == 0 ){
                this->setIfVTA( false ) ;
                continue ;
            }
            else
            {
                for( int i = 0 ; i < this->getLibCount(); i++ )
                {
                    getline( file, line ) ;

                    struct VTH_TECH * ptrTech = new VTH_TECH() ;
                    ptrTech->_VTH_OFFSET     = atof(line.c_str() + 19 ) ;
                    ptrTech->_VTH_CONVGNT[0] = this->calConvergentVth( 0.2, this->getExp() ) ;//20% DCC
                    ptrTech->_VTH_CONVGNT[1] = this->calConvergentVth( 0.4, this->getExp() ) ;//40% DCC
                    ptrTech->_VTH_CONVGNT[2] = this->calConvergentVth( 0.5, this->getExp() ) ;//No  DCC
                    ptrTech->_VTH_CONVGNT[3] = this->calConvergentVth( 0.8, this->getExp() ) ;//80% DCC
            
                    ptrTech->_Sv[0]          = this->calSv( 0.2, this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[0] ) ;//20% DCC
                    ptrTech->_Sv[1]          = this->calSv( 0.4, this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[1] ) ;//40% DCC
                    ptrTech->_Sv[2]          = this->calSv( 0.5, this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[2] ) ;//No  DCC
                    ptrTech->_Sv[3]          = this->calSv( 0.8, this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[3] ) ;//80% DCC
                    ptrTech->_Sv[4]          = this->calSv( 0.2, ptrTech->_VTH_OFFSET + this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[0] ) ;//20% DCC
                    ptrTech->_Sv[5]          = this->calSv( 0.4, ptrTech->_VTH_OFFSET + this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[1] ) ;//40% DCC
                    ptrTech->_Sv[6]          = this->calSv( 0.5, ptrTech->_VTH_OFFSET + this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[2] ) ;//No  DCC
                    ptrTech->_Sv[7]          = this->calSv( 0.8, ptrTech->_VTH_OFFSET + this->getBaseVthOffset(), ptrTech->_VTH_CONVGNT[3] ) ;//80% DCC
                    this->getLibList().push_back( ptrTech ) ;
                    
                    //double bof = this->getBaseVthOffset() ;
                    double tof = ptrTech->_VTH_OFFSET     ;
                    printf( GREEN"[Info] Reading ./setting/Parameter.txt..\n" );
                    printf( CYAN"\t[Setting] " RESET"Fin convergent Year    = %4d years \n", this->getFinYear() );
                    printf( CYAN"\t[Setting] " RESET"Vth offset (VTA)       = " RED"%.2f (V)\n"  , ptrTech->_VTH_OFFSET );
                    printf( CYAN"\t[Setting] " RESET"Vth offset (Baseline)  = %.2f (V)\n" RESET  , this->getBaseVthOffset() );
                    printf( CYAN"\t[Setting] " RESET"Exponential term       = %.2f \n"     , this->getExp() );
                    printf( CYAN"\t---------------------------------------------------------------------------------\n" );
                    printf( CYAN"\t[Note] " RESET"Following is the timing information of clock buffers\n" );
                    printf( CYAN"\t[Note] " RESET"Delay gain includes aging rate\n" );
                    printf( CYAN"\t[Note] " RESET"Delay gain = aging rate + " RED"intrinsic delay" RESET" due to adjusted Vth\n" );
                    printf( CYAN"\t[Note] " RESET"N-Vth denotes clock buffer with 'nominal  Vth'\n" );
                    printf( CYAN"\t[Note] " RESET"S-Vth denotes clock buffer with 'specific Vth'\n" );
                    printf( CYAN"\t------------------------- Nominal Clk buffer -----------------------------------\n" );
                    
                    printf( CYAN"\t20 %%, N-Vth: Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.2, -1 )        - 1 )*100 );
                    printf( CYAN"\t40 %%, N-Vth: Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.4, -1 )        - 1 )*100 );
                    printf( CYAN"\t50 %%, N-Vth: Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.5, -1 )        - 1 )*100 );
                    printf( CYAN"\t80 %%, N-Vth: Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.8, -1 )        - 1 )*100 );
                    printf( CYAN"\t------------------------- Specific Clk buffer -----------------------------------\n" );
                    printf( CYAN"\t20 %%, S-Vth: Delay gain " RESET"= %4.1f %%\n", (getAgingRate_givDC_givVth( 0.2,  0 )               - 1 )*100 );
                    printf( CYAN"\t             Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.2,  0 ) - 2*(tof) - 1 )*100 );
                    
                    printf( CYAN"\t40 %%, S-Vth: Delay gain " RESET"= %4.1f %%\n", (getAgingRate_givDC_givVth( 0.4,  0 )               - 1 )*100 );
                    printf( CYAN"\t             Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.4,  0 ) - 2*(tof) - 1 )*100 );
                    
                    printf( CYAN"\t50 %%, S-Vth: Delay gain " RESET"= %4.1f %%\n", (getAgingRate_givDC_givVth( 0.5,  0 )               - 1 )*100 );
                    printf( CYAN"\t             Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.5,  0 ) - 2*(tof) - 1 )*100 );
                    
                    printf( CYAN"\t80 %%, S-Vth: Delay gain " RESET"= %4.1f %%\n", (getAgingRate_givDC_givVth( 0.8,  0 )               - 1 )*100 );
                    printf( CYAN"\t             Aging rate " RESET"= %4.1f %%\n",  (getAgingRate_givDC_givVth( 0.8,  0 ) - 2*(tof) - 1 )*100 );
                    
                }
            }
        }
    }
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
        else if(strcmp(argv[loop], "-nonVTA") == 0)
            this->_doVTA = 0;
		else if(strcmp(argv[loop], "-nonaging") == 0)
			this->_aging = 0;									// Non-aging
		else if(strcmp(argv[loop], "-mindcc") == 0)
			this->_mindccplace = 1;								// Minimize DCC deployment
		else if(strcmp(argv[loop], "-tc_recheck") == 0)
			this->_tcrecheck = 1;								// Recheck Tc
        else if(strcmp(argv[loop], "-print=path") == 0)
            this->_printpath = 1;
        else if(strcmp(argv[loop], "-dump=UNSAT_CNF") == 0)
            this->_dumpCNF   = 1;
        else if(strcmp(argv[loop], "-checkCNF") == 0)
            this->_checkCNF  = 1;
        else if(strcmp(argv[loop], "-checkFile") == 0)
            this->_checkfile  = 1;
        else if(strcmp(argv[loop], "-print=Clause") == 0)
            this->_printClause  = 1;
        else if(strcmp(argv[loop], "-checkCNF") == 0)
            this->_checkCNF  = 1;
        else if(strcmp(argv[loop], "-print=Node") == 0)
            this->_printClkNode  = 1;
        else if(strcmp(argv[loop], "-aging=Senior") == 0)
            this->_usingSeniorAging = 1;
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
        this->_agingtcq = 1;
        this->_agingdij = 1;
        this->_agingtsu = 1;
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
						this->_totalnodenum += 3;//this->_totalnodenum += 2;//senior
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
                    pathstart = false ; firclkedge = false; parentnode = nullptr;
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
                                this->_totalnodenum += 3;//this->_totalnodenum += 2;//senior
                                findnode = node; parentnode = nullptr;
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
								this->_totalnodenum += 3;//this->_totalnodenum += 2;//senior
                                findnode = node; parentnode = nullptr;
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
								this->_totalnodenum += 3;//this->_totalnodenum += 2;//senior
								if(scratchnextline)
								{
									getline(tim_max, line);
									strspl = stringSplit(line, " ");
									node->getGateData()->setGateTime(stod(strspl.at(3)));
								}
                                findnode = node; parentnode = node;
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
	this->_totalnodenum /= 3;//this->_totalnodenum /= 2;//senior
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
            nodeitptr = buflist.begin(); advance(nodeitptr, rannum);
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
	else if(minslack > (this->_origintc * (roundNPrecision( getAgingRate_givDC_givVth( 0.5, -1 ), PRECISION) - 1)))
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
    this->_tcAfterAdjust = this->_tc ;
	// Initial the boundary of Tc using in Binary search
	this->initTcBound();
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Prohibit DCCs inserting below the mask
//
/////////////////////////////////////////////////////////////////////
void ClockTree::dccPlacementByMasked( )
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
                if( nodeptr->getDepth() <= (this->_maxlevel - this->_masklevel) ){
					//nodeptr->setIfPlaceDcc(1);
                    nodeptr->setifMasked(false);
                    /*
                    if( nodeptr->getGateData()->getGateName() == "CK__L4_I227" )
                    {
                        printf( "Path: %ld, Start side unmasked it\n", pathptr->getPathNum() );
                    }
                     */ //Log code
                }
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
                if(nodeptr->getDepth() <= (this->_maxlevel - this->_masklevel)){
					//nodeptr->setIfPlaceDcc(1)   ;
                    nodeptr->setifMasked(false) ;
                    /*
                    if( nodeptr->getGateData()->getGateName() == "CK__L4_I227" )
                    {
                        printf( "Path: %ld, End side unmasked it\n", pathptr->getPathNum() );
                    }
                     *///log code
                }
		}
	}
}


/*---------------------------------------------------------------------------
 FuncName:
    VTAConstraint(),       VTAConstraintFFtoFF(),
    VTAConstraintPItoFF(), VTAConstraintFFtoPO(),
 Where defined:
    ClockTree Class - Public Method
 Introduction:
    VTA constraint and generate clauses
    Formulate unwanted scenario as clauses (CNF)
    So-called unwanted scenario: More than one "header" exist along one clock path
 Creator:
    Tien-Hung Tseng
----------------------------------------------------------------------------*/
void ClockTree::VTAConstraint(void)
{
    if( _printClause ){
        this->clauseFileName = this->_outputdir + "clause_" +  "VTA_Constraint.txt";
        if( !isDirectoryExist(this->_outputdir) )
            mkdir(this->_outputdir.c_str(), 0775);
        this->fptr = fopen( this->clauseFileName.c_str(), "w" );
        if( !fptr )
            cerr << RED"[Error]" RESET" Cannot open " << this->clauseFileName << endl ;
    }
    
    if( this->ifdoVTA() == false )
    {
        for( auto const& node: this->_buflist )//_buflist = map< string, clknode * >
        {
            string clause ;
            clause = to_string((node.second->getNodeNumber()+2) * -1) + " 0";
            if( this->_VTAconstraintlist.size() < (this->_VTAconstraintlist.max_size()-2) )
                this->_VTAconstraintlist.insert(clause);
            else
                cerr << "\033[32m[Info]: DCC Constraint List Full!\033[0m\n";
            
            if( _printClause )  fprintf( fptr, "NodoVTA: %s(%ld) %s\n", node.second->getGateData()->getGateName().c_str(),  node.second->getNodeNumber(), clause.c_str() );
        }
    }

    //-- Don't Put Header at FF ---------------------------------------------------
    for( auto FF: this->_ffsink )
    {
        string clause = to_string( (FF.second->getNodeNumber() + 2 ) * -1 ) + " 0" ;
        this->_VTAconstraintlist.insert( clause );
        if( _printClause ) fprintf( fptr, "FF: %s(%ld) %s\n", FF.second->getGateData()->getGateName().c_str(), FF.second->getNodeNumber(), clause.c_str() );
    }
    
    if( this->ifdoVTA() == true )
    for( auto path : this->_pathlist )
    {
        if( path->getPathType() == FFtoFF )         this->VTAConstraintFFtoFF( path ) ;
        else if( path->getPathType() == PItoFF )    this->VTAConstraintPItoFF( path ) ;
        else if( path->getPathType() == FFtoPO )    this->VTAConstraintFFtoPO( path ) ;
        else if( path->getPathType() == PItoPO )    continue    ;
        else if( path->getPathType() == NONE   )    continue    ;
    }
    if( _printClause ) fclose( fptr );
}
void ClockTree::VTAConstraintFFtoFF( CriticalPath *path )
{
    assert( path->getPathType() == FFtoFF );
    const vector<ClockTreeNode *> stClkPath = path->getStartPonitClkPath() ;
    const vector<ClockTreeNode *> edClkPath = path->getEndPonitClkPath()   ;
    ClockTreeNode *commonParent = path->findLastSameParentNode() ;
    int     idCommonParent = 0 ;
    string  clause = "" ;
    

    
    //-- Do VTA --------------------------------------------------------------------
    if( this->ifdoVTA() )
    {
        //Each pairs of nodes in stClkPath
        for( int L1 = 0 ; L1 < stClkPath.size()-2; L1++ )
        {
            for( int L2 = L1 + 1 ; L2 < stClkPath.size()-1; L2++ )
            {
                auto findpair = this->getVTASet().find( pair<ClockTreeNode*,ClockTreeNode*>(stClkPath.at(L1),stClkPath.at(L2)));
                if( findpair == this->getVTASet().end() )//Not repeated
                {
                    clause = to_string( (stClkPath.at(L1)->getNodeNumber()+2) * -1 ) + " " + to_string( (stClkPath.at(L2)->getNodeNumber()+2) * -1 )+ " 0" ;
                    this->_VTAconstraintlist.insert( clause );
                    if( _printClause ) fprintf( this->fptr, "%ld with %ld: %s \n",stClkPath.at(L1)->getNodeNumber(), stClkPath.at(L2)->getNodeNumber(), clause.c_str() );
                    this->getVTASet().insert( pair<ClockTreeNode*,ClockTreeNode*>(stClkPath.at(L1),stClkPath.at(L2)) );
                }
            }
            
            if( stClkPath.at(L1) == commonParent ) idCommonParent = L1 ;
        }
        //Each pairs of nodes in edClkPath
        //L1 range: root <-> common parent
        //L2 range: right child of common parent <-> right end
        for( int L1 = 0 ; L1 <= idCommonParent; L1++ )
        {
            for( int L2 = idCommonParent + 1 ; L2 < edClkPath.size()-1; L2++ )
            {
                auto findpair = this->getVTASet().find( pair<ClockTreeNode*,ClockTreeNode*>(edClkPath.at(L1),edClkPath.at(L2)));
                if( findpair == this->getVTASet().end() )//Not repeated
                {
                    clause = to_string( (edClkPath.at(L1)->getNodeNumber()+2) * -1 ) + " " + to_string( (edClkPath.at(L2)->getNodeNumber()+2) * -1 )+ " 0" ;
                    this->_VTAconstraintlist.insert( clause );
                    if( _printClause ) fprintf( this->fptr, "%ld with %ld: %s \n",edClkPath.at(L1)->getNodeNumber(), edClkPath.at(L2)->getNodeNumber(), clause.c_str() );
                    this->getVTASet().insert( pair<ClockTreeNode*,ClockTreeNode*>(edClkPath.at(L1),edClkPath.at(L2)) );
                }
            }
        }
        //Each pairs of nodes in edClkPath
        for( int L1 = idCommonParent + 1 ; L1 < edClkPath.size()-2; L1++ )
        {
            for( int L2 = L1 + 1 ; L2 < edClkPath.size()-1; L2++ )
            {
                auto findpair = this->getVTASet().find( pair<ClockTreeNode*,ClockTreeNode*>(edClkPath.at(L1),edClkPath.at(L2)));
                if( findpair == this->getVTASet().end() )//Not repeated
                {
                    clause = to_string( (edClkPath.at(L1)->getNodeNumber()+2) * -1 ) + " " + to_string( (edClkPath.at(L2)->getNodeNumber()+2) * -1 )+ " 0" ;
                    this->_VTAconstraintlist.insert( clause );
                    if( _printClause ) fprintf( this->fptr, "%ld with %ld: %s \n",edClkPath.at(L1)->getNodeNumber(), edClkPath.at(L2)->getNodeNumber(), clause.c_str() );
                    this->getVTASet().insert( pair<ClockTreeNode*,ClockTreeNode*>(edClkPath.at(L1),edClkPath.at(L2)) );
                }
            }
        }
    }
}
void ClockTree::VTAConstraintPItoFF( CriticalPath *path )
{
    assert( path->getPathType() == PItoFF );
   
    const vector<ClockTreeNode *> edClkPath = path->getEndPonitClkPath()   ;
    string clause = "" ;
    
    //-- Do VTA --------------------------------------------------------------------
    if( this->ifdoVTA() )
    {
        for( int L1 = 0 ; L1 < edClkPath.size()-2; L1++ )
        {
            for( int L2 = L1 + 1 ; L2 < edClkPath.size()-1; L2++ )
            {
                auto findpair = this->getVTASet().find( pair<ClockTreeNode*,ClockTreeNode*>(edClkPath.at(L1),edClkPath.at(L2)));
                if( findpair == this->getVTASet().end() )//Not repeated
                {
                    clause = to_string( (edClkPath.at(L1)->getNodeNumber()+2) * -1 ) + " " + to_string( (edClkPath.at(L2)->getNodeNumber()+2) * -1 )+ " 0" ;
                    this->_VTAconstraintlist.insert( clause );
                    if( _printClause ) fprintf( this->fptr, "%ld with %ld: %s \n",edClkPath.at(L1)->getNodeNumber(), edClkPath.at(L2)->getNodeNumber(), clause.c_str() );
                    this->getVTASet().insert( pair<ClockTreeNode*,ClockTreeNode*>(edClkPath.at(L1),edClkPath.at(L2)) );
                }
            }
        }
    }
}
void ClockTree::VTAConstraintFFtoPO( CriticalPath *path )
{
    assert( path->getPathType() == FFtoPO );
    const vector<ClockTreeNode *> stClkPath = path->getStartPonitClkPath() ;
    string clause = "" ;
    
    //-- Do VTA --------------------------------------------------------------------
    if( this->ifdoVTA() )
    {
        for( int L1 = 0 ; L1 < stClkPath.size()-2; L1++ )
        {
            for( int L2 = L1 + 1 ; L2 < stClkPath.size()-1; L2++ )
            {
                auto findpair = this->getVTASet().find( pair<ClockTreeNode*,ClockTreeNode*>(stClkPath.at(L1),stClkPath.at(L2)));
                if( findpair == this->getVTASet().end() )//Not repeated
                {
                    clause = to_string( (stClkPath.at(L1)->getNodeNumber()+2) * -1 ) + " " + to_string( (stClkPath.at(L2)->getNodeNumber()+2) * -1 )+ " 0" ;
                    this->_VTAconstraintlist.insert( clause );
                    if( _printClause ) fprintf( this->fptr, "%ld with %ld: %s \n",stClkPath.at(L1)->getNodeNumber(), stClkPath.at(L2)->getNodeNumber(), clause.c_str() );
                    this->getVTASet().insert( pair<ClockTreeNode*,ClockTreeNode*>(stClkPath.at(L1),stClkPath.at(L2)) );
                }
            }
        }
    }
}
/*---------------------------------------------------------------------------
 FuncName:
    VTAConstraint()
 Where defined:
    ClockTree Class - Public Method
 Introduction:
    DCC constraint and generate clauses
    Formulate unwanted scenario as clauses (CNF)
    So-called unwanted scenario: More than one DCCs exist along one clock path
 Creator:
    Senior
 ----------------------------------------------------------------------------*/
void ClockTree::dccConstraint(void)
{
	if( !this->_placedcc )
	{
		this->_nonplacedccbufnum = this->_buflist.size();
		return;
	}
    
    if( _printClause )
    {
        this->clauseFileName = this->_outputdir + "clause_" +  "DCC_Constraint.txt";
        if( !isDirectoryExist(this->_outputdir) )
            mkdir(this->_outputdir.c_str(), 0775);
        this->fptr = fopen( this->clauseFileName.c_str(), "w" );
    
        if( !fptr ) cerr << RED"[Error]" RESET" Cannot open " << this->clauseFileName << endl ;
    }
    
	// Generate two clauses for the clock tree root (clock source)
	if( this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-2) )
	{
		string clause1 = to_string(this->_clktreeroot->getNodeNumber() * -1) + " 0";
		string clause2 = to_string((this->_clktreeroot->getNodeNumber() + 1) * -1) + " 0";
		this->_dccconstraintlist.insert(clause1);
		this->_dccconstraintlist.insert(clause2);
        if( _printClause ) fprintf( fptr,"Root: %s, %s \n", clause1.c_str(), clause2.c_str() );
	}
	else
	{
		cerr << "\033[32m[Info]: DCC Constraint List Full!\033[0m\n";
		return;
	}
	// Generate two clauses for each buffer can not insert DCC
	for( auto const& node: this->_buflist )//_buflist = map< string, clknode * >
	{
		//if( !node.second->ifPlacedDcc() )
        if( node.second->ifMasked() )
		{
			string clause1, clause2;
			clause1 = to_string(node.second->getNodeNumber() * -1) + " 0";
			clause2 = to_string((node.second->getNodeNumber() + 1) * -1) + " 0";
			if(this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-2))
			{
				this->_dccconstraintlist.insert(clause1);
				this->_dccconstraintlist.insert(clause2);
                if( _printClause ) fprintf( fptr,"Masked node(%ld): %s, %s \n", node.second->getNodeNumber(), clause1.c_str(), clause2.c_str() );
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
	for( auto const& node: this->_ffsink  )
	{
        //-- Don't Put DCC ahead of FF
		string clause1, clause2;
		clause1 = to_string(node.second->getNodeNumber() * -1) + " 0";
		clause2 = to_string((node.second->getNodeNumber() + 1) * -1) + " 0";
		if(this->_dccconstraintlist.size() < (this->_dccconstraintlist.max_size()-2))
		{
			this->_dccconstraintlist.insert(clause1);
			this->_dccconstraintlist.insert(clause2);
            if( _printClause ) fprintf( fptr,"Masked FF(%ld): %s, %s \n", node.second->getNodeNumber(), clause1.c_str(), clause2.c_str() );
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
			//if( nodeptr->ifPlacedDcc() )
            if( nodeptr->ifMasked() == false )
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
    if( _printClause ) fclose(fptr);
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
    if( this->_placedcc == false && this->ifdoVTA() == false ) return -1 ;
    
    this->_timingconstraintlist.clear();
    //-- Dump Clause log --------------------------------------------------------
    if( _printClause ){
        this->clauseFileName = this->_outputdir + "clause_" + to_string(this->_tc) + ".txt";
        if( !isDirectoryExist(this->_outputdir) )
            mkdir(this->_outputdir.c_str(), 0775);
        this->fptr = fopen( this->clauseFileName.c_str(), "w" );
    }
    
    //-- Path iteration ----------------------------------------------------------
	for( auto const& path: this->_pathlist )
	{
		if( (path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF) ) continue;
		//--No DCC insertion ----------------------------------
		this->timingConstraint_ndoDCC_ndoVTA( path );//done
        this->timingConstraint_ndoDCC_doVTA( path ) ;
        
		//--DCC Insertion && VTA ------------------------------
        if( this->_placedcc )
        {
            this->timingConstraint_doDCC_ndoVTA(path);
            if( this->ifdoVTA() )
                this->timingConstraint_doDCC_doVTA(path);
        }
        
	}
     if( _printClause ) fclose( this->fptr );
    return -1 ;//tentative
}
/*------------------------------------------------------------------------------------
 FuncName:
    timingConstraint_ndoDcc_ndoVTA
 Introduction:
    Do timing constraint Without DCC insertion and Without VTA
 Operation:
    genclause = 0           => Do not generate clause
              = 1 (default) => Generate clause
    update    = 0 (default) => Do not update the timing info of the pipeline
              = 1           => Update timing info of the pipeline
 -------------------------------------------------------------------------------------*/
double ClockTree::timingConstraint_ndoDCC_ndoVTA( CriticalPath *path, bool update, bool genclause )
{
	if( path == nullptr )
		return -1 ;
    
    //------ Declare ------------------------------------------------------------------
    double newslack    = 0 ;
    double dataarrtime = 0 ;
    double datareqtime = 0 ;
	
    //------- Ci & Cj ------------------------------------------------------------------
	dataarrtime =  this->calClkLaten_givDcc_givVTA( path->getStartPonitClkPath(), 0, NULL, -1, NULL  );
    datareqtime =  this->calClkLaten_givDcc_givVTA( path->getEndPonitClkPath()  , 0, NULL, -1, NULL  );

	//------- Require/Arrival time ------------------------------------------------------
	datareqtime += (path->getTsu() * this->_agingtsu) + this->_tc;
    dataarrtime += path->getTinDelay() + (path->getTcq() * this->_agingtcq) + (path->getDij() * this->_agingdij);
	newslack = datareqtime - dataarrtime  ;
    
    string clause  = "" ;
	//-------- Timing Violation ---------------------------------------------------------
	if( newslack < 0 )
	{
		//---- PItoFF or FFtoPO --------------------------------------------------------
		if((path->getPathType() == PItoFF) || (path->getPathType() == FFtoPO))
		{
			vector<ClockTreeNode *> clkpath = ((path->getPathType() == PItoFF) ? path->getEndPonitClkPath() : path->getStartPonitClkPath());
			//- Generate Clause --------------------------------------------------------
			for( auto const& nodeptr: clkpath )
                this->genClauseByDccVTA( nodeptr, &clause, 0.5, -1 ) ;
		}
		else if( path->getPathType() == FFtoFF )//-- FFtoFF -----------------------------
		{
			ClockTreeNode *sameparent = path->findLastSameParentNode();
			//- Generate Clause/Left ----------------------------------------------------
			long sameparentloc = path->nodeLocationInClockPath('s', sameparent);
            
			for( auto const& nodeptr: path->getStartPonitClkPath() )
					this->genClauseByDccVTA( nodeptr, &clause, 0.5, -1 ) ;
        
			//- Generate Clause/Right ---------------------------------------------------
			for( long loop = (sameparentloc + 1);loop < path->getEndPonitClkPath().size(); loop++ )
					this->genClauseByDccVTA( path->getEndPonitClkPath().at(loop), &clause, 0.5, -1 );
		}
		clause += "0";
		if( this->_timingconstraintlist.size() < (this->_timingconstraintlist.max_size()-1) )
			this->_timingconstraintlist.insert(clause) ;
		else
			cerr << "\033[32m[Info]: Timing Constraint List Full!\033[0m\n";
        
	}
    if( _printClause ){
        if( newslack < 0 ) fprintf( this->fptr, "Path(%ld), stDCC(%.1f), edDCC(%.1f), stVTA(%d), edVTA(%d), slack = %f, %s \n", path->getPathNum(), -1.0, -1.0, -1, -1, newslack, clause.c_str() );
        else
            fprintf( this->fptr, "Path(%ld), stDCC(%.1f), edDCC(%.1f), stVTA(%d), edVTA(%d), slack = %f \n", path->getPathNum(), -1.0, -1.0, -1, -1, newslack  );
    }
    
    return newslack ;
}
/*------------------------------------------------------------------------------------
 FuncName:
    UpdatePathTiming
 Introduction:
    Used in UpdateAllPathTiming
 Pata:
    if update is false => The func is used to calculate the slack of pipeline
 -------------------------------------------------------------------------------------*/
double ClockTree::UpdatePathTiming( CriticalPath * path, bool update, bool DCCVTA, bool aging )
{
    //-- (Inserted) DCC Info -----------------------------------------------
    ClockTreeNode *stDCCLoc = NULL ;
    ClockTreeNode *edDCCLoc = NULL ;
    double stDCCType = 0.5 ;
    double edDCCType = 0.5 ;
    //-- (Inserted) VTA Info -----------------------------------------------
    ClockTreeNode *stHeader = NULL ;
    ClockTreeNode *edHeader = NULL ;
    int stLibIndex = -1 ;
    int edLibIndex = -1 ;
    
    //-- DCC Inserted Location --------------------------------------------
    stDCCLoc = path->findDccInClockPath('s');
    edDCCLoc = path->findDccInClockPath('e') ;
    if( stDCCLoc ){
        stDCCType = stDCCLoc->getDccType() ;
    }
    if( edDCCLoc ){
        edDCCType = edDCCLoc->getDccType() ;
    }
    //-- VTA Header Location ----------------------------------------------
    stHeader = path->findVTAInClockPath('s') ;
    edHeader = path->findVTAInClockPath('e') ;
    if( stHeader ){
        stLibIndex = stHeader->getVTAType() ;
    }
    if( edHeader ){
        edLibIndex = edHeader->getVTAType() ;
    }
    if( !DCCVTA ){
        stDCCLoc = edDCCLoc = edHeader = stHeader = NULL ;
        stDCCType = edDCCType = 0 ;
        stLibIndex= edLibIndex = -1 ;
    }
    //-- Timing -----------------------------------------------------------
    double ci = 0 ;
    double cj = 0 ;
    double newslack = 0, req_time = 0, avl_time = 0;

    bool original_aging_setting = this->_aging ;
    this->_aging = aging ;
    
    int PathType = path->getPathType() ;
    if( PathType == FFtoFF || PathType == FFtoPO )
        ci = this->calClkLaten_givDcc_givVTA( path->getStartPonitClkPath(), stDCCType, stDCCLoc, stLibIndex, stHeader );//Has consider aging
    if( PathType == FFtoFF || PathType == PItoFF )
        cj = this->calClkLaten_givDcc_givVTA( path->getEndPonitClkPath(),   edDCCType, edDCCLoc, edLibIndex, edHeader );//Has consider aging
   
    this->_aging = original_aging_setting ;
    //------- Avl/Require time -------------------------------------------------------------
    double Tsu = (aging)? (path->getTsu() * this->_agingtsu) : (path->getTsu()) ;
    double Tcq = (aging)? (path->getTcq() * this->_agingtcq) : (path->getTcq()) ;
    double Dij = (aging)? (path->getDij() * this->_agingdij) : (path->getDij()) ;
    req_time = cj + Tsu + this->_tc;
    avl_time = ci + path->getTinDelay() + Tcq + Dij ;
    
    newslack = req_time - avl_time  ;
    
    
    if( update )
    {
        path->setCi(ci)                         ;
        path->setCj(cj)                         ;
        path->setArrivalTime(avl_time)       ;
        path->setRequiredTime(req_time)      ;
        path->setSlack(newslack)                ;
    }
    
    return newslack ;
}
/*------------------------------------------------------------------------------------
 FuncName:
    timingConstraint_doDcc_ndoVTA
 Introduction:
    Do timing constraint iterate DCC insertion, but don't do VTA
 -------------------------------------------------------------------------------------*/
void ClockTree::timingConstraint_doDCC_ndoVTA( CriticalPath *path, bool update )
{
    if( path == nullptr )
        return  ;
    
    //List all possible combination of VTA with a given DCC deployment
    vector<vector<ClockTreeNode *> > dcccandi = path->getDccPlacementCandi();
    for( int i = 0 ; i < dcccandi.size(); i ++ )
    {
        if( path->getPathType() == FFtoFF )
        {
            long candilocleft  = path->nodeLocationInClockPath('s', dcccandi.at(i).back() /*Clk node*/ );//location id, 's' mean start clk path
            long candilocright = path->nodeLocationInClockPath('e', dcccandi.at(i).back() /*Clk node*/ );//location id, 'e' mean end   clk path
            long sameparentloc = path->nodeLocationInClockPath('s', path->findLastSameParentNode());
            if( dcccandi.at(i).size() == 1 )
            {
                // Insert DCC on common part
                if((candilocleft != -1) && (candilocleft <= sameparentloc))
                {
                    this->timingConstraint_givDCC_givVTA( path, 0.2, -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
                    this->timingConstraint_givDCC_givVTA( path, 0.4, -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
                    this->timingConstraint_givDCC_givVTA( path, 0.8, -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
                }
                // Insert DCC on the right branch part
                else if( candilocleft < candilocright )
                {
                    this->timingConstraint_givDCC_givVTA( path, -1, 0.2, NULL, dcccandi.at(i).front(), -1, -1, NULL, NULL ) ;
                    this->timingConstraint_givDCC_givVTA( path, -1, 0.4, NULL, dcccandi.at(i).front(), -1, -1, NULL, NULL ) ;
                    this->timingConstraint_givDCC_givVTA( path, -1, 0.8, NULL, dcccandi.at(i).front(), -1, -1, NULL, NULL ) ;
                }
                // Insert DCC on the left branch part
                else if( candilocleft > candilocright )
                {
                    this->timingConstraint_givDCC_givVTA( path, 0.2,  -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
                    this->timingConstraint_givDCC_givVTA( path, 0.4,  -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
                    this->timingConstraint_givDCC_givVTA( path, 0.8,  -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
                }
            }
            else//insert 2 dcc
            {
                this->timingConstraint_givDCC_givVTA( path, 0.2, 0.2, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.2, 0.4, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.2, 0.8, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.4, 0.2, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.4, 0.4, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.4, 0.8, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.8, 0.2, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.8, 0.4, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
                this->timingConstraint_givDCC_givVTA( path, 0.8, 0.8, dcccandi.at(i).front(), dcccandi.at(i).back(), -1, -1, NULL, NULL ) ;
            }
        }
        else if( path->getPathType() == FFtoPO )
        {
            this->timingConstraint_givDCC_givVTA( path, 0.2, -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
            this->timingConstraint_givDCC_givVTA( path, 0.4, -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
            this->timingConstraint_givDCC_givVTA( path, 0.8, -1, dcccandi.at(i).front(), NULL, -1, -1, NULL, NULL ) ;
        }
        else if( path->getPathType() == PItoFF )
        {
            this->timingConstraint_givDCC_givVTA( path,  -1, 0.2, NULL, dcccandi.at(i).front(), -1, -1, NULL, NULL ) ;
            this->timingConstraint_givDCC_givVTA( path,  -1, 0.4, NULL, dcccandi.at(i).front(), -1, -1, NULL, NULL ) ;
            this->timingConstraint_givDCC_givVTA( path,  -1, 0.8, NULL, dcccandi.at(i).front(), -1, -1, NULL, NULL ) ;
        }
    }
}
/*------------------------------------------------------------------------------------
 FuncName:
    timingConstraint_doDcc_doVTA
 Introduction:
    Do timing constraint iterate DCC insertion
 -------------------------------------------------------------------------------------*/
void ClockTree::timingConstraint_doDCC_doVTA( CriticalPath *path, bool update )
{
    if( path == nullptr )
        return  ;
    
    //List all possible combination of VTA with a given DCC deployment

    vector<vector<ClockTreeNode *> > dcccandi = path->getDccPlacementCandi();
    for( int i = 0 ; i < dcccandi.size(); i ++ )
    {
        if( path->getPathType() == FFtoFF )
        {
            long candilocleft  = path->nodeLocationInClockPath('s', dcccandi.at(i).back() /*Clk node*/ );//location id, 's' mean start clk path
            long candilocright = path->nodeLocationInClockPath('e', dcccandi.at(i).back() /*Clk node*/ );//location id, 'e' mean end   clk path
            long sameparentloc = path->nodeLocationInClockPath('s', path->findLastSameParentNode());
            if( dcccandi.at(i).size() == 1 )
            {
                assert( dcccandi.at(i).back() == dcccandi.at(i).front() );
                // Insert DCC on common part
                if((candilocleft != -1) && (candilocleft <= sameparentloc))
                {
                    this->timingConstraint_givDCC_doVTA( path, 0.2, -1, dcccandi.at(i).front(), NULL ) ;
                    this->timingConstraint_givDCC_doVTA( path, 0.4, -1, dcccandi.at(i).front(), NULL ) ;
                    this->timingConstraint_givDCC_doVTA( path, 0.8, -1, dcccandi.at(i).front(), NULL ) ;
                }
                // Insert DCC on the right branch part
                else if( candilocleft < candilocright )
                {
                    this->timingConstraint_givDCC_doVTA( path, -1, 0.2, NULL, dcccandi.at(i).front() ) ;
                    this->timingConstraint_givDCC_doVTA( path, -1, 0.4, NULL, dcccandi.at(i).front() ) ;
                    this->timingConstraint_givDCC_doVTA( path, -1, 0.8, NULL, dcccandi.at(i).front() ) ;
                }
                // Insert DCC on the left branch part
                else if( candilocleft > candilocright )
                {
                    this->timingConstraint_givDCC_doVTA( path, 0.2, -1, dcccandi.at(i).front(), NULL ) ;
                    this->timingConstraint_givDCC_doVTA( path, 0.4, -1, dcccandi.at(i).front(), NULL ) ;
                    this->timingConstraint_givDCC_doVTA( path, 0.8, -1, dcccandi.at(i).front(), NULL ) ;
                }
            }
            else//insert 2 dcc
            {
                this->timingConstraint_givDCC_doVTA( path, 0.2, 0.2, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.2, 0.4, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.2, 0.8, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.4, 0.2, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.4, 0.4, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.4, 0.8, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.8, 0.2, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.8, 0.4, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
                this->timingConstraint_givDCC_doVTA( path, 0.8, 0.8, dcccandi.at(i).front(), dcccandi.at(i).back() ) ;
            }
        }
        else if( path->getPathType() == FFtoPO )
        {
            this->timingConstraint_givDCC_doVTA( path, 0.2, -1, dcccandi.at(i).front(), NULL );
            this->timingConstraint_givDCC_doVTA( path, 0.4, -1, dcccandi.at(i).front(), NULL );
            this->timingConstraint_givDCC_doVTA( path, 0.8, -1, dcccandi.at(i).front(), NULL );
            // -1 denotes the path does not exist
        }
        else if( path->getPathType() == PItoFF )
        {
            this->timingConstraint_givDCC_doVTA( path, -1, 0.2, NULL, dcccandi.at(i).front() );
            this->timingConstraint_givDCC_doVTA( path, -1, 0.4, NULL, dcccandi.at(i).front() );
            this->timingConstraint_givDCC_doVTA( path, -1, 0.8, NULL, dcccandi.at(i).front() );
            // -1 denotes the path does not exist
        }
    }
}
/*------------------------------------------------------------------------------------
 FuncName:
    timingConstraint_ndoDcc_doVTA
 Introduction:
    Do timing constraint iterate DCC VTA
 -------------------------------------------------------------------------------------*/
void ClockTree::timingConstraint_ndoDCC_doVTA( CriticalPath *path, bool update )
{
    if( this->ifdoVTA() == false ) return ;
    //----- Checking ---------------------------------
    if( path == nullptr ) return  ;
    
    //----- Declaration ------------------------------
    vector<ClockTreeNode*> stClkPath = path->getStartPonitClkPath() ;
    vector<ClockTreeNode*> edClkPath = path->getEndPonitClkPath() ;
    
    //----- TC ---------------------------------------
    if( path->getPathType() == FFtoPO )
    {
        for( int i = 0 ; i < stClkPath.size()-1; i++ )
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, -1, -1, NULL, NULL, LibIndex, -1, stClkPath.at(i), NULL);
                
    }
    else if( path->getPathType() == PItoFF )
    {
        for( int i = 0 ; i < edClkPath.size()-1; i++ )
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, -1, -1, NULL, NULL, -1, LibIndex, NULL, edClkPath.at(i));
    }
    else if( path->getPathType() == FFtoFF )
    {
        long sameparentloc = path->nodeLocationInClockPath('s', path->findLastSameParentNode() );
        
        //Part 1: One header at left clk path.
        for( long i = 0 ; i < stClkPath.size()-1; i++ )
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, -1, -1, NULL, NULL, LibIndex, -1, stClkPath.at(i), NULL );
        
        
        //Part 2: One header at right clk path.
        for( long i = sameparentloc + 1 ; i < edClkPath.size()-1; i++ )
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, -1, -1, NULL, NULL, -1, LibIndex, NULL, edClkPath.at(i));
               
        
        //Part 3: Two headers at both branches.
        for( long i = sameparentloc + 1 ; i < stClkPath.size()-1; i++ )
            for( int stLibIndex = 0 ; stLibIndex < this->getLibList().size() ; stLibIndex++ )
                for( long j = sameparentloc + 1 ; j < edClkPath.size()-1; j++ )
                    for( int edLibIndex = 0 ; edLibIndex < this->getLibList().size() ; edLibIndex++ )
                        this->timingConstraint_givDCC_givVTA( path, -1, -1, NULL, NULL, stLibIndex, edLibIndex, stClkPath.at(i),edClkPath.at(j) );
    }
}
/*------------------------------------------------------------------------------------
 FuncName:
    B-1, timingConstraint_givDCC_doVTA
 Introduction:
    After DCC insertion is determined (given), iterate the combination of VTA
    in the given pipeline
 Support:
    Do DCC, and Do VTA
 -------------------------------------------------------------------------------------*/
void ClockTree::timingConstraint_givDCC_doVTA(  CriticalPath *path,
                                                double stDccType, double edDccType,
                                                ClockTreeNode *stDccLoc, ClockTreeNode *edDccLoc
                                              )
{
    //----- Checking ---------------------------------
    if( path == nullptr ) return  ;
    //----- Declaration ------------------------------
    vector<ClockTreeNode*> stClkPath = path->getStartPonitClkPath() ;
    vector<ClockTreeNode*> edClkPath = path->getEndPonitClkPath() ;
    
    if( path->getPathType() == FFtoFF )
    {
        long sameparentloc = path->nodeLocationInClockPath('s', path->findLastSameParentNode() );
        
        //Part 1: One header at left clk path.
        for( long i = 0 ; i < stClkPath.size()-1; i++ )
        {
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, stDccType, edDccType, stDccLoc, edDccLoc, LibIndex, -1, stClkPath.at(i), NULL );
        }
        //Part 2: One header at right clk path.
        for( long i = sameparentloc + 1 ; i < edClkPath.size()-1; i++ )
        {
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, stDccType, edDccType, stDccLoc, edDccLoc, -1, LibIndex, NULL, edClkPath.at(i));
        }
        //Part 3: Two headers at both branches.
        for( long i = sameparentloc + 1 ; i < stClkPath.size()-1; i++ )
            for( int stLibIndex = 0 ; stLibIndex < this->getLibList().size() ; stLibIndex++ )
                for( long j = sameparentloc + 1 ; j < edClkPath.size()-1; j++ )
                    for( int edLibIndex = 0 ; edLibIndex < this->getLibList().size() ; edLibIndex++ )
                        this->timingConstraint_givDCC_givVTA( path, stDccType, edDccType, stDccLoc, edDccLoc, stLibIndex, edLibIndex, stClkPath.at(i),edClkPath.at(j) );
                        
    }
    else if( path->getPathType() == PItoFF )
    {
        for( int i = 0 ; i < edClkPath.size()-1; i++ )
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                this->timingConstraint_givDCC_givVTA( path, -1, edDccType, NULL, edDccLoc, -1, LibIndex, NULL, edClkPath.at(i));
                
    }
    else if( path->getPathType() == FFtoPO )
    {
        for( int i = 0 ; i < stClkPath.size()-1; i++ )
            for( int LibIndex = 0 ; LibIndex < this->getLibList().size() ; LibIndex++ )
                timingConstraint_givDCC_givVTA( path, stDccType, -1, stDccLoc, NULL, LibIndex, -1, stClkPath.at(i), NULL);
    }
}

/*------------------------------------------------------------------------------------
 FuncName:
    B-2 timingConstraint_givDCC_givVTA, caller is B-1
 Introduction:
    After DCC insertion, VTA are given, estimate whether timing violation occurs
 Support:
        Do DCC, and     Do VTA
    Not Do DCC, but     Do VTA
        Do DCC, and Not Do VTA
 -------------------------------------------------------------------------------------*/
double ClockTree::timingConstraint_givDCC_givVTA(   CriticalPath *path,
                                                    double stDCCType, double edDCCType,
                                                    ClockTreeNode *stDCCLoc, ClockTreeNode *edDCCLoc,
                                                    int   stLibIndex, int edLibIndex,
                                                    ClockTreeNode *stHeader, ClockTreeNode *edHeader )
{
    if( path == NULL ) return -1 ;
    
    //------ Declare ------------------------------------------------------------------
    double  slack       = 0 ;
    double  ci          = 0 ;
    double  cj          = 0 ;
    double  avl_time    = 0 ;//arrival time
    double  req_time    = 0 ;//require time
    int     PathType    = path->getPathType() ;
    //------- Ci & Cj ------------------------------------------------------------------
    if( PathType == FFtoFF || PathType == FFtoPO )
        ci = this->calClkLaten_givDcc_givVTA( path->getStartPonitClkPath(), stDCCType, stDCCLoc, stLibIndex, stHeader );//Has consider aging
    if( PathType == FFtoFF || PathType == PItoFF )
        cj = this->calClkLaten_givDcc_givVTA( path->getEndPonitClkPath(),   edDCCType, edDCCLoc, edLibIndex, edHeader );//Has consider aging
    //------- Require time -------------------------------------------------------------
    double Tsu = (this->_aging)? (path->getTsu() * this->_agingtsu) : (path->getTsu()) ;
    double Tcq = (this->_aging)? (path->getTcq() * this->_agingtcq) : (path->getTcq()) ;
    double Dij = (this->_aging)? (path->getDij() * this->_agingdij) : (path->getDij()) ;
    req_time = cj + Tsu + this->_tc;
    
    //------- Arrival time --------------------------------------------------------------
    avl_time = ci + path->getTinDelay() + Tcq + Dij ;
    slack    = req_time - avl_time  ;
    
    //-- Formulation ---------------------------------------------------------------------
    string clause = "" ;
    if( slack < 0 )
    {
        //-- PItoFF ----------------------------------------------------------------------
        if( path->getPathType() == FFtoPO )
        {
            for( auto clknode: path->getStartPonitClkPath() )
            {
                if( clknode == path->getStartPonitClkPath().back() ) continue ;
                //-- DCC Formulation -----------------------------------------------------
                if( clknode != stDCCLoc )
                    this->writeClause_givDCC( clause, clknode, 0.5 );
                else if( clknode == stDCCLoc )
                    this->writeClause_givDCC( clause, clknode, stDCCType );
                
                //-- VTA Formulation ------------------------------------------------------
                if( this->ifdoVTA() && clknode == stHeader )
                    this->writeClause_givVTA( clause, clknode, stLibIndex );
                else if( this->ifdoVTA() )
                    this->writeClause_givVTA( clause, clknode, -1 );//-1 denotes that node is not header
            }
        }
        //--  FFtoPO ---------------------------------------------------------------------
        else if( path->getPathType() == PItoFF )
        {
            for( auto clknode: path->getEndPonitClkPath() )
            {
                if( clknode == path->getEndPonitClkPath().back() ) continue ;
                //-- DCC Formulation -----------------------------------------------------
                if( clknode != edDCCLoc )
                    this->writeClause_givDCC( clause, clknode, 0.5 );
                else
                    this->writeClause_givDCC( clause, clknode, edDCCType );
                
                //-- VTA Formulation ------------------------------------------------------
                if( this->ifdoVTA() && clknode == edHeader )
                    this->writeClause_givVTA( clause, clknode, edLibIndex );
                else if( this->ifdoVTA() )
                    this->writeClause_givVTA( clause, clknode, -1 );//-1 denotes that node is not header
            }
        }
        //-- FFtoFF ------------------------------------------------------------------------
        else
        {
            ClockTreeNode *sameparent = path->findLastSameParentNode() ;
            long  commonparent = path->nodeLocationInClockPath( 's',sameparent );
            
            for( auto clknode: path->getStartPonitClkPath() )
            {
                if( clknode == path->getStartPonitClkPath().back() ) continue ;
                //-- DCC Formulation -----------------------------------------------------
                if(  clknode != stDCCLoc )
                    this->writeClause_givDCC( clause, clknode, 0.5 );
                else
                    this->writeClause_givDCC( clause, clknode, stDCCType );
                
                //-- VTA Formulation ------------------------------------------------------
                if( this->ifdoVTA() && clknode == stHeader )
                    this->writeClause_givVTA( clause, clknode, stLibIndex );
                else if( this->ifdoVTA() )
                    this->writeClause_givVTA( clause, clknode, -1 );
            }
            for( long k = commonparent+1; k < path->getEndPonitClkPath().size(); k++ )
            {
                ClockTreeNode* clknode = path->getEndPonitClkPath().at(k) ;
                if( clknode == path->getEndPonitClkPath().back() ) continue ;
                //-- DCC Formulation -----------------------------------------------------
                if(  clknode != edDCCLoc )
                    this->writeClause_givDCC( clause, clknode, 0.5 );
                else
                    this->writeClause_givDCC( clause, clknode, edDCCType );
                
                //-- VTA Formulation ------------------------------------------------------
                if( this->ifdoVTA() && clknode == edHeader )
                    this->writeClause_givVTA( clause, clknode, edLibIndex );
                else if( this->ifdoVTA() )
                    this->writeClause_givVTA( clause, clknode, -1 );
            }//for(k)
        }//FFtoFF
        
        clause += "0" ;
        if( this->_timingconstraintlist.size() < (this->_timingconstraintlist.max_size()-1) )
            this->_timingconstraintlist.insert(clause) ;
        else
            cerr << "\033[32m[Info]: Timing Constraint List Full!\033[0m\n";
    }//if( newslack < 0 )
    
    if( _printClause )
    {
        fprintf( this->fptr,"Path(%4ld), ", path->getPathNum() );
        if( stDCCLoc )  fprintf( this->fptr,"stDCC (%4ld, %.1f ), ", stDCCLoc->getNodeNumber(), stDCCType  );
        else            fprintf( this->fptr,"stDCC (%4d, %.1f ), ",                          -1, -1.0       );
        if( edDCCLoc )  fprintf( this->fptr,"edDCC (%4ld, %.1f ), ", edDCCLoc->getNodeNumber(), edDCCType  );
        else            fprintf( this->fptr,"edDCC (%4d, %.1f ), ",                          -1, -1.0       );
        if( stHeader )  fprintf( this->fptr,"stVTA (%4ld, %2d ), ", stHeader->getNodeNumber(), stLibIndex );
        else            fprintf( this->fptr,"stVTA (%4d, %2d ), ",                           -1, -1         );
        if( edHeader )  fprintf( this->fptr,"edVTA (%4ld, %2d ), ", edHeader->getNodeNumber(), edLibIndex  );
        else            fprintf( this->fptr,"edVTA (%4d, %2d ), ",                           -1, -1         );
        
        if( slack < 0 ) fprintf( this->fptr,"slk = %f: %s \n", slack, clause.c_str() );
        else            fprintf( this->fptr,"slk = %f: %s \n", slack, " " );
    }
    return slack ;
}

/*------------------------------------------------------------------------------------
 FuncName:
    (1) writeClause_givDCC
    (2) writeClsuse_givVTA
 Introduction:
    (1) Write associated clause by given DCC scenario
    (2) Write associated clause by given VTA
 Boolean Variables:
    (A) nodenum     => B0
    (B) nodenum + 1 => B1
    (C) nodenum + 2 => B2
        B0 and B1 are used to encode DCC insertion
        B2 is used to encode VTA
 -------------------------------------------------------------------------------------*/
void ClockTree::writeClause_givDCC( string &clause, ClockTreeNode *node, double DCCType )
{
    if( node == NULL ) return ;
    long nodenum = node->getNodeNumber() ;
    
    if( DCCType == 0.5 || DCCType == -1 || DCCType == 0 )
            clause += to_string(nodenum) + " " + to_string(nodenum + 1) + " ";
    else if( DCCType == 0.2 )
            clause += to_string(nodenum) + " " + to_string((nodenum + 1 ) * -1) + " ";
    else if( DCCType == 0.4 )
            clause += to_string(nodenum * -1) + " " + to_string(nodenum + 1 ) + " ";
    else if( DCCType == 0.8 )
            clause += to_string(nodenum * -1) + " " + to_string((nodenum + 1 ) * -1 ) + " ";
    else
        cerr << "[Error] Unrecongnized duty cycle in func \"writeClause_givDCC( string&, ClockTreeNode*, double ) in clocktree.cpp\"  \n" ;
}
void ClockTree::writeClause_givVTA( string &clause, ClockTreeNode *node, int LibIndex )
{
    if( node == NULL ) return ;
    long nodenum = node->getNodeNumber() ;
    
    //--- Node is not header ----------------------------------------------------------
    if( LibIndex == -1 )
        clause += to_string( nodenum + 2 ) + " ";
    else if( LibIndex == 0 )
        clause += to_string( ( nodenum + 2 )*(-1) ) + " " ;
    //--- Node is header --------------------------------------------------------------
    else
        cerr << "[Error] Unrecongnized Vth Type in func \"writeClause_givVTA( string&, ClockTreeNode*, double ) in clocktree.cpp\"  \n" ;
}
/*------------------------------------------------------------------------------------
 FuncName:
 timingConstraint_givDCC_givVTA
 Introduction:
 After DCC insertion, VTA are given, estimate whether timing violation occurs
 -------------------------------------------------------------------------------------*/
double ClockTree::calClkLaten_givDcc_givVTA(    vector<ClockTreeNode *> clkpath,
                                            double DCCType,  ClockTreeNode *DCCLoc,
                                            int    LibIndex, ClockTreeNode *Header
                                            )
{
    //-- Check ------------------------------------------------------------------------
    //edClkPath of FFtoPO does not exist
    //StClkPath of PItoFF does not exist
    if( clkpath.size() <= 0 ) return 0 ;
    
    
    //----Declare --------------------------------------------------------------------
    double  laten       =   0   ;
    bool    meetDCCLoc  = false ;
    bool    meetHeader  = false ;
    double  DC          = 0.5   ;//Duty Cycle
    
    int     LibVthType    = -1    ;//Vth type of clock buffer (except DCC)
    int     LibVthTypeDCC = -1    ;//Vth type of DCC
    double  agingrate     = getAgingRate_givDC_givVth( DC, -1 ) ;
    double  minbufdelay   = 9999  ;
    double  buftime       = 0     ;
    
    for( int i = 0 ; i < ( clkpath.size()-1 ); i++ )
    {
        buftime = clkpath.at(i)->getGateData()->getWireTime() + clkpath.at(i)->getGateData()->getGateTime() ;
        
        if( clkpath.at(i) != this->_clktreeroot )
            minbufdelay = min( buftime, minbufdelay );
        
        //--- Cal AgingRate of Clock Buffer -----------------------------------------
        //--- First meet DCC Location -----------------------------------------------
        if( ( clkpath.at(i) == DCCLoc ) && (!meetDCCLoc) )
        {
            DC          = DCCType ;
            meetDCCLoc  = true ;
            //--- DCC Loc == VTA Header ---------------------------------------------
            if( clkpath.at(i) == Header )
                LibVthTypeDCC = LibIndex ;
            //--- DCC Loc is ahead of VTA Header ------------------------------------
            else if( !meetHeader )
                LibVthTypeDCC = -1 ;
            //--- DCC Loc is behind VTA Header ------------------------------------
            else if( meetHeader )
                LibVthTypeDCC = LibVthType ;
        }
        //--- First meet VTA Header -------------------------------------------------
        if( ( clkpath.at(i) == Header ) && (!meetHeader) )
        {
            LibVthType = LibIndex ;//Need modifys
            meetHeader = true ;
        }
        
        agingrate = (this->_aging)? getAgingRate_givDC_givVth( DC, LibVthType ) : (1)  ;
        
        laten += buftime*agingrate ;
        //printf("Buf = %f, ", buftime*agingrate );
        /*
        if( clkpath.at(i)->ifClockGating() )
        {
            DC =  DC * (1 - clkpath.at(i)->getGatingProbability()) ;
            agingrate = getAgingRate_givDC_givVth( DC, LibVthType ) ;
        }
         */
    }
    agingrate = (this->_aging)? getAgingRate_givDC_givVth( DC, -1 ) : (1)  ;
    laten += clkpath.back()->getGateData()->getWireTime() * agingrate ;
    //printf("Wire = %f \n", clkpath.back()->getGateData()->getWireTime() * getAgingRate_givDC_givVth( DC, -1 ) );
    if( DCCLoc != NULL )
    {
        double agr_DCC =  (this->_aging)? (getAgingRate_givDC_givVth( 0.5, LibVthTypeDCC )) : (1) ;//DCC use 0.5 DC?
        double DCC_Delay = 0 ;
        if( DCCType == 0.2 )
            DCC_Delay = minbufdelay*agr_DCC*DCCDELAY20PA ;
        else if( DCCType == 0.4 )
            DCC_Delay = minbufdelay*agr_DCC*DCCDELAY40PA ;
        else if( DCCType == 0.5 || DCCType == -1 || DCCType == 0 )
            DCC_Delay = minbufdelay*agr_DCC*DCCDELAY50PA ;
        else if( DCCType == 0.8 )
            DCC_Delay = minbufdelay*agr_DCC*DCCDELAY80PA ;
        laten += DCC_Delay ;
        //printf("MinBuffer = %f, DCC Delay = %f\n", minbufdelay ,DCC_Delay );
        //printf("DCC Type = %f\n\n", DCCType );
    }
    
    return laten ;
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
	if( !this->_placedcc && !(this->ifdoVTA()) )  return ;
	fstream cnffile ;
	string cnfinput = this->_outputdir + "cnfinput_" + to_string(this->_tc);
	if( !isDirectoryExist(this->_outputdir) )
		mkdir(this->_outputdir.c_str(), 0775);
	if( !isFileExist(cnfinput) )
	{
        printf( YELLOW"\t[------CNF--------] " RESET"Encoded as %s...\033[0m\n", cnfinput.c_str());
		cnffile.open(cnfinput, ios::out | fstream::trunc);
		if( !cnffile.is_open() )
		{
			cerr << RED"\t[Error]: Cannot open " << cnfinput << "\033[0m\n";
			cnffile.close();
			return;
		}
		//--- DCC constraint ---------------------------------
        for( auto const& clause: this->_dccconstraintlist )     {    cnffile << clause << "\n" ; }
        //--- VTA constraint ---------------------------------
        for( auto const& clause: this->_VTAconstraintlist )     {    cnffile << clause << "\n" ; }
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
    /*
    if(!this->_placedcc)
        return;
     */
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
void ClockTree::tcBinarySearch( )
//void ClockTree::tcBinarySearch(double slack)
{
    // Place DCCs
    if( this->_placedcc || this->ifdoVTA() )
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
            printf( YELLOW"\t[----MiniSAT------] " RESET "Return: " RED"UNSAT \033[0m\n" ) ;
            printf( YELLOW"\t[--Binary Search--] " RESET "Next Tc range: %f - %f \033[0m\n", _tclowbound, _tcupbound ) ;
        }
        // Change the upper boundary
        else if((line.size() == 3) && (line.find("SAT") != string::npos))
        {
            this->_besttc = this->_tc;
            this->_tcupbound = this->_tc;
            this->_tc = floorNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
            printf( YELLOW"\t[----MiniSAT------] " RESET "Return: " GREEN"SAT \033[0m\n" ) ;
            printf( YELLOW"\t[--Binary Search--] " RESET"Next Tc range: %f - %f \033[0m\n", _tclowbound, _tcupbound ) ;
        }
        cnffile.close();
    }
    else
    {
        double minslack = 9999 ;
        for( auto const& path: this->_pathlist )
        {
            if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
                continue;
            // Update timing information
            double slack = this->UpdatePathTiming( path, false, true, true );
            
            if( slack < minslack )
            {
                minslack = min( slack, minslack );
            }
        }
        // Change the lower boundary
        if( minslack <= 0)
        {
            this->_tclowbound = this->_tc;
            this->_tc = ceilNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
            printf( YELLOW"\t[Slack] " RESET "slack = " RED"%f \033[0m\n", minslack ) ;
            printf( YELLOW"\t[Binary Search] " RESET"Next Tc range: %f - %f \033[0m\n", _tclowbound, _tcupbound ) ;
        }
        // Change the upper boundary
        else if( minslack > 0)
        {
            this->_besttc = this->_tc;
            this->_tcupbound = this->_tc;
            this->_tc = floorNPrecision((this->_tcupbound + this->_tclowbound) / 2, PRECISION);
            printf( YELLOW"\t[Slack] " RESET "slack = " GREEN"%f \033[0m\n", minslack ) ;
            printf( YELLOW"\t[Binary Search] " RESET"Next Tc range: %f - %f \033[0m\n", _tclowbound, _tcupbound ) ;
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
    //-- Declare -------------------------------------------------------------------------
	double minslack = 9999          ;
	
    //-- If Place DCC or VTA --------------------------------------------------------------
    //-- DCC or VTA Decoding --------------------------------------------------------------
    if( this->_placedcc || this->ifdoVTA() )
    {
        //-- Read CNF (Minisat Output) -------------------------------------------------------
        fstream cnffile ;
        string  line = "" ;
        string  lastsatfile = "";
        lastsatfile = this->_outputdir + "cnfoutput_" + to_string(this->_besttc);
        if( !isFileExist(lastsatfile) )
        {
            cerr << "\033[31m[Error]: File \"" << lastsatfile << "\" does not found!\033[0m\n";
            return;
        }
        cnffile.open(lastsatfile, ios::in);
        if( !cnffile.is_open() )
        {
            cerr << "\033[31m[Error]: Cannot open " << lastsatfile << "\033[0m\n";
            cnffile.close();
            return;
        }
        this->_tc = this->_besttc ;
        //-- Init -------------------------------------------------------------------------
        for( auto const& node: this->_buflist )
        {
            node.second->setIfPlaceDcc(0)   ;
            node.second->setIfPlaceHeader(0);
        }
        
        getline(cnffile, line) ;
        //-- Solution Exist ---------------------------------------------------------------
        if( (line.size() == 3) && (line.find("SAT") != string::npos) )
        {
            getline(cnffile, line) ;
            vector<string> strspl = stringSplit(line, " ");
            //------ Clk Node Iteration ---------------------------------------------------
            for( long loop = 0; ; loop += 3 /*2*/ )
            {
                //-- End of CNF -----------------------------------------------------------
                //cout << loop << endl ;
                if( strspl[loop] == ""  )
                {
                    printf( GREEN"Finish Decoding (space)\n");
                    break ;
                }
                if( stoi( strspl[loop] ) == 0 )
                {
                    printf( GREEN"Finish Decoding (0)\n");
                    break ;
                }
                
                //-- Put DCC --------------------------------------------------------------
                if( this->_placedcc && (( stoi(strspl.at(loop)) > 0) || (stoi(strspl.at(loop + 1)) > 0) ) )
                {
                    ClockTreeNode *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
                
                    if( findnode != nullptr )
                    {
                        findnode->setIfPlaceDcc(1)      ;
                        findnode->setDccType(stoi(strspl.at(loop)), stoi(strspl.at(loop + 1)));
                        this->_dcclist.insert(pair<string, ClockTreeNode *> (findnode->getGateData()->getGateName(), findnode));
                        //printf("[Info] Insert DCC\n");
                    }
                    else
                        cerr << "[Error] Clock node mismatch, when decoing DCC and Solution exist!\n" ;
                }
                //-- Put Header ------------------------------------------------------------
                if( this->ifdoVTA() && ( stoi(strspl.at(loop+2)) > 0 ) )
                {
                    ClockTreeNode *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
                    
                    if( findnode != nullptr )
                    {
                        findnode->setIfPlaceHeader(1);
                        findnode->setVTAType(0);
                        this->_VTAlist.insert(pair<string, ClockTreeNode *> (findnode->getGateData()->getGateName(), findnode));
                        //printf("[Info] Insert VTA Header\n");
                    }
                    else
                        cerr << "[Error] Clock node mismatch, when decoing VTA and Solution exist!\n" ;
                }
            }
            cnffile.close() ;
            for( auto const& path: this->_pathlist )
            {
                if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
                    continue;
                // Update timing information
                double slack = this->UpdatePathTiming( path, true, true, true );
                
                if( slack < minslack )
                {
                    this->_mostcriticalpath = path;
                    minslack = min( slack, minslack );
                }
            }
        }
        //-- Solution Not Exist ------------------------------------------------------------
        else
        {
            cerr << "\033[31m[Error]: File \"" << lastsatfile << "\" is not SAT!\033[0m\n";
            return ;
        }
    }
    //-- If No DCC insertion && no VTA -----------------------------------------------------
    else
    {
        for( auto const& path: this->_pathlist )
        {
            if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
                continue;
            // Update timing information
            double slack = this->UpdatePathTiming( path );
            
            if( slack < minslack )
            {
                this->_mostcriticalpath = path;
                minslack = min( slack, minslack );
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
			double slack = this->UpdatePathTiming( path, false ) ;
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
	while( 1 )
	{
		bool endflag = 0;
		// Decrease the optimal Tc
		this->_tc = this->_besttc - (1 / pow(10, PRECISION));
		if( this->_tc < 0 )
			break;
		// Assess if the critical path occurs timing violation
		for(auto const& path: this->_pathlist)
		{
			if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
				continue;
			double slack = 0;
            /*
			if(this->_placedcc)
				slack = this->updatePathTimingWithDcc(path, 0);
			else
                slack = this->timingConstraint_nDcc_nVTA( path, 0, 0);//assessTimingWithoutDcc(path, 0 (genClause), 0 (update) );
                                            //The last 2 args are 0 => Only get slack instead of generating clauses and set timing
            */
            slack = this->UpdatePathTiming( path, false ) ;
			if( slack < 0 )
			{
				endflag = 1;
				break;
			}
		}
		if( endflag )
			break;
		this->_besttc = this->_tc;
	}
	if( oribesttc == this->_besttc )
		return;
	this->_tc = this->_besttc;
	// Update timing information of all critical path based on the new optimal Tc
	for(auto const& path: this->_pathlist)
	{
		if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
			continue;
        /*
		if( this->_placedcc )
			this->UpdatePathTiming( path, true ) ;
		else
            this->timingConstraint_nDcc_nVTA(path, 0, 1);//assessTimingWithoutDcc(path, 0, 1);
         */
        this->UpdatePathTiming( path, true ) ;
	}
}

/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Insert buffers based on the optimal Tc determined by DCC insertion
//
/////////////////////////////////////////////////////////////////////
/*
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
*/
/////////////////////////////////////////////////////////////////////
//
// ClockTree Class - Public Method
// Minimize the inserted buffers
//
/////////////////////////////////////////////////////////////////////
/*
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
*/
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
    
    return NULL ;
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
/*
void ClockTree::dumpToFile(void)
{
	this->dumpDccListToFile();
	this->dumpClockGatingListToFile();
	//this->dumpBufferInsertionToFile();
}
*/
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
                bufnum = 0; ffnum = 0;
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
	if( !this->_placedcc)   return;
	cout << "\t*** # of inserted DCCs             : " << this->_dcclist.size() << "\n";
	cout << "\t*** # of DCCs placed at last buffer: " << this->_dccatlastbufnum << "\n";
	cout << "\t*** Inserted DCCs List             : " ;
	if( !this->_placedcc || this->_dcclist.empty() )
		cout << "N/A\n";
	else
    {
        int  ctr = 0 ;
        //cout << "\t\t\t\t\t\t" ;
		for( auto const& node: this->_dcclist )
        {
			cout << node.first << "(" << node.second->getNodeNumber()<< "," << node.second->getDccType() << ((node.second != this->_dcclist.rbegin()->second) ? "), " : ")\n");
            ctr++ ;
            if( ctr %4 == 0 )
                cout << "\n\t\t\t\t\t\t" ;
        }
    }
	
    cout << "\n\t*** DCCs Placed at Last Buffer     : ";
	if( !this->_placedcc || (this->_dccatlastbufnum == 0) )
		cout << "N/A\n";
	else
	{
        int  ctr = 0 ;
        bool firstprint = true ;
        //cout << "\t\t\t\t\t\t" ;
        for(auto const& node: this->_dcclist)
        {
            if(node.second->isFinalBuffer())
            {
                if( firstprint ) firstprint = false ;
                else    cout << "), " ;
                cout << node.first << "(" << node.second->getNodeNumber()<< "," << node.second->getDccType();
         
                ctr++ ;
                if( ctr %4 == 0 )
                    cout << "\n\t\t\t\t\t\t" ;
             }
        }
        cout << ")\n";
	}
}
/*--------------------------------------------------------------------
 Func Name:
    printVTAList
 --------------------------------------------------------------------*/
void ClockTree::printVTAList()
{
    if( this->ifdoVTA() == false ) return ;
    cout << "\t*** # of inserted headers          : " << this->_VTAlist.size() << endl ;
    cout << "\t*** Inserted header List           : " ;
    if( this->ifdoVTA() == false || this->_VTAlist.size() == 0 )
        cout << "N/A\n";
    else
    {
        int  ctr = 0 ;
        //cout << "\t\t\t\t\t\t" ;
        for( auto const &header: this->_VTAlist )
        {
            cout << header.first << "(" <<  header.second->getNodeNumber() << "," <<header.second->getVTAType() << ")," ;
            ctr++ ;
            if( ctr %4 == 0 )
                cout << "\n\t\t\t\t\t\t" ;
        }
        cout << endl ;
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
/*-------------------------------------------------------------
 Func Name:
    getAgingRatee_givDC_givVth()
 Introduction:
    Calculate the aging rate of buffer
    by given duty cycle and given Vth offset.
 Note:
    The aging rate will differ from ones that gotten from seniors
 --------------------------------------------------------------*/
double ClockTree::getAgingRate_givDC_givVth( double DC, int Libindex )
{
    //---- Sv -------------------------------------------------------
    double Sv = 0 ;
    if( DC == -1 || DC == 0 ) DC = 0.5 ;
    if( this->_usingSeniorAging == true  )
        return (1 + (((-0.117083333333337) * (DC) * (DC)) + (0.248750000000004 * (DC)) + 0.0400333333333325));
    
    if( Libindex != -1 )
    {
        if( DC == 0.2 )
            Sv = this->getLibList().at(Libindex)->_Sv[4] ;
        else if( DC == 0.4 )
            Sv = this->getLibList().at(Libindex)->_Sv[5] ;
        else if( DC == 0.5 || DC == -1 || DC == 0 )
            Sv = this->getLibList().at(Libindex)->_Sv[6] ;
        else if( DC == 0.8 )
            Sv = this->getLibList().at(Libindex)->_Sv[7] ;
        else
        {
            cerr << "[Error] Irrecognized duty cycle in func \"getAgingRate_givDC_givVth (double DC, int LibIndex )\"    \n" ;
            return -1 ;
        }
    }
    else
    {
        if( DC == 0.2 )
            Sv = this->getLibList().at(0)->_Sv[0] ;
        else if( DC == 0.4 )
            Sv = this->getLibList().at(0)->_Sv[1] ;
        else if( DC == 0.5 || DC == -1 || DC == 0 )
            Sv = this->getLibList().at(0)->_Sv[2] ;
        else if( DC == 0.8 )
            Sv = this->getLibList().at(0)->_Sv[3] ;
        else
        {
            cerr << "[Error] Irrecognized duty cycle in func \"getAgingRate_givDC_givVth (double DC, int LibIndex )\"    \n" ;
            return -1 ;
        }
    }
    
    //---- Vth offset -----------------------------------------------
    double Vth_offset = 0 ;
    if( Libindex != -1 )
        Vth_offset = this->getLibList().at(Libindex)->_VTH_OFFSET + this->getBaseVthOffset() ;
    else
        Vth_offset = this->getBaseVthOffset() ;
    //---- Aging rate -----------------------------------------------
    double Vth_nbti = ( 1 - Sv*Vth_offset )*( 0.0039/2 )*( pow( DC*( 315360000 ), this->getExp() ) );
    
    if( Libindex == -1 )
        return (1 + Vth_nbti*2 + 0 ) ;
    else
        return (1 + Vth_nbti*2 + 2*this->getLibList().at(Libindex)->_VTH_OFFSET ) ;
    
}
/*-------------------------------------------------------------
 Func Name:
    removeCNFFile()
 --------------------------------------------------------------*/
void ClockTree::removeCNFFile()
{
    string cmd = "rm -rf *.out" ;
    system( cmd.c_str() ) ;
    cmd = "rm -rf *_output"      ;
    system( cmd.c_str() ) ;
}
/*-------------------------------------------------------------
 Func Name:
    printPath
 --------------------------------------------------------------*/
void ClockTree::printPath()
{
    while( true )
    {
        int mode = -1 ;
        printf("------------ " CYAN"Please input a number " RESET"---------------------\n");
        printf( "   number < 0 : Leave the loop\n" );
        printf( "   0 ~ %ld : ID of path which you wanna anaylyze\n", this->_pathlist.size()-1 );
        printf( "   Your input:" );
        cin >> mode ;
        if( mode < 0 ) break ;
        else if( mode >=  this->_pathlist.size() )
        {
            printf( RED"   [ERROR] " RESET"Index is out of range\n" );
            sleep( 2 );
            system( "clear" );
        }
        else
            printPath( mode );
     }
}
void ClockTree::printPath( int pathid )
{
    //-- Check ---------------------------------------------------------------
    if( pathid >= this->_pathlist.size() || pathid < 0 )
    {
        cerr << RED"[ERROR] Path id is out of range\n ";
        return ;
    }
    //-- Declaratio -----------------------------------------------------------
    system( "clear" );
    CriticalPath* path = this->_pathlist.at( pathid ) ;
    if( path->getPathType() == NONE )
    {
        cerr << RED"[Error]" RESET" The path is not include in the scope that you specified\n";
        cerr << RED"[Error]" RESET" Please check your input args '-path=only', '-path=all'... \n";
        return ;
    }
    
    printf("------------------ " CYAN"Print Path " RESET"--------------------------\n");
    printf("------ Following is the topology of the pipeline -------\n");
    printf( CYAN"[How] " RESET"How to read ?\n");
    printf("==> CLK_Node_Name( NodeID, Duty Cycle, VthType, " GREEN"Buffer Delay" RESET" ) \n");
    printf("==> Duty Cycle: 0.2, 0.4, 0.5, 0.8 \n" );
    printf("==> Vth type  : -1(Nominal), 0(VTA) \n" );
    printPath_givFile( path, false /*DCC/VTA*/, false /*Aging*/, false /*Tc from file*/ ) ;
    printPath_givFile( path, false /*DCC/VTA*/, true  /*Aging*/, true  /*Tc from file*/ ) ;
    printPath_givFile( path, true  /*DCC/VTA*/, true  /*Aging*/, true  /*Tc from file*/ ) ;
}

void ClockTree::printPath_givFile(CriticalPath *path, bool doDCCVTA, bool aging, bool TcFromFile )
{
    //-- Check ---------------------------------------------------------------
    if( path == NULL )
    {
        cerr << RED"[ERROR] Path id is out of range\n ";
        return ;
    }
    readDCCVTAFile() ;
    if( !TcFromFile ) this->_tc = this->_tcAfterAdjust ;
    //-- Cal & Print ClockTree -------------------------------------------------
    printf("--------------------------------------------------------\n");
    printf(CYAN"[Topology]\n" RESET);
    printAssociatedCriticalPathAtStartPoint( path, doDCCVTA, aging ) ;
    printAssociatedCriticalPathAtEndPoint( path, doDCCVTA, aging ) ;
    if( aging ) printf( "Aging   : " YELLOW"10 years" RESET" \n"  );
    else        printf( "Aging   : " YELLOW"Fresh   " RESET" \n"  );
    if( doDCCVTA )
        printf( "DCC/VTA : " YELLOW"DCC/VTA are considered"     RESET" \n"  );
    else
        printf( "DCC/VTA : " YELLOW"DCC/VTA are NOT considered" RESET" \n"  );
    printf( "Tc      : " YELLOW"Given by outer file" RESET" \n"  );
    this->printFFtoFF_givFile( path, doDCCVTA, aging );
    this->printPItoFF_givFile( path, doDCCVTA, aging );
    this->printFFtoPO_givFile( path, doDCCVTA, aging );
}

void ClockTree::printFFtoFF_givFile(CriticalPath *path, bool doDCCVTA, bool aging )
{
    //-- Check ----------------------------------------------------------------
    if( path == NULL || path->getPathType() != FFtoFF ) return ;
    
    //-- Assignment -----------------------------------------------------------
    vector<ClockTreeNode*> stClkPath, edClkPath ;
    if( path->getPathType() == FFtoFF || path->getPathType() == FFtoPO )
        stClkPath = path->getStartPonitClkPath() ;
    if( path->getPathType() == FFtoFF || path->getPathType() == PItoFF )
        edClkPath = path->getEndPonitClkPath() ;
    
    
    //-- Declaration ---------------------------------------------------------
    printf( "PathType: " YELLOW"FFtoFF\n" RESET );
    ClockTreeNode*  Parent      = path->findLastSameParentNode() ;
    ClockTreeNode*  clknode     = NULL ;
    long            common      = Parent->getDepth() ;
    bool            MeetDCC_Com = false;
    bool            MeetVTA_Com = false;
    double          DC_Com      = 0.5  ;
    int             LibIndex_Com= -1   ;
    double          minbuf_right= 9999 ;
    double          minbuf_left = 9999 ;
    int             DCCLib_R    = -1   ;
    int             DCCLib_L    = -1   ;
    bool            DCC_R       = false;//Whether meet dcc for right clk path
    bool            DCC_L       = false;
    double          DCC_R_Type  = 0.5  ;
    double          DCC_L_Type  = 0.5  ;
    double          agr         = 1.0  ;
    double          buftime      = 0    ;
    double          req_time     = 0    ;
    double          avl_time     = 0    ;
    GateData*       gatePtr      = NULL ;
    //-- Common Part (Only Cal) ---------------------------------------------
    for( long i = 0; i <= common; i++ )
    {
        clknode  = edClkPath.at(i) ;
        //-- First Meet DCC -------------------------------------------------
        if( !MeetDCC_Com && doDCCVTA && clknode->ifPlacedDcc() )
        {
            MeetDCC_Com = true ;
            DC_Com      = clknode->getDccType() ;
            DCC_R = DCC_L = 1 ;
            DCC_R_Type = DCC_L_Type = clknode->getDccType() ;
            //-- Clknode is both inserted DCC and header
            if( clknode->getIfPlaceHeader() )
                DCCLib_L = DCCLib_R = clknode->getVTAType() ;
            //-- DCC is ahead of header
            else if( !MeetVTA_Com )
                DCCLib_L = DCCLib_R = -1 ;
            //-- DCC is behind header
            else if( MeetVTA_Com )
                DCCLib_L = DCCLib_R = LibIndex_Com ;
        }
        //-- First Meet Header -------------------------------------------------
        if( !MeetVTA_Com && doDCCVTA && clknode->getIfPlaceHeader() ){
            MeetVTA_Com =  true ;
            LibIndex_Com=  clknode->getVTAType() ;
        }
            
        gatePtr         =  clknode->getGateData() ;
        buftime         =  ( clknode != edClkPath.back() )? (gatePtr->getGateTime()+gatePtr->getWireTime()) : (gatePtr->getWireTime()) ;
        if( clknode != this->_clktreeroot  ){
            minbuf_right=  min( buftime, minbuf_right );
            minbuf_left =  min( buftime, minbuf_left  );
        }
        agr             =  (aging)? ( getAgingRate_givDC_givVth( DC_Com, LibIndex_Com ) ):( 1 )  ;
        buftime         *= agr;//FF's Vth is not changed
        avl_time        += (buftime);
        req_time        += buftime ;
            
        //---- Set --------------------------------------------------------
        clknode->setBufTime(buftime);
        clknode->setDC(DC_Com);
        clknode->setVthType(LibIndex_Com);
    }
    
    //-- Right Branch (only cal)-------------------------------------------------------
    bool    MeetDCC_right  = MeetDCC_Com ;
    bool    MeetVTA_right  = MeetVTA_Com ;
    double  DC_right       = DC_Com      ;
    int     LibIndex_right = LibIndex_Com;
    for( long i = common +1; i < edClkPath.size(); i++ )
    {
        clknode  = edClkPath.at(i)        ;
        gatePtr  = clknode->getGateData() ;
        //-- First Meet DCC -------------------------------------------------
        if( !MeetDCC_right && doDCCVTA && clknode->ifPlacedDcc() )
        {
            MeetDCC_right = true ;
            DC_right      = clknode->getDccType() ;
            DCC_R         = 1 ;
            DCC_R_Type    = clknode->getDccType();
            //-- Clknode is both inserted DCC and header
            if( clknode->getIfPlaceHeader() )
                DCCLib_R = clknode->getVTAType() ;
            //-- DCC is ahead of header
            else if( !MeetVTA_right )
                DCCLib_R = -1 ;
            //-- DCC is behind header
            else if( MeetVTA_right )
                DCCLib_R = LibIndex_right ;
        }
        //-- First Meet Header -------------------------------------------------
        if( !MeetVTA_right && doDCCVTA && clknode->getIfPlaceHeader() ){
            MeetVTA_right = true ;
            LibIndex_right= clknode->getVTAType();
        }
        
        if( clknode != edClkPath.back() ){
            buftime      = gatePtr->getGateTime()+gatePtr->getWireTime() ;
            minbuf_right = min( minbuf_right, buftime ) ;
        }
        else{
            LibIndex_right = -1 ;
            buftime        = gatePtr->getWireTime() ;
        }
        agr          =  (aging)? ( getAgingRate_givDC_givVth( DC_right, LibIndex_right ) ):( 1 )  ;
        buftime     *= agr     ;
        req_time    += buftime ;
        //---- Set --------------------------------------------------------
        clknode->setBufTime(buftime);
        clknode->setDC(DC_right);
        clknode->setVthType(LibIndex_right);
    }

    //-- Right Part (Only Print) ---------------------------------------------
    if( MeetDCC_Com ) printf( "        "   );
    if( MeetVTA_Com ) printf( "          " );
    printSpace(common);
    for( long i = common +1; i < edClkPath.size(); i++ )
    {
        clknode  = edClkPath.at(i) ;
        gatePtr  = clknode->getGateData() ;
        printClkNodeFeature( clknode, doDCCVTA );
        if( clknode->ifMasked() )
            printf( "%s(%3ld,%.1f,%2d," GREEN"%f" RESET"," RED"X" RESET")---", gatePtr->getGateName().c_str() ,clknode->getNodeNumber(), clknode->getDC(), clknode->getVthType(),clknode->getBufTime() );
        else
            printf( "%s(%3ld,%.1f,%2d," GREEN"%f" RESET"," GRN"O" RESET")---", gatePtr->getGateName().c_str() ,clknode->getNodeNumber(), clknode->getDC(), clknode->getVthType(),clknode->getBufTime() );
    }
    printf( "  => " YELLOW"End " RESET"Clk Path\n");
    
    //-- Common Part (Only Print) ---------------------------------------------
    if( MeetDCC_Com ) printf( "        "   );
    if( MeetVTA_Com ) printf( "          " );
    printSpace(common);
    cout << "|\n" ;
    for( long i = 0; i <= common; i++ )
    {
        clknode  = edClkPath.at(i) ;
        gatePtr  = clknode->getGateData() ;
        printClkNodeFeature( clknode, doDCCVTA );
        
        if( clknode->ifMasked() )
            printf( "%s(%3ld,%.1f,%2d," GREEN"%f" RESET"," RED"X" RESET")---", gatePtr->getGateName().c_str() ,clknode->getNodeNumber(), clknode->getDC(), clknode->getVthType(),clknode->getBufTime() );
        else
            printf( "%s(%3ld,%.1f,%2d," GREEN"%f" RESET"," GRN"O" RESET")---", gatePtr->getGateName().c_str() ,clknode->getNodeNumber(), clknode->getDC(), clknode->getVthType(),clknode->getBufTime() );
    }
    cout << endl ;
    
    
    
    //-- Left Branch (Cal & print)------------------------------------------------
    if( MeetDCC_Com ) printf( "        "   );
    if( MeetVTA_Com ) printf( "          " );
    printSpace(common);
    cout << "|\n" ;
    if( MeetDCC_Com ) printf( "        "   );
    if( MeetVTA_Com ) printf( "          " );
    printSpace(common);

    bool    MeetDCC_left  = MeetDCC_Com ;
    bool    MeetVTA_left  = MeetVTA_Com ;
    double  DC_left       = DC_Com      ;
    int     LibIndex_left = LibIndex_Com;
    for( long i = common +1; i < stClkPath.size(); i++ )
    {
        clknode = stClkPath.at(i) ;
        gatePtr = clknode->getGateData() ;
        //-- First Meet DCC -------------------------------------------------
        if( !MeetDCC_left && doDCCVTA && clknode->ifPlacedDcc() )
        {
            MeetDCC_left  = true ;
            DC_left       = clknode->getDccType() ;
            DCC_L         = 1 ;
            DCC_L_Type    = clknode->getDccType();
            //-- Clknode is both inserted DCC and header
            if( clknode->getIfPlaceHeader() )
                DCCLib_L = clknode->getVTAType() ;
            //-- DCC is ahead of header
            else if( !MeetVTA_left )
                DCCLib_L = -1 ;
            //-- DCC is behind header
            else if( MeetVTA_left )
                DCCLib_L = LibIndex_left ;
        }
        //-- First Meet Header -------------------------------------------------
        if( !MeetVTA_left && doDCCVTA && clknode->getIfPlaceHeader() ){
            MeetVTA_left = true ;
            LibIndex_left= clknode->getVTAType() ;
        }

        if( clknode != stClkPath.back() ){
            buftime = gatePtr->getGateTime()+gatePtr->getWireTime();
            minbuf_left = min( buftime, minbuf_left  );
        }
        else{
            buftime = gatePtr->getWireTime() ;
            LibIndex_left = -1;
        }
       
        agr         =  (aging)? ( getAgingRate_givDC_givVth( DC_left, LibIndex_left ) ):( 1 )  ;
        buftime     *= agr     ;
        avl_time   += buftime ;
            
        if( clknode == stClkPath.back() ) LibIndex_left = -1 ;
        printClkNodeFeature( clknode, doDCCVTA );
        
        if( clknode->ifMasked() )
            printf( "%s(%3ld,%.1f,%d," GREEN"%.4f" RESET", " RED  "X" RESET")--", gatePtr->getGateName().c_str() , clknode->getNodeNumber(), DC_left, LibIndex_left, buftime );
        else
            printf( "%s(%3ld,%.1f,%d," GREEN"%.4f" RESET", " GREEN"O" RESET")--", gatePtr->getGateName().c_str() , clknode->getNodeNumber(), DC_left, LibIndex_left, buftime );
    }
    printf( "  => " YELLOW"Start " RESET"Clk Path\n");
        
    //-- A DCC Exist along rifht clk path ----------------------------------------------------
    if( DCC_R && doDCCVTA )
    {
        double dcc_agr = (aging)? getAgingRate_givDC_givVth( 0.5, DCCLib_R ):(1) ;
        double dcc_delay = 0 ;
        if( DCC_R_Type == 0.2 )
            dcc_delay= minbuf_right * dcc_agr * DCCDELAY20PA ;
        else if( DCC_R_Type == 0.4 )
            dcc_delay= minbuf_right * dcc_agr * DCCDELAY40PA ;
        else if( DCC_R_Type == 0.5 || DCC_R_Type == 0 || DCC_R_Type == -1 )
            dcc_delay= minbuf_right * dcc_agr * DCCDELAY50PA ;
        else if( DCC_R_Type == 0.8 )
            dcc_delay= minbuf_right * dcc_agr * DCCDELAY80PA ;
        
        req_time += dcc_delay ;
        printf("DCC(Ed.): %.1f, %f (Fresh), %f (Aging) \n", DCC_R_Type, minbuf_right, dcc_delay );
    }
    //-- A DCC Exist along left clk path -----------------------------------------------------
    if( DCC_L && doDCCVTA  )
    {
        double dcc_agr = (aging)? getAgingRate_givDC_givVth( 0.5, DCCLib_L ):(1) ;
        double dcc_delay = 0 ;
        if( DCC_L_Type == 0.2 )
            dcc_delay = minbuf_left * dcc_agr * DCCDELAY20PA ;
        else if( DCC_L_Type == 0.4 )
            dcc_delay= minbuf_left * dcc_agr * DCCDELAY40PA ;
        else if( DCC_L_Type == 0.5 || DCC_L_Type == 0 || DCC_L_Type == -1 )
            dcc_delay= minbuf_left * dcc_agr * DCCDELAY50PA ;
        else if( DCC_L_Type == 0.8 )
            dcc_delay= minbuf_left * dcc_agr * DCCDELAY80PA ;
        avl_time += dcc_delay ;
        printf("DCC(St.): %.1f, %f (Fresh), %f (Aging) \n", DCC_L_Type, minbuf_left, dcc_delay );
    }
    //-- Print Timing -------------------------------------------------
    //cout << minbuf_left << " " << minbuf_right << endl ;
    this->printPathSlackTiming(path, avl_time, req_time, aging );
}
void ClockTree::printPItoFF_givFile(CriticalPath *path, bool doDCCVTA, bool aging )
{
    //-- Check ---------------------------------------------------------------
    if( path == NULL || path->getPathType() != PItoFF ) return ;
    
    //-- Declaratio -----------------------------------------------------------
    vector<ClockTreeNode*> edClkPath = path->getEndPonitClkPath()   ;
    double          buftime      = 0    ;
    double          req_time     = 0    ;
    double          avl_time     = 0    ;
    GateData*       gatePtr      = NULL ;
    
    //-- Preprocess -----------------------------------------------------------
    printf("PathType: " YELLOW"PItoFF\n" RESET );
    bool    MeetDCC_right  = false  ;
    bool    MeetVTA_right  = false  ;
    double  DC_right       = 0.5    ;
    double  minbuf_right   = 9999   ;
    int     LibIndex_right = -1     ;
    double  DCCLib_R       = -1     ;
    bool    DCC_R          = false  ;
    double  DCC_R_Type     = 0.5    ;
    double  agr            = 1.0    ;
    for( const auto clknode:edClkPath )
    {
        gatePtr     = clknode->getGateData() ;
        //-- First Meet DCC -------------------------------------------------
        if( !MeetDCC_right && doDCCVTA && clknode->ifPlacedDcc() )
        {
            MeetDCC_right = true ;
            DC_right      = clknode->getDccType() ;
            DCC_R         = 1 ;
            DCC_R_Type    = clknode->getDccType();
            //-- Clknode is both inserted DCC and header
            if( clknode->getIfPlaceHeader() )
                DCCLib_R = clknode->getVTAType() ;
            //-- DCC is ahead of header
            else if( !MeetVTA_right )
                DCCLib_R = -1 ;
            //-- DCC is behind header
            else if( MeetVTA_right )
                DCCLib_R = LibIndex_right ;
        }
        //-- First Meet Header -------------------------------------------------
        if( !MeetVTA_right && doDCCVTA && clknode->getIfPlaceHeader() ){
            MeetVTA_right = true ;
            LibIndex_right= clknode->getVTAType() ;
        }
        
        if( clknode != edClkPath.back() ){
            buftime = gatePtr->getGateTime()+gatePtr->getWireTime() ;
            if( clknode != this->_clktreeroot ) minbuf_right = min( minbuf_right, buftime );
        }
        else{
            buftime = gatePtr->getWireTime() ;
            LibIndex_right = -1 ;
        }
    
        agr         = (aging)? (getAgingRate_givDC_givVth( DC_right, LibIndex_right )):(1) ;
        buftime    *= agr     ;
        req_time   += buftime ;
        
        printClkNodeFeature( clknode, doDCCVTA );
        if( clknode->ifMasked() )
            printf( "%s(%3ld,%.1f,%d," GREEN"%.4f" RESET"," RED"X" RESET")--", gatePtr->getGateName().c_str() , clknode->getNodeNumber(), DC_right, LibIndex_right, buftime );
        else
            printf( "%s(%3ld,%.1f,%d," GREEN"%.4f" RESET"," GRN"O" RESET")--", gatePtr->getGateName().c_str() , clknode->getNodeNumber(), DC_right, LibIndex_right, buftime );
    }
    printf( "=> " YELLOW"End " RESET"Clk Path\n");
        
    //-- A DCC Exist along rifht clk path ----------------------------------------------------
    if( DCC_R && doDCCVTA )
    {
        double dcc_agr = (aging)? getAgingRate_givDC_givVth( 0.5, DCCLib_R ):(1) ;
        if( DCC_R_Type == 0.2 )
            req_time += minbuf_right * dcc_agr * DCCDELAY20PA ;
        else if( DCC_R_Type == 0.4 )
            req_time += minbuf_right * dcc_agr * DCCDELAY40PA ;
        else if( DCC_R_Type == 0.5 || DCC_R_Type == 0 || DCC_R_Type == -1 )
            req_time += minbuf_right * dcc_agr * DCCDELAY50PA ;
        else if( DCC_R_Type == 0.8 )
            req_time += minbuf_right * dcc_agr * DCCDELAY80PA ;
    }
    //-- Print Timing -------------------------------------------------
    this->printPathSlackTiming(path, avl_time, req_time, aging );
}

void ClockTree::printFFtoPO_givFile( CriticalPath *path, bool doDCCVTA, bool aging )
{
    //-- Check ---------------------------------------------------------------
    if( path == NULL || path->getPathType() != FFtoPO ) return ;
    
    //-- Declaratio -----------------------------------------------------------
    vector<ClockTreeNode*> stClkPath = path->getStartPonitClkPath() ;
    double          buftime      = 0    ;
    double          req_time     = 0    ;
    double          avl_time     = 0    ;
    GateData*       gatePtr      = NULL ;
    
     //-- Preprocess -----------------------------------------------------------
    printf("PathType: " YELLOW"FFtoPO\n" RESET );
    bool    MeetDCC_left   = false  ;
    bool    MeetVTA_left   = false  ;
    double  DC_left        = 0.5    ;
    int     LibIndex_left  = -1     ;
    double  minbuf_left    = 9999   ;
    double  DCCLib_L       = -1     ;
    bool    DCC_L          = false  ;
    double  DCC_L_Type     = 0.5    ;
    double  agr            = 1.0    ;
    for( const auto clknode:stClkPath )
    {
        gatePtr = clknode->getGateData() ;
        //-- First Meet DCC -------------------------------------------------
        if( !MeetDCC_left && doDCCVTA && clknode->ifPlacedDcc()  )
        {
            MeetDCC_left  = true ;
            DC_left       = clknode->getDccType() ;
            DCC_L         = 1 ;
            DCC_L_Type    = clknode->getDccType();
            //-- Clknode is both inserted DCC and header
            if( clknode->getIfPlaceHeader() )
                DCCLib_L = clknode->getVTAType() ;
            //-- DCC is ahead of header
            else if( !MeetVTA_left )
                DCCLib_L = -1 ;
            //-- DCC is behind header
            else if( MeetVTA_left )
                DCCLib_L = LibIndex_left ;
        }
        //-- First Meet Header -------------------------------------------------
        if( !MeetVTA_left && doDCCVTA && clknode->getIfPlaceHeader() ){
            MeetVTA_left = true ;
            LibIndex_left= clknode->getVTAType() ;
        }
        
        if( clknode != stClkPath.back() ){
            buftime = gatePtr->getGateTime()+gatePtr->getWireTime() ;
            if( clknode != this->_clktreeroot ) minbuf_left = min( buftime, minbuf_left  );
        }
        else{
            buftime = gatePtr->getWireTime() ;
            LibIndex_left = -1 ;
        }
        
        agr         = (aging)? (getAgingRate_givDC_givVth( DC_left, LibIndex_left )):(1) ;
        buftime    *= agr ;
        avl_time   += buftime ;
        
        printClkNodeFeature( clknode, doDCCVTA );
        if( clknode->ifMasked() )
            printf( "%s(%3ld,%.1f,%d," GREEN"%.4f" RESET"," RED"X" RESET")--", gatePtr->getGateName().c_str() , clknode->getNodeNumber(), DC_left, LibIndex_left, buftime );
        else
            printf( "%s(%3ld,%.1f,%d," GREEN"%.4f" RESET"," GRN"O" RESET")--", gatePtr->getGateName().c_str() , clknode->getNodeNumber(), DC_left, LibIndex_left, buftime );
    }
    printf( "=> " YELLOW"Start " RESET"Clk Path\n");
    //-- A DCC Exist along left clk path -----------------------------------------------------
    if( DCC_L && doDCCVTA  )
    {
        double dcc_agr = (aging)? getAgingRate_givDC_givVth( 0.5, DCCLib_L ):(1) ;
        if( DCC_L_Type == 0.2 )
            avl_time += minbuf_left * dcc_agr * DCCDELAY20PA ;
        else if( DCC_L_Type == 0.4 )
            avl_time += minbuf_left * dcc_agr * DCCDELAY40PA ;
        else if( DCC_L_Type == 0.5 || DCC_L_Type == 0 || DCC_L_Type == -1 )
            avl_time += minbuf_left * dcc_agr * DCCDELAY50PA ;
        else if( DCC_L_Type == 0.8 )
            avl_time += minbuf_left * dcc_agr * DCCDELAY80PA ;
    }
    //-- Print Timing -------------------------------------------------
    this->printPathSlackTiming(path, avl_time, req_time, aging );
}
void ClockTree::printPathSlackTiming(CriticalPath *path, double ci, double cj, bool aging )
{
    double avl_time = ci ;
    double req_time = cj ;
    double Tsu = (aging)? (path->getTsu() * this->_agingtsu) : (path->getTsu());
    double Tcq = (aging)? (path->getTcq() * this->_agingtcq) : (path->getTcq());
    double Dij = (aging)? (path->getDij() * this->_agingdij) : (path->getDij());
    req_time += Tsu + this->_tc ;
    avl_time += path->getTinDelay() + Tcq + Dij ;
    double slack = req_time - avl_time  ;
    
    printf( "\n");
    if( slack > 0 ) printf( RESET"slack  = " RESET"%f " RESET"(ns) = Req - Avl = (Cj+Tsu+Tc) - (Ci+Tin+Tcq+Dij)\n", slack );
    else            printf( RESET"slack  = " RED  "%f " RESET"(ns) = Req - Avl = (Cj+Tsu+Tc) - (Ci+Tin+Tcq+Dij)\n", slack );
    printf( RESET"Tc     = %f (ns) [ Period     /Req ]\n", this->_tc );
    printf( RESET"Ci     = %f (ns) [ Src to stFF/Avl ]\n", ci );
    printf( RESET"Cj     = %f (ns) [ Src to edFF/Req ]\n", cj );
    printf( RESET"Cj-Ci  = %f (ns) [ Diff.           ]\n", cj-ci );
    printf( RESET"Tsu    = %f (ns)[ Setup Time /Req ]\n", Tsu );
    printf( RESET"Tcq    = %f (ns) [ Clk to Q   /Avl ]\n", Tcq );
    printf( RESET"Dij    = %f (ns) [ Comb. path /Avl ]\n", Dij );
    printf( RESET"Tin    = %f (ns) [ Input delay/Avl ]\n", path->getTinDelay());
}
void ClockTree::printSpace( long common)
{
    for( int i = 0 ; i <= common; i++ )  printf("                                 " );
}
void ClockTree::dumpDccVTALeaderToFile()
{
    FILE *fPtr;
    string filename = this->_outputdir + "DccVTA_" + to_string( this->_tc ) + ".txt";
    fPtr = fopen( filename.c_str(), "w" );
    if( !fPtr )
    {
        cerr << RED"[Error]" RESET "Cannot open the DCC VTA file\n" ;
        return ;
    }
    fprintf( fPtr, "Tc %f\n", this->_tc );
    for( auto node: this->_buflist )
    {
        if( node.second->ifPlacedDcc() || node.second->getIfPlaceHeader() )
            fprintf( fPtr, "%ld %d %f\n", node.second->getNodeNumber(), node.second->getVTAType(), node.second->getDccType() );
    }
    fclose(fPtr);
    
}
void ClockTree::printAssociatedCriticalPathAtEndPoint( CriticalPath* path, bool doDCCVTA, bool aging )
{
    if( path->getPathType() == NONE   ){
        printf( RED"[Error] " RESET"The path is not included in the scope that you specified\n");
        printf( RED"[Error] " RESET"Please check args i.g., -path=onlyff, -path=pi_ff ... etc \n");
        return ;
    }
    else if( path->getPathType() == PItoPO ){
        printf("The path is " YELLOW"PItoPO" RESET", No Next Pipeline exists\n" );
        return ;
    }else if( path->getPathType() == FFtoPO ){
        printf("Next pipeline: N/A\n");
        return ;
    }
    
    vector<ClockTreeNode*> edClkPath = path->getEndPonitClkPath() ;
    printf( "Next pipeline: " );
    int ctr = 0 ;
    for( auto p: this->_pathlist )
    {
        if( p == path || p->getPathType() == PItoFF || p->getPathType() == NONE || p->getPathType() == PItoPO) continue ;
        
        if( p->getStartPonitClkPath().back() == edClkPath.back() ){
            double slack = UpdatePathTiming( p, false, doDCCVTA, aging ) ;
            if( slack > 0 )     printf( "%ld (slack = %f ), ", p->getPathNum(), slack );
            if( slack < 0 )     printf( "%ld (slack = " RED"%f" RESET" ), ", p->getPathNum(), slack );
            ctr++ ;
            if( ctr % 6 == 0 )  printf( "\n               " );
        }
    }
    cout << endl ;
}
void ClockTree::printAssociatedCriticalPathAtStartPoint( CriticalPath* path, bool doDCCVTA, bool aging )
{
    if( path->getPathType() == NONE   ){
        printf( RED"[Error] " RESET"The path is not included in the scope that you specified\n");
        printf( RED"[Error] " RESET"Please check args i.g., -path=onlyff, -path=pi_ff ... etc \n");
        return ;
    }
    else if( path->getPathType() == PItoPO ){
        printf( "The path is " YELLOW"PItoPO" RESET", No Last/Next Pipeline exists\n" );
        return ;
    }else if( path->getPathType() == PItoFF ){
        printf( "Last pipeline: N/A\n" );
        return ;
    }
    
    vector<ClockTreeNode*> stClkPath = path->getStartPonitClkPath() ;
    printf( "Last pipeline: " );
    int ctr = 0 ;
    for( auto p: this->_pathlist )
    {
        if( p == path || p->getPathType() == FFtoPO || p->getPathType() == NONE || p->getPathType() == PItoPO ) continue ;
        
        if( p->getEndPonitClkPath().back() == stClkPath.back() ){
            double slack = UpdatePathTiming( p, false, doDCCVTA, aging ) ;
            if( slack > 0 )     printf( "%ld (slack = %f ), ", p->getPathNum(), slack );
            if( slack < 0 )     printf( "%ld (slack = " RED"%f" RESET" ), ", p->getPathNum(), slack );
            ctr++ ;
            if( ctr % 6 == 0 )  printf( "\n               " );
        }
    }
    cout << endl ;
}
void ClockTree::printClkNodeFeature( ClockTreeNode *clknode, bool doDCCVTA )
{
    if( !doDCCVTA ) return ;
    if( clknode->ifPlacedDcc())
        printf("-" YELLOW"%.1f DCC"  RESET"-",clknode->getDccType()) ;
    if( clknode->getIfPlaceHeader() )
        printf("-" YELLOW"%d Leader" RESET"-",clknode->getVTAType()) ;
}

void ClockTree::InitClkTree()
{
    for( auto clknode: this->_buflist )
    {
        clknode.second->setIfPlaceDcc(false)    ;
        clknode.second->setIfPlaceHeader(false) ;
        clknode.second->setDccType(0.5)         ;
        clknode.second->setVTAType(-1)          ;
        //clknode.second->setifMasked(true)       ;
    }
    for( auto FF: this->_ffsink )
    {
        FF.second->setIfPlaceDcc(false)         ;
        FF.second->setIfPlaceHeader(false)      ;
        FF.second->setDccType(0.5)              ;
        FF.second->setVTAType(-1)               ;
        //FF.second->setifMasked(true)            ;
    }
    //this->dccPlacementByMasked(1)               ;
}
void ClockTree::dumpCNF()
{
    printf("----------------- " CYAN"Dump CNF File Name " RESET"--------------------\n");
    printf(CYAN"[Info] " RESET"Please Type the Unsat CNF file name\n");
    string  filename = "" ;
    cin     >> filename   ;
    string  line     = "" ;
    fstream cnffile       ;
    FILE    *fPtr         ;
    string  satFile= "DccVTA.txt";
    fPtr    = fopen( satFile.c_str(), "w" );
    if( !fPtr ){
        cerr << RED"[Error]" RESET "Cannot open the DCC VTA file\n" ;
        return ;
    }
    cnffile.open( filename, ios::in );
    if( !cnffile.is_open() ){
        cerr << "\033[31m[Error]: Cannot open " << filename << "\033[0m\n";
        cnffile.close();
        return;
    }
    printf(CYAN"[Info] " RESET"Please Type the Tc of Unsat CNF file \n");
    cin >> this->_tc  ;
    
    getline( cnffile, line ) ;//Read SAT/UNSAT
    getline( cnffile, line ) ;//Read DCC/VTA
    vector<string> strspl = stringSplit( line, " " );
    //------ Clk Node Iteration ---------------------------------------------------
    for( long loop = 0; ; loop += 3 /*2*/ )
    {
        //-- End of CNF -----------------------------------------------------------
        if( strspl[loop] == ""  ){
            printf("Break space\n");
            break ;
        }
        if( stoi( strspl[loop] ) == 0 ){
            printf("Break 0\n");
            break ;
        }
        //-- No DCC/VTA Clknode ----------------------------------------------------
        if( !( stoi(strspl.at(loop+2)) > 0 || stoi(strspl.at(loop)) > 0 || stoi(strspl.at(loop + 1)) > 0 ) ) continue ;
        else
        {
            ClockTreeNode *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
            if( !findnode ) cerr << RED"[Error] " RESET"Unrecognized Clknode ID in dumpUnsatCNF()\n";
            fprintf( fPtr, "%ld ",  findnode->getNodeNumber() );
            //-- Put Header ------------------------------------------------------------
            if( stoi(strspl.at(loop+2)) > 0 )   fprintf( fPtr, "%d ", 0 );
            
            //-- Put DCC --------------------------------------------------------------
            if( stoi(strspl.at(loop)) > 0 || stoi(strspl.at(loop + 1)) > 0  )
            {
                int bit1 = stoi(strspl.at(loop)), bit2 = stoi(strspl.at(loop + 1)) ;
                if((bit1 > 0) && (bit2 > 0))
                    fprintf( fPtr, "%f\n", 0.8 );
                else if((bit1 > 0) && (bit2 < 0))
                    fprintf( fPtr, "%f\n", 0.4 );
                else if((bit1 < 0) && (bit2 > 0))
                    fprintf( fPtr, "%f\n", 0.2 );
                else if((bit1 < 0) && (bit2 < 0))
                    fprintf( fPtr, "%f\n", 0.5 );
                else
                    fprintf( fPtr, "%f\n", 0.5 );
            }
        }
    }//Clk node iteration
    fclose(fPtr);
    cnffile.close() ;
    for( auto const& path: this->_pathlist )
    {
        if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))   continue;
        
        double slack = this->UpdatePathTiming( path, false );
        if( slack < 0 ) printf("%ld slack (%f) < 0 \n", path->getPathNum(), slack );
    }
}

void ClockTree::CheckTiming_givFile()
{
    printf("----------------- " CYAN"Check Constraint " RESET"--------------------\n");
    //-- Read Tc/DCC/Leader Info ----------------------------------------------
    readDCCVTAFile() ;
    bool fail = false ;
    
    for( auto const& path: this->_pathlist )
    {
        if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))   continue;
        
        double slack = this->UpdatePathTiming( path, false );
        if( slack < 0 )
        {
            if( path->getPathType() == PItoPO )
                printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, PItoPO ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
            else if( path->getPathType() == FFtoFF )
                printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, FFtoFF ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
            else if( path->getPathType() == FFtoPO )
                printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, FFtoPO ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
            else if( path->getPathType() == PItoFF )
                printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, PItoFF ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
            fail = true ;
        }
    }
    if( !fail )
        printf( CYAN"[ Timing Constraint ] " RESET"All paths pass!\n");
    if( this->checkDCCVTAConstraint() == true )
        printf( CYAN"[ DCC/VTA Constraint] " RESET"All paths pass!\n");
    
    int pathid = 0 ;
    while( true )
    {
        readDCCVTAFile();
        printf("----------- " CYAN"Check Timing of specific path " RESET"-----------------\n");
        printf("    number < 0 : Leave the program\n");
        printf("    0 ~ %ld : Path ID\n", this->_pathlist.size() );
        printf("    Your Input is: " );
        cin >> pathid ;
        if( pathid < 0 ) break;
        else if( pathid >= _pathlist.size() ){
            printf("    " RED"[Error]" RESET"Your input is out of range\n" );
            continue;
        }
        
        
        for( auto const& path: this->_pathlist )
        {
            if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))   continue;
            if( path->getPathNum() != pathid ) continue ;
            double slack = this->UpdatePathTiming( path, false );
            
            if( slack > 0 ){
                if( path->getPathType() == PItoPO )
                    printf( CYAN"[Timing Constraint] " GREEN"[ Passed ] " RESET"Path( %ld, PItoPO ) pass, slack = %f\n", path->getPathNum() ,slack );
                else if( path->getPathType() == FFtoFF )
                    printf( CYAN"[Timing Constraint] " GREEN"[ Passed ] " RESET"Path( %ld, FFtoFF ) pass, slack = %f\n", path->getPathNum() ,slack );
                else if( path->getPathType() == FFtoPO )
                    printf( CYAN"[Timing Constraint] " GREEN"[ Passed ] " RESET"Path( %ld, FFtoPO ) pass, slack = %f\n", path->getPathNum() ,slack );
                else if( path->getPathType() == PItoFF )
                    printf( CYAN"[Timing Constraint] " GREEN"[ Passed ] " RESET"Path( %ld, PItoFF ) pass, slack = %f\n", path->getPathNum() ,slack );
            }
            else{
                if( path->getPathType() == PItoPO )
                    printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, PItoPO ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
                else if( path->getPathType() == FFtoFF )
                    printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, FFtoFF ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
                else if( path->getPathType() == FFtoPO )
                    printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, FFtoPO ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
                else if( path->getPathType() == PItoFF )
                    printf( CYAN"[Timing Constraint] " RED"[Violated] " RESET"Path( %ld, PItoFF ) fail, slack = " RED"%f\n" RESET, path->getPathNum() ,slack );
            }
            this->checkDCCVTAConstraint() ;
        }//for
    }//while
}
void ClockTree::readDCCVTAFile()
{
    InitClkTree()                  ;//Init ifPlacedDCC();
    //-- Read Tc/DCC/Leader Info ----------------------------------------------
    ifstream        file  ;
    string          line  ;
    file.open( "setting/DccVTA.txt", ios::in ) ;
    if( !file ) return              ;
    getline( file, line )           ;
    string          tc              ;
    istringstream   token( line )   ;
    token     >>  tc   >> this->_tc ;
    
    while( getline( file, line ) )
    {
        long    BufID       = 0   ;
        int     BufVthLib   = -1  ;
        double  BufDCC      = 0.5 ;
        istringstream   token( line )     ;
        token >> BufID >> BufVthLib >> BufDCC ;
        ClockTreeNode *buffer = searchClockTreeNode( BufID ) ;
        if( buffer == NULL )
        {
            printf( RED"[Error] " RESET"Can't find clock node with id = %ld\n", BufID ) ;
            return ;
        }
        if( BufDCC != 0.5 && BufDCC != -1 && BufDCC != 0 ){
            buffer->setIfPlaceDcc(true);
            buffer->setDccType( BufDCC )    ;
        }
        if( BufVthLib != -1  ){
            buffer->setIfPlaceHeader(true);
            buffer->setVTAType( BufVthLib ) ;
        }
    }
}

bool ClockTree::checkDCCVTAConstraint()
{
    readDCCVTAFile()             ;//Set   DCC/VTA location
    bool result = true           ;
    for( auto path: this->_pathlist )
    {
        if( path->getPathType() == NONE || path->getPathType() == PItoPO ) continue ;
        else
            if( this->checkDCCVTAConstraint_givPath( path ) == false )
            {
                //printf( CYAN"[VTA/DCC Constraint] " RED"[Violated] " RESET"Path( %ld ), please check it\n", path->getPathNum() );
                result = false ;
            }
    }
    //------ Do not put DCC ahead of Masked clk node/FF ---------------------------
    for( auto clknode: this->_buflist )
    {
        ClockTreeNode*node = clknode.second ;
        if( node->ifPlacedDcc() == true && node->ifMasked() == true )
        {
            result = false ;
            printf( CYAN"[ DCC Constraint  ] " RED"[Violated] " RESET"Clknode( %ld, %s) is masked, it should not be inserted DCC\n", node->getNodeNumber(), node->getGateData()->getGateName().c_str());
        }
    }
    for( auto clknode: this->_ffsink )
    {
        ClockTreeNode*node = clknode.second ;
        if( node->ifPlacedDcc() == true || node->getIfPlaceHeader() == true )
        {
            result = false ;
            printf( CYAN"[DCC/VTA Constraint] " RED"[Violated] " RESET"FF( %ld, %s) is masked, it should not be inserted DCC/VTA\n", node->getNodeNumber(), node->getGateData()->getGateName().c_str());
        }
    }
    return result ;
}


bool ClockTree::checkDCCVTAConstraint_givPath( CriticalPath * path )
{
    bool correct = true ;
    vector<ClockTreeNode*> stClkPath, edClkPath ;
    if( path->getPathType() == FFtoFF || path->getPathType() == FFtoPO )
        stClkPath = path->getStartPonitClkPath() ;
    if( path->getPathType() == FFtoFF || path->getPathType() == PItoFF )
        edClkPath = path->getEndPonitClkPath() ;
    
    //------ One Clk Path, One DCC/Leader ---------------------------
    int DCC_Ctr_st = 0, VTA_Ctr_st = 0, DCC_Ctr_ed = 0, VTA_Ctr_ed = 0 ;
    for( auto clknode: stClkPath ){
        if( clknode->ifPlacedDcc() )      DCC_Ctr_st++ ;
        if( clknode->getIfPlaceHeader() ) VTA_Ctr_st++ ;
    }
    for( auto clknode: edClkPath ){
        if( clknode->ifPlacedDcc() )      DCC_Ctr_ed++ ;
        if( clknode->getIfPlaceHeader() ) VTA_Ctr_ed++ ;
    }
    
    if( DCC_Ctr_st >= 2 ){
        correct = false ;
        printf( CYAN"[ DCC Constraint  ] " RED"[Violated] " RESET"Path( %ld) has more than 2 DCCs along clk path of start side\n", path->getPathNum() );
    }
    if( DCC_Ctr_ed >= 2 ){
        correct = false ;
        printf( CYAN"[ DCC Constraint  ] " RED"[Violated] " RESET"Path( %ld) has more than 2 DCCs along clk path of end   side\n", path->getPathNum() );
    }
    if( VTA_Ctr_st >= 2 ){
        correct = false ;
        printf( CYAN"[ VTA Constraint  ] " RED"[Violated] " RESET"Path( %ld) has more than 2 VTA leaders along clk path of start side\n", path->getPathNum() );
    }
    if( VTA_Ctr_ed >= 2 ){
        correct = false ;
        printf( CYAN"[ VTA Constraint  ] " RED"[Violated] " RESET"Path( %ld) has more than 2 VTA leaders along clk path of end   side\n", path->getPathNum() );
    }
    
    
    return correct ;
}





