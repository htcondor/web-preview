#include <stdlib.h>
#include <assert.h>
#include "mipnode.hh"
#include <MW.h>
#include <MWRMComm.h>

MIPNode::MIPNode()
{
  depth = 0;
  numBounds = 0;
  boundIx = NULL;
  whichBound = NULL;
  boundVal = NULL;
  bestLowerBound = -DBL_MAX;
  bestEstimate = DBL_MAX;
}

MIPNode::~MIPNode()
{
  delete [] boundIx;
  delete [] whichBound;
  delete [] boundVal;
}

void MIPNode::MWPack( MWRMComm *MWComm )
{
  MWComm->pack( &depth, 1, 1 );
  MWComm->pack( &numBounds, 1, 1 );

  if( numBounds > 0 )
    {
      MWComm->pack( boundIx, numBounds, 1 );
      MWComm->pack( whichBound, numBounds, 1 );
      MWComm->pack( boundVal, numBounds, 1 );
    }
  MWComm->pack( &bestLowerBound, 1, 1 );
  MWComm->pack( &bestEstimate, 1, 1 );

}

void MIPNode::MWUnpack( MWRMComm *MWComm )
{
  MWComm->unpack( &depth, 1, 1 );
  MWComm->unpack( &numBounds, 1, 1 );

  if( numBounds > 0 )
    {
      boundIx = new int [numBounds];
      whichBound = new char [numBounds];
      boundVal = new double [numBounds];

      MWComm->unpack( boundIx, numBounds, 1 );
      MWComm->unpack( whichBound, numBounds, 1 );
      MWComm->unpack( boundVal, numBounds, 1 );
    }
  MWComm->unpack( &bestLowerBound, 1, 1 );
  MWComm->unpack( &bestEstimate, 1, 1 );
}

ostream&  operator<< (ostream& out, const MIPNode& node)
{
  out << "Depth: " << node.depth << endl
      << "lower bound: " << node.bestLowerBound << endl
      << "estimate: " << node.bestEstimate 
      << " Bounds: " << endl;
  for( int i = 0; i < node.numBounds; i++ )
    {
      out << "  x" << node.boundIx[i] << " " 
	  << (node.whichBound[i] == 'L' ? ">= " : "<= ")
	  << node.boundVal[i] << endl;
    }

  return out;
}

void MIPNode::write( FILE *fp )
{
  fprintf ( fp, "%d %f %f %d \n", 
	    depth, bestLowerBound, bestEstimate, numBounds );
  
  for( int i = 0; i < numBounds; i++ ){
    fprintf ( fp, "%d %c %f \n", 
	      boundIx[i], whichBound[i], boundVal[i] );
  }
  
  return;
}


void MIPNode::read( FILE *fp )
{
  fscanf ( fp, "%d %lf %lf %d", 
	   &depth, &bestLowerBound, &bestEstimate, &numBounds );

  boundIx = new int [numBounds];
  whichBound = new char [numBounds];
  boundVal = new double [numBounds];

  for( int i = 0; i < numBounds; i++ ){
    fscanf ( fp, "%d %c %lf", 
	     &boundIx[i], &whichBound[i], &boundVal[i] );
  }
  
  return;
}

