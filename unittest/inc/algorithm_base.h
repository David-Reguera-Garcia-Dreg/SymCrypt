//
// Algorithm_base.h
// base classes for algorithm implementations
//
// Copyright (c) Microsoft Corporation. Licensed under the MIT license. 
// 

//
// AlgorithmImplementation class
// This is the abstact class that represents the common properties
// of all algorithm implementations.
//
class AlgorithmImplementation
{
public:
    AlgorithmImplementation();
    virtual ~AlgorithmImplementation() {};
    
private:    
    AlgorithmImplementation( const AlgorithmImplementation & );
    VOID operator =( const AlgorithmImplementation & );
    
public:

    std::string m_algorithmName;                // Name of algorithm
    std::string m_modeName;                     // Name of algorithm mode
    std::string m_implementationName;           // Name of implementation

    virtual VOID setPerfKeySize( SIZE_T keySize ) {UNREFERENCED_PARAMETER(keySize);};
    PerfKeyFn   m_perfKeyFunction;
    PerfDataFn  m_perfDataFunction;
    PerfDataFn  m_perfDecryptFunction;
    PerfCleanFn m_perfCleanFunction;
    
    //
    // During functional testing we test all implementations of a single algorithm
    // in parallel. This makes debugging bugs triggered by the pseudo-random test cases
    // much easier.
    // When we check the (intermediate or final) result there are three types of errors we can encounter:
    // - Result disagrees with majority of other implementations of the same algorithm
    // - Results disagree but there is no majority to find out what result is correct
    // - Result agrees with majority but not with KAT values.
    //
    // These counters count how often each of these cases happens.
    //
    ULONGLONG   m_nErrorDisagreeWithMajority;
    ULONGLONG   m_nErrorNoMajority;
    ULONGLONG   m_nErrorKatFailure;

    //
    // Number of times this algorithm has produced a result during the test
    //
    ULONGLONG   m_nResults;

    //
    // Performance information
    //
    typedef struct _ALG_PERF_INFO
    {
        SIZE_T                  keySize;        // key size to add to row header. (0 if not used)
        char *                  strPostfix;     // postfix string, must be 3 characters long
        double                  cFixed;         // clocks of fixed overhead. 
        double                  cPerByte;       // clocks average cost per byte (used only for linear records, 0 for non-linear records)
        double                  cRange;         // 90 percentile of deviation from prediction by previous two numbers
    } ALG_PERF_INFO;

    std::vector<ALG_PERF_INFO> m_perfInfo;
};

class HashImplementation: public AlgorithmImplementation
{
public:
    HashImplementation() {};
    virtual ~HashImplementation() {};

private:
    HashImplementation( const HashImplementation & );
    VOID operator=( const HashImplementation & );

public:
    virtual SIZE_T resultLen() = 0;
        // Return the result length of this hash
        
    virtual SIZE_T inputBlockLen() = 0;
        // Return the input block length of this hash

    virtual VOID init() = 0;
        // Initialize for a new hash computation.
        
    virtual VOID append( _In_reads_( cbData ) PCBYTE pbData, SIZE_T cbData ) = 0;
        // Append data to the running hash computation.
        
    virtual VOID result( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult ) = 0;
        // Get the result of the running hash computation.

    virtual VOID hash( _In_reads_( cbData ) PCBYTE pbData, SIZE_T cbData, 
                       _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult );
        // Single hash computation.
        // The default implementation calls init/append/result so implementations that do not
        // have a separate compute-hash function can call the generic implementation in this
        // class.

    virtual NTSTATUS initWithLongMessage( ULONGLONG nBytes ) = 0;
        // nBytes is a multiple of the input block length.
        // Set the computation to the state as if it has processed a message nBytes long
        // which resulted in the internal chaining state having the value with every
        // byte equal to the character 'b'.
        // This allows us to test carry-handling of the message length counters. (A known
        // problem area.)
        // Return zero if success, NT status error if not supported.

    virtual NTSTATUS exportSymCryptFormat( 
            _Out_writes_bytes_to_( cbResultBufferSize, *pcbResult ) PBYTE   pbResult, 
            _In_                                                    SIZE_T  cbResultBufferSize, 
            _Out_                                                   SIZE_T *pcbResult ) = 0;
};

#define MAX_PARALLEL_HASH_STATES        32
#define MAX_PARALLEL_HASH_OPERATIONS    128

class ParallelHashImplementation: public AlgorithmImplementation
{
public:
    ParallelHashImplementation() {};
    virtual ~ParallelHashImplementation() {};

private:
    ParallelHashImplementation( const ParallelHashImplementation & );
    VOID operator=( const ParallelHashImplementation & );

public:

    virtual PCSYMCRYPT_HASH SymCryptHash() = 0;
        // Return a pointer to the SymCrypt implementation of the equivalent hash algorithm.

