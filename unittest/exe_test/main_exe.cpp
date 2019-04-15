//
// Main_test.cpp
// Main file for SymCrypt unit test program
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license.
//

#include "precomp.h"
#include "msbignum_implementations.h"
#include "rsa32_implementations.h"
#include "capi_implementations.h"
#include "cng_implementations.h"
#include "sc_implementations.h"
#include "ref_implementations.h"

SYMCRYPT_ENVIRONMENT_DEFS( Unittest );

char * g_implementationNames[] = 
{
    ImpSc::name,
    ImpRsa32::name,
    ImpRsa32b::name,
    ImpCapi::name,
    ImpCng::name,
    ImpMsBignum::name,
    ImpRef::name,
    NULL,
};

#include "main_exe_common.cpp"

int __cdecl
main( int argc, _In_reads_( argc ) char * argv[] )
{

    initTestInfrastructure( argc, argv );

    TestSaveXmmEnabled = TRUE;
    TestSaveYmmEnabled = TRUE;

    addCapiAlgs();
    addRsa32Algs();
    addCngAlgs();
    addMsBignumAlgs();
    addSymCryptAlgs();
    addRefAlgs();

    if (!g_profile)
    {
        runFunctionalTests();
    }
    
    TestSaveXmmEnabled = FALSE;
    TestSaveYmmEnabled = FALSE;

    if (g_profile)
    {
        runProfiling();
    }
    else
    {
        runPerfTests();

        testMultiThread();

        testSelftest();
    }

    exitTestInfrastructure();

    return 0;
}

