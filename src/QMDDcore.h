// QMDDinclude.h

// includes the files needed to use the QMDD package


#include <sstream>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
//#include "qcost.c"
#include "qcost.h"
//#include "timing.c"
#include "timing.h"
//#include "textFileUtilities.c"  	// basic text handling routines
#include "textFileUtilities.h"  	// basic text handling routines
#include "QMDDpackage.h"		// constants, type definitions and globals
#include "QMDDcomplex.h"
//#ifdef __HADAMARD__
//#include "QMDDcomplexH.c"		// complex Hadamard number package
//#else
//#include "QMDDcomplexD.c"		// complex number package
//#endif
//#include "QMDDpackage.c"    		// QMDD package
//#ifdef __SignatureSynthesis__
//#include "QMDDsigstuff.c"  		// include signature stuff
//#endif
#include "QMDDreorder.h"  		// sifting
#include "QMDDcircuit.h"		// procedures for building a QMDD from a circuit file

