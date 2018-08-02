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
    long  nominal_ctr = 0, HTV_ctr = 0 , HTV_DCC_ctr = 0 ;
    for( auto const& path: this->_pathlist )
    {
        stClkPath = path->getStartPonitClkPath() ;
        edClkPath = path->getEndPonitClkPath()   ;
        
        if( stClkPath.size() > 0 )
        {
            bool meetLeader = false ;
            for( auto const &clknode: stClkPath )
            {
                if( clknode == _clktreeroot || clknode->getVTACtr()  ) continue ;
                else
                {
                    if( clknode->getIfPlaceHeader() ) meetLeader = true ;
                    
                    if( meetLeader )
                    {
                        if( clknode->ifPlacedDcc() ) HTV_DCC_ctr++ ;
                        
                        if( clknode->getVTACtr() == false )
                        {
                            clknode->setVTACtr( true ) ;
                            HTV_ctr++       ;
                        }
                    }
                    else
                        if( !clknode->getVTACtr() )
                        {
                            clknode->setVTACtr( true ) ;
                            nominal_ctr++   ;
                        }
                }
            }
        }
        if( edClkPath.size() > 0 )
        {
            bool meetLeader = false ;
            for( auto const &clknode: edClkPath )
            {
                if( clknode == _clktreeroot || clknode->getVTACtr()  ) continue ;
                else
                {
                    if( clknode->getIfPlaceHeader() ) meetLeader = true ;
                    
                    if( meetLeader )
                    {
                        if( clknode->ifPlacedDcc() ) HTV_DCC_ctr++ ;
                        
                        if( clknode->getVTACtr() == false )
                        {
                            clknode->setVTACtr( true ) ;
                            HTV_ctr++       ;
                        }
                    }
                    else
                        if( !clknode->getVTACtr() )
                        {
                            clknode->setVTACtr( true ) ;
                            nominal_ctr++   ;
                        }
                }
            }
        }
    }

    printf( CYAN"[Info]" RST" Total FF        # = %ld \n", _ffsink.size()  );
    printf( CYAN"[Info]" RST" Total Clk Buf   # = %ld \n", _buflist.size() );
    printf( CYAN"[Info]" RST" Nominal Clk Buf # = %ld \n",  nominal_ctr-_ffsink.size() );
    printf( CYAN"[Info]" RST" HTV     Clk Buf # = %ld \n",  HTV_ctr );
    printf( CYAN"[Info]" RST" HTV     DCC     # = %ld \n",  HTV_DCC_ctr );
    
}





















