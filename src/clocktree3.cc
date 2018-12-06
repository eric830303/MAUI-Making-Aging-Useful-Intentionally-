//
//  clocktree3.cpp
//  MAUI_exe
//
//  Created by TienHungTseng on 2018/9/3.
//  Copyright © 2018年 Eric Tseng. All rights reserved.
//

#include "clocktree3.h"
void ClockTree::getPathType( string &PathType, CP* pptr )
{
	if( pptr == NULL ) return;
	if( pptr->getPathType() == PItoFF ) PathType = "[PItoFF]";
	if( pptr->getPathType() == FFtoFF ) PathType = "[FFtoFF]";
	if( pptr->getPathType() == FFtoPO ) PathType = "[FFtoPO]";
	if( pptr->getPathType() == NONE   ) PathType = "[NONE]"  ;
}
bool ClockTree::PathReasonable( CP* pptr)
{
	if( (pptr->getPathType() != PItoFF) && (pptr->getPathType() != FFtoPO) && (pptr->getPathType() != FFtoFF) ) return false;
	else return true;
}
void ClockTree::getNodeSide( string &Side, CTN*node, CP* pptr )
{
	if(      pptr->getPathType() == NONE   ) return;
	else if( pptr->getPathType() == PItoFF ) Side =  GRN"R"+ to_string(node->getDepth())+ RST;
	else if( pptr->getPathType() == FFtoPO ) Side = CYAN"L"+ to_string(node->getDepth())+ RST;
	else
	{
		bool R = NodeExistInVec( node, pptr->getEndPonitClkPath())   ;
		bool L = NodeExistInVec( node, pptr->getStartPonitClkPath()) ;
		if(      R &&  L ) Side = YELLOW"C" + to_string(node->getDepth())+ RST;
		else if( R && !L ) Side =    GRN"R" + to_string(node->getDepth())+ RST;
		else if( L && !R ) Side =   CYAN"L" + to_string(node->getDepth())+ RST;
	}
}
void ClockTree::PathFailReasonAnalysis( )
{
	int mode = 0;
	while( true )
	{
		printf( "\n\n\n------------------------------------\n" );
		printf( "Type "  RED"0" RST": List Top 200 CPs\n" );
		printf( "Type " RED"-1" RST": Leaving the mode\n" );
		printf( "Type " RED"> 0 && < %ld" RST": check the certain path\n", getPathList().size() );
		printf( "You select: " );
		cin >> mode ;
		system("clear");
		if(      mode <  0 ) return;
		else if( mode == 0 )
		{
			for( long p = 0; p < 200; p++ )
			{
				CP* pptr = getPathList().at(p);
				if( !PathReasonable(pptr) ) continue;
				if( !PathIsPlacedDCCorLeader( pptr, 1 ) && !PathIsPlacedDCCorLeader( pptr, 2 ) ) continue;
				PathFailReasonAnalysis(pptr);
			}
		}
		else
		{
			for( long p = 0; p < getPathList().size(); p++ )
			{
				CP* pptr = getPathList().at(p);
				if( pptr->getPathNum() == mode ) PathFailReasonAnalysis(pptr);
			}
		}
	}//while
}
void ClockTree::PathFailReasonAnalysis( CP* pptr )
{
	set<CTN*>   sDCC, sLeader;
	string       Type = "";
	getPathType( Type, pptr );
	vector<CTN*> stPath = pptr->getStartPonitClkPath();
	vector<CTN*> edPath = pptr->getEndPonitClkPath()  ;
	if( stPath.size() > 1 )
		for( auto const &node: stPath )
		{
			if( node->ifPlacedDcc()      )    sDCC.insert(node);
			if( node->getIfPlaceHeader() ) sLeader.insert(node);
		}
	if( edPath.size() > 1 )
		for( auto const &node: edPath )
		{
			if( node->ifPlacedDcc()      )    sDCC.insert(node);
			if( node->getIfPlaceHeader() ) sLeader.insert(node);
		}
	
	double init_dcc =  0;
	int    init_lib = -1;
	string Side     = "";
	printf("Path(%4ld) %s:\n", pptr->getPathNum(), Type.c_str() );
	for( auto const &dcc: sDCC )
	{
		init_dcc = dcc->getDccType();
		dcc->setIfPlaceDcc(0).setDccType(0.5);
		getNodeSide( Side, dcc, pptr );
		if( UpdatePathTiming( pptr, 0, 1, 1) < 0 )
			printf("\t\t\t If %4ld(%s, %2.1f) is removed =>    Timing failure\n", dcc->getNodeNumber(), Side.c_str(), init_dcc );
		else
			printf("\t\t\t If %4ld(%s, %2.1f) is removed => " RED"NO" RST" Timing failure\n", dcc->getNodeNumber(), Side.c_str(), init_dcc );
		
		dcc->setIfPlaceDcc(1).setDccType(init_dcc);
	}
	for( auto const &leader: sLeader )
	{
		init_lib = leader->getVTAType();
		leader->setIfPlaceHeader(0).setVTAType(-1);
		getNodeSide( Side, leader, pptr );
		if( UpdatePathTiming( pptr, 0, 1, 1) < 0 )
			printf("\t\t\t If %4ld(%s,  H ) is removed =>    Timing failure\n", leader->getNodeNumber(), Side.c_str() );
		else
			printf("\t\t\t If %4ld(%s,  H ) is removed => " RED"NO" RST" Timing failure\n", leader->getNodeNumber(), Side.c_str() );
		
		leader->setIfPlaceHeader(1).setVTAType(init_lib);
	}
}
bool ClockTree::PathIsPlacedDCCorLeader( CP *pptr, int mode )
{
	if( pptr == NULL ) return false;
	
	vector<CTN*> stPath, edPath;
	if( pptr->getPathType() == FFtoFF || pptr->getPathType() == FFtoPO ) stPath = pptr->getStartPonitClkPath();
	if( pptr->getPathType() == FFtoFF || pptr->getPathType() == PItoFF ) stPath = pptr->getEndPonitClkPath()  ;
	
	
	if( mode == 1 )//pptr is placed DCC?
	{
		if( stPath.size() > 1 )
			for( auto const &node: stPath )
				if( node->ifPlacedDcc() ) return true;
		if( edPath.size() > 1 )
			for( auto const &node: edPath )
				if( node->ifPlacedDcc() ) return true;
	}
	else if( mode == 2 )//pptr is placed DCC?
	{
		if( stPath.size() > 1 )
			for( auto const &node: stPath )
				if( node->getIfPlaceHeader() ) return true;
		if( edPath.size() > 1 )
			for( auto const &node: edPath )
				if( node->getIfPlaceHeader() ) return true;
	}
	
	return false;
}

