//
//  clocktree2.cpp
//  MAUI_exe
//
//  Created by TienHungTseng on 2018/1/31.
//  Copyright © 2018年 Eric Tseng. All rights reserved.
//

#include "clocktree2.hpp"

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
    ClockTreeNode *node = NULL ;
    readDCCVTAFile() ;
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
            if( node == NULL )
                cerr << RED"[ERROR]" RESET<< "The node ID " << nodeID << " is not identified\n" ;
            else
            {
                printf("------------------------- " CYAN"Topology " RESET"-------------------------------\n");
                printf("Following is the topology whose root is %d\n\n\n", nodeID );
                printClockNode( node, 0 ) ;
            }
        }
    }//while
}
void ClockTree::printClockNode( ClockTreeNode*node, int layer )
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
            if( node->getDccType() == 0.2 ) printf( RED"20 " RST );
            if( node->getDccType() == 0.4 ) printf( RED"40 " RST );
            if( node->getDccType() == 0.8 ) printf( RED"80 " RST );
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
        ClockTreeNode *buffer = searchClockTreeNode( BufID ) ;
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
        ClockTreeNode *buffer = searchClockTreeNode( BufID ) ;
        if( buffer == NULL )
        {
            printf( RED"[Error] " RESET"Can't find clock node with id = %ld\n", BufID ) ;
            return ;
        }
        if( BufDCC != 0.5 && BufDCC != -1 && BufDCC != 0 ){
            buffer->setIfPlaceDcc(true);
            buffer->setDccType( BufDCC )    ;
        }
        if( BufVthLib != -1  ){
            buffer->setIfPlaceHeader(true);
            buffer->setVTAType( BufVthLib ) ;
        }
    }
    fDCCVTA.close();
    
    calVTABufferCount(true) ;
}