    virtual SIZE_T resultLen() = 0;
        // Return the result length of this hash
        
    virtual SIZE_T inputBlockLen() = 0;
        // Return the input block length of this hash

    virtual VOID init( SIZE_T nHashes ) = 0;
        // Initialize for a new hash computation.
        // nHashes = # hash states, nHashes <= MAX_PARALLEL_HASH_STATES
        
    virtual VOID process( 
        _In_reads_( nOperations )   BCRYPT_MULTI_HASH_OPERATION *   pOperations,
                                    SIZE_T                          nOperations ) = 0;
        // Process BCrypt-style operations on the parallel hash state

    virtual NTSTATUS initWithLongMessage( ULONGLONG nBytes ) = 0;
        // nBytes is a multiple of the input block length.
        // Set the computation to the state as if it has processed a message nBytes long
        // which resulted in the internal chaining state having the value with every
        // byte equal to the character 'b'.
        // This allows us to test carry-handling of the message length counters. (A known
        // problem area.)
        // Return zero if success, NT status error if not supported.

};

class MacImplementation: public AlgorithmImplementation
{
public:
    MacImplementation() {};
    ~MacImplementation() {};

private:
    MacImplementation( const MacImplementation & );
    VOID operator=( const MacImplementation & );

public:
    virtual SIZE_T resultLen() = 0;
        // return the result length of this MAC

    virtual SIZE_T inputBlockLen() = 0;
        // return the input block length of this MAC

    virtual NTSTATUS init( _In_reads_( cbKey ) PCBYTE pbKey, SIZE_T cbKey ) = 0;
        // Start a new MAC computation with the given key.
        // Return zero if success, NT status error if not supported.

    virtual VOID append( _In_reads_( cbData ) PCBYTE pbData, SIZE_T cbData ) = 0;
        // Append data to the running MAC computation.
        
    virtual VOID result( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult ) = 0;
        // Get the result of the running MAC computation.

    virtual NTSTATUS mac( _In_reads_( cbKey )      PCBYTE pbKey,   SIZE_T cbKey, 
                          _In_reads_( cbData )     PCBYTE pbData,  SIZE_T cbData, 
                          _Out_writes_( cbResult )  PBYTE pbResult, SIZE_T cbResult );
        // Complete a full MAC computation.
        // The default implementation merely calls the init/append/result members.
        // Return zero if success, NT status error if not supported.
};

class BlockCipherImplementation: public AlgorithmImplementation
//
// Implements block cipher encryption modes.
// Data is always a multiple of the block length.
// The chaining value is used for CBC/CTR/CFB/etc mode, but is length 0 for ECB
//
{
public:
    BlockCipherImplementation() {};
    virtual ~BlockCipherImplementation() {};

private:
    BlockCipherImplementation( const BlockCipherImplementation & );
    VOID operator=( const BlockCipherImplementation & );

public:
    virtual SIZE_T  msgBlockLen() = 0;

    virtual SIZE_T  chainBlockLen() = 0;

    virtual SIZE_T coreBlockLen() = 0;

    virtual NTSTATUS setKey( PCBYTE pbKey, SIZE_T cbKey ) = 0;

    virtual VOID encrypt( 
        _Inout_updates_opt_( cbChain )  PBYTE pbChain, 
                                        SIZE_T cbChain, 
        _In_reads_( cbData )            PCBYTE pbSrc, 
        _Out_writes_( cbData )          PBYTE pbDst, 
                                        SIZE_T cbData ) = 0;

    virtual VOID decrypt( 
        _Inout_updates_opt_( cbChain )  PBYTE pbChain, 
                                        SIZE_T cbChain, 
        _In_reads_( cbData )            PCBYTE pbSrc, 
        _Out_writes_( cbData )          PBYTE pbDst, 
                                        SIZE_T cbData ) = 0;

};


