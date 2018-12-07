//
//  clocktree2.cpp
//  MAUI_exe
//
//  Created by TienHungTseng on 2018/1/31.
//  Copyright © 2018年 Eric Tseng. All rights reserved.
//

#include "clocktree2.h"




bool ClockTree::DoOtherFunction()
{
	//-------- print pipeline -----------------------------------------
	if( this->_program_ctl == 1 )
	{
		this->printPath();
		return 0;
	}
	//-------- Dump SAT CNF as DCC/VTA ------------------------------
	if( this->_program_ctl == 2 )
	{
		this->dumpCNF() ;
		return 0 ;
	}
	//-------- Check Timing with given DccVTA.txt ---------------------
	if( this->_program_ctl == 3 )
	{
		this->CheckTiming_givFile() ;
		return 0 ;
	}
	//-------- cal VTA ------------------------------------------------
	if( this->_program_ctl == 4 )
	{
		this->calVTABufferCountByFile() ;
		return 0 ;
	}
	//-------- Check CNF/DccVTA.txt -----------------------------------
	if( this->_program_ctl == 5 )
	{
		this->checkCNF() ;
		return 0 ;
	}
	//-------- print Node----------------------------------------------
	if( this->_program_ctl == 6 )
	{
		this->printClockNode();
		return 0;
	}
	//-------- Analysis ------------------------------------------------
	if( this->_program_ctl == 7  )
	{
		this->Analysis();
		return 0 ;
	}
	//-------- Analysis ------------------------------------------------
	if( this->_program_ctl == 8  )
	{
		this->clockgating();
		return 0 ;
	}
    //-------- print DCC ------------------------------------------------
    if( this->ifprintCP() )
    {
        this->readDCCVTAFile();
        this->printPathCriticality();
        return 0 ;
    }
	//-------- Buffer insertion----------------------------------------------
	if( this->_bufinsert == 2 )
	{
		this->bufinsertionbyfile();
		return 0;
	}
	
	
    return 1;
}
/*----------------------------------------------------------------------------------
 Func:
    PrintClockNode
 Intro:
    Show/Print the topology of the clock tree
 -----------------------------------------------------------------------------------*/
void ClockTree::printClockNode()
{
    //---- Iteration ----------------------------------------------------------------
    int nodeID = 0 ;
    CTN *node = NULL ;
    readDCCVTAFile() ;
	
	this->printLeaderLayer();
	
    while( true )
    {
        printf( "---------------------- " CYAN"Print Topology " RESET"----------------------------\n" );
        printf( " Reading DCC/HTV deployment from " CYAN "DccVTA.txt\n" RST );
        printf( " Clk_Name( nodeID, parentID, " BLUE"O" RESET"/" MAGENTA"X" RESET" )\n" );
        printf( BLUE     " O" RESET": those clk nodes that are Not masked\n" );
        printf( MAGENTA  " X" RESET": those clk nodes that are     masked\n" );
        printf( " number <= 0 : leave the loop\n" );
        printf( " number > 0 : The ID of clock node\n" );
        printf( " Please type the clknode ID:" );
        
        cin >> nodeID ;
        
        if( nodeID < 0 ) break ;
        else
        {
            node =  searchClockTreeNode( nodeID ) ;
            if( node == NULL )  fprintf( stderr, RED"[ERROR]" RST "The node ID %d is not identified\n", nodeID );
            else
            {
                printf("------------------------- " CYAN"Topology " RESET"-------------------------------\n");
                printf("Following is the topology whose root is %d\n\n\n", nodeID );
                printClockNode( node, 0 ) ;
            }
        }
    }//while
}
void ClockTree::printClockNode( CTN*node, int layer )
{
    if( node == NULL ) return ;
    else
    {
        int targetStrLen = 16;
        const char *padding="------------------------------";
        
        long int padLen = targetStrLen - strlen(node->getGateData()->getGateName().c_str()); // Calc Padding length
        
        
        //-- Print Clock Node -----------------------------------------------------
        printf( RST"--" );
        
        if( node->ifPlacedDcc() ){
            if( node->getDccType() == this->DC_1 ) printf( RED"20 " RST );
            if( node->getDccType() == this->DC_2 ) printf( RED"40 " RST );
            if( node->getDccType() == this->DC_3 ) printf( RED"80 " RST );
            padLen--;
        }
        if( node->getIfPlaceHeader()){
            printf( GRN"HTV" RST );
            padLen += -3 ;
        }
        if( padLen < 0 ) padLen = 0;    // Avoid negative length
        if( node->ifMasked())
            printf( "%*.*s%s( %4ld, " MAGENTA"X" RESET" )", (int)padLen, (int)padLen, padding,node->getGateData()->getGateName().c_str(), node->getNodeNumber() );
        else
            printf( "%*.*s%s( %4ld, " BLUE   "O" RESET" )", (int)padLen, (int)padLen, padding, node->getGateData()->getGateName().c_str(), node->getNodeNumber() );
        
        //--- Node is FF -----------------------------------------------------------
        if( _ffsink.find(node->getGateData()->getGateName()) !=  _ffsink.end() )
        {
            printf( YELLOW );
            int ctr = 0 ;
            for( auto path: _pathlist )
            {
                if( path->getPathType() == FFtoFF || path->getPathType() == FFtoPO )
                    if( path->getStartPonitClkPath().back() == node )
                    {
                        printf( YELLOW"%4ld(St.) " RESET, path->getPathNum() );
                        ctr++ ;
                        if( ctr%5 == 0 ){
                            cout << endl ;
                            printNodeLayerSpacel( layer  ) ;
                            printNodeLayerSpace( 1 ) ;
                        }
                    }
                if( path->getPathType() == FFtoFF || path->getPathType() == PItoFF )
                    if( path->getEndPonitClkPath().back() == node )
                    {
                        printf( YELLOW"%4ld(Ed.) " RESET, path->getPathNum() );
                        ctr++ ;
                        if( ctr%5 == 0 ){
                            cout << endl ;
                            printNodeLayerSpacel( layer  ) ;
                            printNodeLayerSpace( 1 ) ;
                        }
                    }
            }
            printf( RESET"\n" );
        }
        //--- Iteration of node's children -------------------------------------------
        for( int j = 0 ; j < node->getChildren().size(); j++ )
        {
            if( j != 0 ) printNodeLayerSpacel( layer + 1 ) ;
            printClockNode( node->getChildren().at(j), layer+1 ) ;
        }
        
        if( node->getChildren().size() != 0 )
        {
            printNodeLayerSpacel( layer  ) ; //printNodeLayerSpace( 1 ) ;
            printf( RESET"\n" );
            printNodeLayerSpacel( layer  ) ; //printNodeLayerSpace( 1 ) ;
            printf( RESET"\n" );
        }
    }
    
}
void ClockTree::printNodeLayerSpacel( int layer )
{
    for( int i = 0 ; i < layer ; i++ )  printf( "                               |" );
}
void ClockTree::printNodeLayerSpace( int layer )
{
    for( int i = 0 ; i < layer ; i++ )  printf( "                                " );
}
/*----------------------------------------------------------------------------------
 Func:
    checkCNF
 Intro:
    Check the CNF clause with the given DCC/VTA deployments.
 -----------------------------------------------------------------------------------*/
