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
#include "clocktree2.hpp"
#include <assert.h>
#include "utility.h"
#include <map>
#include <set>

// Factor of DCC delay based on logic effort
#define DCCDELAY20PA    (1.33)		// 20% DCC Delay
#define DCCDELAY40PA    (1.33)		// 40% DCC Delay
#define DCCDELAY50PA    (1.33)		// 50% DCC Delay
#define DCCDELAY80PA    (1.67)		// 80% DCC Delay

#define DC_20DCC        (0.22)
#define DC_40DCC        (0.44)
#define DC_80DCC        (0.83)
#define PRECISION       (4)			// Precision of real number
#define PATHMASKPERCENT (1.0)	    // Mask how many percentage of critical path (0~1)
#define TenYear_Sec     (315360000)
#define COF_A           (0.0039/2)

using namespace std;


/*------------------------------------------------------------------
 Data Type Name:
    VTH_TECH
 Introduction:
    A data structure used to store the info of tech library of VTA
 -------------------------------------------------------------------*/
struct VTH_TECH
{
    double _VTH_OFFSET     ;//Vth offset due to technology
    double _VTH_CONVGNT[8] ;//Convergent Vth value over a long period
    double _Sv[16]          ;//Sv value of 20/40/50/80
    VTH_TECH()
    {
        _VTH_OFFSET = _Sv[0] = _Sv[1] = _Sv[2] = _Sv[3] = 0.0 ;
        _VTH_CONVGNT[0] = _VTH_CONVGNT[1] =_VTH_CONVGNT[2] =_VTH_CONVGNT[3] = 0.0 ;
    }
};
/*------------------------------------------------------------------
 Data Type Name:
    Clock Tree
 Introduction:
    A data structure used to store the info of entire circuit
 -------------------------------------------------------------------*/
class ClockTree
{
private:
	int     _pathselect, _bufinsert, _gpupbound, _gplowbound, _minisatexecnum;
	bool    _placedcc, _aging, _mindccplace, _tcrecheck, _clkgating, _dumpdcc, _dumpcg, _dumpbufins, _doVTA, _printpath, _dumpCNF, _checkCNF, _checkfile, _printClause, _calVTA, _dcc_leader, _dc_formulation ;
    bool    _usingSeniorAging, _printClkNode ;
	long    _pathusednum, _pitoffnum, _fftoffnum, _fftoponum, _nonplacedccbufnum;
	long    _totalnodenum, _ffusednum, _bufferusednum, _dccatlastbufnum;
	long    _masklevel, _maxlevel, _insertbufnum;
	double  _maskleng, _cgpercent;
    double _tcAfterAdjust ;
    //Timing-related attribute
    double _origintc, _besttc, _tc, _tcupbound, _tclowbound, _agingtcq, _agingdij, _agingtsu;
    
    //filename-related attribute
	string _timingreport, _timingreportfilename, _timingreportloc, _timingreportdesign;
	string _cgfilename, _outputdir;
    
    //circuit-related attribute
	ClockTreeNode   *_clktreeroot, *_firstchildrennode;
	CriticalPath    *_mostcriticalpath;
	vector<CriticalPath *> _pathlist;
	map<string, ClockTreeNode *> _ffsink, _buflist, _cglist, _dcclist, _VTAlist ;
    set< pair<ClockTreeNode*,ClockTreeNode*> > _setVTALeader ;
    set< pair<int,int> > _setDCC ;
    //Constraint-related attribute
	set<string> _VTAconstraintlist, _dccconstraintlist, _timingconstraintlist;
	