class AuthEncImplementation: public AlgorithmImplementation
//
// Implements authenticated encryption modes.
//
{
public:
    AuthEncImplementation() {};
    virtual ~AuthEncImplementation() {};

private:
    AuthEncImplementation( const AuthEncImplementation & );
    VOID operator=( const AuthEncImplementation & );

public:
    virtual std::set<SIZE_T> getNonceSizes() = 0;

    virtual std::set<SIZE_T> getTagSizes() = 0;

    virtual std::set<SIZE_T> getKeySizes() = 0;

    virtual NTSTATUS setKey( _In_reads_( cbKey ) PCBYTE pbKey, SIZE_T cbKey ) = 0;

    // The encrypt/decrypt can be called in two ways.
    // First: process an entire message in one call. This requires no flags.
    // Second: incremental processing of a message.
    // For incremental processing, the AUTHENC_FLAG_PARTIAL flag is passed to all
    // calls that are part of the incremental processing.
    // All authdata has to be passed in the first incremental call.
    // The last incremental call is marked by a nonzero pbTag.
    // setTotalCbData() must be called before each sequence of incremental calls.
    // Implementations that don't do incremental processing can simply return 
    // STATUS_NOT_SUPPORTED for all incremental calls.

#define AUTHENC_FLAG_PARTIAL 1   

    virtual VOID setTotalCbData( SIZE_T cbData ) = 0;   // Set total cbData up front for partial processing (used by CCM)
                
    virtual NTSTATUS encrypt(
        _In_reads_( cbNonce )       PCBYTE  pbNonce,      
                                    SIZE_T  cbNonce, 
        _In_reads_( cbAuthData )    PCBYTE  pbAuthData, 
                                    SIZE_T  cbAuthData, 
        _In_reads_( cbData )        PCBYTE  pbSrc, 
        _Out_writes_( cbData )      PBYTE   pbDst, 
                                    SIZE_T  cbData,
        _Out_writes_( cbTag )       PBYTE   pbTag, 
                                    SIZE_T  cbTag,
                                    ULONG   flags ) = 0;

    virtual NTSTATUS decrypt(
        _In_reads_( cbNonce )       PCBYTE  pbNonce,      
                                    SIZE_T  cbNonce, 
        _In_reads_( cbAuthData )    PCBYTE  pbAuthData, 
                                    SIZE_T  cbAuthData, 
        _In_reads_( cbData )        PCBYTE  pbSrc, 
        _Out_writes_( cbData )      PBYTE   pbDst, 
                                    SIZE_T  cbData,
        _In_reads_( cbTag )         PCBYTE  pbTag, 
                                    SIZE_T  cbTag,
                                    ULONG   flags ) = 0;
};

class XtsImplementation: public AlgorithmImplementation
{
public:
    XtsImplementation() {};
    virtual ~XtsImplementation() {};

private:
    XtsImplementation( const XtsImplementation & );
    VOID operator=( const XtsImplementation & );

public:
    virtual NTSTATUS setKey( PCBYTE pbKey, SIZE_T cbKey ) = 0;

    virtual VOID encrypt( 
                                        SIZE_T      cbDataUnit,
                                        ULONGLONG   tweak,
        _In_reads_( cbData )            PCBYTE      pbSrc, 
        _Out_writes_( cbData )          PBYTE       pbDst, 
                                        SIZE_T      cbData ) = 0;

    virtual VOID decrypt( 
                                        SIZE_T      cbDataUnit,
                                        ULONGLONG   tweak,
        _In_reads_( cbData )            PCBYTE      pbSrc, 
        _Out_writes_( cbData )          PBYTE       pbDst, 
                                        SIZE_T      cbData ) = 0;

};

class StreamCipherImplementation: public AlgorithmImplementation
{
public:
    StreamCipherImplementation() {};
    virtual ~StreamCipherImplementation() {};

private:
    StreamCipherImplementation( const StreamCipherImplementation & );
    VOID operator=( const StreamCipherImplementation & );

public:
    virtual std::set<SIZE_T> getNonceSizes() = 0;

    virtual std::set<SIZE_T> getKeySizes() = 0;

    virtual NTSTATUS setKey( _In_reads_( cbKey ) PCBYTE pbKey, SIZE_T cbKey ) = 0;

    virtual NTSTATUS setNonce( _In_reads_( cbKey ) PCBYTE pbNonce, SIZE_T cbNonce ) = 0;

    virtual BOOL isRandomAccess() = 0;

    virtual VOID setOffset( UINT64 offset ) = 0;

    virtual VOID encrypt( 
        _In_reads_( cbData )    PCBYTE  pbSrc, 
        _Out_writes_( cbData )  PBYTE   pbDst, 
                                SIZE_T  cbData ) = 0;
};


class RngSp800_90Implementation: public AlgorithmImplementation
{
public:
    RngSp800_90Implementation() {};
    virtual ~RngSp800_90Implementation() {};

private:
    RngSp800_90Implementation( const RngSp800_90Implementation & );
    VOID operator=( const RngSp800_90Implementation & );

public:
    virtual NTSTATUS instantiate( _In_reads_( cbEntropy ) PCBYTE pbEntropy, SIZE_T cbEntropy ) = 0;
    virtual NTSTATUS reseed( _In_reads_( cbEntropy ) PCBYTE pbEntropy, SIZE_T cbEntropy ) = 0;
    virtual VOID generate( _Out_writes_( cbData ) PBYTE pbData, SIZE_T cbData ) = 0;
};


//
// KDF implementation
//

typedef enum _KDF_ARGUMENT_TYPE {
    KdfArgumentGeneric = 1,            // numeric values are used in KAT files, do not change.
    KdfArgumentPbkdf2 = 2,
    KdfArgumentSp800_108 = 3,
    KdfArgumentTlsPrf = 4,
    KdfArgumentHkdf = 5,
} KDF_ARGUMENT_TYPE;