void ClockTree::checkCNF()
{
    //-- Declaration --------------------------------------------------------------
    ifstream        fDCCVTA  ;
    ifstream        fCNF     ;
    string          line     ;
    string          fn_DCCVTA;
    string          fn_CNF   ;
    
    printf("-------------------- " CYAN"Check CNF and DCC/VTA " RST"-------------------------------\n");
    fn_CNF = "setting/cnfinput" ;
    fDCCVTA.open( "setting/DccVTA.txt", ios::in ) ;
    fCNF.open( fn_CNF.c_str(), ios::in )          ;
    if( !fDCCVTA )
    {
        cerr << RED"[Error]" << "Can not read 'DccVTA.txt'\n";
        return                ;
    }
    if( !fCNF )
    {
        cerr << RED"[Error]" << "Can not read 'cnfinput_XXXXX.txt'\n";
        return                ;
    }
        
    long nodenum = _totalnodenum*3;
    bool *bolarray = new bool [ nodenum + 1000 ]   ;
    for( int i = 0 ; i <= nodenum; i++ ) bolarray[i] = false;
    //-- Read DCC/VTA Deployment ------------------------------------------------------
    getline( fDCCVTA, line )        ;
    string          tc              ;
    istringstream   token( line )   ;
    token     >>  tc   >> tc        ;
    
    while( getline( fDCCVTA, line ) )
    {
        long    BufID       = 0   ;
        int     BufVthLib   = -1  ;
        double  BufDCC      = 0.5 ;
        istringstream   token( line )     ;
        token >> BufID >> BufVthLib >> BufDCC ;
        CTN *buffer = searchClockTreeNode( BufID ) ;
        if( buffer == NULL )
        {
            printf( RED"[Error] " RESET"Can't find clock node with id = %ld\n", BufID ) ;
            return ;
        }
        //-- Set Boolean Vars of DCC Types -------------------------------------------
        if( BufDCC == 0.8 ){
            bolarray[ BufID ] = bolarray[ BufID + 1 ] = true ;
        }else if( BufDCC == 0.2 ){
            bolarray[ BufID     ] = true  ;
            bolarray[ BufID + 1 ] = false ;
        }else if( BufDCC == 0.4 ){
            bolarray[ BufID     ] = false ;
            bolarray[ BufID + 1 ] = true  ;
        }else{
            bolarray[ BufID ] = bolarray[ BufID + 1 ] = false ;
        }
        
        //-- Set Boolean Vars of VTA Types -------------------------------------------
        if( BufVthLib == -1  )
            bolarray[ BufID + 2 ] = false  ;
        else if( BufVthLib == 0 )
            bolarray[ BufID + 2 ] = true  ;
    }
    fDCCVTA.close();
    //-- Read CNF ------------------------------------------------------------------
    vector< string > vClause;
    bool correct = true ;
    while( getline( fCNF, line ) )
    {
        vClause = stringSplit( line, " " ) ;
        if( clauseJudgement( vClause, bolarray ) == false )
        {
            printf( RED"[Violation]" RST"Clause %s violated !\n", line.c_str() ) ;
            correct = false ;
        }
        
        vClause.clear();
    }
    free( bolarray );
    if( correct )
        printf( GRN"[PASS]" RST"All clauses pass !\n" ) ;
}

bool ClockTree::clauseJudgement( vector<string> &vBVar, bool *BolArray )
{
    for( const auto &Var: vBVar )
    {
        int index = abs( stoi( Var ) );
        int iVar  = stoi( Var );
        if( iVar == 0 || index == 0 )         continue    ;
        if( BolArray[ index ]  && iVar > 0 )  return true ;
        if( !BolArray[ index ] && iVar < 0 )  return true ;
    }
    return false ;
}


void ClockTree::calVTABufferCountByFile()
{
    //-- Declaration --------------------------------------------------------------
    ifstream        fDCCVTA  ;
    string          line     ;
    string          fn_DCCVTA;
    
    fDCCVTA.open( "setting/DccVTA.txt", ios::in ) ;
    if( !fDCCVTA )
    {
        cerr << RED"[Error]" << "Can not read 'DccVTA.txt'\n";
        return                ;
    }
	readDCCVTAFile();
    calVTABufferCount(true) ;
}