void ClockTree::minimizeBufferInsertion2()
{
	if( this->_bufinsert < 1 || this->_bufinsert > 3 ) return;
	for( auto node: this->_clktreeroot->getChildren() )
		minimizeBufferInsertion2( node );
	this->calInsertBufCount();
	
	bool timingvio = 0;
	for( auto path: this->_pathlist )
	{
		if( path->getPathType() == NONE || path->getPathType() == PItoPO ) continue;
		if( UpdatePathTiming( path, 1, 1, 1 ) < 0 ) timingvio = 1;
	}
	
	if( timingvio ) printf( RED"[Timing error]" RST" After Buffer insertion, timing violation takes place\n");
	else			printf( GRN"[Timing PASS]"  RST" After Buffer insertion, All paths pass\n");
}

void ClockTree::minimizeBufferInsertion2( CTN* node )
{
	if( node == NULL ) return;
	if( node->ifInsertBuffer() ) return;
	if( node->getChildren().size() == 0 ) return;
	
	
	//bool   buf_upward	= true;
	double max_buf_delay= 0;
	double buf_delay  	= 0;
	double buf_ctr      = 0;
	for( auto child: node->getChildren() )
	{
		minimizeBufferInsertion2( child );
		
		if( child->ifInsertBuffer()  )  buf_ctr++;
	}
	
	//if( buf_upward )
	double ratio = buf_ctr/node->getChildren().size();
	string Gate = "";
	if( ratio >= 0.5 )
	{
		
		printf( "Try (ratio = %3.2f) clknode(%6ld)\n 1 <-- %ld:\n", ratio, node->getNodeNumber(), node->getChildren().size() );
		for( auto child: node->getChildren() )
		{
			
			Gate = ( child->ifInsertBuffer() )? (CYAN"Buf" RST):("   ");
			
			buf_delay = child->getInsertBufferDelay() ;
			max_buf_delay = ( buf_delay > max_buf_delay )? ( buf_delay ):( max_buf_delay );
			printf("\t%s clknode(%6ld), Inserted buf delay = %f\n",  Gate.c_str(), child->getNodeNumber(), buf_delay );
			child->setIfInsertBuffer(0);
		}
		node->setIfInsertBuffer(1);
		node->setInsertBufferDelay(max_buf_delay);
		printf( "\tInserted Buf delay = %f\n", max_buf_delay );
		//--- Check Timing after buffer lifting -------------
		for( auto path: this->_pathlist )
		{
			if( path->getPathType() == NONE || path->getPathType() == PItoPO ) continue;
			if( UpdatePathTiming( path, 1 ,1, 1 ) < 0 )
			{
				//while timing violation takes place, cancel buffer lifting
				for( auto child: node->getChildren() )
					if( child->getInsertBufferDelay()  != 0 ) child->setIfInsertBuffer(1);//recover its status
				node->setIfInsertBuffer(0);
				node->setInsertBufferDelay(0);
				printf( RED"\tCancel buf upwarding (due to timing violation)\n" RST);
				break;
			}
		}
		//while succeed in buffer lifting, clean the children's inserted buffer
		for( auto child: node->getChildren() )
			if( child->getInsertBufferDelay() != 0 ) child->setInsertBufferDelay(0);
		
		
		printf( GRN"\tSucceed in buf upwarding\n" RST);
	}
}

