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
        circuit.readParameter() ;//read VTA Lib Info
		if( message.empty() )
		{	
			// Commands & Help message
			cout << "\033[33m[Usage]: program <option...> <timing report file>\033[0m\n";
			cout << "  <option>:\n";
			cout << "      -nondcc                Don't consider placing any DCC in clock tree. (default disable)\n";
			cout << "      -nonaging              Don't consider any aging in clock tree. (default disable)\n";
            cout << "      -nonVTA                Don't do Vth assignment\n";
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
			cout << "      -path=[choice]         Choice which category of path you want to consider.\n";
			cout << "                               -path=all   : consider all types of path. (default)\n";
			cout << "                               -path=pi_ff : consider input port to FF and FF to FF two types of path.\n";
			cout << "                               -path=onlyff: consider only FF to FF path.\n";
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
	
	// Parameters storing execution time of each part of framework
	chrono::steady_clock::time_point starttime, endtime, midtime;
	chrono::duration<double> totaltime, preprocesstime, dccconstrainttime, timingconstrainttime, sattime, checktime, bufinstime;
	
	cout << "\033[32m[Info]: Open the timing report file.\033[0m\n";
	starttime = chrono::steady_clock::now();
	//---- Parse *.rpt -----------------------------------------------
	circuit.parseTimingReport();
	cout << "\033[32m[Info]: Close the timing report file.\033[0m\n";
    
	//---- CLK Gating ------------------------------------------------
    //1. Replace some buffers in the clock tree to clock gating cells
	circuit.clockGatingCellReplacement();
	cout << "\033[32m[Info]: Adjusting original Tc...\033[0m\n";
    
    //---- Tc Adjust --------------------------------------------------
	//1. Adjust the primitive Tc in the timing report
    //2. Init BS upper/lower Bound
	circuit.adjustOriginTc();
	endtime = chrono::steady_clock::now();
	preprocesstime = chrono::duration_cast<chrono::duration<double>>(endtime - starttime);
	cout << "---------------------------------------------------------------------------\n";
	
	chrono::system_clock::time_point nowtime = chrono::system_clock::now();
	time_t tt = chrono::system_clock::to_time_t(nowtime);
	cout << "\t*** Time                           : " << ctime(&tt);
	cout << "\t*** Timing report                  : " << circuit.getTimingReportFileName()                              << "\n";
	cout << "\t*** Timing report location         : " << circuit.getTimingReportLocation()                              << "\n";
	cout << "\t*** Timing report design           : " << circuit.getTimingReportDesign()                                << "\n";
	cout << "\t*** Tc in timing report (Origin)   : " << circuit.getOriginTc()                                          << "\n";
	cout << "\t*** Total critical paths           : " << circuit.getTotalPathNumber()                                   << "\n";
	cout << "\t*** # of critical paths in used    : " << circuit.getPathUsedNumber()                                    << "\n";
	cout << "\t*** # of PI to FF paths            : " << circuit.getPiToFFNumber()                                      << "\n";
	cout << "\t*** # of FF to PO paths            : " << circuit.getFFToPoNumber()                                      << "\n";
	cout << "\t*** # of FF to FF paths            : " << circuit.getFFToFFNumber()                                      << "\n";
	cout << "\t*** Clock tree level (max)         : " << circuit.getMaxTreeLevel()                                      << "\n";
	cout << "\t*** Total clock tree nodes         : " << circuit.getTotalNodeNumber()                                   << "\n";
	cout << "\t*** Total # of FF nodes            : " << circuit.getTotalFFNumber()                                     << "\n";
	cout << "\t*** Total # of clock buffer nodes  : " << circuit.getTotalBufferNumber()                                 << "\n";
	cout << "\t*** # of FF nodes in used          : " << circuit.getFFUsedNumber()                                      << "\n";
	cout << "\t*** # of clock buffer nodes in used: " << circuit.getBufferUsedNumber()                                  << "\n";
	cout << "\t*** No. last same parent           : " << circuit.getFirstChildrenNode()->getGateData()->getGateName();
	cout << " ("                                      << circuit.getFirstChildrenNode()->getNodeNumber()                << ")\n";
	cout << "\t*** # of clock gating cells        : " << circuit.getTotalClockGatingNumber()                            << "\n";
	circuit.printClockGatingList();
	cout << "---------------------------------------------------------------------------\n";
	
	cout << "\033[32m[Info]: Analyzing DCC Constraint...\033[0m\n";
	midtime = chrono::steady_clock::now();
	//-------- DCC Constraint -----------------------
    //1.
	circuit.dccPlacementByMasked();
	//2.
	circuit.dccConstraint();
    //3.
    circuit.VTAConstraint();
	//-------- Generate all kinds of DCC deployment--
	circuit.genDccPlacementCandidate();//100% understand
	endtime = chrono::steady_clock::now();
	dccconstrainttime = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
	
    //-------- Binary Search -------------------------
	printf( "\033[32m[Info]: Analyzing Timing Constraint and Searching Optimal Tc...\033[0m\n" );
	double pretc = 0, prepretc = 0;
	while( 1 )
	{
		cout << CYAN"\033[36mTc " << RESET "= " << circuit.getTc() << "\033[0m\n";
		midtime = chrono::steady_clock::now();
		//---- Timing constraint method (Clauses)--------
		circuit.timingConstraint();//100% understand
		//---- Generate CNF file ------------------------
		circuit.dumpClauseToCnfFile();//100% understand
		endtime = chrono::steady_clock::now();
        //---- Constraint Time --------------------------
		timingconstrainttime += chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
		
        prepretc = pretc; pretc = circuit.getTc();
		midtime = chrono::steady_clock::now();
		//---- MiniSat ----------------------------------
		circuit.execMinisat();
		//---- Set UB/LB Tc -----------------------------
		circuit.tcBinarySearch();//100% understand
        //---- MiniSat Time -----------------------------
		endtime = chrono::steady_clock::now();
		sattime += chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
		
		// End of binary search
		if( prepretc == circuit.getTc() )
			break;
	}
	
	midtime = chrono::steady_clock::now();
	cout << "\033[32m[Info]: Updating Timing Information of Each Path...\033[0m\n";
	// Update the timing of each critical path with given "Optimal tc"
	circuit.updateAllPathTiming();
	// Minimize DCC deployment
	circuit.minimizeDccPlacement();
	// Recheck the "Optimal tc"
	circuit.tcRecheck();
	endtime = chrono::steady_clock::now();
	checktime = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
	
	midtime = chrono::steady_clock::now();
	// Insert buffers with the given "optimal Tc"
	circuit.bufferInsertion();
	// Minimize buffers placement
	circuit.minimizeBufferInsertion();
	endtime = chrono::steady_clock::now();
	bufinstime = chrono::duration_cast<chrono::duration<double>>(endtime - midtime);
	totaltime = chrono::duration_cast<chrono::duration<double>>(endtime - starttime);
	
    
    
    
    
    
    
    
    
    
    
	cout << "---------------------------------------------------------------------------\n";
	cout << "\t*** # of most critical path        : " << circuit.getMostCriticalPath()->getPathNum();
	cout << " (" << circuit.getMostCriticalPath()->getPathType() << ", " << circuit.getMostCriticalPath()->getStartPointName();
	cout << " -> " << circuit.getMostCriticalPath()->getEndPointName() << ")\n";
	cout << "\t*** Minimal slack                  : " << circuit.getMostCriticalPath()->getSlack() << "\n";
	cout << "\t*** Optimal tc                     : \033[36m" << circuit.getBestTc() << "\033[0m\n";
	if(circuit.ifPlaceDcc())
	{
		cout << "\t*** # of minisat executions        : " << circuit.getMinisatExecuteNumber() << "\n";
		cout << "\t*** # of DCC condidates            : " << circuit.getTotalBufferNumber() - circuit.getNonPlacedDccBufferNumber() << "\n";
	}
	circuit.printDccList();
	circuit.printBufferInsertedList();
	cout << "---------------------------------------------------------------------------\n";
	cout << " ***** Execution time (unit: s) *****\n";
	cout << "   Total                 : " << totaltime.count() << "\n";
	cout << "   Parser & Preprocessing: " << preprocesstime.count() << "\n";
	cout << "   DCC constraint        : " << dccconstrainttime.count() << "\n";
	cout << "   Timing constraint     : " << timingconstrainttime.count() << "\n";
	cout << "   MiniSAT & Search      : " << sattime.count() << "\n";
	cout << "   Finally check         : " << checktime.count() << "\n";
	cout << "   Buffer insertion      : " << bufinstime.count() << "\n";
	
	circuit.dumpToFile();
	circuit.~ClockTree();
	
	exit(0);
}
