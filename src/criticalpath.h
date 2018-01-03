//////////////////////////////////////////////////////////////
//
// Header File
//
// File name: criticalpath.h
// Author: Tien-Hung Tseng, Ting-Wei Chang
// Date: 2017-10
//
//////////////////////////////////////////////////////////////

#ifndef CRITICALPATH_H
#define CRITICALPATH_H

#include "utility.h"

// Type critical path
#define NONE (0)				// none
#define PItoPO (1)				// input port to output port
#define PItoFF (2)				// input port to flip-flop
#define FFtoPO (3)				// flip-flop to output port
#define FFtoFF (4)				// flip-flop to flip-flop

using namespace std;

/////////////////////////////////////////////////////////////////////
//
// Data structure for a gate
//
/////////////////////////////////////////////////////////////////////
class GateData
{
private:
	// _gatename: Gate name
	// _wiretime: Wire delay
	// _gatetime: Gate delay
	string _gatename;
	double _wiretime, _gatetime;

public:
	// Constructor & Destructor
	// Initial value:
	// _gatename = ""
	// _wiretime = 0
	// _gatetime = 0
	GateData(string name = "", double wireime = 0, double gatetime = 0)
	        : _gatename(name), _wiretime(wireime), _gatetime(gatetime) {}
	~GateData() {}
	
	// Setter methods
	void setGateName(string name) { this->_gatename = name; }
	void setWireTime(double wiretime) { this->_wiretime = wiretime; }
	void setGateTime(double gatetime) { this->_gatetime = gatetime; }
	
	// Getter methods
	string getGateName(void) { return _gatename; }
	double getWireTime(void) { return _wiretime; }
	double getGateTime(void) { return _gatetime; }
};

/////////////////////////////////////////////////////////////////////
//
// Data structure for a node in clock tree
//
/////////////////////////////////////////////////////////////////////
class ClockTreeNode
{
private:
	// _parent           : Parent node (buffer)
	// _gatedata         : Gata data information
	// _nodenum          : Node number
	// _depth            : Location of clock tree level
	// _dcctype          : Type of DCC
	// _ifused           : Bit for used/unused (0 => unused, 1 => used)
	// _iflook           : Bit for checking
	// _ifplacedcc       : Bit for inserted DCC (1 => insert DCC)
	// _ifclkgating      : Bit for replacing clock gating cell (1 => replace CG cell)
	// _ifinsertbuf      : Bit for inserted buffer (1 => insert buffer)
	// _gatingprobability: Clock gating probability
	// _insbufdelay      : Delay of inserted buffer
	// _children         : Children nodes (buffers/FFs)
	ClockTreeNode *_parent;
	GateData _gatedata;
	long _nodenum, _depth;
	int _dcctype, _ifused;
	bool _iflook, _ifplacedcc, _ifclkgating, _ifinsertbuf;
	double _gatingprobability, _insbufdelay;
	vector<ClockTreeNode *> _children;

public:
	// Constructor & Destructor
	// Initial value:
	// _parent            = nullptr
	// _gatedata          = empty
	// _nodenum           = 0
	// _depth             = -1
	// _dcctype           = 0
	// _ifused            = -1
	// _iflook            = 0
	// _ifplacedcc        = 0
	// _ifclkgating       = 0
	// _ifinsertbuf       = 0
	// _gatingprobability = 0
	// _insbufdelay       = 0
	// _children          = empty
	ClockTreeNode(ClockTreeNode *parent = nullptr, long num = 0, long depth = -1, int used = -1, int type = 0,
	              bool look = 0, bool placedcc = 0, bool gating = 0, double probability = 0)
	             : _parent(parent), _nodenum(num), _depth(depth), _ifused(used), _dcctype(type), _iflook(look),
				   _ifplacedcc(placedcc), _ifclkgating(gating), _gatingprobability(probability),
				   _ifinsertbuf(0), _insbufdelay(0) {}
	~ClockTreeNode() {}
	
	// Setter methods
	void setDccType(int, int) ;
	void setDccType(int type)                       { this->_dcctype = type; }
	void setIfUsed(int used)                        { this->_ifused = used; }
	void setIfLook(bool look)                       { this->_iflook = look; }
	void setIfPlaceDcc(bool place)                  { this->_ifplacedcc = place; }
	void setIfInsertBuffer(bool insert)             { this->_ifinsertbuf = insert; }
	void setInsertBufferDelay(double delay)         { this->_insbufdelay = delay; }
	void setIfClockGating(bool gating)              { this->_ifclkgating = gating; }
	void setGatingProbability(double probability)   { this->_gatingprobability = probability; }
	//void setParent(ClockTreeNode *parent) { this->_parent = parent; }
	//void setNodeNumber(long number) { this->_nodenum = number; }
	//void setDepth(long depth) { this->_depth = depth; }
	
	// Getter methods
	ClockTreeNode *getParent(void)              { return _parent; }
	GateData *getGateData(void)                 { return &_gatedata; }
	long getNodeNumber(void)                    { return _nodenum; }
	long getDepth(void)                         { return _depth; }
	int getDccType(void)                        { return _dcctype; }
	double getInsertBufferDelay(void)           { return _insbufdelay; }
	double getGatingProbability(void)           { return _gatingprobability; }
	vector<ClockTreeNode *>& getChildren(void)  { return _children; }
	
	// Other methods
	int  ifUsed(void)           { return _ifused            ; }
	bool ifLook(void)           { return _iflook            ; }
	bool ifPlacedDcc(void)      { return _ifplacedcc        ; }
	bool ifInsertBuffer(void)   { return _ifinsertbuf       ; }
	bool ifClockGating(void)    { return _ifclkgating       ; }
	bool isFFSink(void)         { return _children.empty()  ; }
	bool isFinalBuffer(void)    ;
	ClockTreeNode *searchChildren(string);
	//dump();
};

