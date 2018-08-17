//////////////////////////////////////////////////////////////
//
// Source File
//
// File name: criticalpath.cpp
// Author: Ting-Wei Chang
// Date: 2017-07
//
//////////////////////////////////////////////////////////////

#include "criticalpath.h"

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"
/////////////////////////////////////////////////////////////////////
//
// ClockTreeNode Class - Public Method
// Set which type of DCC inserted before buffer
// 00 => None
// 01 => 20% DCC
// 10 => 40% DCC
// 11 => 80% DCC
//
/////////////////////////////////////////////////////////////////////
ClockTreeNode & ClockTreeNode::setDccType(int Lowbit, int Highbit)
{
    if( Highbit > 0 && Lowbit > 0 )//11
        this->_dcctype = 0.8 ;
    else if( Highbit < 0 && Lowbit > 0)//01
        this->_dcctype = 0.2 ;
    else if( Highbit > 0 && Lowbit < 0)//10
        this->_dcctype = 0.4 ;
    else if( Highbit < 0 && Lowbit < 0)//00
        this->_dcctype = 0.5 ;
        /*
	if((bit1 > 0) && (bit2 > 0))
		this->_dcctype = 0.8;
	else if((bit1 > 0) && (bit2 < 0))
		this->_dcctype = 0.4;
	else if((bit1 < 0) && (bit2 > 0))
		this->_dcctype = 0.2;
	else if((bit1 < 0) && (bit2 < 0))
		this->_dcctype = 0.5;
	else
		this->_dcctype = 0.5;
         */
    return *this;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTreeNode Class - Public Method
// Judge if all children of this buffer are FF
//
/////////////////////////////////////////////////////////////////////
bool ClockTreeNode::isFinalBuffer(void)
{
	for(auto const &nodeptr : this->getChildren())
		if(!nodeptr->isFFSink())
			return false;
	return true;
}

/////////////////////////////////////////////////////////////////////
//
// ClockTreeNode Class - Public Method
// Search if this buffer has child with the specific name
// Input parameter:
// nodename: specific buffer/FF name
//
/////////////////////////////////////////////////////////////////////
ClockTreeNode *ClockTreeNode::searchChildren(string nodename)
{
	ClockTreeNode *findnode = nullptr;
	if(this->_children.size() == 0)
		return findnode;
	
	for(int loop = 0;loop < this->_children.size();loop++)
	{
		if(nodename.compare(this->_children.at(loop)->getGateData()->getGateName()) == 0)
		{
			findnode = this->_children.at(loop);
			break;
		}
	}
	return findnode;
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Private Method
// Add the location of DCC placement to the candidate list
// Input parameter:
// expand: 0 => store the location
//         1 => resize the container for new candidate (combination)
//
/////////////////////////////////////////////////////////////////////
void CriticalPath::addDccPlacementCandidate(ClockTreeNode *node, bool expand)
{
	if(expand)
		this->_dccplacementcandi.resize(this->_dccplacementcandi.size()+1);
	this->_dccplacementcandi.back().resize(this->_dccplacementcandi.back().size()+1);
	this->_dccplacementcandi.back().back() = node;
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Destructor
//
/////////////////////////////////////////////////////////////////////
CriticalPath::~CriticalPath(void)
{
	for(long loop = 0;loop < this->_gatelist.size();loop++)
		delete this->_gatelist.at(loop);
	this->_gatelist.clear();
	this->_gatelist.shrink_to_fit();
	this->_startpclkpath.clear();
	this->_startpclkpath.shrink_to_fit();
	this->_endpclkpath.clear();
	this->_endpclkpath.shrink_to_fit();
	this->_dccplacementcandi.clear();
	this->_dccplacementcandi.shrink_to_fit();
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Store all DCC deployment of this critical path to the candidate
// list
//
/////////////////////////////////////////////////////////////////////
void CriticalPath::setDccPlacementCandidate(void)
{
	switch( this->_pathtype )
	{
		// Path type: input to FF
		case PItoFF:
			for( auto const &nodeptr : this->_endpclkpath )
				if( nodeptr->ifMasked() == false )
					addDccPlacementCandidate(nodeptr, 1);
			break;
		// Path type: FF to output
		case FFtoPO:
			for( auto const &nodeptr : this->_startpclkpath )
				if( nodeptr->ifMasked() == false  )
					addDccPlacementCandidate(nodeptr, 1);
			break;
		// Path type: FF to FF
		case FFtoFF:
		{
			long diffparentloc = 0, count = 0;
			ClockTreeNode *sameparent = this->findLastSameParentNode();
			// Deal the common part of clock path, and end clk path
			for(auto const &nodeptr : this->_endpclkpath)//loc=location
			{
				count++;
				if( nodeptr == sameparent )
					diffparentloc = count;
				if( nodeptr->ifMasked() == false  )
					addDccPlacementCandidate(nodeptr, 1);
			}
			// Deal the remain part of clock path
			// Store every combination of DCC placement
			for(long loop1 = diffparentloc;loop1 < this->_startpclkpath.size()-1;loop1++)
			{
				if(this->_startpclkpath.at(loop1)->ifMasked() == false )
				{
					addDccPlacementCandidate(this->_startpclkpath.at(loop1), 1);
					for(long loop2 = diffparentloc;loop2 < this->_endpclkpath.size()-1;loop2++)
					{
						if(this->_endpclkpath.at(loop2)->ifMasked() == false )
						{
							addDccPlacementCandidate(this->_startpclkpath.at(loop1), 1);
							addDccPlacementCandidate(this->_endpclkpath.at(loop2), 0)  ;
						}
					}
				}
			}
			break;
		}
		default:
			break;
	}
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Report if the startpoint is equal to the endpoint
//
/////////////////////////////////////////////////////////////////////
bool CriticalPath::isEndPointSameAsStartPoint(void)
{
	if( this->_pathtype != FFtoFF )
		return false;
	return ((this->_startpointname.compare(this->_endpointname) == 0 /*equal*/) && (this->_startpclkpath.back() == this->_endpclkpath.back()));
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Report the location of the buffer/FF in the clock path of
// startpoint/endpoint
// Input parameter:
// who: 's' => startpoint
//      'e' => endpoint
//
/////////////////////////////////////////////////////////////////////
long CriticalPath::nodeLocationInClockPath( char who, ClockTreeNode *node )
{
	long findloc = -1;
	vector<ClockTreeNode *> clkpath;
	switch(who)
	{
		case 's':// startpoint
			clkpath = this->_startpclkpath;
			break;
		case 'e':// endpoint
			clkpath = this->_endpclkpath;
			break;
		default:
			break;
	}
	for( long loop = 0; loop < clkpath.size(); loop++ )
	{
		if( clkpath.at(loop) == node )
		{
			findloc = loop;
			break;
		}
	}
	return findloc;
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Report the last common buffer of startpoint and endpoint clock
// path
//
/////////////////////////////////////////////////////////////////////
ClockTreeNode *CriticalPath::findLastSameParentNode(void)
{
	if( this->_pathtype != FFtoFF )
		return nullptr;
	ClockTreeNode *findnode = nullptr;
	for( long loop = 0;;loop++ )
	{
		if(!((loop < this->_startpclkpath.size()) && (loop < this->_endpclkpath.size())))
			break;
		if( this->_startpclkpath.at(loop) == this->_endpclkpath.at(loop) )
			findnode = this->_startpclkpath.at(loop);
		else
			break;
	}
	return findnode;
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Find if DCC was inserted in clock path of startpoint/endpoint of
// the critical path
// Input parameter:
// who: 's' => startpoint
//      'e' => endpoint
//
/////////////////////////////////////////////////////////////////////
ClockTreeNode *CriticalPath::findDccInClockPath(char who)
{
	ClockTreeNode *findnode = nullptr;
	vector<ClockTreeNode *> clkpath;
	// startpoint
	if(who == 's')
		clkpath = this->_startpclkpath;
	// endpoint
	else if(who == 'e')
		clkpath = this->_endpclkpath;
	else
		return nullptr;
	for(auto const &nodeptr : clkpath)
	{
		if( nodeptr->ifPlacedDcc() )
		{
			findnode = nodeptr;
			break;
		}
	}
	return findnode;
}
/*---------------------------------------------------------------------------------
 Func Name:
    findVTAInClockPath
 Intro:
    Used in UpdateAllPathTiming
-----------------------------------------------------------------------------------*/
ClockTreeNode *CriticalPath::findVTAInClockPath(char who)
{
    ClockTreeNode *findnode = nullptr;
    vector<ClockTreeNode *> clkpath;
    // startpoint
    if(who == 's')
        clkpath = this->_startpclkpath;
    // endpoint
    else if(who == 'e')
        clkpath = this->_endpclkpath;
    else
        return nullptr;
    for(auto const &nodeptr : clkpath)
    {
        if( nodeptr->getVTAType() != -1 )
        {
            findnode = nodeptr;
            break;
        }
    }
    return findnode;
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Print some information of this critical path
// Input parameter:
// verbose: 0 => briefness
//          1 => detail
//
/////////////////////////////////////////////////////////////////////
void CriticalPath::printCriticalPath(bool verbose)
{
	cout << "\t**Path NO." << this->_pathnum << " **\n";
	if(verbose)
	{
		cout << "\t\tStartpoint              : " << this->_startpointname << "\n";
		cout << "\t\tEndpoint                : " << this->_endpointname << "\n";
		cout << "\t\tPath type               : " << this->_pathtype << "\n";
		cout << "\t\tCi                      : " << this->_ci << "\n";
		cout << "\t\tCj                      : " << this->_cj << "\n";
		cout << "\t\tTcq                     : " << this->_tcq << "\n";
		cout << "\t\tDij                     : " << this->_dij << "\n";
		cout << "\t\tInput port delay        : " << this->_tindelay << "\n";
		cout << "\t\tSetup time              : " << this->_tsu << "\n";
		cout << "\t\tArrival time            : " << this->_arrivaltime << "\n";
		cout << "\t\tRequired time           : " << this->_requiredtime << "\n";
		cout << "\t\tSlack                   : " << this->_slack << "\n";
		cout << "\t\t# of Combinational logic: " << this->_gatelist.size() << "\n";
		cout << "\t\tStartpoint Path         : ";
		for(auto const &nodeptr : this->_startpclkpath)
		{
			cout << nodeptr->getGateData()->getGateName();
			if(nodeptr != this->_startpclkpath.back())
				cout << ", ";
		}
		cout << ((this->_startpclkpath.empty()) ? "N/A\n" : "\n");
		cout << "\t\tEndpoint Path           : ";
		for(auto const &nodeptr : this->_endpclkpath)
		{
			cout << nodeptr->getGateData()->getGateName();
			if(nodeptr != this->_endpclkpath.back())
				cout << ", ";
		}
		cout << ((this->_endpclkpath.empty()) ? "N/A\n" : "\n");
	}
	else
	{
		cout << "\t\tStartpoint: " << this->_startpointname << "\n";
		cout << "\t\tEndpoint:   " << this->_endpointname << "\n";
		cout << "\t\tPath type:  " << this->_pathtype << "\n";
		cout << "\t\tSlack:      " << this->_slack << "\n";
		cout << "\t\tTotal node: " << this->_gatelist.size() << "\n";
	}
}

/////////////////////////////////////////////////////////////////////
//
// CriticalPath Class - Public Method
// Print all DCC deployment of this critical path
//
/////////////////////////////////////////////////////////////////////
void CriticalPath::printDccPlacementCandidate(void)
{
	cout << "\t**Path NO." << this->_pathnum << " Dcc lacement Candidates **\n";
	for(long loop = 0;loop < this->_dccplacementcandi.size();loop++)
	{
		cout << "\t\t" << loop << ": ";
		for(auto const &nodeptr : this->_dccplacementcandi.at(loop))
			cout << nodeptr->getNodeNumber() << " ";
		cout << "\n";
	}
}

void CriticalPath::coutPathType()
{
    if( this->getPathType() == FFtoFF )
        cout << "FFtoFF" ;
    else if( this->getPathType() == PItoFF )
        cout << "PItoFF" ;
    else if( this->getPathType() == FFtoPO )
        cout << "FFtoPO" ;
}
