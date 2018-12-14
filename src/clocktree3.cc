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
	
	printf("The threshold of ratio while lifting buffers\nYour thred = ");
	double thred = 0;
	cin >> thred;
	this->minimizeBufferInsertion2( this->_clktreeroot, thred );
	this->calBufInserOrClockGating(0);
	
	
	FILE *fPtr;
	string filename = "./bufinsertion_" + to_string( this->_tc ) + ".txt";
	fPtr = fopen( filename.c_str(), "w" );
	fprintf( fPtr, "Tc %f\n", this->_tc );
	for( auto node: this->_buflist )
	{
		if( node.second->ifInsertBuffer() )
			fprintf( fPtr, "%ld %f\n", node.second->getNodeNumber(), node.second->getInsertBufferDelay() );
	}
	for( auto node: this->_ffsink )
	{
		if( node.second->ifInsertBuffer() )
			fprintf( fPtr, "%ld %f\n", node.second->getNodeNumber(), node.second->getInsertBufferDelay()  );
	}
	fclose(fPtr);
}

void ClockTree::minimizeBufferInsertion2( CTN* node, double thred )
{
	if( node == NULL ) return;
	if( node->ifInsertBuffer() ) return;
	if( node->getChildren().size() == 0 ) return;
	
	double max_buf_delay= 0;
	double buf_ctr      = 0;
	for( auto child: node->getChildren() )
	{
		minimizeBufferInsertion2( child, thred );
		
		if( child->ifInsertBuffer()  )  buf_ctr++;
	}

	double ratio = buf_ctr/node->getChildren().size();
	
	if( ratio > thred && node != this->_clktreeroot )
	{
		printf( " clknode(%5ld), 1 <= %ld:\n", node->getNodeNumber(), node->getChildren().size() );
		for( auto child: node->getChildren() )
		{
			if( child->ifInsertBuffer() )
			{
				max_buf_delay = ( child->getInsertBufferDelay() > max_buf_delay )? ( child->getInsertBufferDelay() ):( max_buf_delay );
				child->setIfInsertBuffer(0);
				printf( RED"\tBuf " RST"clknode(%5ld, p = %f)\n", child->getNodeNumber(), child->getInsertBufferDelay() );
			}else
				printf( RED"\t    " RST"clknode(%5ld, p = %f)\n", child->getNodeNumber(), child->getInsertBufferDelay() );
		}
		node->setIfInsertBuffer(1);
		node->setInsertBufferDelay(max_buf_delay);
		
		//--- Check Timing after buffer lifting -------------
		bool timing_vio = 0;
		for( auto const &path: this->_pathlist )
		{
			if( path->getPathType() == NONE || path->getPathType() == PItoPO ) continue;
			double slack = this->UpdatePathTiming( path, 1, 0, 1, 1 );
			if( slack < 0 ){
				timing_vio = 1;
				break;
			}
		}
		
		if( timing_vio )
		{
			//while timing violation takes place, cancel buffer lifting
			for( auto child: node->getChildren() )
				if( child->getInsertBufferDelay()  != 0 )
					child->setIfInsertBuffer(1);//recover its status
			node->setIfInsertBuffer(0);
			node->setInsertBufferDelay(0);
			printf( RED"\t[Failing in lifting]\n" RST );
		}else{
			//while succeed in buffer lifting, clean the children's inserted buffer
			for( auto child: node->getChildren() )
			{
				if( child->getInsertBufferDelay() != 0 )
					child->setInsertBufferDelay(0);
			}
			printf( GRN"\t[Succeding in lifting]\n" RST );
		}
	}
}

long ClockTree::calBufInserOrClockGating( int status )
{
	long ctr = 0;
	
	for( auto node: this->_buflist )
	{
		if( node.second->ifInsertBuffer() && status == 0 ) ctr++;
		if( node.second->ifClockGating() && status == 1 ) ctr++;
	}
	for( auto node: this->_ffsink )
	{
		if( node.second->ifInsertBuffer() && status == 0 ) ctr++;
		if( node.second->ifClockGating() && status == 1 ) ctr++;
	}
	
	if( status == 0 )		printf("Inserted Bufs = %ld\n", ctr );
	else if( status == 1 )	printf("Clock-Gating cells = %ld\n", ctr );
	else					cerr << "Unreconized status in calBufInserOrClockGating( int status )\n" ;
	return ctr ;
}

void ClockTree::bufinsertionbyfile()
{
	readDCCVTAFile( "./setting/DccVTA.txt", 3 );//Only read Tc from the file
	bufferInsertion();
	minimizeBufferInsertion2();
}

