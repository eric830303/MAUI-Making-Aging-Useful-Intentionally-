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
#include "clocktree2.h"
#include "clocktree3.h"
#include <assert.h>
#include "utility.h"
#include <map>
#include <set>

// Factor of DCC delay based on logic effort
#define DCCDELAY20PA    (1.33)		// 20% DCC Delay
#define DCCDELAY40PA    (1.33)		// 40% DCC Delay
#define DCCDELAY50PA    (1.33)		// 50% DCC Delay
#define DCCDELAY80PA    (1.67)		// 80% DCC Delay

//#define DC_20DCC        (0.22)
//#define DC_40DCC        (0.44)
//#define DC_80DCC        (0.83)
#define PRECISION       (4)			// Precision of real number
#define PATHMASKPERCENT (1.0)	    // Mask how many percentage of critical path (0~1)
#define TenYear_Sec     (315360000)
#define COF_A           (0.0039/2)
#define CTN             ClockTreeNode
#define CP              CriticalPath
#define CT              ClockTree

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
	int		_program_ctl;
	bool    _placedcc, _aging, _mindccplace, _tcrecheck, _clkgating, _dumpdcc, _dumpcg, _dumpbufins, _doVTA;
	bool    _usingSeniorAging, _printClkNode ;
	long    _pathusednum, _pitoffnum, _fftoffnum, _fftoponum, _nonplacedccbufnum;
	long    _totalnodenum, _ffusednum, _bufferusednum, _dccatlastbufnum;
	long    _masklevel, _maxlevel, _insertbufnum, _dcc_constraint_ctr, _leader_constraint_ctr ;
	double  _maskleng, _cgpercent;
	double  _tcAfterAdjust ;
    
    //--- Control of function ----------------------------------------------------------
    bool    _printClause, _calVTA, _dcc_leader, _bufinsertion ;
    bool    _dc_formulation, _printCP        ;
    
    //--- Timing-related ----------------------------------------------------------------
    double  _origintc, _besttc, _tc         ;
    double  _tcupbound, _tclowbound         ;
    double  _agingtcq, _agingdij, _agingtsu ;
    
    //--- FileName-Related --------------------------------------------------------------
	string _timingreport, _timingreportfilename, _timingreportloc, _timingreportdesign;
	string _cgfilename, _outputdir;
	
	
	double DC_1, DC_2, DC_N, DC_3;
	double DC_1_age, DC_2_age, DC_N_age, DC_3_age;
	
	
    //--- Benchmark-Related --------------------------------------------------------------
	CTN *_clktreeroot, *_firstchildrennode;
	CP  *_mostcriticalpath;
    
    //-- Vec-Container ------------------------------------------------------------------
	vector< CP* > _pathlist;
    
    //-- Map-Container ------------------------------------------------------------------
	map   < string, CTN* > _ffsink     ;
    map   < string, CTN* > _buflist    ;
    map   < string, CTN* > _cglist     ;
    map   < string, CTN* > _dcclist    ;
    map   < string, CTN* > _VTAlist    ;
    
    //-- Set-Container ------------------------------------------------------------------
    set   < tuple< CTN*, double, int > >    _DccLeaderset ;
    set   < pair< CTN*, CTN* >  >           _setVTALeader ;
    set   < pair< int, int >    >           _setDCC       ;
	set   < string >  _VTAconstraintlist    ;
    set   < string >  _dccconstraintlist    ;
    set   < string >  _timingconstraintlist ;
	
    //-- HTV ---------------------------------------------------------------------------
    int     _VTH_LIB_cnt    ;
    int     _FIN_CONV_Year  ;
    double  _baseVthOffset  ;
    vector< VTH_TECH* > _VthTechList ;
    double  _exp            ;
    double  _nominal_agr[8] ;
    double  _HTV_agr[8]     ;
	double  _HTV_fresh      ;
    
    
    long long int Max_timing_count;
	bool ifSkipLine(string)                 ;
	bool AnotherSolution(void)              ;
	void pathTypeChecking(void)             ;
	void recordClockPath(char)              ;
	void checkFirstChildrenFormRoot(void)   ;
	void initTcBound(void)                  ;
	void genDccConstraintClause(vector<vector<long> > *);
	void genClauseByDccVTA(CTN*, string *, double, int);
	void deleteClockTree(void)              ;
	//-- Dumper ------------------------------------------------------------------
	void dumpDccListToFile(void)            ;
	void dumpClockGatingListToFile(void)    ;
	void dumpBufferInsertionToFile(void)    ;
	
