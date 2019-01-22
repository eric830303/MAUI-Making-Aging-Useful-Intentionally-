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
	
	if( this->_program_ctl != 10 ) return;
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
			if( node.second->getChildren().size() == 0 )
				printf("\t%d. node(%5ld), " RED"FF " RST" prob = %3.2f\n", index, node.second->getNodeNumber(), node.second->getGatingProbability() );
			else
				printf("\t%d. node(%5ld), " GRN"Buf" RST" prob = %3.2f\n", index, node.second->getNodeNumber(), node.second->getGatingProbability() );
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
		for( auto const &child: node->getChildren() )
		{
			//Temporarily remove the gated cell
			if( child->ifClockGating() )	child->setIfClockGating(0);
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
				printf("Cannot merged to node(%ld) due to timing error\n", node->getNodeNumber() );
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

void ClockTree::PV_simulation()
{
	int    ins_ctr 		= 1000;
	double ins_Tc  		= 0;
	double fresh_Tc		= 0;
	double aged_Tc		= 0;
	double *vTc    		= NULL;
	double *vImp        = NULL;
	double Imp_noPV     = 0;
	
	
	double min_Tc 		=  10000;
	double max_Tc 		= -10000;
	double min_Imp 		=  10000;
	double max_Imp 		= -10000;
	string color  		= RED;
	
	readDCCVTAFile();//read Tc, DCCs, CGs and leaders from file named with "./setting/DccVTA.txt".
	

	printf("Tc = %f\n", this->_besttc );
	printf("How many instances you wanna create?\nYour loop times: ");
	cin >> ins_ctr ;
	printf("Tc_fresh for the benchmark?\nYour Tc_fresh: ");
	cin >> fresh_Tc ;
	printf("Tc_aged for the benchmark?\nYour Tc_aged: ");
	cin >> aged_Tc ;
	vTc = new double [ ins_ctr + 1 ];
	vImp= new double [ ins_ctr + 1 ];
	Imp_noPV = ( 1 - ( this->_tc - fresh_Tc )/( aged_Tc - fresh_Tc ) )*100;
	FILE *fdis;
	string fileDis=  "./Imp_dist.txt";
	
	fdis= fopen( fileDis.c_str(), "w" );
	
	
	
	for( int i = 1; i <= ins_ctr; i++ )
	{
		printf("%4d ", i );
		this->PV_instantiation( 9900, 10100, 4 );
		ins_Tc = this->PV_Tc_estimation();
		vTc[i] = ins_Tc;
		min_Tc = ( ins_Tc < min_Tc )? ( ins_Tc ):( min_Tc );
		max_Tc = ( ins_Tc > max_Tc )? ( ins_Tc ):( max_Tc );
		vImp[i]= ( 1 - ( vTc[i] - fresh_Tc )/( aged_Tc - fresh_Tc ) )*100;
		min_Imp = ( vImp[i] < min_Imp )? ( vImp[i] ):( min_Imp );
		max_Imp = ( vImp[i] > max_Imp )? ( vImp[i] ):( max_Imp );
		
		
		color = ( vImp[i] < Imp_noPV )? ( RED ):( GRN );
		
		printf("Tc = %f (Imp = %s%3.2f%%" RST")\n" RST, vTc[i], color.c_str(), vImp[i] );
	}
	printf("Min Tc = %f (Max Imp = %3.2f%%)\n", min_Tc, max_Imp );
	printf("Max Tc = %f (Min Imp = %3.2f%%)\n", max_Tc, min_Imp );
	
	int grand_ctr = 50;
	double *Imp_index = new double [ grand_ctr+1 ];
	int *Imp_ctr = new int [ grand_ctr+1 ];
	
	double grand_size = ( max_Imp - min_Imp )/grand_ctr;
	for( int i = 0; i <= grand_ctr; i++ )
	{
		Imp_index[i] = min_Imp + i*grand_size;
		Imp_ctr[i] = 0;//Initialize
	}
	
	for( int i = 1; i <= ins_ctr; i++ )
	{
		int index = ( vImp[i] - min_Imp )/grand_size;
		Imp_ctr[ index ]++;
	}

	//----------- Dump -----------------------------------------
	for( int i = 0; i < grand_ctr; i++ )
		fprintf( fdis, "%f %d\n", Imp_index[i], Imp_ctr[i] );
	
	delete [] Imp_index ;
	delete [] Imp_ctr ;
}


void ClockTree::PV_instantiation( double LB, double UB, int precision )
{
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	default_random_engine generator (seed);
	normal_distribution<double> distribution( (LB+UB)/2, (UB-LB)/2 );
	double rate = 0;
	
	for( auto const &node: this->_buflist )
	{
		do{
			rate = distribution(generator) ;
		}
		while( rate < LB || rate > UB );
		rate /= pow(10, precision);
		//cout << rate << endl;
		node.second->setPVrate( rate );
	}
	for( auto const &node: this->_ffsink )
	{
		do{
			rate = distribution(generator) ;
		}
		while( rate < LB || rate > UB );
		rate /= pow(10, precision);
		node.second->setPVrate( rate );
	}
	
	double avg_rate = 0;
	for( auto const &path: this->_pathlist )
	{
		avg_rate = 0;
		for( int i = 0; i < 10; i++ )
		{
			do{
				rate = distribution(generator) ;
			}
			while( rate < LB || rate > UB );
			avg_rate += rate ;
		}
		avg_rate /= 10;
		avg_rate /= pow(10, precision);
		path->setPVrate( avg_rate );
		 
	}
}

double ClockTree::PV_Tc_estimation()
{
	this->_tc = this->_besttc;//from "./setting/DccVTA.txt
	double Tc_PV = this->_tc;
	double Min_slack = 999 ;
	double slk		 = 0   ;
	CP*    CP_minslk = NULL;
	
	for( auto const &path: this->_pathlist )
	{
		if( path->getPathType() == NONE || path->getPathType() == PItoPO ) continue;
		slk = UpdatePathTiming( path, 1, 1, 1, 1, 1 );
		
		if( slk < Min_slack )
		{
			Min_slack = slk ;
			CP_minslk = path;
		}
		
	}
	if( Min_slack < 0 )
		Tc_PV -= Min_slack - 0.0000001;
	else//Min_slack > 0
		Tc_PV -= ( Min_slack - 0.0000001 );
	if( CP_minslk )
	{
		string PathType = "PItoFF";
		if( CP_minslk->getPathType() == FFtoFF ) PathType = "FFtoFF";
		if( CP_minslk->getPathType() == FFtoPO ) PathType = "FFtoPO";
		printf("\t MCP(%4ld, %s) \t", CP_minslk->getPathNum(), PathType.c_str() );
	}
	return Tc_PV;
}