long ClockTree::calInsertBufCount()
{
	long bufcount = 0;
	
	for( auto node: this->_buflist )
		if( node.second->ifInsertBuffer() ) bufcount++;
	for( auto node: this->_ffsink )
		if( node.second->ifInsertBuffer() ) bufcount++;
	
	printf("Inserted Bufs = %ld\n", bufcount );
	return bufcount;
	
}

void ClockTree::bufinsertionbyfile()
{
	readDCCVTAFile();//read Tc from the file
	this->_besttc = this->_tc;
	bufferInsertion();
	minimizeBufferInsertion2();
}

void ClockTree::clockgating()
{
	//this->SortCPbySlack( 0 /*Do not consider DCC*/, 0);
	bool flag = 1;
	
	for( auto pptr: this->_pathlist )
	{
		if( pptr->getPathType() != NONE || pptr->getPathType() == PItoPO ) continue;
		
		//Do not select gated cells along Cj/end clk path.
		for( auto node: pptr->getEndPonitClkPath()  )
			if( node->ifClockGating() ) flag = 0;
		
		
		if( !flag ) break ;
		vector<CTN*> stClkPath = pptr->getStartPonitClkPath() ;
		if( stClkPath.size() == 0 ) continue;
		else
		{
			CTN* node = stClkPath.back();
			node->setIfClockGating(1);
			node->setGatingProbability( genRandomNum("float", this->_gplowbound, this->_gpupbound, 2) );
			this->_cglist.insert(pair<string, ClockTreeNode *> ( node->getGateData()->getGateName(), node));
		}
	}
	
	this->GatedCellRecursive();
	
	printf("Gated Cells:\n");
	int c = 0 ;
	for( auto node: this->_cglist )
	{
		printf("\t%d. %s(%ld), prob = %f\n", c, node.second->getGateData()->getGateName().c_str(), node.second->getNodeNumber(), node.second->getGatingProbability() );
		c++;
	}
}

void ClockTree::GatedCellRecursive()
{
	for( auto node: this->_clktreeroot->getChildren() )
		GatedCellRecursive2( node );
}
void ClockTree::GatedCellRecursive2( CTN* node )
{
	if( node == NULL ) return;
	if( node->ifClockGating() ) return;
	if( node->getChildren().size() == 0 ) return;
	
	
	bool   cell_upward	= 1;
	double min_prob     = 1;//sleep prob
	double cell_prob  	= 0;
	for( auto child: node->getChildren() )
	{
		GatedCellRecursive2( child );
		
		if( child->ifClockGating() == false )  cell_upward = false;
	}
	
	if( cell_upward )
	{
		//I should check timing here
		
		printf( "clknode(%ld, %s), 1 <= %ld:\n", node->getNodeNumber(), node->getGateData()->getGateName().c_str(), node->getChildren().size() );
		for( auto child: node->getChildren() )
		{
			assert( child->ifInsertBuffer() );
			child->setIfInsertBuffer(0);
			cell_prob = child->getGatingProbability() ;
			min_prob = ( min_prob > cell_prob )? ( cell_prob ):( min_prob );
			printf("\tclknode(%ld, %s)\n", child->getNodeNumber(), child->getGateData()->getGateName().c_str() );
		}
		node->setIfClockGating(1);
		node->setGatingProbability( min_prob );
	}
}




