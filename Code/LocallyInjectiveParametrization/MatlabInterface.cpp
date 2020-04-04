#include <cassert>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "MatlabInterface.h"

#include "engine.h"  // Matlab engine header

#define ERROR(msg)   std::cerr << "ERROR: "   << msg << std::endl
#define WARNING(msg) std::cerr << "WARNING: " << msg << std::endl
#define MESSAGE(msg) std::cerr << msg << std::endl

#include <sys/stat.h>
// from http://www.codeproject.com/KB/files/filesize.aspx
inline static long FileSize(const char* fname)
{
    struct stat finfo;
    if (stat(fname, &finfo) == 0)
        return finfo.st_size;
    else
        return 0;
}

////////////////////////////////////////////////////////////
// Explicit instantiation of exported template methods

// Creates a matrix in matlab.
template int MatlabInterface::SetEngineIndexMatrix         <int>(const char *name, unsigned int m, unsigned int n, const                  int *vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::SetEngineIndexMatrix<unsigned int>(const char *name, unsigned int m, unsigned int n, const         unsigned int *vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::SetEngineRealMatrix       <double>(const char *name, unsigned int m, unsigned int n, const               double *vals, bool colmaj);
template int MatlabInterface::SetEngineComplexMatrix    <double>(const char *name, unsigned int m, unsigned int n, const std::complex<double> *vals, bool colmaj);
template int MatlabInterface::SetEngineRealMatrix       <float>(const char *name, unsigned int m, unsigned int n, const               float *vals, bool colmaj);
template int MatlabInterface::SetEngineComplexMatrix    <float>(const char *name, unsigned int m, unsigned int n, const std::complex<float> *vals, bool colmaj);

template
int MatlabInterface::SetEngineEncodedSparseRealMatrix<unsigned int, double>(const char *name, unsigned int n,
        const unsigned int *rowind, const unsigned int *colind, const double *vals);

template
int MatlabInterface::SetEngineSparseRealMatrix<int, double>(const char *name, unsigned int n,
        const int *rowind, const int *colind, const double *vals, unsigned int nrows, unsigned int ncols);
template
int MatlabInterface::SetEngineSparseRealMatrix<unsigned int, double>(const char *name, unsigned int n,
        const unsigned int *rowind, const unsigned int *colind, const double *vals, unsigned int nrows, unsigned int ncols);

template
int MatlabInterface::SetEngineEncodedSparseComplexMatrix<unsigned int, double>(const char *name, unsigned int n,
        const unsigned int *rowind, const unsigned int *colind, const std::complex<double> *vals);

template
int MatlabInterface::SetEngineSparseComplexMatrix<unsigned int, double>(const char *name, unsigned int n,
        const unsigned int *rowind, const unsigned int *colind, const std::complex<double> *vals, unsigned int nrows, unsigned int ncols);

// Reads a matrix from matlab.
template int MatlabInterface::GetEngineIndexMatrix<int>(const char *name, unsigned int m, unsigned int n,         int *vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::GetEngineIndexMatrix<unsigned int>(const char *name, unsigned int m, unsigned int n,         unsigned int *vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::GetEngineRealMatrix       <double>(const char *name, unsigned int m, unsigned int n,               double *vals, bool colmaj);
template int MatlabInterface::GetEngineComplexMatrix    <double>(const char *name, unsigned int m, unsigned int n, std::complex<double> *vals, bool colmaj);
template int MatlabInterface::GetEngineRealMatrix       <float>(const char *name, unsigned int m, unsigned int n,               float *vals, bool colmaj);
template int MatlabInterface::GetEngineComplexMatrix    <float>(const char *name, unsigned int m, unsigned int n, std::complex<float> *vals, bool colmaj);


// Reads a variable-sized matrix from matlab.
template int MatlabInterface::GetEngineIndexMatrix<int>(const char *name, unsigned int& m, unsigned int& n, std::vector<         int>& vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::GetEngineIndexMatrix<unsigned int>(const char *name, unsigned int& m, unsigned int& n, std::vector<         unsigned int>& vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::GetEngineRealMatrix       <double>(const char *name, unsigned int& m, unsigned int& n, std::vector<         double      >& vals, bool colmaj);
template int MatlabInterface::GetEngineComplexMatrix    <double>(const char *name, unsigned int& m, unsigned int& n, std::vector<std::complex<double> >& vals, bool colmaj);


// matrix creation helper functions
template mxArray* MatlabInterface::CreateIndexMatrix<unsigned int>(unsigned int m, unsigned int n, const         unsigned int *vals, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template mxArray* MatlabInterface::CreateRealMatrix       <double>(unsigned int m, unsigned int n, const               double *vals, bool colmaj);
template mxArray* MatlabInterface::CreateComplexMatrix    <double>(unsigned int m, unsigned int n, const std::complex<double> *vals, bool colmaj);

template
mxArray* MatlabInterface::CreateEncodedSparseRealMatrix<unsigned int, double>(unsigned int n,
        const unsigned int *rowind, const unsigned int *colind, const double *vals);

template
mxArray* MatlabInterface::CreateEncodedSparseComplexMatrix<unsigned int, double>(unsigned int n,
        const unsigned int *rowind, const unsigned int *colind, const std::complex<double> *vals);

// matrix copy-back helper functions
template int MatlabInterface::CopyFromIndexMatrix<unsigned int>(mxArray *M, unsigned int m, unsigned int n,         unsigned int *dest, bool colmaj); // shifts indices by 1 (C/C++ vs matlab)
template int MatlabInterface::CopyFromRealMatrix       <double>(mxArray *M, unsigned int m, unsigned int n,               double *dest, bool colmaj);
template int MatlabInterface::CopyFromComplexMatrix    <double>(mxArray *M, unsigned int m, unsigned int n, std::complex<double> *dest, bool colmaj);

////////////////////////////////////////////////////////////

MatlabInterface::MatlabInterface() : m_A(NULL), m_b(NULL), m_x(NULL), m_ep(NULL)
{
    EngineOpen();
}



MatlabInterface::~MatlabInterface()
{
    Deinitialize();
//  EngineClose();
}

void
MatlabInterface::EngineOpen()
{
    // Start the MATLAB engine locally by executing the string
    // "matlab"
    //
    // To start the session on a remote host, use the name of
    // the host as the string rather than \0
    //
    // For more complicated cases, use any string with whitespace,
    // and that string will be executed literally to start MATLAB
    //
    if (!(m_ep = engOpen("\0"))) {
        ERROR("Can't start MATLAB engine");
    }

	int res = engSetVisible(m_ep, false); //turn the white matlab command window OFF
	res = engEvalString(m_ep, "desktop");  //turn the entire GUI of matlab ON
}

void
MatlabInterface::EngineClose()
{
    engClose(m_ep);
    m_ep = NULL;
}


void
MatlabInterface::DestroyMatrix(mxArray *&M)
{
    if (M) {
        mxDestroyArray(M);
        M = NULL;
    }
}



void
MatlabInterface::Deinitialize()
{
    DestroyMatrix(m_A);
    DestroyMatrix(m_b);
    DestroyMatrix(m_x);
}


// This also shifts all the values by 1 to account for the difference
// between matlab and C/C++ indexing
template <typename T>
mxArray*
MatlabInterface::CreateIndexMatrix(unsigned int m, unsigned int n, const T *vals, bool colmaj)
{
    mxArray *M = mxCreateDoubleMatrix(m, n, mxREAL);
    if (vals != NULL) {
        double *pM = mxGetPr(M);
        // note that matlab expects the data in column-major order
        for (unsigned int j = 0; j < n; ++j)
            for (unsigned int i = 0; i < m; ++i)
            {
                unsigned int idxM = j*m+i;
                unsigned int idx = colmaj ? idxM : i*n+j;
                pM[idxM] = double(vals[idx]+1);
            }
    }
    return M;
}



template <typename T>
mxArray*
MatlabInterface::CreateRealMatrix(unsigned int m, unsigned int n, const T *vals, bool colmaj)
{
    mxArray *M = mxCreateDoubleMatrix(m, n, mxREAL);
    if (vals != NULL) {
        double *pM = mxGetPr(M);
        // note that matlab expects the data in column-major order
        for (unsigned int j = 0; j < n; ++j)
            for (unsigned int i = 0; i < m; ++i)
            {
                unsigned int idxM = j*m+i;
                unsigned int idx = colmaj ? idxM : i*n+j;
                pM[idxM] = double(vals[idx]);
            }
    }
    return M;
}



template <typename T>
mxArray*
MatlabInterface::CreateComplexMatrix(unsigned int m, unsigned int n, const std::complex<T> *vals, bool colmaj)
{
    mxArray *M = mxCreateDoubleMatrix(m, n, mxCOMPLEX);
    if (vals != NULL) {
        /*double *pMr = mxGetPr(M);
        double *pMi = mxGetPi(M);*/
		//
		mxComplexDouble *pMc = mxGetComplexDoubles(M);
		//
        // note that matlab expects the data in column-major order
        for (unsigned int j = 0; j < n; ++j)
            for (unsigned int i = 0; i < m; ++i)
            {
                unsigned int idxM = j*m+i;
                unsigned int idx = colmaj ? idxM : i*n+j;
                /*pMr[idxM] = double(vals[idx].real());
                pMi[idxM] = double(vals[idx].imag());*/
				//
				pMc[idxM].real = double(vals[idx].real());
				pMc[idxM].imag = double(vals[idx].imag());
				//
            }
    }
    return M;
}

//template <typename T>
//mxArray*
//MatlabInterface::CreateComplexMatrix(unsigned int m, unsigned int n, const std::complex<T> *vals, bool colmaj)
//{
//	mxComplexDouble *pMc = NULL;
//	//mxComplexDouble *pMc;
//	
//	if (vals != NULL) {
//		pMc = (mxComplexDouble*)mxMalloc(m * n * sizeof(mxComplexDouble));
//		for (unsigned int j = 0; j < n; ++j) {
//			for (unsigned int i = 0; i < m; ++i) {
//				unsigned int idxM = j*m + i;
//				unsigned int idx = colmaj ? idxM : i*n + j;
//				pMc[idxM].real = double(vals[idx].real());
//				pMc[idxM].imag = double(vals[idx].imag());
//			}
//		}
//	}
//
//	mxArray *M = NULL;
//	if (pMc != NULL) {
//		M = mxCreateDoubleMatrix(0, 0, mxCOMPLEX);
//		mxSetComplexDoubles(M, pMc);
//		mxSetM(M, m);
//		mxSetN(M, n);
//	}
//	else {
//		M = mxCreateDoubleMatrix(m, n, mxCOMPLEX);
//	}
//	
//	return M;
//}



template <typename IndexType, typename ValueType>
mxArray*
MatlabInterface::CreateEncodedSparseRealMatrix(unsigned int n,
        const IndexType *rowind, const IndexType *colind, const ValueType *vals)
{
    mxArray *M = mxCreateDoubleMatrix(n, 3, mxREAL);
    if (vals != NULL) {
        assert(rowind != NULL);
        assert(colind != NULL);
        double *pM = mxGetPr(M);
        // note that matlab expects the data in column-major order
        for (unsigned int i = 0; i < n; ++i)
        {
            pM[0*n+i] = double(rowind[i]+1);
            pM[1*n+i] = double(colind[i]+1);
            pM[2*n+i] = double(  vals[i]  );
        }
    }
    return M;
}



template <typename IndexType, typename ValueType>
mxArray*
MatlabInterface::CreateEncodedSparseComplexMatrix(unsigned int n,
        const IndexType *rowind, const IndexType *colind, const std::complex<ValueType> *vals)
{
    mxArray *M = mxCreateDoubleMatrix(n, 3, mxCOMPLEX);
    if (vals != NULL) {
        assert(rowind != NULL);
        assert(colind != NULL);
        /*double *pMr = mxGetPr(M);
        double *pMi = mxGetPi(M);*/
		//
		mxComplexDouble *pMc = mxGetComplexDoubles(M);
		//
        // note that matlab expects the data in column-major order
        for (unsigned int i = 0; i < n; ++i)
        {
            /*pMr[0*n+i] = double(rowind[i]+1); pMi[0*n+i] = double(0);
            pMr[1*n+i] = double(colind[i]+1); pMi[1*n+i] = double(0);
            pMr[2*n+i] = double(vals[i].real());
            pMi[2*n+i] = double(vals[i].imag());*/
			//
			pMc[0 * n + i].real = double(rowind[i] + 1); pMc[0 * n + i].imag = double(0);
			pMc[1 * n + i].real = double(colind[i] + 1); pMc[1 * n + i].imag = double(0);
			pMc[2 * n + i].real = double(vals[i].real());
			pMc[2 * n + i].imag = double(vals[i].imag());
			//
        }
    }
    return M;
}



mxArray*
MatlabInterface::CreateStringArray(unsigned int n, const char **val)
{
    assert(val != NULL);
    mxArray *M = mxCreateCharMatrixFromStrings(n, val);
    assert(M);
    return M;
}




// This also shifts all the values by 1 to account for the difference
// between matlab and C/C++ indexing
template <typename T>
int
MatlabInterface::CopyFromIndexMatrix(mxArray *M, unsigned int m, unsigned int n, T *dest, bool colmaj)
{
    assert(dest != NULL);
    if ( mxGetM(M) != m || mxGetN(M) != n ) {
        ERROR("CopyFromIndexMatrix: expected size " << m << "x" << n << ". Got " << mxGetM(M) << "x" << mxGetN(M) << ".");
        return 1;
    }
    assert(mxGetM(M) == m);
    assert(mxGetN(M) == n);
    double *pM = mxGetPr(M);
    // note that matlab expects the data in column-major order
    for (unsigned int j = 0; j < n; ++j)
        for (unsigned int i = 0; i < m; ++i)
        {
            unsigned int idxM = j*m+i;
            unsigned int idx = colmaj ? idxM : i*n+j;
            dest[idx] = T(pM[idxM]-1);
        }

    return 0;
}

template <typename T>
int
MatlabInterface::CopyFromRealMatrix(mxArray *M, unsigned int m, unsigned int n, T *dest, bool colmaj)
{
    assert(dest != NULL);
    if ( mxGetM(M) != m || mxGetN(M) != n ) {
        ERROR("CopyFromRealMatrix: expected size " << m << "x" << n << ". Got " << mxGetM(M) << "x" << mxGetN(M) << ".");
        return 1;
    }
    assert(mxGetM(M) == m);
    assert(mxGetN(M) == n);
    double *pM = mxGetPr(M);
    // note that matlab expects the data in column-major order
    for (unsigned int j = 0; j < n; ++j)
        for (unsigned int i = 0; i < m; ++i)
        {
            unsigned int idxM = j*m+i;
            unsigned int idx = colmaj ? idxM : i*n+j;
            dest[idx] = T(pM[idxM]);
        }

    return 0;
}


//template <typename T>
//int
//MatlabInterface::CopyFromComplexMatrix(mxArray *M, unsigned int m, unsigned int n, std::complex<T> *dest, bool colmaj)
//{
//    assert(dest != NULL);
//    if ( mxGetM(M) != m || mxGetN(M) != n ) {
//        ERROR("CopyFromComplexMatrix: expected size " << m << "x" << n << ". Got " << mxGetM(M) << "x" << mxGetN(M) << ".");
//        return 1;
//    }
//    assert(mxGetM(M) == m);
//    assert(mxGetN(M) == n);
//    double *pMr = mxGetPr(M);
//    double *pMi = mxGetPi(M);
//	
//    bool pure_real = (pMi == NULL); // seems to be NULL when the matrix doesn't have complex values
//
//    // note that matlab expects the data in column-major order
//    for (unsigned int j = 0; j < n; ++j)
//        for (unsigned int i = 0; i < m; ++i)
//        {
//            unsigned int idxM = j*m+i;
//            unsigned int idx = colmaj ? idxM : i*n+j;
//			dest[idx] = std::complex<double>(T(pMr[idxM]), pure_real ? T(0) : T(pMi[idxM]));
//        }
//
//    return 0;
//}

template <typename T>
int
MatlabInterface::CopyFromComplexMatrix(mxArray *M, unsigned int m, unsigned int n, std::complex<T> *dest, bool colmaj)
{
	assert(dest != NULL);
	if (mxGetM(M) != m || mxGetN(M) != n) {
		ERROR("CopyFromComplexMatrix: expected size " << m << "x" << n << ". Got " << mxGetM(M) << "x" << mxGetN(M) << ".");
		return 1;
	}
	assert(mxGetM(M) == m);
	assert(mxGetN(M) == n);
	
	if (mxIsComplex(M)) {
		mxComplexDouble *pMc = mxGetComplexDoubles(M);
		// note that matlab expects the data in column-major order
		for (unsigned int j = 0; j < n; ++j)
			for (unsigned int i = 0; i < m; ++i)
			{
				unsigned int idxM = j*m + i;
				unsigned int idx = colmaj ? idxM : i*n + j;
				dest[idx] = std::complex<double>(T(pMc[idxM].real), T(pMc[idxM].imag));
			}
	}
	else {
		double *pM = mxGetDoubles(M);
		for (unsigned int j = 0; j < n; ++j)
			for (unsigned int i = 0; i < m; ++i)
			{
				unsigned int idxM = j*m + i;
				unsigned int idx = colmaj ? idxM : i*n + j;
				dest[idx] = std::complex<double>(T(pM[idxM]), T(0));
			}
	}

	return 0;
}

// Creates a matrix in the Matlab engine.
template <typename T>
int
MatlabInterface::SetEngineIndexMatrix(const char *name, unsigned int m, unsigned int n, const T *vals, bool colmaj) // shifts indices by 1 (C/C++ vs matlab)
{
    mxArray *ary = CreateIndexMatrix(m, n, vals, colmaj);
    assert(ary);
    int res = engPutVariable(m_ep, name, ary);
    mxDestroyArray(ary);
    return res;
}

template <typename T>
int
MatlabInterface::SetEngineRealMatrix(const char *name, unsigned int m, unsigned int n, const T *vals, bool colmaj)
{
    mxArray *ary = CreateRealMatrix(m, n, vals, colmaj);
    assert(ary);
    int res = engPutVariable(m_ep, name, ary);
    mxDestroyArray(ary);
    return res;
}

template <typename T>
int
MatlabInterface::SetEngineComplexMatrix(const char *name, unsigned int m, unsigned int n, const std::complex<T> *vals, bool colmaj)
{
    mxArray *ary = CreateComplexMatrix(m, n, vals, colmaj);
    assert(ary);
    int res = engPutVariable(m_ep, name, ary);
    mxDestroyArray(ary);
    return res;
}

template <typename IndexType, typename ValueType>
int
MatlabInterface::SetEngineEncodedSparseRealMatrix(const char *name, unsigned int n,
        const IndexType *rowind, const IndexType *colind, const ValueType *vals)
{
    mxArray *ary = CreateEncodedSparseRealMatrix(n, rowind, colind, vals);
    assert(ary);
    int res = engPutVariable(m_ep, name, ary);
    mxDestroyArray(ary);
    return res;
}

template <typename IndexType, typename ValueType>
int
MatlabInterface::SetEngineSparseRealMatrix(const char *name, unsigned int n,
        const IndexType *rowind, const IndexType *colind, const ValueType *vals, unsigned int nrows, unsigned int ncols)
{
    int res = SetEngineEncodedSparseRealMatrix(name, n, rowind, colind, vals);
    if (res != 0) return res;
    char cmd[1024];
    if (ncols == 0 || nrows == 0) {
        sprintf(cmd, "%s = sparse(%s(:,1), %s(:,2), %s(:,3));", name, name, name, name);
    }
    else {
        assert(ncols > 0 && nrows > 0);
        sprintf(cmd, "%s = sparse(%s(:,1), %s(:,2), %s(:,3), %d, %d);", name, name, name, name, nrows, ncols);
    }
    res = engEvalString(m_ep, cmd);
    return res;
}

template <typename IndexType, typename ValueType>
int
MatlabInterface::SetEngineEncodedSparseComplexMatrix(const char *name, unsigned int n,
        const IndexType *rowind, const IndexType *colind, const std::complex<ValueType> *vals)
{
    mxArray *ary = CreateEncodedSparseComplexMatrix(n, rowind, colind, vals);
    assert(ary);
    int res = engPutVariable(m_ep, name, ary);
    mxDestroyArray(ary);
    return res;
}

template <typename IndexType, typename ValueType>
int
MatlabInterface::SetEngineSparseComplexMatrix(const char *name, unsigned int n,
        const IndexType *rowind, const IndexType *colind, const std::complex<ValueType> *vals, unsigned int nrows, unsigned int ncols)
{
    int res = SetEngineEncodedSparseComplexMatrix(name, n, rowind, colind, vals);
    if (res != 0) return res;
    char cmd[1024];
    if (ncols == 0 || nrows == 0) {
        sprintf(cmd, "%s = sparse(%s(:,1), %s(:,2), %s(:,3));", name, name, name, name);
    }
    else {
        assert(ncols > 0 && nrows > 0);
        sprintf(cmd, "%s = sparse(%s(:,1), %s(:,2), %s(:,3), %d, %d);", name, name, name, name, nrows, ncols);
    }
    res = engEvalString(m_ep, cmd);
    return res;
}

int MatlabInterface::CreateAllZerosSparseMatrix(const char* name, int nRows, int nCols)
{
	char cmd[1024];
	sprintf(cmd, "%s = sparse(%d, %d);", name, nRows, nCols);
	int res = engEvalString(m_ep, cmd);
	return res;
}

// Create a string in matlab.
int
MatlabInterface::SetEngineStringArray(const char *name, unsigned int n, const char **val)
{
    assert(val != NULL);
    mxArray *ary = CreateStringArray(n, val);
    assert(ary);
    int res = engPutVariable(m_ep, name, ary);
    mxDestroyArray(ary);
    return res;
}


// Reads a matrix from the matlab engine.
template <typename T>
int
MatlabInterface::GetEngineIndexMatrix(const char *name, unsigned int m, unsigned int n, T *dest, bool colmaj)
{
    assert(dest != NULL);
    mxArray *ary = engGetVariable(m_ep, name);
    if (ary == NULL) return 2;
    assert(ary != NULL);
    int res = CopyFromIndexMatrix(ary, m, n, dest, colmaj);
    mxDestroyArray(ary);
    return res;
}

template <typename T>
int
MatlabInterface::GetEngineRealMatrix(const char *name, unsigned int m, unsigned int n, T *dest, bool colmaj)
{
    assert(dest != NULL);
    mxArray *ary = engGetVariable(m_ep, name);
    if (ary == NULL) return 2;
    assert(ary != NULL);
    int res = CopyFromRealMatrix(ary, m, n, dest, colmaj);
    mxDestroyArray(ary);
    return res;
}



template <typename T>
int
MatlabInterface::GetEngineComplexMatrix(const char *name, unsigned int m, unsigned int n, std::complex<T> *dest, bool colmaj)
{
    assert(dest != NULL);
    mxArray *ary = engGetVariable(m_ep, name);
    if (ary == NULL) return 2;
    assert(ary != NULL);
    int res = CopyFromComplexMatrix(ary, m, n, dest, colmaj);
    mxDestroyArray(ary);
    return res;
}

// Reads a matrix from the matlab engine.
template <typename T>
int
MatlabInterface::GetEngineIndexMatrix(const char* name, unsigned int& m, unsigned int& n, std::vector<T>& dest, bool colmaj)
{
    mxArray *ary = engGetVariable(m_ep, name);
    if (ary == NULL)
    {
    	m = 0;
    	n = 0;
    	dest.clear();
    	return 2;
    }
    assert(ary != NULL);
    m = mxGetM(ary);
    n = mxGetN(ary);
    dest.resize(m*n);
    
    int res = CopyFromIndexMatrix(ary, m, n, &dest[0], colmaj);
    mxDestroyArray(ary);
    return res;
}

template <typename T>
int
MatlabInterface::GetEngineRealMatrix(const char* name, unsigned int& m, unsigned int& n, std::vector<T>& dest, bool colmaj)
{
    mxArray *ary = engGetVariable(m_ep, name);
    if (ary == NULL)
    {
    	m = 0;
    	n = 0;
    	dest.clear();
    	return 2;
    }
    assert(ary != NULL);
    m = mxGetM(ary);
    n = mxGetN(ary);
    dest.resize(m*n);
    
    int res = CopyFromRealMatrix(ary, m, n, &dest[0], colmaj);
    mxDestroyArray(ary);
    return res;
}



template <typename T>
int
MatlabInterface::GetEngineComplexMatrix(const char *name, unsigned int& m, unsigned int& n, std::vector<std::complex<T> >& dest, bool colmaj)
{
    mxArray *ary = engGetVariable(m_ep, name);
    if (ary == NULL)
    {
    	m = 0;
    	n = 0;
    	dest.clear();
    	return 2;
    }
    assert(ary != NULL);
    m = mxGetM(ary);
    n = mxGetN(ary);
    dest.resize(m*n);
    
    int res = CopyFromComplexMatrix(ary, m, n, &dest[0], colmaj);
    mxDestroyArray(ary);
    return res;
}


// Sets up the sparse matrix that defines the LHS of the system.
void
MatlabInterface::SetupLHS(unsigned int n, unsigned int nonzeros, unsigned int *row_indices, unsigned int *col_indices, double *mat_entries)
{
    assert(n > 0);

    SetEngineIndexMatrix("LS_rind", nonzeros, 1, row_indices);
    SetEngineIndexMatrix("LS_cind", nonzeros, 1, col_indices);
    SetEngineRealMatrix("LS_Aij" , nonzeros, 1, mat_entries);

    DestroyMatrix(m_A); // in case it had been previously created

    // create and retrieve the sparse matrix
    engEvalString(m_ep, "LS_A = sparse(LS_rind, LS_cind, LS_Aij)");

    //SetEngineSparseComplexMatrix("LS_A", nonzeros, row_indices, col_indices, mat_entries);
    m_A = engGetVariable(m_ep, "LS_A");

    // A should have been created just fine
    // XXX define better error behavior here?
    assert(m_A != NULL);

    // XXX define better error behavior here?
    assert(mxGetM(m_A) == n);
    assert(mxGetN(m_A) == n);
}



// Sets up the RHS vector of the system.
void
MatlabInterface::SetupRHS(unsigned int nb, double *b)
{
    DestroyMatrix(m_b); // in case it had been previously created
//  engEvalString(m_ep, "clear LS_b;"); // clear the variable name to let the error checking happen

    // create and retrieve the sparse matrix
    engPutVariable(m_ep, "LS_b", m_b = CreateRealMatrix(nb, 1, b));
    m_b = engGetVariable(m_ep, "LS_b");

    // A should have been created just fine
    // XXX define better error behavior here?
    assert(m_b != NULL);
}



// Solves an n x n (square) linear system Ax=b.
void
MatlabInterface::SolveSparseLinearSystem(unsigned int n, unsigned int nonzeros,
        unsigned int *row_indices, unsigned int *col_indices, double *mat_entries,
        double *b, double *x)
{
    assert(row_indices != NULL);
    assert(col_indices != NULL);
    assert(mat_entries != NULL);
    assert(b != NULL);
    assert(x != NULL);

    engEvalString(m_ep, "clear LS_*;"); // clear all the variables with prefix LS_

    SetupLHS(n, nonzeros, row_indices, col_indices, mat_entries);
    SetupRHS(n, b);

    DestroyMatrix(m_x); // in case it had been previously created

    // solve the system
    engEvalString(m_ep, "LS_x = LS_A \\ LS_b");

    // assert that the system was solved successfully
    // XXX define better error behavior here?
    assert(engGetVariable(m_ep, "LS_x") != NULL);

    GetEngineRealMatrix("LS_x", n, 1, x, false);
}


// Executes a matlab script.
// Returns non-zero on error.
int
MatlabInterface::LoadAndRunScript(const char *full_script_path, char *output_buffer, int buffer_size)
{
    assert(full_script_path != NULL);

    // Use RAII ensure that on leaving this scope, the output buffer is
    // always nullified (to prevent Matlab from accessing memory that might
    // have already been deallocated).
    struct cleanup {
        Engine *m_ep;
        cleanup(Engine *ep) : m_ep(ep) { }
        ~cleanup() { engOutputBuffer(m_ep, NULL, 0); }
    } cleanup_obj(m_ep);

    if (output_buffer != NULL) {
        int res = engOutputBuffer(m_ep, output_buffer, buffer_size);
        if (res != 0) {
            ERROR("Unable to set Matlab output buffer");
            return res;
        }
    }

    long size = FileSize(full_script_path);
    if (size <= 0) {
        ERROR("Matlab script \"" << full_script_path << "\" has size 0");
        return -1;
    }
    std::ifstream fin(full_script_path, std::ios::in|std::ios::binary);
    if (!fin.good() || !fin.is_open()) {
        ERROR("Unable to read from matlab script \"" << full_script_path << "\"");
        return -2;
    }

    std::string code(size+1, '\0');
    fin.read(&code[0], size);
    code[size] = '\0'; // unnecessary due to initialization, but being safe
    int res = engEvalString(m_ep, code.c_str());
    if (res != 0)
        ERROR("Error running matlab script \"" << full_script_path << "\"");

    return res;
}


std::string
MatlabInterface::LoadAndRunScriptToString(const char *full_script_path)
{
    const int BUF_SIZE = 4096*4096;
    // allocate on the heap to avoid running out of stack
    std::string bufauto(BUF_SIZE+1, '\0');
    char *buf = &bufauto[0];

    int res = this->LoadAndRunScript(full_script_path, buf, BUF_SIZE);
    if (res != 0) {
        std::ostringstream oss;
        oss << "ERROR: Matlab script '" << full_script_path
            << "' failed with error code " << res << ".\n";
        return oss.str();
    }

    if (buf[0] == '>' && buf[1] == '>' && buf[2] == ' ')
        buf += 3;
    if (buf[0] == '\n') ++buf;

    return std::string(buf);
}



int
MatlabInterface::RunScript(const char *script_name, char *output_buffer, int buffer_size)
{
    assert(script_name != NULL);

    std::string script_extension = GetExtension(script_name);
    if (script_extension.size() > 0 && script_extension != ".m")
    {
        ERROR("RunScript: Unknown matlab script extension '" << script_extension << "'.");
        return -99;
    }

    std::string script_prefix = GetPrefix(script_name);

    // Use RAII ensure that on leaving this scope, the output buffer is
    // always nullified (to prevent Matlab from accessing memory that might
    // have already been deallocated).
    struct cleanup {
        Engine *m_ep;
        cleanup(Engine *ep) : m_ep(ep) { }
        ~cleanup() { engOutputBuffer(m_ep, NULL, 0); }
    } cleanup_obj(m_ep);

    if (output_buffer != NULL) {
        int res = engOutputBuffer(m_ep, output_buffer, buffer_size);
        if (res != 0) {
            ERROR("Unable to set Matlab output buffer");
            return res;
        }
    }

    int res;
    engEvalString(m_ep, "clear functions;"); // to prevent Matlab from loading from cache
    res = engEvalString(m_ep, script_prefix.c_str());
    if (res != 0)
        ERROR("Error running matlab script \"" << script_name << "\"");

    return res;
}

std::string
MatlabInterface::RunScriptToString(const char *script_name)
{
    const int BUF_SIZE = 4096*4096;
    // allocate on the heap to avoid running out of stack
    std::string bufauto(BUF_SIZE+1, '\0');
    char *buf = &bufauto[0];

    int res = this->RunScript(script_name, buf, BUF_SIZE);
    if (res != 0) {
        std::ostringstream oss;
        oss << "ERROR: Matlab script '" << script_name
            << "' failed with error code " << res << ".\n";
        return oss.str();
    }

    if (buf[0] == '>' && buf[1] == '>' && buf[2] == ' ')
        buf += 3;
    if (buf[0] == '\n') ++buf;

    return std::string(buf);
}


// Evaluate a single line
// Returns non-zero on error.
int
MatlabInterface::Eval(const char *matlab_code, char *output_buffer, int buffer_size)
{
    assert(matlab_code != NULL);

    // Use RAII ensure that on leaving this scope, the output buffer is
    // always nullified (to prevent Matlab from accessing memory that might
    // have already been deallocated).
    struct cleanup {
        Engine *m_ep;
        cleanup(Engine *ep) : m_ep(ep) { }
        ~cleanup() { engOutputBuffer(m_ep, NULL, 0); }
    } cleanup_obj(m_ep);

    if (output_buffer != NULL) {
        int res = engOutputBuffer(m_ep, output_buffer, buffer_size);
        if (res != 0) {
            ERROR("Unable to set Matlab output buffer");
            return res;
        }
    }

    int res = engEvalString(m_ep, matlab_code);
    if (res != 0)
        ERROR("Error running matlab command \"" << matlab_code << "\"");
    return res;
}


std::string
MatlabInterface::EvalToString(const char *matlab_code)
{
    const int BUF_SIZE = 4096*4096;
    // allocate on the heap to avoid running out of stack
    std::string bufauto(BUF_SIZE+1, '\0');
    char *buf = &bufauto[0];

    int res = this->Eval(matlab_code, buf, BUF_SIZE);
    if (res != 0) {
        std::ostringstream oss;
        oss << "ERROR: Matlab command failed with error code " << res << ".\n";
        return oss.str();
    }

    if (buf[0] == '>' && buf[1] == '>' && buf[2] == ' ')
        buf += 3;
    if (buf[0] == '\n') ++buf;

    return std::string(buf);
}


int
MatlabInterface::AddScriptPath(const char *path)
{
    std::string matlab_addpath = std::string("addpath('") + path + "')";
    int res = this->Eval(matlab_addpath.c_str());
    return res;
}


// Get a singleton MatlabInterface object
MatlabInterface &MatlabInterface::GetEngine(bool restart)
{
    static MatlabInterface matlab;
    static bool first_time = true;
    if (restart && !first_time) {
        matlab.EngineClose();
        matlab.EngineOpen();
    }
    first_time = false;
    return matlab;
}


int MatlabInterface::GetSparseRealMatrix(const char* name, std::vector<unsigned int>& rowind, std::vector<unsigned int>& colind, std::vector<double>& vals, unsigned int& nentries, unsigned int& m, unsigned int& n)
{ 
	assert(name != NULL && name[0] != 0);

	bool res = GetMatrixDimensions(name, m, n);

	if(!res || m <= 0 || n <= 0)
	{
		return -1;
	}

	char cmd[1024];
	std::string tempName("temp_");
	tempName = tempName + name;

	sprintf(cmd, "clear %s; [%s(:, 1), %s(:, 2), %s(:, 3)] = find(%s);", tempName.c_str(), tempName.c_str(), tempName.c_str(), tempName.c_str(), name);

	int result = engEvalString(m_ep, cmd);
	
    mxArray *M = engGetVariable(m_ep, tempName.c_str());
  
	if(M == NULL)
	{
        WARNING("matrix " << name << " could not be loaded");
        return - 1;
    }
    
	nentries =  mxGetM(M);
    int ncols =  mxGetN(M);
    assert(ncols == 3); 
    
	double *pM = mxGetPr(M);
	if(pM == NULL) return -1;

    rowind.resize(nentries);
    colind.resize(nentries);
    vals.resize(nentries);

    for(unsigned int i = 0; i < nentries; ++i)
    {
        rowind[i] = (unsigned int)(pM[           i]) - 1; assert(rowind[i] >= 0); 
        colind[i] = (unsigned int)(pM[  nentries+i]) - 1; assert(colind[i] >= 0); 
        vals  [i] = (double      )(pM[2*nentries+i]);
    }
	mxDestroyArray(M);

	sprintf(cmd, "clear %s;", tempName.c_str());
	result = engEvalString(m_ep, cmd);

	return result;
}


int MatlabInterface::GetSparseComplexMatrix(const char* name, std::vector<unsigned int>& rowind, std::vector<unsigned int>& colind, std::vector<std::complex<double> >& vals, unsigned int& nentries, unsigned int& m, unsigned int& n)
{ 
	assert(name != NULL && name[0] != 0);

	bool res = GetMatrixDimensions(name, m, n);

	if(!res || m <= 0 || n <= 0)
	{
		return -1;
	}

	char cmd[1024];
	std::string tempName("temp_");
	tempName = tempName + name;

	sprintf(cmd, "clear %s; [%s(:, 1), %s(:, 2), %s(:, 3)] = find(%s);", tempName.c_str(), tempName.c_str(), tempName.c_str(), tempName.c_str(), name);

	int result = engEvalString(m_ep, cmd);

	mxArray *M = engGetVariable(m_ep, tempName.c_str());

	if(M == NULL)
	{
		WARNING("matrix " << name << " could not be loaded");
		return - 1;
	}

	nentries =  mxGetM(M);
	int ncols =  mxGetN(M);
	assert(ncols == 3); 

	//
	/*double *pMr = mxGetPr(M);
	double *pMi = mxGetPi(M);

	if(pMr == NULL || pMi == NULL) return -1;*/
	//
	mxComplexDouble *pMc = mxGetComplexDoubles(M);
	if (pMc == NULL) return -1;
	//

	rowind.resize(nentries);
	colind.resize(nentries);
	vals.resize(nentries);

	for(unsigned int i = 0; i < nentries; ++i)
	{
		/*rowind[i] = (unsigned int)(pMr[           i]) - 1; assert(rowind[i] >= 0); 
		colind[i] = (unsigned int)(pMr[  nentries+i]) - 1; assert(colind[i] >= 0); 
		vals  [i] = std::complex<double>(pMr[2*nentries+i], pMi[2*nentries+i]);*/
		//
		rowind[i] = (unsigned int)(pMc[           i].real) - 1; assert(rowind[i] >= 0);
		colind[i] = (unsigned int)(pMc[nentries + i].real) - 1; assert(colind[i] >= 0);
		vals[i] = std::complex<double>(pMc[2 * nentries + i].real, pMc[2 * nentries + i].imag);
	}
	mxDestroyArray(M);

	sprintf(cmd, "clear %s;", tempName.c_str());
	result = engEvalString(m_ep, cmd);

	return result;
}



int MatlabInterface::GetEngineEncodedSparseRealMatrix(const char* name, std::vector<unsigned int>& Ir, std::vector<unsigned int>& Jc, std::vector<double>& vals, unsigned int& m, unsigned int& n)
{ 
	assert(name != NULL && name[0] != 0);

	mxArray *M = engGetVariable(m_ep, name);

	if(M == NULL || !mxIsSparse(M))
	{ 
		WARNING("matrix " << name << " could not be loaded");
		return -1;
	}
	m =  mxGetM(M);
	n =  mxGetN(M);

	mwIndex nzmax =  mxGetNzmax(M);

	double *pM = mxGetPr(M);
	mwIndex *ir = mxGetIr(M);
	mwIndex *jc = mxGetJc(M);
	if(pM == NULL || ir == NULL || jc == NULL) return -1;

	Ir.resize(nzmax);
	Jc.resize(n + 1);
	vals.resize(nzmax);

	memcpy(pM, &vals.front(), vals.size()*sizeof(vals.front()));
	memcpy(ir, &Ir.front(), Ir.size()*sizeof(Ir.front()));
	memcpy(jc, &Jc.front(), Jc.size()*sizeof(Jc.front()));

	return 0;
}

int MatlabInterface::GetEngineEncodedSparseComplexMatrix(const char* name, std::vector<unsigned int>& Ir, std::vector<unsigned int>& Jc, std::vector<std::complex<double> >& vals, unsigned int& m, unsigned int& n)
{ 
	assert(name != NULL && name[0] != 0);

	mxArray *M = engGetVariable(m_ep, name);

	if(M == NULL || !mxIsSparse(M))
	{ 
		WARNING("matrix " << name << " could not be loaded");
		return -1;
	}
	m =  mxGetM(M);
	n =  mxGetN(M);

	mwIndex nzmax =  mxGetNzmax(M);

	/*double *pMr = mxGetPr(M);
	double *pMi = mxGetPi(M);*/
	//
	mxComplexDouble *pMc = mxGetComplexDoubles(M);
	//
	mwIndex *ir = mxGetIr(M);
	mwIndex *jc = mxGetJc(M);
	//if(pMr == NULL || pMi == NULL || ir == NULL || jc == NULL) return -1;
	if (pMc == NULL || ir == NULL || jc == NULL) return -1;

	Ir.resize(nzmax);
	Jc.resize(n + 1);
	vals.resize(nzmax);

	for(int i = 0; i < nzmax; i++)
	{
		//vals[i] = std::complex<double>(pMr[i], pMi[i]);
		vals[i] = std::complex<double>(pMc[i].real, pMc[i].imag);
	}
	memcpy(&Ir.front(), ir, Ir.size()*sizeof(Ir.front()));
	memcpy(&Jc.front(), jc, Jc.size()*sizeof(Jc.front()));

	return 0;
}




////////////////////////////////////////////////////////////////////////////////

// Runs a matlab script within the specified directory where it may have
// more helper scripts. Prints the output on the screen.
int
MatlabInterface::RunMatlabScript(const char *script_path, const char *script_name)
{
    MatlabInterface &matlab = *this;

    if (!script_path) {
        ERROR("RunMatlabScript: No script path specified.");
        return -1;
    }

    int res = matlab.AddScriptPath(script_path);
    if (res != 0) {
        ERROR("Unable to add Matlab script directory to path.");
        return res;
    }

    std::string output = matlab.RunScriptToString(script_name);

    // matlab output
    MESSAGE("MATLAB OUTPUT\n--------------------\n" << output << "--------------------" << std::endl);
    return 0;
}



// returns the file name prefix (i.e. without the extension)
std::string MatlabInterface::GetPrefix(const std::string &path)
{
	if (path.size() > 0)
	{
		return path.substr(0, path.find_last_of('.'));
	}
	else
	{
		return std::string("");
	}
}

// returns the lowercase file name extension
std::string MatlabInterface::GetExtension(const std::string &path)
{
	std::string extension;
	size_t ext_start = path.find_last_of('.');
	if (ext_start < path.length())
	{
		extension = path.substr(ext_start, path.length());
	}
	for(std::string::iterator i = extension.begin(); i != extension.end(); ++i)
	{
		*i = tolower(*i);
	}
	return extension;
}



bool MatlabInterface::GetMatrixDimensions(const char* variableName, unsigned int& m, unsigned int& n)
{
	#define BUFFER_SIZE (50)

	m = 0;
	n = 0;

	char output[BUFFER_SIZE];

	std::string command("size(");
	command = command + variableName;
	command = command + ")";

	int res = Eval(command.c_str(), output, BUFFER_SIZE);
	if(res != 0)
	{
		return false;
	}
	std::stringstream stream(std::stringstream::in | std::stringstream::out);

	stream << output;

	std::string str1; //to capture the "ans"
	std::string str2; //to capture the "="
	stream >> str1 >> str2;

	if((stream >> m >> n) && m > 0 && n > 0)
	{
		return true;
	}
	else
	{
		return false;
	}

	return true;
}