public:
    FILE *fptr ;
    string clauseFileName ;
    long refine_time ;
    //-Constructor-----------------------------------------------------------------
	ClockTree(void)
			 : _pathselect(0), _bufinsert(0), _placedcc(1), _doVTA(1), _VTH_LIB_cnt(0), _FIN_CONV_Year(100) ,_aging(1), _mindccplace(0), _tcrecheck(0), _clkgating(0),
			   _dumpdcc(0), _dumpcg(0), _dumpbufins(0), _agingtcq(1.2), _agingdij(1.17), _agingtsu(1),
			   _cgpercent(0.02), _pathusednum(0), _pitoffnum(0), _fftoffnum(0), _fftoponum(0),
			   _masklevel(0), _maskleng(0.5), _maxlevel(0), _nonplacedccbufnum(0), _dccatlastbufnum(0), _insertbufnum(0),
			   _totalnodenum(1), _ffusednum(0), _bufferusednum(0), _minisatexecnum(0), _gpupbound(70), _gplowbound(20), 
			   _origintc(0), _besttc(0), _tc(0), _tcupbound(0), _tclowbound(0),
			   _clktreeroot(nullptr), _firstchildrennode(nullptr), _mostcriticalpath(nullptr),
			   _timingreport(""), _timingreportfilename(""), _timingreportloc(""), _timingreportdesign(""),
			   _cgfilename(""), _outputdir(""), _tcAfterAdjust(0), _printClause(false), _baseVthOffset(0), _exp(0.2),  _usingSeniorAging(false),
               _printClkNode(false), _calVTA(false), _dcc_leader(false), _dc_formulation(false), Max_timing_count(0), refine_time(100), _dcc_constraint_ctr(0), _leader_constraint_ctr(0), _printCP(false), _program_ctl(0), DC_1(0.2), DC_2(0.4), DC_3(0.8), DC_N(0.5), DC_1_age(0.22), DC_2_age(0.44), DC_3_age(0.83), DC_N_age(0.5) {}
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
	void setDC1( double d )   					{ this->DC_1 			= d 	; }
	void setDC2( double d )   					{ this->DC_2 			= d 	; }
	void setDC3( double d )   					{ this->DC_3 			= d 	; }
	void setDCN( double d )   					{ this->DC_N 			= d 	; }
	void setDC1_Ag( double d )   				{ this->DC_1_age		= d 	; }
	void setDC2_Ag( double d )   				{ this->DC_2_age 		= d 	; }
	void setDC3_Ag( double d )   				{ this->DC_3_age 		= d 	; }
	void setDCN_Ag( double d )   				{ this->DC_N_age 		= d 	; }
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
    long    getDCCConstraintCtr(void)               { return _dcc_constraint_ctr; }
    long    getHTVConstraintCtr(void)               { return _leader_constraint_ctr; }
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
	vector<CP*>& getPathList(void)       { return _pathlist          ; }
    vector<VTH_TECH*>&      getLibList(void)        { return _VthTechList       ; }
    set< pair<CTN*,CTN*> >& getVTASet(void) { return _setVTALeader ; }
    set< pair<int,int> >& getDCCSet(void) { return _setDCC ; }
	//-- Bool Attr Access -------------------------------------------------------
    bool ifprintCP(void)                            { return _printCP           ; }
    bool ifCalVTA(void)                             { return _calVTA            ; }
    bool ifdccleader(void)                          { return _dcc_leader        ; }
    bool ifprintNode(void)                          { return _printClkNode      ; }
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
	void    minimizeBufferInsertion2(void);
	void    minimizeBufferInsertion2(ClockTreeNode*, double thred = 0.5);
	long    calBufInserOrClockGating( int status = 0 );
    
    //---Refine: Minimize DCC Count ------------------------------------------------
    void    minimizeDccPlacement(void);//Actually, minimal rather than minimum
    void    minimizeLeader(void);
    void    minimizeLeader2(double tc = 0); //minimum
    
   
    //---DCC Constraint ------------------------------------------------------------
    //Constraint: At most 1 dcc exist along a clock path
	void    dccConstraint(void);
    void    dccPlacementByMasked(void);
    //---Leader Constraint ---------------------------------------------------------
    //Constraint: At most 1 leader exist along a clock path
    void    VTAConstraint(void);
    void    VTAConstraintFFtoFF( CP* );
    void    VTAConstraintPItoFF( CP* );
    void    VTAConstraintFFtoPO( CP* );
    
    //---DCC-Leader Constraint ----------------------------------------------------
    //Constraints: Leader must be put in the downstream of dcc
    void    DCCLeaderConstraint(void);
    void    DCCLeaderConstraintFFtoFF( CP* );
    void    DCCLeaderConstraintPItoFF( CP* );
    void    DCCLeaderConstraintFFtoPO( CP* );
    
    
    
	void    genDccPlacementCandidate(void);
    
    //---Timing Constraint---------------------------------------------------------
	long    timingConstraint( void );
    void    timingConstraint_doDCC_ndoVTA ( CP*, bool aging = 1 );
    void    timingConstraint_doDCC_doVTA  ( CP*, bool aging = 1 );
    void    timingConstraint_ndoDCC_doVTA ( CP*, bool aging = 1 );
    double  timingConstraint_ndoDCC_ndoVTA( CP*, bool aging = 1 );
	

    double  timingConstraint_givDCC_givVTA( CP*, double, double, CTN*, CTN*, int, int,CTN*, CTN*ed, bool aging = 1);
    void    timingConstraint_givDCC_doVTA(  CP*, double, double, CTN*, CTN*, bool aging = 1);
    void    timingConstraint_givDCC_ndoVTA( CP*, double, double, CTN*, CTN*, bool aging = 1);
    //---Clause ------------------------------------------------------------------
    void    writeClause_givDCC( string &clause, CTN* node, double DCCType  );
    void    writeClause_givVTA( string &clause, CTN* node, int    LibIndex );
    //---VTA-related -------------------------------------------------------------
	double  getAgingRate_givDC_givVth( double DC, int LibIndex, bool initial = 0, bool cAging = 1 ) ;
    //---Timing-related ----------------------------------------------------------
    void    adjustOriginTc( void )        ;
    void    updateAllPathTiming( void )   ;
    void    tcRecheck( void )             ;
    double  calClkLaten_givDcc_givVTA   (vector<CTN*> path, double DC, CTN* Loc1, int Lib, CTN* Loc2, bool aging=1, bool set=0, bool cPV=0 );
    
    //---Dumper ------------------------------------------------------------------
	void    dumpClauseToCnfFile(void)      ;
    void    dumpCNF(void)                  ;
    void    dumpToFile(void)               ;
    void    dumpDccVTALeaderToFile(void)   ;
    double  UpdatePathTiming(CP*,bool update = true, bool DCCVTA = true, bool aging = true, bool set = false, bool cPV = false );
    
	//---Printer --------------------------------------------------------------------
	void    printDccList(void)          ;
    void    printVTAList(void)          ;
	void    printClockGatingList(void)  ;
	void    printBufferInsertedList(void);
    void    printSpace(long common)     ;
    void    printClauseCount(void)      ;
    
    //--- "-print=path" ------------------------------------------------------------------
    void    printPath(void);
    void    printPath(int);
    void    printPath(CP*, int Mode );
    void    printPath_givFile(   CP*, bool doDCCVTA, bool aging = true, bool givTc = true );
    void    printFFtoFF_givFile( CP*, bool doDCCVTA, bool aging = true );
    void    printPI_PO_givFile(  CP*, bool doDCCVTA, bool aging = true );
    void    printPathSlackTiming(CP*, double ci, double cj, bool aging = true );
    void    printAssociatedCriticalPathAtEndPoint(   CP* path, bool doDCCVTA = true, bool aging = true );
    void    printAssociatedCriticalPathAtStartPoint( CP* path, bool doDCCVTA = true, bool aging = true  );
    void    printClkNodeFeature( CTN*, bool ) ;
    //---Vth Lib --------------------------------------------------------------------
    double  calConvergentVth( double dc , double Exp = 0.2 ) ;
    double  calSv( double dc, double VthOffset, double VthFin ) ;
    //---Other ----------------------------------------------------------------------
    void    readDCCVTAFile( string="./setting/DccVTA.txt", int status=0 );
    int     calBufChildSize( CTN* )             ;
    bool    checkDCCVTAConstraint( void )       ;
    bool    checkDCCVTAConstraint_givPath( CP* );
    void    removeCNFFile(void)                 ;
    bool    DoOtherFunction( void )             ;
    void    execMinisat( void )                 ;
    bool    tcBinarySearch( void )              ;
    void    printFinalResult( void )            ;
    long    calVTABufferCount(       bool=0 )   ;
    void    calVTABufferCountByFile( void   )   ;
    bool    SolveCNFbyMiniSAT( double, bool=0 ) ;
    void    EncodeDccLeader( double )           ;
    CTN*    searchClockTreeNode( string )       ;
    CTN*    searchClockTreeNode( long   )       ;
	void    bufinsertionbyfile();
    vector<CP*>     searchCriticalPath( char, string );
    vector<CTN*>    getFFChildren(CTN*)         ;
    
    //---- "-checkFile" ------------------------------------------------------------------
    void    CheckTiming_givFile(void)           ;
    
    //---- "-print=Node" ----------------------------------------------------------------
    void    printNodeLayerSpacel( int )         ;
    void    printNodeLayerSpace(  int )         ;
    void    printClockNode( void )              ;
    void    printClockNode( CTN*, int=0 )       ;
	void    printLeaderLayer()                  ;
    //---- "-checkCNF" ------------------------------------------------------------------
    void    checkCNF(void)                      ;
    bool    clauseJudgement( vector<string>&, bool * );

    //---- "-print=CP" ------------------------------------------------------------------
    void    printPathCriticality(void);
    void    printDCCList(void);
    void    printAssociatedDCCLeaderofPath( CP* path );
    //---- "-analysis" -------------------------------------------------------------------
    pair<int,int> FindDCCLeaderInPathVector( CP* );
    void          FindDCCLeaderInPathVector( set<CTN*>&, CP* );
	void          RemoveDCCandSeeResult2( CTN*, int=1);
    void    Analysis(void);
    void    SortCPbySlack(bool,bool=true);
    CT&     DisplayDCCLeaderinSet(        set<CTN*>&, int=1 );
    CT&     DisplayDCCLeaderinVec(     vector<CTN*>&, int=1 );
    void    RemoveDCCandSeeResult(     vector<CTN*>&, int=1 );
    int     printCP_before_After( CP*, vector<CTN*>& );
    bool    NodeExistInVec(      CTN*, vector<CTN*>& );
	void    PathFailReasonAnalysis( );
	void    PathFailReasonAnalysis( CP* );
	bool    PathIsPlacedDCCorLeader(    CP*, int=1);
	void    getPathType( string &PathType, CP* pptr );
	bool    PathReasonable( CP* pptr);
	void    getNodeSide( string &Side, CTN*node, CP* pptr );
	//---- "-CG" clock gating--------------------------------------------------------------
	void    clockgating();
	void    GatedCellRecursive( CTN*, double thred = 0.5 );
	//---- "-PV" -------------------------------------------------------------------
	void	PV_simulation();
	void    PV_instantiation( double LB=97, double UB=103 );
	double	PV_Tc_estimation();
	
};

inline double getAgingRateByDutyCycle(double dc)
	{ return (1 + (((-0.117083333333337) * (dc) * (dc)) + (0.248750000000004 * (dc)) + 0.0400333333333325)); }

#endif  // CLOCKTREE_H
