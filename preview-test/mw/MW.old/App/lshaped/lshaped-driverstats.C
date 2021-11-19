#include "lshaped-driverstats.h"
#include "lshaped-task.h"

LShapedDriverStats::LShapedDriverStats()
{
  tasksCompleted = 0;
  totalTaskTime = 0.0;
}

LShapedDriverStats::~LShapedDriverStats()
{
}

void LShapedDriverStats::Update( LShapedTask *lst )
{
  tasksCompleted++;
  totalTaskTime += lst->taskTime;
}

void LShapedDriverStats::write( FILE *fp )
{
  if ( fp )
    fprintf( fp, "%d %lf\n", tasksCompleted, totalTaskTime );
}

void LShapedDriverStats::read( FILE *fp )
{
  if ( fp )
    fscanf( fp, "%d %lf", &tasksCompleted, &totalTaskTime );
}

ostream &operator<<( ostream &out, LShapedDriverStats &s )
{

  int oldp = out.precision( 2 );

  out << "Total Number of Tasks: " << s.tasksCompleted << endl
      << "Average Task Time: " 
      << ( s.totalTaskTime / s.tasksCompleted ) 
      << " seconds" << endl;

  out.precision( oldp );

  return out;
}