long ClockTree::calVTABufferCount(bool print)
{
    vector<CTN *> stClkPath, edClkPath ;
    long HTV_ctr = 0 ;
    long HTV_ctr_2 = 0 ;
    
    if( print ) printf( GRN"[2.HTV Buf Leader Selection] " RST"\n" );

    for( auto const &node: this->_VTAlist )
    {
        HTV_ctr_2 = calBufChildSize( node.second ) ;
        HTV_ctr += HTV_ctr_2 ;
        if( print ) printf( "\t%s(%ld): %ld\n", node.second->getGateData()->getGateName().c_str(), node.second->getNodeNumber(), HTV_ctr_2 ) ;
    }
    if( print ) printf( RST"\t==> Clk Buf HTV/Total = " RED"%ld" RST"/%ld\n",  HTV_ctr, _buflist.size() );
    return HTV_ctr ;
}

int ClockTree::calBufChildSize( CTN *buffer )
{
    if( buffer == NULL ) return 0 ;
    
    
    //If buffer is flip-flop
    auto it = _ffsink.find( buffer->getGateData()->getGateName() );
    if( it != _ffsink.end() ) return 0 ;
    //printf( "%s (%ld)\n", buffer->getGateData()->getGateName().c_str(), buffer->getNodeNumber() ) ;
    
    if( buffer->getChildren().size() <= 0 ) return 0;
    
    int Descendants = 1;//(int)buffer->getChildren().size() ;
    
    for( auto const& child_buff: buffer->getChildren() )
    {
        Descendants += calBufChildSize( child_buff );
    }
    return Descendants ;
}


/*----------------------------------------------------------------------------------
 Func:
    minimizeLeader
 Intro:
    Actually, the function minimalize the leader count
 -----------------------------------------------------------------------------------*/
void ClockTree::minimizeLeader(void)
{
    if( this->ifdoVTA() == false ) return;
    
    map<string, CTN *> redun_leader = this->_VTAlist;//Initialize the list of redundant leader
    map<string, CTN *>::iterator leaderitr;
    
    //Remove necessary leaders from the list of redundant leader.
    for(auto const& path: this->_pathlist)
    {
        CTN *st_leader = nullptr, *ed_leader = nullptr;
        // If DCC locate in the clock path of startpoint
        st_leader = path->findVTAInClockPath('s');
        // If DCC locate in the clock path of endpoint
        ed_leader = path->findVTAInClockPath('e');
        if( st_leader != nullptr)
        {
            leaderitr = redun_leader.find( st_leader->getGateData()->getGateName() );
            if( leaderitr != redun_leader.end() )
                redun_leader.erase(leaderitr);
        }
        if( ed_leader != nullptr)
        {
            leaderitr = redun_leader.find( ed_leader->getGateData()->getGateName() );
            if( leaderitr != redun_leader.end() )
                redun_leader.erase(leaderitr);
        }
        if( path == this->_mostcriticalpath )   break;
    }
    
    //Remove leaders from redundant leader list
    for( auto const& node: redun_leader )
    {
        node.second->setIfPlaceHeader( false );
        node.second->setVTAType( -1 );
    }
    this->_tc = this->_besttc;
    // Greedy minimization
    while( true )
    {
        if( redun_leader.empty()) break;
        bool endflag = 1, findstartpath = 1;
        // Reserve one of the rest of DCCs above if one of critical paths occurs timing violation
        for(auto const& path: this->_pathlist)
        {
            if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF)) continue;
            // Assess if the critical path occurs timing violation in the DCC deployment
            double slack = this->UpdatePathTiming( path, true, true, true ) ;
            
            if( slack < 0 )
            {
                endflag = 0;
                for( auto const& node: redun_leader )
                    if( node.second->getIfPlaceHeader() )
                    {
                        node.second->setVTAType(0);
                        redun_leader.erase(node.first);
                    }
                // Reserve the DCC locate in the clock path of endpoint
                for(auto const& node: path->getEndPonitClkPath())
                {
                    leaderitr = redun_leader.find(node->getGateData()->getGateName());
                    if(( leaderitr != redun_leader.end()) && !leaderitr->second->getIfPlaceHeader())
                    {
                        leaderitr->second->setIfPlaceHeader(1);
                        leaderitr->second->setVTAType(0);
                        findstartpath = 0;
                    }
                }
                // Reserve the DCC locate in the clock path of startpoint
                if(findstartpath)
                {
                    for(auto const& node: path->getStartPonitClkPath())
                    {
                        leaderitr = redun_leader.find(node->getGateData()->getGateName());
                        if((leaderitr != redun_leader.end()) && !leaderitr->second->getIfPlaceHeader()){
                            leaderitr->second->setIfPlaceHeader(1);
                            leaderitr->second->setVTAType(0);
                        }
                    }
                }
                break;
            }
        }
        if( endflag ) break;
    }
    
    for(auto const& node: this->_VTAlist )
    {
        if( !node.second->getIfPlaceHeader())
        {
            node.second->setVTAType(-1);
            this->_VTAlist.erase(node.first);
        }
    }
    //printf("[Info] After Leader Minimization:\n");
    //this->calVTABufferCount(true);
}

