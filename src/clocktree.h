//////////////////////////////////////////////////////////////
//
// Header File
//
// File name: clocktree.h
// Author: Ting-Wei Chang
// Date: 2017-07
//
//////////////////////////////////////////////////////////////

#ifndef CLOCKTREE_H
#define CLOCKTREE_H

#include "criticalpath.h"
#include "utility.h"
#include <map>
#include <set>

// Factor of DCC delay based on logic effort
#define DCCDELAY20PA (1.33)		// 20% DCC Delay
#define DCCDELAY40PA (1.33)		// 40% DCC Delay
#define DCCDELAY50PA (1.33)		// 50% DCC Delay
#define DCCDELAY80PA (1.67)		// 80% DCC Delay

#define PRECISION (4)			// Precision of real number
#define PATHMASKPERCENT (1.0)	// Mask how many percentage of critical path (0~1)

using namespace std;

/////////////////////////////////////////////////////////////////////
//
// Data structure for a clock tree
//
/////////////////////////////////////////////////////////////////////
class ClockTree
{
private:
	// _pathselect          : Critical path consideration
	// _bufinsert           : Buffer insertion (1 => enable)
	// _gpupbound           : Upper bound of clock gating probability
	// _gplowbound          : Lower bound of clock gating probability
	// _minisatexecnum      : # of minisat executions (# of binary search)
	// _placedcc            : DCC insertion (1 => enable)
	// _aging               : Aging consideration (1 => enable)
	// _mindccplace         : DCC insertion minimization (1 => enable)
	// _tcrecheck           : Tc rechecked (1 => enable)
	// _clkgating           : Clock gating replacement (1 => enable)
	// _dumpdcc             : Dump of inserted DCCs (1 => enable)
	// _dumpcg              : Dump of clock gating (1 => enable)
	// _dumpbufins          : Dump of inserted buffers (1 => enable)
	// _pathusednum         : # of critical paths in used
	// _pitoffnum           : # of PI to FF critical paths
	// _fftoffnum           : # of FF to FF critical paths
	// _fftoponum           : # of FF to PO critical paths
	// _nonplacedccbufnum   : # of uninserted DCC buffers 
	// _totalnodenum        : Total # of clock tree nodes
	// _ffusednum           : # of FFs in used
	// _bufferusednum       : # of clock buffers in used
	// _dccatlastbufnum     : # of inserted DCCs at last buffer
	// _masklevel           : Mask level from bottom of clock tree
	// _maxlevel            : Maximum level of clock tree
	// _insertbufnum        : # of inserted buffers
	// _maskleng            : Mask length
	// _cgpercent           : Percentage of clock gating cell replacement
	// _agingtcq            : Aging rate of Tcq
	// _agingdij            : Aging rate of Dij
	// _agingtsu            : Aging rate of Tsu
	// _origintc            : Tc in timing report
	// _besttc              : Optimal Tc
	// _tc                  : Tc for binary search
	// _tcupbound           : Upper bound of Tc
	// _tclowbound          : Lower bound of Tc
	// _timingreport        : Timing report from "argv"
	// _timingreportfilename: File name of timing report
	// _timingreportloc     : File location of timing report
	// _timingreportdesign  : Design of timing report
	// _cgfilename          : Clock gating input file
	// _outputdir           : Name of output file directory
	// _clktreeroot         : Root of clock tree
	// _firstchildrennode   : Last common buffer in clock tree
	// _mostcriticalpath    : Critical path dominating Tc
	// _pathlist            : List for critical path of circuit
	// _ffsink              : List for FFs of circuit
	// _buflist             : List for buffers of circuit
	// _cglist              : List for clock gating cells of circuit
	// _dcclist             : List for DCCs of circuit
	// _dccconstraintlist   : All clauses of DCC constraints
	// _timingconstraintlist: All clauses of timing constraints
	int _pathselect, _bufinsert, _gpupbound, _gplowbound, _minisatexecnum;
	bool _placedcc, _aging, _mindccplace, _tcrecheck, _clkgating, _dumpdcc, _dumpcg, _dumpbufins, _printClause;
	long _pathusednum, _pitoffnum, _fftoffnum, _fftoponum, _nonplacedccbufnum, _printClkNode;
	long _totalnodenum, _ffusednum, _bufferusednum, _dccatlastbufnum;
	long _masklevel, _maxlevel, _insertbufnum;
	double _maskleng, _cgpercent, _agingtcq, _agingdij, _agingtsu;
	double _origintc, _besttc, _tc, _tcupbound, _tclowbound;
	string _timingreport, _timingreportfilename, _timingreportloc, _timingreportdesign;
	string _cgfilename, _outputdir;
	ClockTreeNode *_clktreeroot, *_firstchildrennode;
	CriticalPath *_mostcriticalpath;
	vector<CriticalPath *> _pathlist;
	map<string, ClockTreeNode *> _ffsink, _buflist, _cglist, _dcclist;
	set<string> _dccconstraintlist, _timingconstraintlist;
	