typedef struct _KDF_GENERIC_ARGUMENTS {
    PCBYTE      pbSelector;
    SIZE_T      cbSelector;
} KDF_GENERIC_ARGUMENTS;

typedef struct _KDF_PBKDF2_ARGUMENTS {
    PCBYTE      pbSalt;
    SIZE_T      cbSalt;
    ULONGLONG   iterationCnt;
} KDF_PBKDF2_ARGUMENTS;

typedef struct _KDF_SP800_108_ARGUMENTS {
    PCBYTE      pbLabel;
    SIZE_T      cbLabel;
    PCBYTE      pbContext;
    SIZE_T      cbContext;
} KDF_SP800_108_ARGUMENTS;

typedef struct _KDF_TLSPRF_ARGUMENTS {
    PCBYTE      pbLabel;
    SIZE_T      cbLabel;
    PCBYTE      pbSeed;
    SIZE_T      cbSeed;
} KDF_TLSPRF_ARGUMENTS;

typedef struct _KDF_HKDF_ARGUMENTS {
    PCBYTE      pbSalt;
    SIZE_T      cbSalt;
    PCBYTE      pbInfo;
    SIZE_T      cbInfo;
} KDF_HKDF_ARGUMENTS;

typedef struct _KDF_ARGUMENTS {
    KDF_ARGUMENT_TYPE   argType;
    union {
        KDF_GENERIC_ARGUMENTS   uGeneric;
        KDF_PBKDF2_ARGUMENTS    uPbkdf2;
        KDF_SP800_108_ARGUMENTS uSp800_108;
        KDF_TLSPRF_ARGUMENTS    uTlsPrf;
        KDF_HKDF_ARGUMENTS      uHkdf;
    };
} KDF_ARGUMENTS, *PKDF_ARGUMENTS;        
typedef const KDF_ARGUMENTS *PCKDF_ARGUMENTS;

class KdfImplementation: public AlgorithmImplementation
{
public:
    KdfImplementation() {};
    virtual ~KdfImplementation() {};

private:
    KdfImplementation( const KdfImplementation & );
    VOID operator=( const KdfImplementation & );

public:

    virtual VOID derive( 
        _In_reads_( cbKey )     PCBYTE          pbKey,
                                SIZE_T          cbKey,
        _In_                    PKDF_ARGUMENTS  args,
        _Out_writes_( cbDst )   PBYTE           pbDst, 
                                SIZE_T          cbDst ) = 0;

};

class TlsCbcHmacImplementation: public AlgorithmImplementation
{
public:
    TlsCbcHmacImplementation() {};
    ~TlsCbcHmacImplementation() {};

private:
    TlsCbcHmacImplementation( const TlsCbcHmacImplementation & );
    VOID operator=( const TlsCbcHmacImplementation & );

public:

    virtual NTSTATUS verify(
        _In_reads_( cbKey )     PCBYTE  pbKey,
                                SIZE_T  cbKey,
        _In_reads_( cbHeader )  PCBYTE  pbHeader,
                                SIZE_T  cbHeader,
        _In_reads_( cbData )    PCBYTE  pbData,
                                SIZE_T  cbData ) = 0;
    // Verify a TLS 1.2 CBC HMAC padded record in constant time
};

// ArithImplementation class is only used for performance measurement
class ArithImplementation: public AlgorithmImplementation
{
public:
    ArithImplementation() {};
    virtual ~ArithImplementation() {};

private:
    ArithImplementation( const ArithImplementation & );
    VOID operator=( const ArithImplementation & );

public:
};

// RsaImplementation class is only used for performance measurements of RSA
class RsaImplementation: public AlgorithmImplementation
{
public:
    RsaImplementation() {};
    virtual ~RsaImplementation() {};

private:
    RsaImplementation( const RsaImplementation & );
    VOID operator=( const RsaImplementation & );

public:
};

// DlImplementation class is only used for performance measurements of Discrete Log group algorithms
class DlImplementation: public AlgorithmImplementation
{
public:
    DlImplementation() {};
    virtual ~DlImplementation() {};

private:
    DlImplementation( const DlImplementation & );
    VOID operator=( const DlImplementation & );

public:
};

// EccImplementation class is only used for performance measurements of elliptic curve cryptography
class EccImplementation: public AlgorithmImplementation
{
public:
    EccImplementation() {};
    virtual ~EccImplementation() {};

private:
    EccImplementation( const EccImplementation & );
    VOID operator=( const EccImplementation & );

public:
};


//////////////////////////////////////////////////////////////////////////////////
// Template classes for actual concrete implementations
//

//
// A template class to store the state of a hash implementation in.
//
template< class Implementation, class Algorithm> class HashImpState;

