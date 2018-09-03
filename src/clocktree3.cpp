//
//  clocktree3.cpp
//  MAUI_exe
//
//  Created by TienHungTseng on 2018/9/3.
//  Copyright © 2018年 Eric Tseng. All rights reserved.
//

#include "clocktree3.hpp"
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
