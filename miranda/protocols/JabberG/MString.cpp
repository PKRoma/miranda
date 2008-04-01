#include "jabber.h"
#include "MString.h"

CNilMStringData CMBaseString::m_nil;

CMStringData* CMBaseString::Allocate( int nChars, int nCharSize )
{
	CMStringData* pData;
	nChars++; // nil char
	size_t nDataBytes = nCharSize * nChars;
	size_t nTotalSize = nDataBytes + sizeof(CMStringData);

	pData = static_cast<CMStringData*>(malloc(nTotalSize));
	if (pData == NULL)
		return NULL;

	pData->nRefs = 1;
	pData->nAllocLength = nChars - 1;
	pData->nDataLength = 0;
	return pData;
}

void CMBaseString::Free(CMStringData* pData)
{
	free(pData);
}

CMStringData* CMBaseString::Realloc(CMStringData* pData, int nChars, int nCharSize)
{
	CMStringData* pNewData;
	nChars++; // nil char
	ULONG nDataBytes = nCharSize * nChars;
	ULONG nTotalSize = nDataBytes + sizeof(CMStringData);

	pNewData = static_cast<CMStringData*>(realloc(pData, nTotalSize));
	if (pNewData == NULL)
		return NULL;

	pNewData->nAllocLength = nChars - 1;
	return pNewData;
}

CMStringData* CMBaseString::GetNilString()
{
	m_nil.AddRef();
	return &m_nil;
}

void* CMStringData::data()
{
	return (this + 1);
}

void CMStringData::AddRef()
{
	InterlockedIncrement(&nRefs);
}

bool CMStringData::IsLocked() const
{
	return nRefs < 0;
}

bool CMStringData::IsShared() const
{
	return (nRefs > 1);
}

void CMStringData::Lock()
{
	nRefs--;  // Locked buffers can't be shared, so no interlocked operation necessary
	if( nRefs == 0 )
		nRefs = -1;
}

void CMStringData::Release()
{
	if (InterlockedDecrement(&nRefs) <= 0)
		CMBaseString::Free(this);
}

void CMStringData::Unlock()
{
	if (IsLocked())
	{
		nRefs++;  // Locked buffers can't be shared, so no interlocked operation necessary
		if (nRefs == 0)
			nRefs = 1;
	}
}

CNilMStringData::CNilMStringData()
{
	nRefs = 2;  // Never gets freed
	nDataLength = 0;
	nAllocLength = 0;
	achNil[0] = 0;
	achNil[1] = 0;
}
