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
#include <assert.h>
#include "utility.h"
#include <map>
#include <set>

// Factor of DCC delay based on logic effort
#define DCCDELAY20PA    (1.33)		// 20% DCC Delay
#define DCCDELAY40PA    (1.33)		// 40% DCC Delay
#define DCCDELAY50PA    (1.33)		// 50% DCC Delay
#define DCCDELAY80PA    (1.67)		// 80% DCC Delay

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
    double _VTH_CONVGNT[4] ;//Convergent Vth value over a long period
    double _Sv[4]          ;//Sv value of 20/40/50/80
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
	bool    _placedcc, _aging, _mindccplace, _tcrecheck, _clkgating, _dumpdcc, _dumpcg, _dumpbufins, _doVTA ;
	long    _pathusednum, _pitoffnum, _fftoffnum, _fftoponum, _nonplacedccbufnum;
	long    _totalnodenum, _ffusednum, _bufferusednum, _dccatlastbufnum;
	long    _masklevel, _maxlevel, _insertbufnum;
	double  _maskleng, _cgpercent;
    //Timing-related attribute
	double _origintc, _besttc, _tc, _tcupbound, _tclowbound, _agingtcq, _agingdij, _agingtsu;

    //filename-related attribute
	string _timingreport, _timingreportfilename, _timingreportloc, _timingreportdesign;
	string _cgfilename, _outputdir;
    
    //circuit-related attribute
	ClockTreeNode   *_clktreeroot, *_firstchildrennode;
	CriticalPath    *_mostcriticalpath;
	vector<CriticalPath *> _pathlist;
	map<string, ClockTreeNode *> _ffsink, _buflist, _cglist, _dcclist;
    
    //Constraint-related attribute
	set<string> _VTAconstraintlist, _dccconstraintlist, _timingconstraintlist;
	
    //VTA-related attribute
    int     _VTH_LIB_cnt    ;
    int     _FIN_CONV_Year  ;
    vector< VTH_TECH* > _VthTechList ;
    
    
	bool ifSkipLine(string)                 ;
    bool AnotherSolution(void)              ;
	void pathTypeChecking(void)             ;
	void recordClockPath(char)              ;
	void checkFirstChildrenFormRoot(void)   ;
	void initTcBound(void)                  ;
	void genDccConstraintClause(vector<vector<long> > *);
	void genClauseByDccVTA(ClockTreeNode *, string *, int, int);
	void deleteClockTree(void)              ;
	void dumpDccListToFile(void)            ;
	void dumpClockGatingListToFile(void)    ;
	void dumpBufferInsertionToFile(void)    ;
    double updatePathTimingWithDcc(CriticalPath *, bool update = 0);
	
