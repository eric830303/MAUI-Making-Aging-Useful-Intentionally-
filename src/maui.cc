//////////////////////////////////////////////////////////////
//
// Main Source File
//
// File name: maui.cpp
// Author: Ting-Wei Chang
// Date: 2017-07
//
//////////////////////////////////////////////////////////////

#include "clocktree.h"
#include "utility.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cinttypes>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"
using namespace std;

int main(int argc, char **argv)
{
	string message;
	ClockTree circuit;
	
	// Check commands from input
	if( circuit.checkParameter(argc, argv, &message) != 0 )//-1 mean error
	{
		if( message.empty() )
		{	
			// Commands & Help message
			cout << "\033[33m[Usage]: program <option...> <timing report file>\033[0m\n";
			cout << "  <option>:\n";
			cout << "      -nondcc                Don't consider placing any DCC in clock tree. (default disable)\n";
			cout << "      -nonaging              Don't consider any aging in clock tree. (default disable)\n";
            cout << "      -nonVTA                Don't do Vth assignment\n";
            cout << "      -dcc_leader            Leader is in the downstream part of dcc\n";
            cout << "      -dump=SAT_CNF          Decode other CNF files and dump its DCC/Leader Deployment/Selection\n";
            cout << "      -print=path            Print the pipeline\n";
            cout << "      -print=Clause          Dump clauses while execution\n";
            cout << "      -print=CP              print associated DCC/Leader deployment of top 10 CP\n";
            cout << "      -dc_for                formulat the situation that dcc impacted by leader\n";
            cout << "      -checkCNF              Check the DCC/Leader deployment/Selection, based on given CNF output file\n";
            cout << "      -checkFile             Check the DCC/Leader deployment/Selection, based on given DccVTA.txt \n";
            cout << "      -calVTA                Calculate HTV Buffer # of associated Leader, based on given DccVTA.txt \n";
            cout << "      -aging=Senior           \n";
			cout << "      -mindcc                Minimize the number of DCCs placed in clock tree. (default disable)\n";
			cout << "                               Enable when \'-nondcc\' option enable.\n";
			cout << "      -tc_recheck            Check Tc again after binary search. (default disable)\n";
			cout << "      -mask_leng [ratio]     Set the length of mask on clock path. [ratio] = 0~1. (default [ratio] = 0.5)\n";
			cout << "      -mask_level [num]      Mask number of clock tree level from bottom. [num] >= 0. (default [num] = 0)\n";
			cout << "      -agingrate_tcq [rate]  Set the aging rate of Tcq. [rate] > 0. (default [rate] = 1.2)\n";
			cout << "      -agingrate_dij [rate]  Set the aging rate of Dij. [rate] > 0. (default [rate] = 1.17)\n";
			cout << "      -agingrate_tsu [rate]  Set the aging rate of Tsu. [rate] > 0. (default [rate] = 1)\n";
			cout << "      -cg_percent [ratio]    Set the percentage of clock gating cells replacement. [ratio] = 0~1. (default [ratio] = 0.02)\n";
			cout << "      -gp_upbound [num]      Set the upperbound probability of clock gating. [num] = 0~100. (default [num] = 70)\n";
			cout << "      -gp_lowbound [num]     Set the lowerbound probability of clock gating. [num] = 0~100. (default [num] = 20)\n";
			cout << "      -gatingfile=[file]     Given the location of clock gating cells. (default None)\n";
			cout << "      -bufinsert=[choice]    Show how many buffers inseted by given a Tc. (default None).\n";
			cout << "                               -bufinsert=insert    : just buffer insertion.\n";
			cout << "                               -bufinsert=min_insert: minimize buffer insertion.\n";
			cout << "      -bufinsert=file        Do buffer insertion based on the Tc given in the file DccVTA.txt.\n";
			cout << "      -path=[choice]         Choice which category of path you want to consider.\n";
			cout << "                               -path=all   : consider all types of path. (default)\n";
			cout << "                               -path=pi_ff : consider input port to FF and FF to FF two types of path.\n";
			cout << "                               -path=onlyff: consider only FF to FF path.\n";
			cout << "      -CG					  Do clock gating: select cells along Ci.\n";
			cout << "      -clockgating=[choice]  Consider insertion of clock gating cells.\n";
			cout << "                               -clockgating=yes   : insert clock gating cells.\n";
			cout << "                               -clockgating=no    : otherwise. (default)\n";
			cout << "      -dump=[choice]         Dump data to files. (default None)\n";
			cout << "                               -dump=all    : dump all data below.\n";
			cout << "                               -dump=dcc    : dump dcc placements.\n";
			cout << "                               -dump=cg     : dump location of clock gating cells.\n";
			cout << "                               -dump=buf_ins: dump location of inserted buffers.\n";
			cout << "      -h,--help              Help messages.\n";
			cout << "  <timing report file>:      File location and name of timing report.\n\n";
			return 0;
		}
		else
		{
			cout << message << "\n";
			return 0;
		}
	}
    else
        circuit.readParameter() ;//read VTA Lib Info
	
	// Parameters storing execution time of each part of framework
	chrono::steady_clock::time_point starttime, endtime, midtime;
	chrono::duration<double> totaltime, preprocesstime, DccVTAconstrainttime, timingconstrainttime1, timingconstrainttime2, sattime, sattime2, minitime, bufinstime;
	
    printf( YELLOW"[Parser]" RST" Reading timing report...\n" );
	starttime = chrono::steady_clock::now();
	//---- Parse *.rpt -----------------------------------------------
	circuit.parseTimingReport();
    
    
	//---- CLK Gating ------------------------------------------------
    //1. Replace some buffers in the clock tree to clock gating cells
	circuit.clockGatingCellReplacement();
    
    //---- Tc Adjust --------------------------------------------------
    printf( YELLOW"[Setting]" RST" Adjusting original Tc...\n" );
	circuit.adjustOriginTc();
	endtime = chrono::steady_clock::now();
	preprocesstime = chrono::duration_cast<chrono::duration<double>>(endtime - starttime);
    
    //---- Mask -------------------------------------------------------
    circuit.dccPlacementByMasked();
    
    //---- Do other Fuction --------------------------------------------
    if( !circuit.DoOtherFunction() ) return 0;
    
	cout << "---------------------------------------------------------------------------\n";
    //-------- Remove CNF file ------------------------------------------------------------
    circuit.removeCNFFile() ;
    midtime = chrono::steady_clock::now();
	//-------- Constraint -----------------------------------------------------------------
	//1.
    printf( YELLOW"[HTV Buf Leader Constraint]" RST" Analyze leader constraint and corresponding clauses\n" );
    circuit.VTAConstraint();
    //2.
    printf( YELLOW"[DCC Constraint]" RST" Analyze DCC constraint and corresponding clauses\n" );
    circuit.dccConstraint();
    //3.
    printf( YELLOW"[DCC-Leader Constraint]" RST" Analyze DCC-leader constraint and corresponding clauses\n" );
    circuit.DCCLeaderConstraint();
    endtime = chrono::steady_clock::now();
    DccVTAconstrainttime = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
	//-------- Generate all kinds of DCC deployment ----------------------------------------
	circuit.genDccPlacementCandidate();
	
    string Sat = "";
    int    itr_ctr = 1 ;
    double Tc  = 0, Tc_L = 0, Tc_U =  0;
    FILE *fPtr;
    string filename = "./setting/BS_log.txt";
    fPtr = fopen( filename.c_str(), "w" );
    
    
    
    //-------- Binary Search ----------------------------------------------------------------
	printf( YELLOW"[Binary Search for Tc]" RST" Analyzing Timing Constraint and Searching Optimal Tc...\033[0m\n" );
	double pretc = 0, prepretc = 0;
    long   clause_ctr = 0 ;
	while( 1 )
	{
        Tc = circuit.getTc();
        printf( RST"\n\t" YELLOW"[" CYAN"%3d" YELLOW" th of Binary Search for Tc]\n", itr_ctr );
        printf( YELLOW"\t[--Clock Period---] " RST"Tc = %f \n", Tc );
        
		midtime = chrono::steady_clock::now();
		//---- Timing constraint method (Clauses)--------------------------------------------
		clause_ctr = circuit.timingConstraint();
		//---- Generate CNF file ------------------------------------------------------------
		circuit.dumpClauseToCnfFile();
		endtime = chrono::steady_clock::now();
        //---- Constraint Time --------------------------------------------------------------
        timingconstrainttime1 = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
        timingconstrainttime2 += timingconstrainttime1;
		printf( YELLOW"\t[--Clause (time)--] " RST"runtime: %f (only CNF generation)\n", timingconstrainttime1.count());
        prepretc = pretc; pretc = circuit.getTc();
		//---- MiniSat ----------------------------------------------------------------------
        midtime = chrono::steady_clock::now();
		circuit.execMinisat();
        endtime = chrono::steady_clock::now();
        sattime2 = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
        sattime += sattime2;
        printf( YELLOW"\t[-MiniSAT (time)--] " RST"runtime: %f (only MiniSAT)\n", sattime2.count());
		//---- Set UB/LB Tc -----------------------------------------------------------------
        Tc_L = circuit.getTcLowerBound(); Tc_U = circuit.getTcUpperBound();
        Sat = ( circuit.tcBinarySearch())?("SAT"):("UNSAT");
        fprintf( fPtr, "%d. Cl#=%ld, Tc_U=%f, Tc_L=%f,T_m=%f, T_tim_c=%f, T_solver=%f, %s\n", itr_ctr, clause_ctr, Tc_U, Tc_L, Tc, timingconstrainttime1.count(), sattime2.count(), Sat.c_str() );
        
        itr_ctr++ ;
		if( prepretc == circuit.getTc() ) break;
	}
	
    //4. Update the timing of each critical path with given "Optimal tc"
    printf( YELLOW"[Update]" RST"Update path timing (formally DCC deployment and leader selection)\n" );
	circuit.updateAllPathTiming();
    
    
    //---------- Minimize DCC deployment -----------------------------------------------------
    midtime = chrono::steady_clock::now();
    printf( YELLOW"[Overhead Minimization]" RST"Minimize DCC # and HTV buf #\n" );
	circuit.minimizeDccPlacement();
    //circuit.minimizeLeader2(0);
	endtime = chrono::steady_clock::now();
	minitime = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
	
	midtime = chrono::steady_clock::now();
	// Insert buffers with the given "optimal Tc"
	circuit.bufferInsertion();
	circuit.minimizeBufferInsertion();
	circuit.minimizeBufferInsertion2();
	endtime = chrono::steady_clock::now();
	bufinstime = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
	totaltime = chrono::duration_cast<chrono::duration<double>>(endtime - starttime);
    
   
    circuit.tcRecheck();
    circuit.printFinalResult();
    cout << "---------------------------------------------------------------------------\n";
    cout << " ***** Execution time (unit: s) *****\n";
    cout << " \tTotal                 : " << totaltime.count() << "\n";
    cout << " \tParser & Preprocessing: " << preprocesstime.count() << "\n";
    cout << " \tDCC & VTA constraint  : " << DccVTAconstrainttime.count() << "\n";
    cout << " \tTiming constraint     : " << timingconstrainttime2.count() << "\n";
    cout << " \tMiniSAT & Search      : " << sattime.count() << "\n";
    cout << " \tOverhead reduction    : " << minitime.count() << "\n";
    cout << " \tBuffer insertion      : " << bufinstime.count() << "\n";
    circuit.dumpDccVTALeaderToFile() ;//circuit.dumpToFile();
    //circuit.printPathCriticality();
    circuit.printBufferInsertedList();
    fclose(fPtr);
	//circuit.~ClockTree();
	
	exit(0);
}