	bool ifSkipLine(string)                 ;
    bool AnotherSolution(void)              ;
	void pathTypeChecking(void)             ;
	void recordClockPath(char)              ;
	void checkFirstChildrenFormRoot(void)   ;
	void initTcBound(void)                  ;
	void genDccConstraintClause(vector<vector<long> > *);
	void writeClauseByDccType(ClockTreeNode *, string *, int);
	void deleteClockTree(void)              ;
	void dumpDccListToFile(void)            ;
	void dumpClockGatingListToFile(void)    ;
	void dumpBufferInsertionToFile(void)    ;
    double updatePathTimingWithDcc(CriticalPath *, bool update = 0);
	
public:
	// Constructor & Destructor
	// Initial value:
	// _pathselect           = 0
	// _bufinsert            = 0
	// _gpupbound            = 70
	// _gplowbound           = 20
	// _minisatexecnum       = 0
	// _placedcc             = 1
	// _aging                = 1
	// _mindccplace          = 0
	// _tcrecheck            = 0
	// _clkgating            = 0
	// _dumpdcc              = 0
	// _dumpcg               = 0
	// _dumpbufins           = 0
	// _pathusednum          = 0
	// _pitoffnum            = 0
	// _fftoffnum            = 0
	// _fftoponum            = 0
	// _nonplacedccbufnum    = 0
	// _totalnodenum         = 1
	// _ffusednum            = 0
	// _bufferusednum        = 0
	// _dccatlastbufnum      = 0
	// _masklevel            = 0
	// _maxlevel             = 0
	// _insertbufnum         = 0
	// _maskleng             = 0.5
	// _cgpercent            = 0.02
	// _agingtcq             = 1.2
	// _agingdij             = 1.17
	// _agingtsu             = 1
	// _origintc             = 0
	// _besttc               = 0
	// _tc                   = 0
	// _tcupbound            = 0
	// _tclowbound           = 0
	// _timingreport         = ""
	// _timingreportfilename = ""
	// _timingreportloc      = ""
	// _timingreportdesign   = ""
	// _cgfilename           = ""
	// _outputdir            = ""
	// _clktreeroot          = nullptr
	// _firstchildrennode    = nullptr
	// _mostcriticalpath     = nullptr
	// _pathlist             = empty
	// _ffsink               = empty
	// _buflist              = empty
	// _cglist               = empty
	// _dcclist              = empty
	// _dccconstraintlist    = empty
	// _timingconstraintlist = empty
	ClockTree(void)
			 : _pathselect(0), _bufinsert(0), _placedcc(1), _aging(1), _mindccplace(0), _tcrecheck(0), _clkgating(0),
			   _dumpdcc(0), _dumpcg(0), _dumpbufins(0), _agingtcq(1.2), _agingdij(1.17), _agingtsu(1),
			   _cgpercent(0.02), _pathusednum(0), _pitoffnum(0), _fftoffnum(0), _fftoponum(0),
			   _masklevel(0), _maskleng(0.5), _maxlevel(0), _nonplacedccbufnum(0), _dccatlastbufnum(0), _insertbufnum(0),
			   _totalnodenum(1), _ffusednum(0), _bufferusednum(0), _minisatexecnum(0), _gpupbound(70), _gplowbound(20), 
			   _origintc(0), _besttc(0), _tc(0), _tcupbound(0), _tclowbound(0),
			   _clktreeroot(nullptr), _firstchildrennode(nullptr), _mostcriticalpath(nullptr),
			   _timingreport(""), _timingreportfilename(""), _timingreportloc(""), _timingreportdesign(""),
			   _cgfilename(""), _outputdir(""), _printClkNode(false) {}
	~ClockTree(void);
	
