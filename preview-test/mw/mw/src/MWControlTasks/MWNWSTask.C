#include "MWNWSTask.h"

MWNWSTask::MWNWSTask ( double interval, double length )
{
	taskType = MWNWS;
	timeInterval = interval;
	timeLength = length;
	min = max = median = -1;
}

MWNWSTask::MWNWSTask ( )
{
	taskType = MWNWS;
	min = max = median = -1;
}

MWNWSTask::~MWNWSTask()
{
}

void 
MWNWSTask::pack_work( void )
{
	RMC->pack ( &timeInterval, 1, 1 );
	RMC->pack ( &timeLength, 1, 1 );
}

void 
MWNWSTask::unpack_work( void )
{
	RMC->unpack ( &timeInterval, 1, 1 );
	RMC->unpack ( &timeLength, 1, 1 );
}
    
void 
MWNWSTask::pack_results( void )
{
	RMC->pack ( &max, 1, 1 );
	RMC->pack ( &min, 1, 1 );
	RMC->pack ( &median, 1, 1 );
}
    
void 
MWNWSTask::unpack_results( void )
{
	RMC->unpack ( &max, 1, 1 );
	RMC->unpack ( &min, 1, 1 );
	RMC->unpack ( &median, 1, 1 );
}

void 
MWNWSTask::printself( int level = 60 )
{
	MWprintf ( level, "n:%d Intervak:%f Length:%f\n", timeInterval, 
								timeLength );
	if ( max > 0 )
		MWprintf ( level, "results:- min:%f max:%f median:%f\n",
						min, max, median );
}

void 
MWNWSTask::write_ckpt_info( FILE *fp )
{
	fprintf ( fp, "%f %f ", timeInterval, timeLength );
	if ( max >= 0 )
		fprintf ( fp, "%f %f %f", max, min, median );
	else
		fprintf ( fp, "-1 " );
}

void 
MWNWSTask::read_ckpt_info( FILE *fp )
{
	fscanf ( fp, "%f %f ", &timeInterval, &timeLength );
	fscanf ( fp, "%f ", &max );
	if ( max >= 0 )
		fscanf ( fp, "%f %f ", &min, &median );
}