void ClockTree::clockgating()
{
	this->_tc = this->_tcAfterAdjust;
	this->SortCPbySlack( 0 /*Do not consider DCC*/, 0);
	
	if( this->_program_ctl != 8 ) return;
	for( auto pptr: this->_pathlist )
	{
		if( pptr->getPathType() == NONE || pptr->getPathType() == PItoPO ) continue;
		
		vector<CTN*> stClkPath = pptr->getStartPonitClkPath() ;
		vector<CTN*> edClkPath = pptr->getEndPonitClkPath();
		
		//Do not select gated cells along Cj/end clk path.
		if( edClkPath.size() == 0 ) continue;
		if( edClkPath.back()->ifClockGating() )
		{
			edClkPath.back()->setIfClockGating(0);
			edClkPath.back()->setGatingProbability(0);
			printf("Stop clockgatinh at pptr(%ld)\n", pptr->getPathNum() );
			break;
		}
		
		if( stClkPath.size() == 0 || stClkPath.back()->ifClockGating() ) continue;
		else
		{
			CTN* node = stClkPath.back();
			node->setIfClockGating(1);
			//node->setGatingProbability( genRandomNum("float", this->_gplowbound, this->_gpupbound, 2) );
			node->setGatingProbability( 0.9 );
		}
	}
	
	
	int index = 1 ;
	map <string, CTN*> nodes = this->_buflist;
	nodes.insert( _ffsink.begin(), _ffsink.end() );
	
	this->calBufInserOrClockGating(1);//0:Buf insertion, 1:Clock gating
	double thd = 0;
	cin >> thd ;
	this->GatedCellRecursive( this->_clktreeroot, thd );
	
	FILE *fPtr;
	string filename = "./clock_gating.txt";
	fPtr = fopen( filename.c_str(), "w" );
	printf("\nGated Cells (" GRN"After" RST" minimization by recursive):\n");
	
	index = 1;
	for( auto const &node: nodes )
	{
		if( node.second->ifClockGating() == 0 ) continue;
		else
		{
			node.second->setGatingProbability( genRandomNum("float", this->_gplowbound, this->_gpupbound, 2) );
			printf("\t%d. node(%5ld), prob = %3.2f\n", index, node.second->getNodeNumber(), node.second->getGatingProbability() );
			fprintf( fPtr, "%ld %f\n", node.second->getNodeNumber(), node.second->getGatingProbability() );
			index++;
		}
	}
	this->calBufInserOrClockGating(1);//0:Buf insertion, 1:Clock gating
}

void ClockTree::GatedCellRecursive( CTN* node, double thred )
{
	if( !node || node->ifClockGating() || node->getChildren().size() == 0 ) return;
	
	double min_prob = 1;//sleep prob
	double ctr		= 0;
	for( auto child: node->getChildren() )
	{
		GatedCellRecursive( child, thred );
	
		if( child->ifClockGating() ){
			ctr++;
			min_prob = ( child->getGatingProbability() < min_prob )? ( child->getGatingProbability() ):( min_prob );
		}
	}
	
	if( ctr/node->getChildren().size() >= thred && node != this->_clktreeroot )
	{
		//printf( "Try clknode(%5ld), 1 <= %ld:\n", node->getNodeNumber(), node->getChildren().size() );
		for( auto const &child: node->getChildren() )
		{
			//Temporarily remove the gated cell
			if( child->ifClockGating() ){
				child->setIfClockGating(0);
				//printf( RED"\tCG-cell " RST"clknode(%5ld, p = %3.2f)\n", child->getNodeNumber(), child->getGatingProbability() );
			}//else
				//printf("\t        clknode(%5ld)\n", child->getNodeNumber() );
		}
		node->setIfClockGating(1);
		node->setGatingProbability( min_prob );
		
		//After trying insert gated cell, checking timing
		this->_tc = this->_tcAfterAdjust;
		for( auto const & path: this->_pathlist )
		{
			if( path->getPathType() == NONE || path->getPathType() == PItoPO || path->getPathType() == FFtoPO ) continue;
			if( UpdatePathTiming( path, 0, 0, 1, 0 ) < 0 )
			{
				node->setIfClockGating(0);
				node->setGatingProbability(0);
				//recover
				for( auto const &child: node->getChildren() )
					if( child->getGatingProbability() != 0 )
						child->setIfClockGating(1);
			}
		}
	}//thred
}