    //VTA-related attribute
    int     _VTH_LIB_cnt    ;
    int     _FIN_CONV_Year  ;
    long    Max_timing_count;
    double  _baseVthOffset  ;
    vector< VTH_TECH* > _VthTechList ;
    double  _exp            ;
    double  _nominal_agr[8] ;
    double  _HTV_agr[8]     ;
	bool ifSkipLine(string)                 ;
    bool AnotherSolution(void)              ;
	void pathTypeChecking(void)             ;
	void recordClockPath(char)              ;
	void checkFirstChildrenFormRoot(void)   ;
	void initTcBound(void)                  ;
	void genDccConstraintClause(vector<vector<long> > *);
	void genClauseByDccVTA(ClockTreeNode *, string *, double, int);
	void deleteClockTree(void)              ;
    //-- Dumper ------------------------------------------------------------------
	void dumpDccListToFile(void)            ;
	void dumpClockGatingListToFile(void)    ;
	void dumpBufferInsertionToFile(void)    ;
	
public:
    FILE *fptr ;
    string clauseFileName ;
    //-Constructor-----------------------------------------------------------------
	ClockTree(void)
			 : _pathselect(0), _bufinsert(0), _placedcc(1), _doVTA(1), _VTH_LIB_cnt(0), _FIN_CONV_Year(100) ,_aging(1), _mindccplace(0), _tcrecheck(0), _clkgating(0),
			   _dumpdcc(0), _dumpcg(0), _dumpbufins(0), _agingtcq(1.2), _agingdij(1.17), _agingtsu(1),_printpath(0),
			   _cgpercent(0.02), _pathusednum(0), _pitoffnum(0), _fftoffnum(0), _fftoponum(0),
			   _masklevel(0), _maskleng(0.5), _maxlevel(0), _nonplacedccbufnum(0), _dccatlastbufnum(0), _insertbufnum(0),
			   _totalnodenum(1), _ffusednum(0), _bufferusednum(0), _minisatexecnum(0), _gpupbound(70), _gplowbound(20), 
			   _origintc(0), _besttc(0), _tc(0), _tcupbound(0), _tclowbound(0),
			   _clktreeroot(nullptr), _firstchildrennode(nullptr), _mostcriticalpath(nullptr),
			   _timingreport(""), _timingreportfilename(""), _timingreportloc(""), _timingreportdesign(""),_dumpCNF(false), _checkCNF(false), _checkfile(false),
			   _cgfilename(""), _outputdir(""), _tcAfterAdjust(0), _printClause(false), _baseVthOffset(0), _exp(0.2),  _usingSeniorAging(false),
               _printClkNode(false), _calVTA(false), _dcc_leader(false), _dc_formulation(false), Max_timing_count(0) {}
	//-Destructor------------------------------------------------------------------
    ~ClockTree(void);
	
