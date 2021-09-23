#ifndef LSHAPED_DRIVER_STATS_H
#define LSHAPED_DRIVER_STATS_H

#include <iostream.h>
#include <stdio.h>

/// A forward declaration.
class LShapedTask;

/** A simple class to keep track of some (application) statistics.
 */
class LShapedDriverStats
{

public:
  /// Constructor
  LShapedDriverStats();

  /// Destructor
  ~LShapedDriverStats();

  /// Update stats with result of task
  void Update( LShapedTask * );

  /// Write to fp
  void write( FILE *fp );

  /// Read from fp
  void read( FILE *fp );

  friend ostream &operator<<( ostream &o, LShapedDriverStats &s );

private:
  int tasksCompleted;
  double totalTaskTime;
  
};

#endif