//
// A template class for the actual hash algorithm implementations
//
template< class Implementation, class Algorithm > 
class HashImp: public HashImplementation
{
public:
    HashImp();
    virtual ~HashImp();

private:
    HashImp( const HashImp & );
    VOID operator=( const HashImp & );

public:

    static const String s_algName;             // Algorithm name
    static const String s_modeName;
    static const String s_impName;             // Implementation name

    virtual SIZE_T resultLen();
    virtual SIZE_T inputBlockLen();

    virtual void init();
    virtual void append( _In_reads_( cbData ) PCBYTE pbData, SIZE_T cbData );
    virtual void result( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult );
    virtual VOID hash( 
        _In_reads_( cbData )       PCBYTE pbData, 
                                    SIZE_T cbData, 
        _Out_writes_( cbResult )    PBYTE pbResult, 
                                    SIZE_T cbResult );
    virtual NTSTATUS initWithLongMessage( ULONGLONG nBytes );
    virtual NTSTATUS exportSymCryptFormat( 
            _Out_writes_bytes_to_( cbResultBufferSize, *pcbResult ) PBYTE   pbResult, 
            _In_                                                    SIZE_T  cbResultBufferSize, 
            _Out_                                                   SIZE_T *pcbResult );

    HashImpState<Implementation,Algorithm> state;
};

//
// A template class to store the state of a hash implementation in.
//
template< class Implementation, class Algorithm> class ParallelHashImpState;

//
// A template class for the actual hash algorithm implementations
//
template< class Implementation, class Algorithm > 
class ParallelHashImp: public ParallelHashImplementation
{
public:
    ParallelHashImp();
    virtual ~ParallelHashImp();

private:
    ParallelHashImp( const ParallelHashImp & );
    VOID operator=( const ParallelHashImp & );

public:

    static const String s_algName;             // Algorithm name
    static const String s_modeName;
    static const String s_impName;             // Implementation name

    virtual PCSYMCRYPT_HASH SymCryptHash();

    virtual SIZE_T resultLen();
        
    virtual SIZE_T inputBlockLen();

    virtual VOID init( SIZE_T nHashes );
        
    virtual VOID process( 
        _In_reads_( nOperations )   BCRYPT_MULTI_HASH_OPERATION *   pOperations,
                                    SIZE_T                          nOperations );
        

    virtual NTSTATUS initWithLongMessage( ULONGLONG nBytes );

    ParallelHashImpState<Implementation,Algorithm> state;
};


//
// Template class to store the state of a MAC implementation
//
template< class Implementation, class Algorithm> class MacImpState;

//
// Template class for the actual MAC implementations
//
template< class Implementation, class Algorithm > 
class MacImp: public MacImplementation
{
public:
    MacImp();
    virtual ~MacImp();

private:
    MacImp( const MacImp & );
    VOID operator=( const MacImp & );

public:

    static const String s_algName;
    static const String s_modeName;
    static const String s_impName;
    
    virtual SIZE_T resultLen();
    virtual SIZE_T inputBlockLen();
    
    virtual NTSTATUS init( _In_reads_( cbKey ) PCBYTE pbKey, SIZE_T cbKey );
    virtual VOID append( _In_reads_( cbData ) PCBYTE pbData, SIZE_T cbData );
    virtual VOID result( _Out_writes_( cbResult ) PBYTE pbResult, SIZE_T cbResult );
    virtual NTSTATUS mac( 
        _In_reads_( cbKey )        PCBYTE pbKey, 
                                    SIZE_T cbKey, 
        _In_reads_( cbData )       PCBYTE pbData, 
                                    SIZE_T cbData, 
        _Out_writes_( cbResult )    PBYTE pbResult, 
                                    SIZE_T cbResult );

    MacImpState<Implementation,Algorithm> state;
};


template< class Implementation, class Algorithm, class Mode > class BlockCipherImpState;

template< class Implementation, class Algorithm, class Mode >
class BlockCipherImp: public BlockCipherImplementation
{
public:
    BlockCipherImp();
    virtual ~BlockCipherImp();

private:
    BlockCipherImp( const BlockCipherImp & );
    VOID operator=( const BlockCipherImp & );

public:
    static const String s_algName;
    static const String s_modeName;
    static const String s_impName;
    
    virtual SIZE_T msgBlockLen();       // block length of mode (msg must be multiple of this)
    virtual SIZE_T chainBlockLen();     // length of chaining field

    virtual SIZE_T coreBlockLen();      // block length of underlying cipher

    virtual NTSTATUS setKey( _In_reads_( cbKey ) PCBYTE pbKey, SIZE_T cbKey );
    virtual VOID encrypt( 
        _Inout_updates_opt_( cbChain )   PBYTE pbChain, 
                                        SIZE_T cbChain, 
        _In_reads_( cbData )           PCBYTE pbSrc, 
        _Out_writes_( cbData )          PBYTE pbDst, 
                                        SIZE_T cbData );
    virtual VOID decrypt( 
        _Inout_updates_opt_( cbChain )   PBYTE pbChain, 
                                        SIZE_T cbChain, 
        _In_reads_( cbData )           PCBYTE pbSrc, 
        _Out_writes_( cbData )          PBYTE pbDst, 
                                        SIZE_T cbData );

