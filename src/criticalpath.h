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
	string _gatename;
	double _wiretime, _gatetime;

public:
	GateData(string name = "", double wireime = 0, double gatetime = 0)
	        : _gatename(name), _wiretime(wireime), _gatetime(gatetime) {}
	~GateData() {}
	
	//-- Setter methods ----------------------------------------------------------------------
	void    setGateName(string name)        { this->_gatename = name    ; }
	void    setWireTime(double wiretime)    { this->_wiretime = wiretime; }
	void    setGateTime(double gatetime)    { this->_gatetime = gatetime; }
	
	//-- Getter methods ---------------------------------------------------------------------
	string getGateName(void)                { return _gatename          ; }
	double getWireTime(void)                { return _wiretime          ; }
	double getGateTime(void)                { return _gatetime          ; }
};

/////////////////////////////////////////////////////////////////////
//
// Data structure for a node in clock tree
//
/////////////////////////////////////////////////////////////////////
class ClockTreeNode
{
private:
	ClockTreeNode * _parent         ;
	GateData        _gatedata       ;
	long            _nodenum, _depth;
	int             _ifused,_LibIndex;
	bool            _iflook , _ifplacedcc, _ifclkgating, _ifinsertbuf, _ifplaceHeader;
    double          _dcctype,_gatingprobability, _insbufdelay ;
    double          _buftime ;//only use in -print=path mode
    double          _DC      ;
    int             _VthType ;
	vector<ClockTreeNode *> _children;

public:
    //-- Constructor/Destructor -------------------------------------------------------------
	ClockTreeNode(ClockTreeNode *parent = nullptr, long num = 0, long depth = -1, int used = -1, int type = 0,
	              bool look = 0, bool placedcc = 0, bool gating = 0, double probability = 0)
	             : _parent(parent), _nodenum(num), _depth(depth), _ifused(used), _dcctype(type), _iflook(look),
				   _ifplacedcc(placedcc), _ifclkgating(gating), _gatingprobability(probability),
				   _ifinsertbuf(0), _insbufdelay(0), _ifplaceHeader(0), _LibIndex(-1), _buftime(0), _DC(0.5), _VthType(-1) { }
	~ClockTreeNode() {}
	
	//-- Setter methods ----------------------------------------------------------------------
	void    setDccType(int, int) ;
    void    setDC(double DC)                           { this->_DC             = DC    ; }//only used in -print=path mode
    void    setVthType(int Lib)                        { this->_VthType        = Lib   ; }//only used in -print=path mode
	void    setDccType(double type)                    { this->_dcctype        = type  ; }
    void    setVTAType(int Lib)                        { this->_LibIndex       = Lib   ; }
	void    setIfUsed(int used)                        { this->_ifused         = used  ; }
	void    setIfLook(bool look)                       { this->_iflook         = look  ; }
	void    setIfPlaceDcc(bool place)                  { this->_ifplacedcc     = place ; }
    void    setIfPlaceHeader(bool place)               { this->_ifplaceHeader  = place ; }
	void    setIfInsertBuffer(bool insert)             { this->_ifinsertbuf    = insert; }
	void    setInsertBufferDelay(double delay)         { this->_insbufdelay    = delay ; }
	void    setIfClockGating(bool gating)              { this->_ifclkgating    = gating; }
	void    setGatingProbability(double probability)   { this->_gatingprobability = probability; }
	void    setParent(ClockTreeNode *parent)           { this->_parent         = parent; }
	void    setNodeNumber(long number)                 { this->_nodenum        = number; }
	void    setDepth(long depth)                       { this->_depth          = depth ; }
    void    setBufTime(double b)                       { this->_buftime        = b     ; }//only used in -print=path mode
	//-- Getter methods ---------------------------------------------------------------------
    double  getDC(void)                                { return _DC         ; }//only used in -print=path mode
    int     getVthType(void)                           { return _VthType    ; }//only used in -print=path mode
	long    getNodeNumber(void)                        { return _nodenum    ; }
	long    getDepth(void)                             { return _depth      ; }
	double  getDccType(void)                           { return _dcctype    ; }
    int     getVTAType(void)                           { return _LibIndex   ; }
    bool    getIfPlaceHeader(void)                     { return _ifplaceHeader ; }
	double  getInsertBufferDelay(void)                 { return _insbufdelay; }
	double  getGatingProbability(void)                 { return _gatingprobability; }
    double  getBufTime()                               { return _buftime    ; }//only used in -print=path mode
	vector<ClockTreeNode *>& getChildren(void)         { return _children   ; }
    ClockTreeNode   *getParent(void)                   { return _parent     ; }
    GateData        *getGateData(void)                 { return &_gatedata  ; }
    