	// Setter methods
	void setTc(double tc) { this->_tc = tc; }
	void setTcUpperBound(double tc) { this->_tcupbound = tc; }
	void setTcLowerBound(double tc) { this->_tclowbound = tc; }
	void setOutputDirectoryPath(string path) { this->_outputdir = path; }
	//void setBestTc(double tc) { this->_besttc = tc; }
	
	// Getter methods
	int getPathSelection(void) { return _pathselect; }
	int getBufferInsertionMode(void) { return _bufinsert; }
	int getGatingProbabilityUpperBound(void) { return _gpupbound; }
	int getGatingProbabilityLowerBound(void) { return _gplowbound; }
	int getMinisatExecuteNumber(void) { return _minisatexecnum; }
	long getTotalPathNumber(void) { return _pathlist.size(); }
	long getPathUsedNumber(void) { return _pathusednum; }
	long getPiToFFNumber(void) { return _pitoffnum; }
	long getFFToFFNumber(void) { return _fftoffnum; }
	long getFFToPoNumber(void) { return _fftoponum; }
	long getTotalNodeNumber(void) { return _totalnodenum; }
	long getTotalFFNumber(void) { return _ffsink.size(); }
	long getTotalBufferNumber(void) { return _buflist.size(); }
	long getTotalClockGatingNumber(void) { return _cglist.size(); }
	long getTotalDccNumber(void) { return _dcclist.size(); }
	long getNonPlacedDccBufferNumber(void) { return _nonplacedccbufnum; }
	long getDccPlacedAtLastBufferNumber(void) { return _dccatlastbufnum; }
	long getFFUsedNumber(void) { return _ffusednum; }
	long getBufferUsedNumber(void) { return _bufferusednum; }
	long getMaskByLevel(void) { return _masklevel; }
	long getMaxTreeLevel(void) { return _maxlevel; }
	long getBufferInsertionNumber(void) { return _insertbufnum; }
	double getMaskByLength(void) { return _maskleng; }
	double getClockGatingPercent(void) { return _cgpercent; }
	double getTcqAgingRate(void) { return _agingtcq; }
	double getDijAgingRate(void) { return _agingdij; }
	double getTsuAgingRate(void) { return _agingtsu; }
	double getOriginTc(void) { return _origintc; }
	double getBestTc(void) { return _besttc; }
	double getTc(void) { return _tc; }
	double getTcUpperBound(void) { return _tcupbound; }
	double getTcLowerBound(void) { return _tclowbound; }
	string getTimingReportFileName(void) { return _timingreportfilename; }
	string getTimingReportLocation(void) { return _timingreportloc; }
	string getTimingReportDesign(void) { return _timingreportdesign; }
	string getClockGatingFileName(void) { return _cgfilename; }
	string getOutputDirectoryPath(void) { return this->_outputdir; }
	//ClockTreeNode *getClockTreeRoot(void) { return _clktreeroot; }
	ClockTreeNode *getFirstChildrenNode(void) { return _firstchildrennode; }
	CriticalPath *getMostCriticalPath(void) { return _mostcriticalpath; }
	vector<CriticalPath *>& getPathList(void) { return _pathlist; }
	//map<string, ClockTreeNode *>& getFFList(void) { return _ffsink; }
	//map<string, ClockTreeNode *>& getBufferList(void) { return _buflist; }
	//map<string, ClockTreeNode *>& getClockGatingList(void) { return _cglist; }
	//map<string, ClockTreeNode *>& getDccList(void) { return _dcclist; }
	//set<string *>& getDccConstraintList(void) { return _dccconstraintlist; }
	//set<string *>& getTimingConstraintList(void) { return _timingconstraintlist; }