	//-Setter methods---------------------------------------------------------------
    void setTc_adjust( double t )               { this->_tcAfterAdjust  = t     ; }
    void setExp( double t )                     { this->_exp            = t     ; }
    void setLibCount( int c )                   { this->_VTH_LIB_cnt    = c     ; }
    void setFinYear( int y )                    { this->_FIN_CONV_Year  = y     ; }
	void setTc( double tc )                     { this->_tc             = tc    ; }
    void setTcUpperBound( double tc )           { this->_tcupbound      = tc    ; }
	void setTcLowerBound( double tc )           { this->_tclowbound     = tc    ; }
	void setOutputDirectoryPath( string path )  { this->_outputdir      = path  ; }
    void setIfVTA( bool b )                     { this->_doVTA          = b     ; }
    void setBaseVthOffset( double b )           { this->_baseVthOffset  = b     ; }
	//-Getter methods--------------------------------------------------------------
    double  getExp(void)                            { return _exp               ; }
    double  getTc_adjust(void)                      { return _tcAfterAdjust     ; }
    int     getLibCount(void)                       { return _VTH_LIB_cnt       ; }
    int     getFinYear(void)                        { return _FIN_CONV_Year     ; }
	int     getPathSelection(void)                  { return _pathselect        ; }
	int     getBufferInsertionMode(void)            { return _bufinsert         ; }
	int     getGatingProbabilityUpperBound(void)    { return _gpupbound         ; }
	int     getGatingProbabilityLowerBound(void)    { return _gplowbound        ; }
	int     getMinisatExecuteNumber(void)           { return _minisatexecnum    ; }
	long    getTotalPathNumber(void)                { return _pathlist.size()   ; }
	long    getPathUsedNumber(void)                 { return _pathusednum       ; }
	long    getPiToFFNumber(void)                   { return _pitoffnum         ; }
	long    getFFToFFNumber(void)                   { return _fftoffnum         ; }
	long    getFFToPoNumber(void)                   { return _fftoponum         ; }
	long    getTotalNodeNumber(void)                { return _totalnodenum      ; }
	long    getTotalFFNumber(void)                  { return _ffsink.size()     ; }
	long    getTotalBufferNumber(void)              { return _buflist.size()    ; }
	long    getTotalClockGatingNumber(void)         { return _cglist.size()     ; }
	long    getTotalDccNumber(void)                 { return _dcclist.size()    ; }
	long    getNonPlacedDccBufferNumber(void)       { return _nonplacedccbufnum ; }
	long    getDccPlacedAtLastBufferNumber(void)    { return _dccatlastbufnum   ; }
	long    getFFUsedNumber(void)                   { return _ffusednum         ; }
	long    getBufferUsedNumber(void)               { return _bufferusednum     ; }
	long    getMaskByLevel(void)                    { return _masklevel         ; }
	long    getMaxTreeLevel(void)                   { return _maxlevel          ; }
	long    getBufferInsertionNumber(void)          { return _insertbufnum      ; }
	double  getMaskByLength(void)                   { return _maskleng          ; }
	double  getClockGatingPercent(void)             { return _cgpercent         ; }
	double  getTcqAgingRate(void)                   { return _agingtcq          ; }
	double  getDijAgingRate(void)                   { return _agingdij          ; }
	double  getTsuAgingRate(void)                   { return _agingtsu          ; }
	double  getOriginTc(void)                       { return _origintc          ; }
	double  getBestTc(void)                         { return _besttc            ; }
	double  getTc(void)                             { return _tc                ; }
	double  getTcUpperBound(void)                   { return _tcupbound         ; }
	double  getTcLowerBound(void)                   { return _tclowbound        ; }
    double  getBaseVthOffset(void)                  { return _baseVthOffset     ; }
	string  getTimingReportFileName(void)           { return _timingreportfilename; }
	string  getTimingReportLocation(void)           { return _timingreportloc   ; }
	string  getTimingReportDesign(void)             { return _timingreportdesign; }
	string  getClockGatingFileName(void)            { return _cgfilename        ; }
	string  getOutputDirectoryPath(void)            { return this->_outputdir   ; }
	ClockTreeNode   *getFirstChildrenNode(void)     { return _firstchildrennode ; }
	CriticalPath    *getMostCriticalPath(void)      { return _mostcriticalpath  ; }
	vector<CriticalPath *>& getPathList(void)       { return _pathlist          ; }
    vector<VTH_TECH*>&      getLibList(void)        { return _VthTechList       ; }
    set< pair<ClockTreeNode*,ClockTreeNode*> >& getVTASet(void) { return _setVTALeader ; }
    set< pair<int,int> >& getDCCSet(void) { return _setDCC ; }
	//-- Bool Attr Access -------------------------------------------------------
    bool ifCalVTA(void)                             { return _calVTA            ; }
    bool ifdccleader(void)                          { return _dcc_leader        ; }
    bool ifprintNode(void)                          { return _printClkNode      ; }
    bool ifCheckFile(void)                          { return _checkfile         ; }
    bool ifCheckCNF(void)                           { return _checkCNF          ; }
    bool ifDumpCNF(void)                            { return _dumpCNF           ; }
    bool ifprintPath(void)                          { return _printpath         ; }
	bool ifPlaceDcc(void)                           { return _placedcc          ; }
	bool ifAging(void)                              { return _aging             ; }
	bool ifMinDccNumber(void)                       { return _mindccplace       ; }
	bool ifTcRecheck(void)                          { return _tcrecheck         ; }
	bool ifClockGating(void)                        { return _clkgating         ; }
	bool ifDumpDcc(void)                            { return _dumpdcc           ; }
	bool ifDumpClockGating(void)                    { return _dumpcg            ; }
	bool ifDumpBufferInsertion(void)                { return _dumpbufins        ; }
    bool ifdoVTA(void)                              { return _doVTA             ; }
    //---Setting ----------------------------------------------------------------
	int     checkParameter(int, char **, string *);//read parameter from cmd line
    void    readParameter(void);                   //read parameter from text file
    void    InitClkTree(void);
    //---Parser -----------------------------------------------------------------
	void    parseTimingReport(void);
    