long ClockTree::calVTABufferCount(bool print)
{
    vector<ClockTreeNode *> stClkPath, edClkPath ;
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

int ClockTree::calBufChildSize( ClockTreeNode *buffer )
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
    
    map<string, ClockTreeNode *> redun_leader = this->_VTAlist;//Initialize the list of redundant leader
    map<string, ClockTreeNode *>::iterator leaderitr;
    
    //Remove necessary leaders from the list of redundant leader.
    for(auto const& path: this->_pathlist)
    {
        ClockTreeNode *st_leader = nullptr, *ed_leader = nullptr;
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
                ClockTreeNode *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
                    
                if( findnode != nullptr )
                {
                    findnode->setIfPlaceDcc(1)      ;
                    findnode->setDccType(stoi(strspl.at(loop)), stoi(strspl.at(loop + 1)));
                    this->_dcclist.insert(pair<string, ClockTreeNode *> (findnode->getGateData()->getGateName(), findnode));
                }
                else
                    cerr << "[Error] Clock node mismatch, when decoing DCC and Solution exist!\n" ;
            }
            //-- Put Header ------------------------------------------------------------
            if( stoi(strspl.at(loop+2)) > 0 )
            {
                ClockTreeNode *findnode = this->searchClockTreeNode(abs(stoi(strspl.at(loop))));
                    
                if( findnode != nullptr )
                {
                    findnode->setIfPlaceHeader(1);
                    findnode->setVTAType(0);
                    this->_VTAlist.insert(pair<string, ClockTreeNode *> (findnode->getGateData()->getGateName(), findnode));
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
                    ClockTreeNode* buf = node.second ;
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
        ClockTreeNode *buf = get<0>(node);
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

bool compare( CriticalPath* A, CriticalPath*B )
{
    if( A == NULL || B == NULL ) cerr << "[Error] Null pointer for CriticalPath*\n";
    if( A->getSlack() > B->getSlack() ) return false ;
    else                                return true  ;
}

void ClockTree::SortCPbySlack( bool DCCHTV )
{
    for( auto const& path: this->_pathlist )
    {
        if( (path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF) ) continue;
        UpdatePathTiming( path, true, DCCHTV, true );
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
        if( i == 100 ) break;
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
        if( i == 100 ) break;
        if( (pptr->getPathType() != PItoFF) && (pptr->getPathType() != FFtoPO) && (pptr->getPathType() != FFtoFF) ) continue;
        printf("%2d. P(%3ld) ", i, pptr->getPathNum() );
        printAssociatedDCCLeaderofPath( pptr );
        i++;
    }
}

void ClockTree::printDCCList()
{
    ClockTreeNode* buf = NULL ;
    printf( GRN"[1.DCC Deployment] " RST"\n" );
    for( auto const& node: this->_buflist )
    {
        buf = node.second ;
        if( buf->ifPlacedDcc() )    printf("\t%s(%ld):%2.1f\n", node.first.c_str(), buf->getNodeNumber(), buf->getDccType());
    }
    printf( "\t==> DCC Ctr = " RED"%ld" RST"\n", this->_dcclist.size() );
}


 void ClockTree::printAssociatedDCCLeaderofPath( CriticalPath * path )
 {
     if( path == NULL ) return ;
     vector<ClockTreeNode *> stpath = path->getStartPonitClkPath() ;
     vector<ClockTreeNode *> edpath = path->getEndPonitClkPath() ;
     
     string NodeType = "";
     if     ( path->getPathType() == FFtoFF) { printf(" FFtoFF:"); NodeType = YELLOW"C"; }
     else if( path->getPathType() == PItoFF) { printf(" PItoFF:"); NodeType =    GRN"R"; }
     else if( path->getPathType() == FFtoPO) { printf(" FFtoPO:"); NodeType =   CYAN"L"; }
     else if( path->getPathType() == NONE  ) { printf("   NONE:\n"); return; }
     
     
     ClockTreeNode * comnode = path->findLastSameParentNode();
     long comID = path->nodeLocationInClockPath('s', path->findLastSameParentNode() );
     
     long idL = 0 ;
     
     if( stpath.size() > 1  )
     {
         for( auto const& node: stpath )
         {
             if( node->ifPlacedDcc() || node->getIfPlaceHeader() )
                 printf(" %4ld( %s%ld" RST", %2.1f, %2d)", node->getNodeNumber(), NodeType.c_str(), idL, node->getDccType(), node->getVTAType() );
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


void ClockTree::Compare()
{
    vector<CriticalPath*>   vCP         ;
    vector<CriticalPath*>   vCP2        ;
    set<ClockTreeNode*>     DCCLeader   ;
    set<ClockTreeNode*>     DCCLeader2  ;
    readDCCVTAFile("./setting/DccOnly.txt");
    SortCPbySlack(true);
    
    //--- Find CPs, which are not placed DCCs/Leaders on their associated clock network--------
    int rank = 0 ;
    for( auto const& path: this->_pathlist )
    {
        if( (path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF) ) continue;
        bool placed = false;
        vector<ClockTreeNode *> stpath = path->getStartPonitClkPath() ;
        vector<ClockTreeNode *> edpath = path->getEndPonitClkPath() ;
        if( stpath.size() > 1  )
            for( auto const& node: stpath )
                if( node->ifPlacedDcc() || node->getIfPlaceHeader() ) placed = true;
        if( edpath.size() > 1  )
            for( auto const& node: edpath )
                if( node->ifPlacedDcc() || node->getIfPlaceHeader() ) placed = true;
        
        if( !placed ) vCP.push_back( path );
        else          vCP2.push_back( path);
        rank++;
        if( rank == 1000 ) break;
    }
    readDCCVTAFile("./setting/DccVTA.txt");
    FindDCCLeaderInPathVector( DCCLeader, vCP );
    
    printf("---------- DCCs deployed on original non-deployed CPs  --------------\n");
    DisplayDCCinSet( DCCLeader );
    
    for( auto const & path: vCP2 )
    {
        vector<ClockTreeNode *> stpath = path->getStartPonitClkPath() ;
        vector<ClockTreeNode *> edpath = path->getEndPonitClkPath() ;
        if( stpath.size() > 1  )
            for( auto const& node: stpath )
                if( DCCLeader.find(node) != DCCLeader.end() )   DCCLeader.erase(node);
        
        if( edpath.size() > 1  )
            for( auto const& node: edpath )
                if( DCCLeader.find(node) != DCCLeader.end() )   DCCLeader.erase(node);
            
    }
    printf("---------- DCCs deployed on original non-deployed CPs (exclusive)  --------------\n");
    DisplayDCCinSet( DCCLeader );
    FindDCCLeaderInPathVector( DCCLeader2, vCP2 );
    
    printf("---------- DCCs deployed on original deployed CPs  --------------\n");
    DisplayDCCinSet( DCCLeader2 );
    
    //printCP_before_after();
    
}
void ClockTree::DisplayDCCinSet( set<ClockTreeNode *>DCCLeader )
{
    long k = 1;
    for( auto const & node: DCCLeader )
        if( node->ifPlacedDcc() ){ printf( "%2ld. %4ld(%2.1f)\n", k, node->getNodeNumber(), node->getDccType() ); k++; }
}

void ClockTree::FindDCCLeaderInPathVector(set<ClockTreeNode *> &DCCLeader, vector<CriticalPath *> &vCP, bool display)
{
    int rank = 0;
    for( auto const& path: vCP )
    {
        vector<ClockTreeNode *> stpath = path->getStartPonitClkPath() ;
        vector<ClockTreeNode *> edpath = path->getEndPonitClkPath() ;
        string NodeType = "";
        if( display )printf("%2d. P(%3ld) ", rank, path->getPathNum() );
        if     ( path->getPathType() == FFtoFF && display ) { printf(" FFtoFF:"); NodeType = YELLOW"C"; }
        else if( path->getPathType() == PItoFF && display ) { printf(" PItoFF:"); NodeType =    GRN"R"; }
        else if( path->getPathType() == FFtoPO && display ) { printf(" FFtoPO:"); NodeType =   CYAN"L"; }
        else if( path->getPathType() == NONE   && display ) { printf("   NONE:\n"); return; }
        ClockTreeNode * comnode = path->findLastSameParentNode();
        long comID = path->nodeLocationInClockPath('s', path->findLastSameParentNode() );
        
        long idL = 0 ;
        if( stpath.size() > 1  )
            for( auto const& node: stpath )
            {
                if( node->ifPlacedDcc() || node->getIfPlaceHeader() )
                {
                    if( display ) printf("%4ld( %s%ld" RST", %2.1f, %2d)", node->getNodeNumber(), NodeType.c_str(), idL, node->getDccType(), node->getVTAType());
                    DCCLeader.insert(node);
                }
                if( node == comnode && comnode ) NodeType = CYAN"L";
                idL++;
            }
        if( edpath.size() > 1  )
        {
            long j = 0 ;
            if( path->getPathType() == FFtoFF ) j = comID + 1;
            NodeType = GRN"R";
            for( ; j < edpath.size()-1; j++)
            {
                if( edpath.at(j)->ifPlacedDcc() || edpath.at(j)->getIfPlaceHeader() )
                {
                    if( display ) printf(" %4ld( %s%ld" RST", %2.1f, %2d)", edpath.at(j)->getNodeNumber(), NodeType.c_str(), j, edpath.at(j)->getDccType(), edpath.at(j)->getVTAType() );
                    DCCLeader.insert(edpath.at(j));
                }
            }
        }
        if( display ) printf("\n");
        rank++;
    }
}