	// Other methods
    bool ifprintNode(void)                          { return _printClkNode      ; }
	bool ifPlaceDcc(void) { return _placedcc; }
	bool ifAging(void) { return _aging; }
	bool ifMinDccNumber(void) { return _mindccplace; }
	bool ifTcRecheck(void) { return _tcrecheck; }
	bool ifClockGating(void) { return _clkgating; }
	bool ifDumpDcc(void) { return _dumpdcc; }
	bool ifDumpClockGating(void) { return _dumpcg; }
	bool ifDumpBufferInsertion(void) { return _dumpbufins; }
	
	int checkParameter(int, char **, string *);
	void parseTimingReport(void);
	void clockGatingCellReplacement(void);
	void adjustOriginTc(void);
	void dccPlacementByMasked(void);
	void dccConstraint(void);
	void genDccPlacementCandidate(void);
	double timingConstraint(void);
	double assessTimingWithoutDcc(CriticalPath *, bool genclause = 1, bool update = 0);
	void assessTimingWithDcc(CriticalPath *);
	double assessTimingWithDcc(CriticalPath *, double, double, int, int, ClockTreeNode *,
							   ClockTreeNode *, vector<ClockTreeNode *> clkpath1 = vector<ClockTreeNode *>(),
							   vector<ClockTreeNode *> clkpath2 = vector<ClockTreeNode *>(),
							   bool genclause = 1, bool update = 0);
	void calculateClockLatencyWithoutDcc(CriticalPath *, double *, double *);
	vector<double> calculateClockLatencyWithDcc(vector<ClockTreeNode *>, ClockTreeNode *);
	void dumpClauseToCnfFile(void);
	void execMinisat(void);
	void tcBinarySearch(double);
	void updateAllPathTiming(void);
	void minimizeDccPlacement(void);
	void tcRecheck(void);
	void bufferInsertion(void);
	void minimizeBufferInsertion(void);
	ClockTreeNode *searchClockTreeNode(string);
	ClockTreeNode *searchClockTreeNode(long);
	vector<CriticalPath *> searchCriticalPath(char, string);
	vector<ClockTreeNode *> getFFChildren(ClockTreeNode *);
	void dumpToFile(void);
	
	void printClockTree(void);
	void printSingleCriticalPath(long, bool verbose = 1);
	void printSingleCriticalPath(char, string, bool verbose = 1);
	void printAllCriticalPath(void);
	void printDccList(void);
	void printClockGatingList(void);
	void printBufferInsertedList(void);
    void dumpDcctoFile();
    void    printNodeLayerSpace(int);
    void    printClockNode(void);
    void    printClockNode(ClockTreeNode*, int layer = 0 );
};

/////////////////////////////////////////////////////////////////////
//
// Calculate the aging rate of buffer from duty cycle
//
/////////////////////////////////////////////////////////////////////
inline double getAgingRateByDutyCycle(double dc)
{
    /*
    if( dc == 0.2 )         return 1.118 ;
    else if( dc == 0.4 )    return 1.1397;
    else if( dc == 0.5 )    return 1.1472;
    else if( dc == 0.8 )    return 1.1639;
     */
    return (1 + (((-0.117083333333337) * (dc) * (dc)) + (0.248750000000004 * (dc)) + 0.0400333333333325));
    
    //else return -1 ;
}

#endif  // CLOCKTREE_H