public:
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
			   _cgfilename(""), _outputdir("") {}
	//-Destructor------------------------------------------------------------------
    ~ClockTree(void);
	
	//-Setter methods---------------------------------------------------------------
    void setLibCount( int c )                   { this->_VTH_LIB_cnt    = c     ; }
    void setFinYear( int y )                    { this->_FIN_CONV_Year  = y     ; }
	void setTc( double tc )                     { this->_tc             = tc    ; }
    void setTcUpperBound( double tc )           { this->_tcupbound      = tc    ; }
	void setTcLowerBound( double tc )           { this->_tclowbound     = tc    ; }
	void setOutputDirectoryPath( string path )  { this->_outputdir      = path  ; }
    void setIfVTA( bool b )                     { this->_doVTA          = b     ; }
	//-Getter methods--------------------------------------------------------------
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
	string  getTimingReportFileName(void)           { return _timingreportfilename; }
	string  getTimingReportLocation(void)           { return _timingreportloc   ; }
	string  getTimingReportDesign(void)             { return _timingreportdesign; }
	string  getClockGatingFileName(void)            { return _cgfilename        ; }
	string  getOutputDirectoryPath(void)            { return this->_outputdir   ; }
	ClockTreeNode   *getFirstChildrenNode(void)     { return _firstchildrennode ; }
	CriticalPath    *getMostCriticalPath(void)      { return _mostcriticalpath  ; }
	vector<CriticalPath *>& getPathList(void)       { return _pathlist          ; }
    vector<VTH_TECH*>&      getLibList(void)        { return _VthTechList       ; }
	//-- Bool Attr Access -------------------------------------------------------
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
	int  checkParameter(int, char **, string *);//read parameter from cmd line
    void readParameter(void);                   //read parameter from text file
    
    //---Parser -----------------------------------------------------------------
	void parseTimingReport(void);
    
    //---Compared: Clock Gating -------------------------------------------------
	void clockGatingCellReplacement(void);
    
    //---Buffer Insertion -----------------------------------------------------
    void bufferInsertion(void);
    void minimizeBufferInsertion(void);
    
    //---Refine: Minimize DCC Count ------------------------------------------
    void minimizeDccPlacement(void);
    
   
    //---DCC Constraint ---------------------------------------------------------
	void dccConstraint(void);
    void dccPlacementByMasked(void);
    //---VTA Constraint ---------------------------------------------------------
    void VTAConstraint(void);
    void VTAConstraintFFtoFF( CriticalPath* );
    void VTAConstraintPItoFF( CriticalPath* );
    void VTAConstraintFFtoPO( CriticalPath* );
    
	void genDccPlacementCandidate(void);
    
    //---Timing Constraint---------------------------------------------------------
	void    timingConstraint(void);//A
    void    timingConstraint_doDCC_ndoVTA (CriticalPath*);//A-B-1, Not Done
    void    timingConstraint_doDCC_doVTA  (CriticalPath*);//A-B-2, Done
    void    timingConstraint_ndoDCC_doVTA (CriticalPath*);//A-B-3,
    void    timingConstraint_ndoDCC_ndoVTA(CriticalPath * , bool update = 0, bool genclause = 1 );//A-B-4, Done
	

    void    timingConstraint_givDCC_givVTA(CriticalPath *,
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
    void    timingConstraint_ndoDCC_givVTA( );//Not Done
    //---Clause ------------------------------------------------------------------
    void    writeClause_givDCC( string &clause, ClockTreeNode* node, double DCCType  );
    void    writeClause_givVTA( string &clause, ClockTreeNode* node, int    LibIndex );
    //---VTA-related -------------------------------------------------------------
    double  getAgingRate_givDC_givVth( double DC, int LibIndex ) ;
    //---Timing-related ----------------------------------------------------------
    void    adjustOriginTc(void)        ;
    void    updateAllPathTiming(void)   ;
    void    tcRecheck(void)             ;
    double  calClkLaten_givDcc_givVTA   (vector<ClockTreeNode*>clkpath, double DCCType, ClockTreeNode* DCCLoc, int LibIndex, ClockTreeNode* Header );
    
    //---Dumper -------------------------------------------------------------------
	void    dumpClauseToCnfFile(void)      ;
    void    dumpToFile(void)               ;
	
	//---Printer --------------------------------------------------------------------
	void    printClockTree(void);
	void    printSingleCriticalPath(long, bool verbose = 1);
	void    printSingleCriticalPath(char, string, bool verbose = 1);
	void    printAllCriticalPath(void);
	void    printDccList(void);
	void    printClockGatingList(void);
	void    printBufferInsertedList(void);
    
    //---Vth Lib --------------------------------------------------------------------
    double  calConvergentVth( double dc, double VthOffset ) ;
    double  calSv( double dc, double VthOffset, double VthFin ) ;
    //---Other ----------------------------------------------------------------------
    void    execMinisat(void);
    void    tcBinarySearch(void);
    ClockTreeNode *searchClockTreeNode(string);
    ClockTreeNode *searchClockTreeNode(long);
    vector<CriticalPath *> searchCriticalPath(char, string);
    vector<ClockTreeNode *> getFFChildren(ClockTreeNode *);
};

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
    if( Libindex != -1 )
        assert( Libindex >= this->getLibList().size() )  ;
    //---- Sv -------------------------------------------------------
    double Sv = 0 ;
    if( Libindex != -1 )
    {
        if( DC == 0.2 )
            Sv = this->getLibList().at(Libindex)->_Sv[0] ;
        else if( DC == 0.4 )
            Sv = this->getLibList().at(Libindex)->_Sv[1] ;
        else if( DC == 0.5 )
            Sv = this->getLibList().at(Libindex)->_Sv[2] ;
        else if( DC == 0.8 )
            Sv = this->getLibList().at(Libindex)->_Sv[3] ;
        else
        {
            cerr << "[Error] Irrecognized duty cycle in func \"getAgingRate_givDC_givVth (double DC, int LibIndex )\"    \n" ;
            return -1 ;
        }
    }
    else Sv = 0 ;
    
    //---- Vth offset -----------------------------------------------
    double Vth_offset = 0 ;
    if( Libindex != -1 )
        Vth_offset = this->getLibList().at(Libindex)->_VTH_OFFSET ;
    
    //---- Aging rate -----------------------------------------------
    double Vth_nbti = ( 1 - Sv*Vth_offset )*( COF_A )*( pow( DC*( TenYear_Sec ), 0.2) );
    return (1 + Vth_nbti*2) ;
    
}
inline double getAgingRateByDutyCycle(double dc)
	{ return (1 + (((-0.117083333333337) * (dc) * (dc)) + (0.248750000000004 * (dc)) + 0.0400333333333325)); }

#endif  // CLOCKTREE_H