    BlockCipherImpState< Implementation, Algorithm, Mode > state;

};

template< class Implementation, class Algorithm > class XtsImpState;

template< class Implementation, class Algorithm >
class XtsImp: public XtsImplementation
{
public:
    XtsImp();
    virtual ~XtsImp();

private:
    XtsImp( const XtsImp & );
    VOID operator=( const XtsImp & );

public:
    static const String s_algName;
    static const String s_modeName;
    static const String s_impName;
    
    virtual NTSTATUS setKey( PCBYTE pbKey, SIZE_T cbKey );

    virtual VOID encrypt( 
                                        SIZE_T      cbDataUnit,
                                        ULONGLONG   tweak,
        _In_reads_( cbData )            PCBYTE      pbSrc, 
        _Out_writes_( cbData )          PBYTE       pbDst, 
                                        SIZE_T      cbData );

    virtual VOID decrypt( 
                                        SIZE_T      cbDataUnit,
                                        ULONGLONG   tweak,
        _In_reads_( cbData )            PCBYTE      pbSrc, 
        _Out_writes_( cbData )          PBYTE       pbDst, 
                                        SIZE_T      cbData );

    XtsImpState< Implementation, Algorithm > state;
};

template< class Implementation, class Algorithm, class Mode >
SIZE_T BlockCipherImp<Implementation, Algorithm, Mode>::chainBlockLen()
{
    if( (Mode::flags & MODE_FLAG_CHAIN) == 0 )
    {
        return 0;
    }

    return coreBlockLen();
}

template< class Implementation, class Algorithm, class Mode >
SIZE_T BlockCipherImp<Implementation, Algorithm, Mode>::msgBlockLen()
{
    if( (Mode::flags & MODE_FLAG_CFB) != 0 )
    {
        return g_modeCfbShiftParam;
    }

    return coreBlockLen();
}

template< class Implementation, class Algorithm, class Mode> class AuthEncImpState;

template< class Implementation, class Algorithm, class Mode >
class AuthEncImp: public AuthEncImplementation
{
public:
    AuthEncImp();
    virtual ~AuthEncImp();

private:
    AuthEncImp( const AuthEncImp & );
    VOID operator=( const AuthEncImp & );

public:
    static const String s_algName;
    static const String s_modeName;
    static const String s_impName;
    
    virtual std::set<SIZE_T> getNonceSizes();

    virtual std::set<SIZE_T> getTagSizes();

    virtual std::set<SIZE_T> getKeySizes();

    virtual NTSTATUS setKey( PCBYTE pbKey, SIZE_T cbKey );

    virtual VOID setTotalCbData( SIZE_T cbData );

    virtual NTSTATUS encrypt(   
        _In_reads_( cbNonce )       PCBYTE  pbNonce,      
                                    SIZE_T  cbNonce, 
        _In_reads_( cbAuthData )    PCBYTE  pbAuthData, 
                                    SIZE_T  cbAuthData, 
        _In_reads_( cbData )        PCBYTE  pbSrc, 
        _Out_writes_( cbData )      PBYTE   pbDst, 
                                    SIZE_T  cbData,
        _Out_writes_( cbTag )       PBYTE   pbTag, 
                                    SIZE_T  cbTag,
                                    ULONG   flags );
        // returns an error only if the request is not supported; only allowed for partial requests.

    virtual NTSTATUS decrypt(   
        _In_reads_( cbNonce )       PCBYTE  pbNonce,      
                                    SIZE_T  cbNonce, 
        _In_reads_( cbAuthData )    PCBYTE  pbAuthData, 
                                    SIZE_T  cbAuthData, 
        _In_reads_( cbData )        PCBYTE  pbSrc, 
        _Out_writes_( cbData )      PBYTE   pbDst, 
                                    SIZE_T  cbData,
        _In_reads_( cbTag )         PCBYTE  pbTag, 
                                    SIZE_T  cbTag,
                                    ULONG   flags );
        // returns STATUS_AUTH_TAG_MISMATCH if the tag is wrong.
        // returns STATUS_NOT_SUPPORTED if the request is not supported (only for partial requests)

    AuthEncImpState< Implementation, Algorithm, Mode > state;

};


template< class Implementation, class Algorithm> class StreamCipherImpState;