bool ClockTree::SolveCNFbyMiniSAT( double tc, bool ifDeploy )
{
    string cnfinput = this->_outputdir + "cnfinput_" + to_string( tc );
    // MiniSat output file
    string cnfoutput = this->_outputdir + "cnfoutput_" + to_string( tc );
    if( !isDirectoryExist(this->_outputdir) ) mkdir(this->_outputdir.c_str(), 0775);
    //----- Call MiniSAT -----------------------------------------------
    if( isFileExist(cnfinput) )
    {
        pid_t childpid = fork();
        if( childpid == -1 )    cerr << RED"[Error]: Cannot fork child process!\033[0m\n";
        else if( childpid == 0 )
        {
            // Child process
            string minisatfile = this->_outputdir + "minisat_output";
            int fp = open(minisatfile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
            if( fp == -1) cerr << RED"[Error]: Cannot dump the executive output of minisat!\033[0m\n";
            else
            {
                dup2(fp, STDOUT_FILENO);
                dup2(fp, STDERR_FILENO);
                close(fp);
            }
            if( execlp("./minisat", "./minisat", cnfinput.c_str(), cnfoutput.c_str(), (char *)0) == -1 )
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
    else return 0;
        
    //----- See result from the output returned from MiniSAT -----------------------------------------------
    fstream cnffile;
    string line;
    if( !isFileExist(cnfoutput) ) return false;
    
    cnffile.open(cnfoutput, ios::in);
    if( !cnffile.is_open() )
    {
        cerr << RED"\t[Error]: Cannot open " << cnfoutput << "\033[0m\n";
        cnffile.close();
        return false;
    }
    getline(cnffile, line);
    
    //------ Check SAT/UNSAT --------------------------------------------------------------
    //If SAT, decoding the DCC deployment and leader selection
    if((line.size() == 5) && (line.find("UNSAT") != string::npos))    return false ;
    else if((line.size() == 3) && (line.find("SAT") != string::npos))
    {
        if( !ifDeploy ) return true ;
        
        //-- Init -------------------------------------------------------------------------
        this->_dcclist.clear();
        this->_VTAlist.clear();
        for( auto const& node: this->_buflist )
        {
            node.second->setIfPlaceDcc(0)   ;
            node.second->setDccType(0) ;
            node.second->setIfPlaceHeader(0);
            node.second->setVTAType(-1);
        }
        getline(cnffile, line) ;
        vector<string> strspl = stringSplit(line, " ");
        //------ Clk Node Iteration ---------------------------------------------------
        for( long loop = 0; ; loop += 3 /*2*/ )
        {
            if( stoi( strspl[loop] ) == 0 ) break ;
            
            //-- Put DCC --------------------------------------------------------------
            if( ( stoi(strspl.at(loop)) > 0) || (stoi(strspl.at(loop + 1)) > 0)  )
            {
                CTN *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
                    
                if( findnode != nullptr )
                {
                    findnode->setIfPlaceDcc(1)      ;
                    findnode->setDccType(stoi(strspl.at(loop)), stoi(strspl.at(loop + 1)));
                    this->_dcclist.insert(pair<string, CTN *> (findnode->getGateData()->getGateName(), findnode));
                }
                else
                    cerr << "[Error] Clock node mismatch, when decoing DCC and Solution exist!\n" ;
            }
            //-- Put Header ------------------------------------------------------------
            if( stoi(strspl.at(loop+2)) > 0 )
            {
                CTN *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
                    
                if( findnode != nullptr )
                {
                    findnode->setIfPlaceHeader(1);
                    findnode->setVTAType(0);
                    this->_VTAlist.insert(pair<string, CTN *> (findnode->getGateData()->getGateName(), findnode));
                }
                else
                    cerr << "[Error] Clock node mismatch, when decoing VTA and Solution exist!\n" ;
            }
        }
        cnffile.close() ;
        double minslack = 9999;
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
            if( slack < 0 )
                printf( RED"[Error] path(%ld) slk = %f \n", path->getPathNum(), path->getSlack() );
        }
        
        
        return true ;
    }//elif SAT
    return false;
}

void ClockTree::EncodeDccLeader( double tc )
{
    fstream cnffile ;
    string cnfinput = this->_outputdir + "cnfinput_" + to_string( tc );
    if( !isDirectoryExist(this->_outputdir) )
        cerr << RED"\t[Error]: No such directory named " << _outputdir << RESET << endl ;
    
    if( isFileExist(cnfinput) )
    {
        cnffile.open(cnfinput, ios::out | fstream::app );
        if( !cnffile.is_open() )
        {
            cerr << RED"\t[Error]: Cannot open " << cnfinput << "\033[0m\n";
            cnffile.close();
            return;
        }
        //------- Encding --------------------
        string clause = "";
        for( auto const& node: this->_buflist )
        {
            if( node.second->getIfPlaceHeader() || node.second->ifPlacedDcc() )
            {
                writeClause_givDCC( clause, node.second, node.second->getDccType() );
                writeClause_givVTA( clause, node.second, node.second->getVTAType() );
            }
        }
        cnffile << clause << "0 \n" ;
        cnffile.close();
    }
    else
        cerr << RED"\t[Error]: No such a file named " << cnfinput << RESET << endl ;
}


void ClockTree::minimizeLeader2( double tc )
{
    if( tc == 0 ) tc = this->_besttc ;
    bool nosol = true ;
    long least_HTV_buf_ctr = 9999999, HTV_buf_ctr = 0, refine_ctr = 1 ;
    
    //---- Print original DCC/Leader deployment without minimization -----------------
    cout << "---------------------------------------------------------------------------\n";
    printf( YELLOW"[---Results without Refinement---] \n" RST );
    printDCCList();
    calVTABufferCount(true);
    
    //---- Begin minimization ----------------------------------------------------------
    while( true )
    {
        EncodeDccLeader( tc );
        
        if( !SolveCNFbyMiniSAT( this->_besttc, true ) || refine_ctr > this->refine_time ) break;
        else
        {
            cout << "---------------------------------------------------------------------------\n";
            printf( YELLOW"[---Refinement---] " RST"%ld st CNF Reversion\n", refine_ctr );
            printDCCList();
            HTV_buf_ctr = calVTABufferCount(true);
            
            if( HTV_buf_ctr < least_HTV_buf_ctr )
            {
                this->_DccLeaderset.clear() ;
                this->minimizeLeader();
                for( auto const& node: this->_buflist )
                {
                    CTN* buf = node.second ;
                    if( buf->ifPlacedDcc() || buf->getIfPlaceHeader() )
                        this->_DccLeaderset.insert( make_tuple( buf, buf->getDccType(), buf->getVTAType() ) );
                }
                least_HTV_buf_ctr = HTV_buf_ctr;
                nosol = false;
            }
        }
        refine_ctr++;
    }//while
    
    if( nosol ) return ;
    
    InitClkTree();
    
    for( auto const& node: this->_DccLeaderset )
    {
        CTN *buf = get<0>(node);
        double     Dcctype = get<1>(node);
        int        HTVtype = get<2>(node);
        if( Dcctype != 0 && Dcctype != 0.5 )
        {
            buf->setIfPlaceDcc(1).setDccType(Dcctype);
            this->_dcclist.insert( make_pair( buf->getGateData()->getGateName(), buf ) );
        }
        if( HTVtype != -1 )
        {
            buf->setIfPlaceHeader(1).setVTAType(HTVtype);
            this->_VTAlist.insert( make_pair( buf->getGateData()->getGateName(), buf ) );
        }
    }
    cout << "---------------------------------------------------------------------------\n";
    printf( YELLOW"[Optimized reuslts]\n" RST);
    printDCCList();
    calVTABufferCount(true);
}

void ClockTree::printFinalResult()
{
    chrono::system_clock::time_point nowtime = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(nowtime);
    cout << "---------------------------------------------------------------------------\n";
    cout << "\t*** Time                           : " << ctime(&tt);
    cout << "\t*** Timing report                  : " << this->getTimingReportFileName()                              << "\n";
    cout << "\t*** Timing report location         : " << this->getTimingReportLocation()                              << "\n";
    cout << "\t*** Timing report design           : " << this->getTimingReportDesign()                                << "\n";
    cout << "\t*** Tc in timing report (Origin)   : " << this->getOriginTc()                                          << "\n";
    cout << "\t*** Total critical paths           : " << this->getTotalPathNumber()                                   << "\n";
    cout << "\t*** # of critical paths in used    : " << this->getPathUsedNumber()                                    << "\n";
    cout << "\t*** # of PI to FF paths            : " << this->getPiToFFNumber()                                      << "\n";
    cout << "\t*** # of FF to PO paths            : " << this->getFFToPoNumber()                                      << "\n";
    cout << "\t*** # of FF to FF paths            : " << this->getFFToFFNumber()                                      << "\n";
    cout << "\t*** Clock tree level (max)         : " << this->getMaxTreeLevel()                                      << "\n";
    cout << "\t*** Total clock tree nodes         : " << this->getTotalNodeNumber()                                   << "\n";
    cout << "\t*** Total # of FF nodes            : " << this->getTotalFFNumber()                                     << "\n";
    cout << "\t*** Total # of clock buffer nodes  : " << this->getTotalBufferNumber()                                 << "\n";
    cout << "\t*** # of FF nodes in used          : " << this->getFFUsedNumber()                                      << "\n";
    cout << "\t*** # of clock buffer nodes in used: " << this->getBufferUsedNumber()                                  << "\n";
    cout << "\t*** No. last same parent           : " << this->getFirstChildrenNode()->getGateData()->getGateName();
    cout << " ("                                      << this->getFirstChildrenNode()->getNodeNumber()                << ")\n";
    cout << "\t*** # of clock gating cells        : " << this->getTotalClockGatingNumber()                            << "\n";
    this->printClockGatingList();
    cout << "---------------------------------------------------------------------------\n";
    cout << "\t*** # of most critical path        : " << this->getMostCriticalPath()->getPathNum();
    cout << " (" ;
    this->getMostCriticalPath()->coutPathType();
    cout << ", " << this->getMostCriticalPath()->getStartPointName();
    cout << " -> " << this->getMostCriticalPath()->getEndPointName() << ")\n";
    cout << "\t*** Minimal slack                  : " << this->getMostCriticalPath()->getSlack() << "\n";
    cout << "\t*** Optimal tc                     : \033[36m" << this->getBestTc() << "\033[0m\n";
    if(this->ifPlaceDcc())
    {
        cout << "\t*** # of minisat executions        : " << this->getMinisatExecuteNumber() << "\n";
        cout << "\t*** # of DCC condidates            : " << this->getTotalBufferNumber() - this->getNonPlacedDccBufferNumber() << "\n";
    }
    this->printDccList();
    this->printBufferInsertedList();
    this->printVTAList();
    this->printClauseCount();
}

bool compare( CP* A, CP*B )
{
    if( A == NULL || B == NULL ) cerr << "[Error] Null pointer for CP*\n";
    if( A->getSlack() > B->getSlack() ) return false ;
    else                                return true  ;
}

void ClockTree::SortCPbySlack( bool DCCHTV, bool update )
{
    for( auto const& path: this->_pathlist )
    {
        if( (path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF) ) continue;
        UpdatePathTiming( path, update, DCCHTV, true );
    }
    sort( this->getPathList().begin(), this->getPathList().end(), compare );
    
}
void ClockTree::printPathCriticality()
{
    cout << "---------------------------------------------------------------------------\n";
    printf( YELLOW"[----Path's Slack Rank----]" RST"\nTop 20 CPs " RED"after " RST"optimization \n" );
    SortCPbySlack(true);
    
    int i = 0;
    for( auto const& pptr: this->_pathlist )
    {
        if( i == 60 ) break;
        if( (pptr->getPathType() != PItoFF) && (pptr->getPathType() != FFtoPO) && (pptr->getPathType() != FFtoFF) ) continue;
        printf("%2d. P(%3ld) ", i, pptr->getPathNum() );
        printAssociatedDCCLeaderofPath( pptr );
        i++;
    }
    printf( RST"\nTop 20 CPs " RED"before " RST"optimization (using original Tc )\n" );
    this->_tc = this->_origintc;
    SortCPbySlack(false);
    
    i = 0;
    for( auto const& pptr: this->_pathlist )
    {
        if( i == 60 ) break;
        if( (pptr->getPathType() != PItoFF) && (pptr->getPathType() != FFtoPO) && (pptr->getPathType() != FFtoFF) ) continue;
        printf("%2d. P(%3ld) ", i, pptr->getPathNum() );
        printAssociatedDCCLeaderofPath( pptr );
        i++;
    }
}

void ClockTree::printDCCList()
{
    CTN* buf = NULL ;
    printf( GRN"[1.DCC Deployment] " RST"\n" );
    for( auto const& node: this->_buflist )
    {
        buf = node.second ;
        if( buf->ifPlacedDcc() )    printf("\t%s(%ld):%2.1f\n", node.first.c_str(), buf->getNodeNumber(), buf->getDccType());
    }
    printf( "\t==> DCC Ctr = " RED"%ld" RST"\n", this->_dcclist.size() );
}


 void ClockTree::printAssociatedDCCLeaderofPath( CP* path )
 {
     if( path == NULL ) return ;
     vector<CTN *> stpath = path->getStartPonitClkPath() ;
     vector<CTN *> edpath = path->getEndPonitClkPath() ;
     
     string NodeType = "";
     if     ( path->getPathType() == FFtoFF) { printf(" FFtoFF:"); NodeType = YELLOW"C"; }
     else if( path->getPathType() == PItoFF) { printf(" PItoFF:"); NodeType =    GRN"R"; }
     else if( path->getPathType() == FFtoPO) { printf(" FFtoPO:"); NodeType =   CYAN"L"; }
     else if( path->getPathType() == NONE  ) { printf("   NONE:\n"); return; }
     
     
     CTN * comnode = path->findLastSameParentNode();
     long comID = path->nodeLocationInClockPath('s', path->findLastSameParentNode() );
     
     long idL = 0 ;
     
     if( stpath.size() > 1  )
     {
         for( auto const& node: stpath )
         {
             if( node->ifPlacedDcc() || node->getIfPlaceHeader() )
                 printf("%4ld( %s%ld" RST", %2.1f, %2d)", node->getNodeNumber(), NodeType.c_str(), idL, node->getDccType(), node->getVTAType() );
             if( node == comnode && comnode ) NodeType = CYAN"L";
             idL++;
         }
     }
     if( edpath.size() > 1  )
     {
        long j = 0 ;
        if( path->getPathType() == FFtoFF ) j = comID + 1;
        NodeType = GRN"R";
        for( ; j < edpath.size()-1; j++)
        {
            if( edpath.at(j)->ifPlacedDcc() || edpath.at(j)->getIfPlaceHeader() )
                printf(" %4ld( %s%ld" RST", %2.1f, %2d)", edpath.at(j)->getNodeNumber(), NodeType.c_str(), j, edpath.at(j)->getDccType(), edpath.at(j)->getVTAType() );
        }
     }
     printf("\n");
 }


void ClockTree::Analysis()
{
    vector<CP*>   vCP1, vCP2         ;//CPs without inserted DCCs
    vector<CTN*>  vDeploy1(300),vDeploy2(300), vDeploy3(300), vDeploy4, vDeploy5, vDeploy6 ;
    set   <CTN*>  sDeploy1, sDeploy2 ;//DCC/Leader deployment for CP1, CP2
    
    readDCCVTAFile("./setting/DccOnly.txt");
    SortCPbySlack(true);
    
    //--- Find CPs, which are not placed DCCs/Leaders on their associated clock network--------
    int placed = 0 ;
    for( auto const& path: this->_pathlist )
    {
        if( !PathReasonable(path) ) continue;
        placed = 0;
        
        for( auto const& node: path->getStartPonitClkPath() )   if( node->ifPlacedDcc() ) placed = 1;
        for( auto const& node: path->getEndPonitClkPath()   )   if( node->ifPlacedDcc() ) placed = 1;
        if( !placed ) vCP1.push_back( path );
        else          vCP2.push_back( path );
    }
	for( auto const &n: this->_dcclist ) vDeploy6.push_back( n.second );
    readDCCVTAFile("./setting/DccVTA.txt");
    
    for( auto const& path: vCP1 ) FindDCCLeaderInPathVector( sDeploy1, path );
    for( auto const& path: vCP2 ) FindDCCLeaderInPathVector( sDeploy2, path );
    
    auto itr1 = set_difference(   sDeploy1.begin(), sDeploy1.end(), sDeploy2.begin(), sDeploy2.end(), vDeploy1.begin() ); vDeploy1.resize(itr1-vDeploy1.begin());
    auto itr2 = set_intersection( sDeploy1.begin(), sDeploy1.end(), sDeploy2.begin(), sDeploy2.end(), vDeploy2.begin() ); vDeploy2.resize(itr2-vDeploy2.begin());
    auto itr3 = set_difference(   sDeploy2.begin(), sDeploy2.end(), sDeploy1.begin(), sDeploy1.end(), vDeploy3.begin() ); vDeploy3.resize(itr3-vDeploy3.begin());
    for( auto const &n: this->_dcclist ) vDeploy4.push_back( n.second );
	for( auto const &n: this->_VTAlist ) vDeploy5.push_back( n.second );
    
    int mode = 0, CP_ctr = 0;
    while( true )
    {
		printf("\n\n\n\n\n\n-------------------------------------------------\n" );
        printf("In \"-analysis\", please type the mode number\n" );
        printf("mode <= 0: Leaving the program\n" );
        printf("mode  = 1: See " GRN"DCCs classification " RST"depending on " GRN"DccOnly.txt" RST" and " GRN"DccVTA.txt" RST" \n" );
        printf("mode  = 2: See the variation of top X CPs " GRN"before and after " RST"applying the framework\n" );
        printf("mode  = 3: " GRN"Remove each    " RST"DCC (from DccVTA.txt)  and see corresponding influence on path timing\n" );
        printf("mode  = 4: " GRN"Remove each " RST"Leader (from DccVTA.txt)  and see corresponding influence on path timing\n" );
		printf("mode  = 5: " GRN"Remove each    " RST"DCC (from DccOnly.txt) and see corresponding influence on path timing\n" );
		printf("mode  = 6: Similar to mode 3/4, but it's from the angle of CPs\n" );
		printf("mode  = 7: Sorting CPs based on DccOnly.txt\n" );
		printf("mode  = 8: Sorting CPs based on  DccVTA.txt\n" );
        printf("Your mode is:" );
        cin >> mode ;
        system("clear");
		
        if( mode <= 0 ) return;
		switch( mode )
		{
			case 1:
				DisplayDCCLeaderinVec( vDeploy1, 1 ).DisplayDCCLeaderinVec( vDeploy2, 1 ).DisplayDCCLeaderinVec( vDeploy3, 1 );
				DisplayDCCLeaderinVec( vDeploy4, 1 ).DisplayDCCLeaderinVec( vDeploy5, 2 );
				break;
			case 2:
				printf("How many Top CPs do you wanna see\n" );
				printf("The order depending on the DCC deployment in DccOnly.txt\n" );
				printf("Quantity of CPs:" );
				cin >> CP_ctr ;
				for( long rank = 0; rank < getPathList().size(); rank++ )
				{
					CP* path = getPathList().at(rank);
					if( !PathReasonable(path) ) continue;
					printCP_before_After( path, vDeploy3 );
					if( rank == CP_ctr ) break;
				}
				break;
			case 3:
				RemoveDCCandSeeResult( vDeploy4, 1 );
				break;
			case 4:
				RemoveDCCandSeeResult( vDeploy5, 2 );
				break;
			case 5:
				readDCCVTAFile("./setting/DccOnly.txt");
				RemoveDCCandSeeResult( vDeploy6, 1 );
				readDCCVTAFile("./setting/DccVTA.txt");
				break;
			case 6:
				PathFailReasonAnalysis();
				break;
			case 7:
				readDCCVTAFile("./setting/DccOnly.txt");
				SortCPbySlack(true);
				readDCCVTAFile("./setting/DccVTA.txt");
				break;
			case 8:
				readDCCVTAFile("./setting/DccVTA.txt");
				SortCPbySlack(true);
				break;
			default:
				break;
		}
    }
}

void ClockTree::RemoveDCCandSeeResult2( CTN*node, int mode )
{
	//
	if( node == NULL ) 							{ printf("The node is not found        \n"); return;}
	if( mode == 1 && !node->ifPlacedDcc()      ){ printf("The node is not placed DCC   \n"); return;}
	if( mode == 2 && !node->getIfPlaceHeader() ){ printf("The node is not placed Leader\n"); return;}
	//---- Declaration --------------------------------------
	double	init_DCCType = 0.0 ;
	int     init_LibType = -1  ;
	CP *	pptr = NULL;
	string  Side = ""  ;
	//---- Remove each of DCC/Leader and see results --------
	init_LibType = node->getVTAType()           ;
	init_DCCType = node->getDccType() 			;
	if( mode == 1 )
	{
		node->setIfPlaceDcc(0).setDccType(0.5)   ;
		printf("If %4ld(%2.1f, Depth = %ld ) is removed:\n", node->getNodeNumber(), init_DCCType, node->getDepth() );
	}
	if( mode == 2 )
	{
		node->setIfPlaceHeader(0).setVTAType(-1) ;
		printf("If %4ld(H, Depth = %ld) is removed:\n", node->getNodeNumber(), node->getDepth()  );
	}
		
	for( long p = 0; p < getPathList().size(); p++ )
	{
		pptr = getPathList().at(p);
		if( pptr->getPathType() == NONE ) continue;
			
		if( UpdatePathTiming( pptr, false, true, true ) < 0 )
		{
			getNodeSide( Side, node, pptr );
			if( mode ) printf("\t%4ld(%s" RST") in failing path ", node->getNodeNumber(), Side.c_str() );
			FindDCCLeaderInPathVector( pptr );
		}
	}//for-path
	if( mode == 1 ){ node->setIfPlaceDcc(1).setDccType(init_DCCType)   ; }
	if( mode == 2 ){ node->setIfPlaceHeader(1).setVTAType(init_LibType); }
}
void ClockTree::RemoveDCCandSeeResult( vector<CTN*> &vDeploy, int mode )
{
	if( mode == 1 ) printf("--------- Begin removing each of " GRN"DCCs "    RST"----------------------\n");
	if( mode == 2 ) printf("--------- Begin removing each of " GRN"Leaders " RST"----------------------\n");
    //---- Declaration --------------------------------------
	CTN* node = NULL;
	long ctrl = 0   ;
	//---- Mode ---------------------------------------------
	while( true )
	{
		printf("\n---------------------------------------------------------------------------\n");
		printf("Input " GRN"< 0  " RST": leaving QQ\n");
		printf("Input " GRN"  0  " RST": List all DCCs/Leaders removement\n");
		printf("Input " GRN"other" RST": check the certain DCC/Leader removement\n");
		printf("Your selection is:");
		cin >> ctrl;
		system("clear");
		if( ctrl < 0 ) return;
		if( ctrl == 0 )
			for( auto const &dcc: vDeploy )	RemoveDCCandSeeResult2( dcc, mode );
		else
		{
			node = searchClockTreeNode( ctrl )  ;
			RemoveDCCandSeeResult2( node, mode );
		}
	}
}
CT& ClockTree::DisplayDCCLeaderinVec( vector<CTN *> &vDeploy, int mode )
{
    if( vDeploy.size() == 0 ) return *this ;
    printf("--------------------------------------------------\n");
    long k = 1;
    for( auto const & node: vDeploy )
	{
        if( node->ifPlacedDcc()      && mode == 1 ){ printf( "%2ld. %4ld(%2.1f)\n", k, node->getNodeNumber(), node->getDccType() ); k++; }
		if( node->getIfPlaceHeader() && mode == 2 ){ printf( "%2ld. %4ld(H)\n"    , k, node->getNodeNumber() )                    ; k++; }
	}
	return *this;
}
CT& ClockTree::DisplayDCCLeaderinSet( set<CTN *> &sDeployment, int mode )
{
    long k = 1;
    for( auto const & node: sDeployment )
	{
		if( node->ifPlacedDcc()      && mode == 1 ){ printf( "%2ld. %4ld(%2.1f)\n", k, node->getNodeNumber(), node->getDccType() ); k++; }
		if( node->getIfPlaceHeader() && mode == 2 ){ printf( "%2ld. %4ld(H)\n"    , k, node->getNodeNumber() )                    ; k++; }
	}
	return *this;
}
bool ClockTree::NodeExistInVec( CTN*node, vector<CTN*> &vDeploy )
{
    if( !node ) return false;
    for( auto const &n: vDeploy )
        if( n == node ) return true;
    return false;
}

pair<int,int> ClockTree::FindDCCLeaderInPathVector( CP* path )
{
	if( path == NULL ) return make_pair(0,0);
    vector<CTN *> stpath = path->getStartPonitClkPath() ;
    vector<CTN *> edpath = path->getEndPonitClkPath() ;
    string NodeSide   = "";
	string PathType   = "";
    int    DCC_ctr    =  0;
    int    Leader_ctr =  0;
    getPathType( PathType, path );
    printf("P(%4ld) %s", path->getPathNum(), PathType.c_str() );
	
    long comID   = path->nodeLocationInClockPath('s', path->findLastSameParentNode() );
    
    string DC  = "XX", HTV = "X";

    if( stpath.size() > 1  )
        for( auto const& node: stpath )
        {
            if( node->ifPlacedDcc() || node->getIfPlaceHeader() )
            {
                DC  = (node->getDccType() == 0.5)?("XX"):(to_string((int)(node->getDccType()*100)));
                HTV = (node->getVTAType() == -1) ?("X"):("H");
				getNodeSide( NodeSide, node, path );
                printf( "%4ld( %s, %s, %s)", node->getNodeNumber(), NodeSide.c_str(), DC.c_str(), HTV.c_str());
                

                if( node->ifPlacedDcc() )       DCC_ctr++;
                if( node->getIfPlaceHeader() )  Leader_ctr++;
            }
        }
    if( edpath.size() > 1  )
    {
        long j = 0 ;
        if( path->getPathType() == FFtoFF ) j = comID + 1;
		
        for( ; j < edpath.size()-1; j++)
        {
            if( edpath.at(j)->ifPlacedDcc() || edpath.at(j)->getIfPlaceHeader() )
            {
                CTN * node = edpath.at(j);
                DC  = (node->getDccType() == 0.5)?("XX"):(to_string((int)(node->getDccType()*100)));
                HTV = (node->getVTAType() == -1) ?("X"):("H");
                getNodeSide( NodeSide, node, path );
                printf( "%4ld( %s, %s, %s)", node->getNodeNumber(), NodeSide.c_str(), DC.c_str(), HTV.c_str());
                
                if( node->ifPlacedDcc() )       DCC_ctr++;
                if( node->getIfPlaceHeader() )  Leader_ctr++;
            }
        }
    }
    printf("\n");
    return make_pair(DCC_ctr, Leader_ctr );
}

int ClockTree::printCP_before_After( CP *path, vector<CTN*>& vDeploy )
{
    printf("------------------------------------------\n");
    readDCCVTAFile("./setting/DccOnly.txt");
    printf("Tc = %f, None    , slk = %f\n", this->_tc, UpdatePathTiming( path, false, false, true) );
    printf("Tc = %f, DCC-Only, slk = %f " , this->_tc, UpdatePathTiming( path, false, true , true) );
    pair<int,int> pBefore = FindDCCLeaderInPathVector( path );
    
    
    readDCCVTAFile("./setting/DccVTA.txt");
    printf("Tc = %f, None    , slk = %f\n", this->_tc, UpdatePathTiming( path, false, false, true) );
    printf("Tc = %f, DCC+HTV , slk = %f " , this->_tc, UpdatePathTiming( path, false, true , true) );
    pair<int,int> pAfter  = FindDCCLeaderInPathVector( path );
    if( pBefore.first < pAfter.first )  printf("=> DCC # change by " GRN"%d\n" RST,  pAfter.first - pBefore.first);
    if( pBefore.first > pAfter.first )  printf("=> DCC # change by " RED"%d\n" RST,  pAfter.first - pBefore.first);
    
    return pAfter.first - pBefore.first;
}

void ClockTree::FindDCCLeaderInPathVector( set<CTN*> &DCCLeader, CP *path )
{
    vector<CTN *> stpath = path->getStartPonitClkPath() ;
    vector<CTN *> edpath = path->getEndPonitClkPath() ;
    if( stpath.size() > 1  )
        for( auto const& node: stpath )
            if( node->ifPlacedDcc() || node->getIfPlaceHeader() ) DCCLeader.insert(node);
        
    if( edpath.size() > 1  )
		for( auto const& node: edpath )
			if( node->ifPlacedDcc() || node->getIfPlaceHeader() ) DCCLeader.insert(node);
}









