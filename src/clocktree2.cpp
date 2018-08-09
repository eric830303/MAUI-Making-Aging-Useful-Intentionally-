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
            else{
                printf("------------------------- " CYAN"Topology " RESET"-------------------------------\n");
                printf("Following is the topology whose root is %d\n\n\n", nodeID );
                printClockNode( node, 0 ) ;
                
            }
        }
    }
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
        
    
    bool *bolarray = new bool [ _totalnodenum + 5000 ]   ;
    for( int i = 0 ; i <= _totalnodenum; i++ ) bolarray[i] = false;
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
    
    calVTABufferCount() ;
}

void ClockTree::calVTABufferCount()
{
    vector<ClockTreeNode *> stClkPath, edClkPath ;
    long HTV_ctr = 0 ;
    long HTV_ctr_2 = 0 ;
    for( auto const &node: this->_VTAlist )
    {
        HTV_ctr_2 = calBufChildSize( node.second ) ;
        HTV_ctr += HTV_ctr_2 ;
        printf( "%s (%ld): %ld\n", node.second->getGateData()->getGateName().c_str(), node.second->getNodeNumber(), HTV_ctr_2 ) ;
    }

    printf( CYAN"[Info]" RST" Total FF        # = %ld \n", _ffsink.size()  );
    printf( CYAN"[Info]" RST" Total Clk Buf   # = %ld \n", _buflist.size() );
    printf( CYAN"[Info]" RST" HTV   Clk Buf   # = %ld \n",  HTV_ctr );
    
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

void ClockTree::minimizeLeader(void)
{
    if( this->ifdoVTA() == false ) return;
    printf("[Info] Remove Redundant Leader...\n");
    printf("[Info] Before Leader Minimization:\n");
    this->calVTABufferCount();
    
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
        if( path == this->_mostcriticalpath)
            break;
    }
    
    //Remove leaders from redundant leader list
    for( auto const& node: redun_leader ) node.second->setIfPlaceHeader( false );
    this->_tc = this->_besttc;
    // Greedy minimization
    while( true )
    {
        if( redun_leader.empty()) break;
        bool endflag = 1, findstartpath = 1;
        // Reserve one of the rest of DCCs above if one of critical paths occurs timing violation
        for(auto const& path: this->_pathlist)
        {
            if((path->getPathType() != PItoFF) && (path->getPathType() != FFtoPO) && (path->getPathType() != FFtoFF))
                continue;
            // Assess if the critical path occurs timing violation in the DCC deployment
            double slack = this->UpdatePathTiming( path, false ) ;
            if( slack < 0 )
            {
                endflag = 0;
                for( auto const& node: redun_leader )
                    if( node.second->getIfPlaceHeader() )
                        redun_leader.erase(node.first);
                // Reserve the DCC locate in the clock path of endpoint
                for(auto const& node: path->getEndPonitClkPath())
                {
                    leaderitr = redun_leader.find(node->getGateData()->getGateName());
                    if(( leaderitr != redun_leader.end()) && !leaderitr->second->ifPlacedDcc())
                    {
                        leaderitr->second->setIfPlaceHeader(1);
                        findstartpath = 0;
                    }
                }
                // Reserve the DCC locate in the clock path of startpoint
                if(findstartpath)
                {
                    for(auto const& node: path->getStartPonitClkPath())
                    {
                        leaderitr = redun_leader.find(node->getGateData()->getGateName());
                        if((leaderitr != redun_leader.end()) && !leaderitr->second->ifPlacedDcc())
                            leaderitr->second->setIfPlaceHeader(1);
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
}

