template< class Implementation, class Algorithm>
class StreamCipherImp: public StreamCipherImplementation
{
public:
    StreamCipherImp();
    virtual ~StreamCipherImp();

private:
    StreamCipherImp( const StreamCipherImp & );
    VOID operator=( const StreamCipherImp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
    static const BOOL   s_isRandomAccess;
    
    virtual std::set<SIZE_T> getNonceSizes();

    virtual std::set<SIZE_T> getKeySizes();

    virtual NTSTATUS setKey( _In_reads_( cbKey ) PCBYTE pbKey, SIZE_T cbKey );

    virtual NTSTATUS setNonce( _In_reads_( cbKey ) PCBYTE pbNonce, SIZE_T cbNonce );

    virtual BOOL isRandomAccess() { return s_isRandomAccess; };

    virtual VOID setOffset( UINT64 offset );

    virtual VOID encrypt( 
        _In_reads_( cbData )    PCBYTE  pbSrc, 
        _Out_writes_( cbData )  PBYTE   pbDst, 
                                SIZE_T  cbData );

    StreamCipherImpState< Implementation, Algorithm> state;
};


template< class Implementation, class Algorithm> class RngSp800_90ImpState;

template< class Implementation, class Algorithm>
class RngSp800_90Imp: public RngSp800_90Implementation
{
public:
    RngSp800_90Imp();
    virtual ~RngSp800_90Imp();

private:
    RngSp800_90Imp( const RngSp800_90Imp & );
    VOID operator=( const RngSp800_90Imp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
    
    virtual NTSTATUS instantiate( _In_reads_( cbEntropy ) PCBYTE pbEntropy, SIZE_T cbEntropy ) ;
    virtual NTSTATUS reseed( _In_reads_( cbEntropy ) PCBYTE pbEntropy, SIZE_T cbEntropy );
    virtual VOID generate( _Out_writes_( cbData ) PBYTE pbData, SIZE_T cbData );

    RngSp800_90ImpState< Implementation, Algorithm> state;
};

template< class Implementation, class Algorithm, class BaseAlg > class KdfImpState;

template< class Implementation, class Algorithm, class BaseAlg >
class KdfImp: public KdfImplementation
{
public:
    KdfImp();
    virtual ~KdfImp();

private:
    KdfImp( const KdfImp & );
    VOID operator=( const KdfImp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
    
    virtual VOID derive( 
        _In_reads_( cbKey )     PCBYTE          pbKey,
                                SIZE_T          cbKey,
        _In_                    PKDF_ARGUMENTS  args,
        _Out_writes_( cbDst )   PBYTE           pbDst, 
                                SIZE_T          cbDst );

    KdfImpState<Implementation,Algorithm,BaseAlg> state;
};

template< class Implementation, class Algorithm > 
class TlsCbcHmacImp: public TlsCbcHmacImplementation
{
public:
    TlsCbcHmacImp();
    virtual ~TlsCbcHmacImp();

private:
    TlsCbcHmacImp( const TlsCbcHmacImp & );
    VOID operator=( const TlsCbcHmacImp & );

public:
    static const String s_algName;
    static const String s_modeName;
    static const String s_impName;