    //---Compared: Clock Gating -------------------------------------------------
	void    clockGatingCellReplacement(void);
    
    //---Buffer Insertion -----------------------------------------------------
    void    bufferInsertion(void);
    void    minimizeBufferInsertion(void);
    
    //---Refine: Minimize DCC Count ------------------------------------------------
    void    minimizeDccPlacement(void);
    
   
    //---DCC Constraint ------------------------------------------------------------
    //Constraint: At most 1 dcc exist along a clock path
	void    dccConstraint(void);
    void    dccPlacementByMasked(void);
    //---Leader Constraint ---------------------------------------------------------
    //Constraint: At most 1 leader exist along a clock path
    void    VTAConstraint(void);
    void    VTAConstraintFFtoFF( CriticalPath* );
    void    VTAConstraintPItoFF( CriticalPath* );
    void    VTAConstraintFFtoPO( CriticalPath* );
    
    //---DCC-Leader Constraint ----------------------------------------------------
    //Constraints: Leader must be put in the downstream of dcc
    void    DCCLeaderConstraint(void);
    void    DCCLeaderConstraintFFtoFF( CriticalPath* );
    void    DCCLeaderConstraintPItoFF( CriticalPath* );
    void    DCCLeaderConstraintFFtoPO( CriticalPath* );
    
    
    
	void    genDccPlacementCandidate(void);
    
    //---Timing Constraint---------------------------------------------------------
	double  timingConstraint(void);//A
    void    timingConstraint_doDCC_ndoVTA (CriticalPath*, bool update = false );//A-B-1, Not Done
    void    timingConstraint_doDCC_doVTA  (CriticalPath*, bool update = false );//A-B-2, Done
    void    timingConstraint_ndoDCC_doVTA (CriticalPath*, bool update = false );//A-B-3,
    double  timingConstraint_ndoDCC_ndoVTA(CriticalPath*, bool update = false, bool genclause = 1 );//A-B-4, Done
	

    double  timingConstraint_givDCC_givVTA(CriticalPath *,
                                           double stDCCType , double edDCCType,
                                           ClockTreeNode *n1, ClockTreeNode *n2,
                                           int stLibIndex   , int edLibIndex ,
                                           ClockTreeNode *stHeader, ClockTreeNode *edHeader
                                            );//A-B-2-C-D   Done
    void    timingConstraint_givDCC_doVTA(  CriticalPath *,
                                            double stDCCType  , double edDCCType,
                                            ClockTreeNode *n1, ClockTreeNode *n2
                                         );//A-B-2-C    Done
    void    timingConstraint_givDCC_ndoVTA( CriticalPath *,
                                            double stDCCType  , double edDCCType,
                                            ClockTreeNode *n1, ClockTreeNode *n2
                                           );//A-B-1-C
    double  timingConstraint_ndoDCC_givVTA( );//Not Done
    //---Clause ------------------------------------------------------------------
    void    writeClause_givDCC( string &clause, ClockTreeNode* node, double DCCType  );
    void    writeClause_givVTA( string &clause, ClockTreeNode* node, int    LibIndex );
    //---VTA-related -------------------------------------------------------------
    double  getAgingRate_givDC_givVth( double DC, int LibIndex, bool initial = false ) ;
    //---Timing-related ----------------------------------------------------------
    void    adjustOriginTc(void)        ;
    void    updateAllPathTiming(void)   ;
    void    tcRecheck(void)             ;
    double  calClkLaten_givDcc_givVTA   (vector<ClockTreeNode*>clkpath, double DCCType, ClockTreeNode* DCCLoc, int LibIndex, ClockTreeNode* Header );
    