/////////////////////////////////////////////////////////////////////
//
// Data structure for a critical path
//
/////////////////////////////////////////////////////////////////////
class CriticalPath
{
private:
	// _startpointname   : Name of start point (FF/input port)
	// _endpointname     : Name of end point (FF/output port)
	// _pathtype         : Type of critical path (see define above)
	// _pathnum          : Critical path number
	// _ci               : Clock latency of start point
	// _cj               : Clock latency of end point
	// _clkuncertainty   : Clock uncertainty
	// _tcq              : Tcq (delay of clock to Q of FF)
	// _dij              : Dij (longest path delay)
	// _tsu              : Tsu (setup time of FF)
	// _tindelay         : Input external delay
	// _arrivaltime      : Arrival time
	// _requiredtime     : Required time
	// _slack            : Slack
	// _startpclkpath    : Clock path of start point (FF)
	// _endpclkpath      : Clock path of end point (FF)
	// _gatelist         : Each gate of combinational logic of critical path
	// _dccplacementcandi: All combination of DCC placement (2 dimension)
	string _startpointname, _endpointname;
	int _pathtype;
	long _pathnum;
	double _ci, _cj, _clkuncertainty;
	double _tcq, _dij, _tsu, _tindelay;
	double _arrivaltime, _requiredtime, _slack;
	vector<ClockTreeNode *> _startpclkpath, _endpclkpath;
	vector<GateData *> _gatelist;
	vector<vector<ClockTreeNode *> > _dccplacementcandi;
	
	void addDccPlacementCandidate(ClockTreeNode *, bool);
	
public:
	// Constructor & Destructor
	// Initial value:
	// _startpointname    = ""
	// _endpointname      = ""
	// _pathtype          = 0
	// _pathnum           = -1
	// _ci                = 0
	// _cj                = 0
	// _clkuncertainty    = 0
	// _tcq               = 0
	// _dij               = 0
	// _tsu               = 0
	// _tindelay          = 0
	// _arrivaltime       = 0
	// _requiredtime      = 0
	// _slack             = 0
	// _startpclkpath     = empty
	// _endpclkpath       = empty
	// _gatelist          = empty
	// _dccplacementcandi = empty
	CriticalPath(string sname = "", int type = 0, long pathnum = -1, string ename = "",
				 double ci = 0, double cj = 0, double clkuncert = 0,
				 double tcq = 0, double dij = 0, double tsu = 0, double tindelay = 0,
				 double arrivaltime = 0, double requiredtime = 0, double slack = 0)
				: _startpointname(sname), _pathtype(type), _pathnum(pathnum), _endpointname(ename), _ci(ci),
				_cj(cj), _clkuncertainty(clkuncert), _tcq(tcq), _dij(dij), _tsu(tsu), _tindelay(tindelay),
				_arrivaltime(arrivaltime), _requiredtime(requiredtime), _slack(slack) {}
	~CriticalPath(void);
	
	// Setter methods
	void setEndPointName(string name) { this->_endpointname = name; }
	void setPathType(int type) { this->_pathtype =  type; }
	void setCi(double ci) { this->_ci = ci; }
	void setCj(double cj) { this->_cj = cj; }
	void setTcq(double tcq) { this->_tcq = tcq; }
	void setDij(double dij) { this->_dij = dij; }
	void setTsu(double tsu) { this->_tsu = tsu; }
	void setTinDelay(double tindelay) { this->_tindelay = tindelay; }
	void setClockUncertainty(double clkuncert) { this->_clkuncertainty = clkuncert; }
	void setArrivalTime(double arrivaltime) { this->_arrivaltime = arrivaltime; }
	void setRequiredTime(double requiredtime) { this->_requiredtime = requiredtime; }
	void setSlack(double slack) { this->_slack = slack; }
	void setDccPlacementCandidate(void);
	//void setStartPointName(string name) { this->_startpointname = name; }
	//void setPathNum(long number) { this->_pathnum = number; }
	
	// Getter methods
	string getStartPointName(void) { return _startpointname; }
	string getEndPointName(void) { return _endpointname; }
	int getPathType(void) { return _pathtype; }
	long getPathNum(void) { return _pathnum; }
	double getCi(void) { return _ci; }
	double getCj(void) { return _cj; }
	double getTcq(void) { return _tcq; }
	double getDij(void) { return _dij; }
	double getTsu(void) { return _tsu; }
	double getTinDelay(void) { return _tindelay; }
	double getClockUncertainty(void) { return _clkuncertainty; }
	double getArrivalTime(void) { return _arrivaltime; }
	double getRequiredTime(void) { return _requiredtime; }
	double getSlack(void) { return _slack; }
	vector<ClockTreeNode *>& getStartPonitClkPath(void) { return _startpclkpath; }
	vector<ClockTreeNode *>& getEndPonitClkPath(void) { return _endpclkpath; }
	vector<GateData *>& getGateList(void) { return _gatelist; }
	vector<vector<ClockTreeNode *> >& getDccPlacementCandi(void) { return _dccplacementcandi; }
	
	// Other methods
	bool isEndPointSameAsStartPoint(void);
	long nodeLocationInClockPath(char, ClockTreeNode *);
	ClockTreeNode *findLastSameParentNode(void);
	ClockTreeNode *findDccInClockPath(char);
	void printCriticalPath(bool verbose = 1);
	void printDccPlacementCandidate(void);
};

#endif	// CRITICALPATH_H