    virtual NTSTATUS verify(
        _In_reads_( cbKey )     PCBYTE  pbKey,
                                SIZE_T  cbKey,
        _In_reads_( cbHeader )  PCBYTE  pbHeader,
                                SIZE_T  cbHeader,
        _In_reads_( cbData )    PCBYTE  pbData,
                                SIZE_T  cbData );

};

template< class Implementation, class Algorithm>
class ArithImp: public ArithImplementation
{
public:
    ArithImp();
    virtual ~ArithImp();

private:
    ArithImp( const ArithImp & );
    VOID operator=( const ArithImp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
};

template< class Implementation, class Algorithm>
class RsaImp: public RsaImplementation
{
public:
    RsaImp();
    virtual ~RsaImp();

private:
    RsaImp( const RsaImp & );
    VOID operator=( const RsaImp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
};

template< class Implementation, class Algorithm>
class DlImp: public DlImplementation
{
public:
    DlImp();
    virtual ~DlImp();

private:
    DlImp( const DlImp & );
    VOID operator=( const DlImp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
};

template< class Implementation, class Algorithm>
class EccImp: public EccImplementation
{
public:
    EccImp();
    virtual ~EccImp();

private:
    EccImp( const EccImp & );
    VOID operator=( const EccImp & );

public:
    static const String s_impName;
    static const String s_modeName;
    static const String s_algName;
};


//
// The stub classes we use to distinguish our implementations and algorithms contain the
// name of said implementation/algorithm. We use this to auto-define the algorithm name
// and implementation name of the *Imp<imp,alg> classes.
//
template< class Implementation, class Algorithm >
const String HashImp<Implementation,Algorithm>::s_algName = Algorithm::name;

template< class Implementation, class Algorithm >
const String HashImp<Implementation,Algorithm>::s_modeName;

template< class Implementation, class Algorithm >
const String HashImp<Implementation,Algorithm>::s_impName = Implementation::name;

template< class Implementation, class Algorithm >
const String ParallelHashImp<Implementation,Algorithm>::s_algName = Algorithm::name;

template< class Implementation, class Algorithm >
const String ParallelHashImp<Implementation,Algorithm>::s_modeName;

template< class Implementation, class Algorithm >
const String ParallelHashImp<Implementation,Algorithm>::s_impName = Implementation::name;


template< class Implementation, class Algorithm >
const String MacImp<Implementation,Algorithm>::s_algName = Algorithm::name;

template< class Implementation, class Algorithm >
const String MacImp<Implementation,Algorithm>::s_modeName;

template< class Implementation, class Algorithm >
const String MacImp<Implementation,Algorithm>::s_impName = Implementation::name;

template< class Implementation, class Algorithm, class Mode >
const String BlockCipherImp<Implementation,Algorithm,Mode>::s_algName = Algorithm::name ;

template< class Implementation, class Algorithm, class Mode >
const String BlockCipherImp<Implementation,Algorithm,Mode>::s_modeName = Mode::name ;

template< class Implementation, class Algorithm, class Mode >
const String BlockCipherImp<Implementation,Algorithm,Mode>::s_impName = Implementation::name;

template< class Implementation, class Algorithm, class Mode >
const String AuthEncImp<Implementation, Algorithm, Mode>::s_algName = Algorithm::name;

template< class Implementation, class Algorithm, class Mode >
const String AuthEncImp<Implementation,Algorithm,Mode>::s_modeName = Mode::name ;

template< class Implementation, class Algorithm, class Mode >
const String AuthEncImp<Implementation,Algorithm,Mode>::s_impName = Implementation::name;


template< class Implementation, class Algorithm>
const String StreamCipherImp<Implementation,Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String StreamCipherImp<Implementation,Algorithm>::s_modeName;
template< class Implementation, class Algorithm>
const String StreamCipherImp<Implementation,Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const BOOL StreamCipherImp<Implementation,Algorithm>::s_isRandomAccess = Algorithm::isRandomAccess;

template< class Implementation, class Algorithm>
const String RngSp800_90Imp<Implementation,Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String RngSp800_90Imp<Implementation,Algorithm>::s_modeName;
template< class Implementation, class Algorithm>
const String RngSp800_90Imp<Implementation,Algorithm>::s_impName = Implementation::name;

template< class Implementation, class Algorithm, class BaseAlg >
const String KdfImp<Implementation,Algorithm,BaseAlg>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm, class BaseAlg >
const String KdfImp<Implementation,Algorithm,BaseAlg>::s_modeName = BaseAlg::name;
template< class Implementation, class Algorithm, class BaseAlg >
const String KdfImp<Implementation,Algorithm,BaseAlg>::s_impName = Implementation::name;

template< class Implementation, class Algorithm>
const String XtsImp<Implementation, Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const String XtsImp<Implementation, Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String XtsImp<Implementation, Algorithm>::s_modeName;

template< class Implementation, class Algorithm>
const String TlsCbcHmacImp<Implementation, Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const String TlsCbcHmacImp<Implementation, Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String TlsCbcHmacImp<Implementation, Algorithm>::s_modeName;

template< class Implementation, class Algorithm>
const String ArithImp<Implementation, Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const String ArithImp<Implementation, Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String ArithImp<Implementation, Algorithm>::s_modeName;

template< class Implementation, class Algorithm>
const String RsaImp<Implementation, Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const String RsaImp<Implementation, Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String RsaImp<Implementation, Algorithm>::s_modeName;

template< class Implementation, class Algorithm>
const String DlImp<Implementation, Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const String DlImp<Implementation, Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String DlImp<Implementation, Algorithm>::s_modeName;

template< class Implementation, class Algorithm>
const String EccImp<Implementation, Algorithm>::s_impName = Implementation::name;
template< class Implementation, class Algorithm>
const String EccImp<Implementation, Algorithm>::s_algName = Algorithm::name;
template< class Implementation, class Algorithm>
const String EccImp<Implementation, Algorithm>::s_modeName;

//
// Template declaration for performance functions (for those implementations that wish to use them)
//
template< class Implementation, class Algorithm >
VOID algImpKeyPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T keySize );

template< class Implementation, class Algorithm, class Mode >
VOID algImpKeyPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T keySize );

template< class Implementation, class Algorithm >
VOID algImpDataPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T dataSize );

template< class Implementation, class Algorithm, class Mode >
VOID algImpDataPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T dataSize );

template< class Implementation, class Algorithm >
VOID algImpDecryptPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T dataSize );

template< class Implementation, class Algorithm, class Mode >
VOID algImpDecryptPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3, SIZE_T dataSize );

template< class Implementation, class Algorithm >
VOID algImpCleanPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3 );

template< class Implementation, class Algorithm, class Mode >
VOID algImpCleanPerfFunction( PBYTE buf1, PBYTE buf2, PBYTE buf3 );

;