    //---Dumper ------------------------------------------------------------------
	void    dumpClauseToCnfFile(void)      ;
    void    dumpCNF(void)                  ;
    void    dumpToFile(void)               ;
    void    dumpDccVTALeaderToFile(void)   ;
    double  UpdatePathTiming(CriticalPath*,bool update = true, bool DCCVTA = true, bool aging = true );
	//---Printer --------------------------------------------------------------------
	void    printClockTree(void);
	void    printSingleCriticalPath(long, bool verbose = 1);
	void    printSingleCriticalPath(char, string, bool verbose = 1);
	void    printAllCriticalPath(void);
	void    printDccList(void);
    void    printVTAList(void);
	void    printClockGatingList(void);
	void    printBufferInsertedList(void);
    void    printSpace(long common);
    void    printClauseCount();
    //---Printer 2 ------------------------------------------------------------------
    void    printPath(void);
    void    printPath(int);
    void    printPath(CriticalPath*, int Mode );
    void    printPath_givFile(   CriticalPath*, bool doDCCVTA, bool aging = true, bool givTc = true );
    void    printFFtoFF_givFile( CriticalPath*, bool doDCCVTA, bool aging = true );
    void    printPItoFF_givFile( CriticalPath*, bool doDCCVTA, bool aging = true );
    void    printFFtoPO_givFile( CriticalPath*, bool doDCCVTA, bool aging = true );
    void    printPathSlackTiming(CriticalPath*, double ci, double cj, bool aging = true );
    void    printAssociatedCriticalPathAtEndPoint( CriticalPath* path  , bool doDCCVTA = true, bool aging = true );
    void    printAssociatedCriticalPathAtStartPoint( CriticalPath* path, bool doDCCVTA = true, bool aging = true  );
    void    printClkNodeFeature( ClockTreeNode*,bool ) ;
    //---Vth Lib --------------------------------------------------------------------
    double  calConvergentVth( double dc , double Exp = 0.2 ) ;
    double  calSv( double dc, double VthOffset, double VthFin ) ;
    //---Other ----------------------------------------------------------------------
    bool    checkDCCVTAConstraint(void);
    bool    checkDCCVTAConstraint_givPath(CriticalPath*);
    void    readDCCVTAFile(void);
    void    readDCCVTAFile2(void);
    void    CheckTiming_givCNF();
    void    CheckTiming_givFile();
    void    removeCNFFile(void);
    void    execMinisat(void);
    void    tcBinarySearch(void);
    void    printNodeLayerSpacel(int);
    void    printNodeLayerSpace(int);
    void    printClockNode(void);
    void    printClockNode(ClockTreeNode*, int layer = 0 );
    void    checkCNF(void);
    void    calVTABufferCount();
    void    calVTABufferCountByFile();
    bool    clauseJudgement( vector<string>&, bool * );
    ClockTreeNode *searchClockTreeNode(string);
    ClockTreeNode *searchClockTreeNode(long);
    vector<CriticalPath *> searchCriticalPath(char, string);
    vector<ClockTreeNode *> getFFChildren(ClockTreeNode *);
};

inline double getAgingRateByDutyCycle(double dc)
	{ return (1 + (((-0.117083333333337) * (dc) * (dc)) + (0.248750000000004 * (dc)) + 0.0400333333333325)); }

#endif  // CLOCKTREE_H