	//-- Other methods -------------------------------------------------------------------------
	int  ifUsed(void)                                  { return _ifused            ; }
	bool ifLook(void)                                  { return _iflook            ; }
	bool ifPlacedDcc(void)                             { return _ifplacedcc        ; }
	bool ifInsertBuffer(void)                          { return _ifinsertbuf       ; }
	bool ifClockGating(void)                           { return _ifclkgating       ; }
	bool isFFSink(void)                                { return _children.empty()  ; }
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
	CriticalPath(string sname = "", int type = 0, long pathnum = -1, string ename = "",
				 double ci = 0, double cj = 0, double clkuncert = 0,
				 double tcq = 0, double dij = 0, double tsu = 0, double tindelay = 0,
				 double arrivaltime = 0, double requiredtime = 0, double slack = 0)
				: _startpointname(sname), _pathtype(type), _pathnum(pathnum), _endpointname(ename), _ci(ci),
				_cj(cj), _clkuncertainty(clkuncert), _tcq(tcq), _dij(dij), _tsu(tsu), _tindelay(tindelay),
				_arrivaltime(arrivaltime), _requiredtime(requiredtime), _slack(slack) {}
	~CriticalPath(void);
	
	//-- Setter methods ----------------------------------------------------------------------
	void    setEndPointName(string name)            { this->_endpointname   = name          ; }
	void    setPathType(int type)                   { this->_pathtype       = type          ; }
	void    setCi(double ci)                        { this->_ci             = ci            ; }
	void    setCj(double cj)                        { this->_cj             = cj            ; }
	void    setTcq(double tcq)                      { this->_tcq            = tcq           ; }
	void    setDij(double dij)                      { this->_dij            = dij           ; }
	void    setTsu(double tsu)                      { this->_tsu            = tsu           ; }
	void    setTinDelay(double tindelay)            { this->_tindelay       = tindelay      ; }
	void    setClockUncertainty(double clkuncert)   { this->_clkuncertainty = clkuncert     ; }
	void    setArrivalTime(double arrivaltime)      { this->_arrivaltime    = arrivaltime   ; }
	void    setRequiredTime(double requiredtime)    { this->_requiredtime   = requiredtime  ; }
	void    setSlack(double slack)                  { this->_slack          = slack         ; }
	void    setDccPlacementCandidate(void);
	//void  setStartPointName(string name)          { this->_startpointname = name          ; }
	//void  setPathNum(long number)                 { this->_pathnum        = number        ; }
	
	//-- Getter methods ---------------------------------------------------------------------
	string  getStartPointName(void)                 { return _startpointname                ; }
	string  getEndPointName(void)                   { return _endpointname                  ; }
	int     getPathType(void)                       { return _pathtype                      ; }
	long    getPathNum(void)                        { return _pathnum                       ; }
	double  getCi(void)                             { return _ci                            ; }
	double  getCj(void)                             { return _cj                            ; }
	double  getTcq(void)                            { return _tcq                           ; }
	double  getDij(void)                            { return _dij                           ; }
	double  getTsu(void)                            { return _tsu                           ; }
	double  getTinDelay(void)                       { return _tindelay                      ; }
	double  getClockUncertainty(void)               { return _clkuncertainty                ; }
	double  getArrivalTime(void)                    { return _arrivaltime                   ; }
	double  getRequiredTime(void)                   { return _requiredtime                  ; }
	double  getSlack(void)                          { return _slack                         ; }
    void    coutPathType(void) ;
	vector<ClockTreeNode *>& getStartPonitClkPath(void) { return _startpclkpath             ; }
	vector<ClockTreeNode *>& getEndPonitClkPath(void)   { return _endpclkpath               ; }
	vector<GateData *>& getGateList(void)               { return _gatelist                  ; }
	vector<vector<ClockTreeNode *> >& getDccPlacementCandi(void) { return _dccplacementcandi; }
	
	//-- Other methods -------------------------------------------------------------------------
	bool isEndPointSameAsStartPoint(void)               ;
	long nodeLocationInClockPath(char, ClockTreeNode *) ;
	ClockTreeNode *findLastSameParentNode(void)         ;
	ClockTreeNode *findDccInClockPath(char)             ;
    ClockTreeNode *findVTAInClockPath(char)             ;
	void printCriticalPath(bool verbose = 1)            ;
	void printDccPlacementCandidate(void)               ;
};

#endif	// CRITICALPATH_H
