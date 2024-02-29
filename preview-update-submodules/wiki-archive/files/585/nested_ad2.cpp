#include <stdio.h>

#include "classad/classad_distribution.h"

int main( int argc, char **argv )
{
	classad::ClassAdParser parser;
	classad::ClassAdUnParser unparser;
	classad::ClassAd ad;
	classad::ExprTree *tree;

	bool do_caching = true;
	if ( argc > 1 ) {
		do_caching = atoi( argv[1] ) != 0;
	}

	ad.InsertAttr( "A", 4 );
	if ( !parser.ParseExpression( "{ [ Y = 1; Z = A; ] }", tree ) ) {
		printf("Failed to parse 2a\n" );
		return 1;
	}
	ad.Insert( "B", tree, do_caching );
	if ( !parser.ParseExpression( "B[0].Z", tree ) ) {
		printf("Failed to parse 2b\n" );
		return 1;
	}
	ad.Insert( "C", tree, do_caching );

	std::string str;
	unparser.Unparse( str, &ad );
	printf( "ad: %s\n", str.c_str() );

	int result;
	if ( !ad.EvaluateAttrInt( "A", result ) ) {
		printf( "A did not evaluate to an integer\n" );
	} else {
		printf( "A is %d\n", result );
	}

	if ( !ad.EvaluateAttrInt( "C", result ) ) {
		printf( "C did not evaluate to an integer\n" );
	} else {
		printf( "C is %d\n", result );
	}

	return 0;
}
