#pragma once
#define AFX_INLINE inline
#define ASSERT(X)
#define AfxThrowInvalidArgException(X)
#define __max(X, Y) (X > Y ? X : Y)
#define __min(X, Y) (X < Y ? X : Y)
#define AFXAPI

#include <limits.h>
#define ENSURE(X)
#define ASSERT_VALID(X)
#define memcpy_s(X, Y, Z, S) memcpy(X, Z, S)

template<class TYPE>
AFX_INLINE void AFXAPI CopyElements(TYPE* pDest, const TYPE* pSrc, INT_PTR nCount)
{
	ENSURE(nCount == 0 || pDest != 0 && pSrc != 0);
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pDest, (size_t)nCount * sizeof(TYPE)));
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pSrc, (size_t)nCount * sizeof(TYPE)));

	// default is element-copy using assignment
	while (nCount--)
		*pDest++ = *pSrc++;
}

/*============================================================================*/
// CArray<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE = const TYPE&>
class CArray : public CObject
{
public:
// Construction
	CArray();

// Attributes
	INT_PTR GetSize() const;
	INT_PTR GetCount() const;
	BOOL IsEmpty() const;
	INT_PTR GetUpperBound() const;
	void SetSize(INT_PTR nNewSize, INT_PTR nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	const TYPE& GetAt(INT_PTR nIndex) const;
	TYPE& GetAt(INT_PTR nIndex);
	void SetAt(INT_PTR nIndex, ARG_TYPE newElement);
	const TYPE& ElementAt(INT_PTR nIndex) const;
	TYPE& ElementAt(INT_PTR nIndex);

	// Direct Access to the element data (may return NULL)
	const TYPE* GetData() const;
	TYPE* GetData();

	// Potentially growing the array
	void SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement);
	INT_PTR Add(ARG_TYPE newElement);
	INT_PTR Append(const CArray& src);
	void Copy(const CArray& src);

	// overloaded operator helpers
	const TYPE& operator[](INT_PTR nIndex) const;
	TYPE& operator[](INT_PTR nIndex);

	// Operations that move elements around
	void InsertAt(INT_PTR nIndex, ARG_TYPE newElement, INT_PTR nCount = 1);
	void RemoveAt(INT_PTR nIndex, INT_PTR nCount = 1);
	void InsertAt(INT_PTR nStartIndex, CArray* pNewArray);

// Implementation
protected:
	TYPE* m_pData;   // the actual array of data
	INT_PTR m_nSize;     // # of elements (upperBound - 1)
	INT_PTR m_nMaxSize;  // max allocated
	INT_PTR m_nGrowBy;   // grow amount

public:
	~CArray();
	void Serialize(CArchive&) override;
#ifdef _DEBUG
	void Dump(CDumpContext&) const override;
	void AssertValid() const override;
#endif
};

/*============================================================================*/
// CArray<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
AFX_INLINE INT_PTR CArray<TYPE, ARG_TYPE>::GetSize() const
	{ return m_nSize; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE INT_PTR CArray<TYPE, ARG_TYPE>::GetCount() const
	{ return m_nSize; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE BOOL CArray<TYPE, ARG_TYPE>::IsEmpty() const
	{ return m_nSize == 0; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE INT_PTR CArray<TYPE, ARG_TYPE>::GetUpperBound() const
	{ return m_nSize-1; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE void CArray<TYPE, ARG_TYPE>::RemoveAll()
	{ SetSize(0, -1); }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CArray<TYPE, ARG_TYPE>::GetAt(INT_PTR nIndex)
{
	ASSERT(nIndex >= 0 && nIndex < m_nSize);
	if(nIndex >= 0 && nIndex < m_nSize)
		return m_pData[nIndex];
	AfxThrowInvalidArgException();
}
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CArray<TYPE, ARG_TYPE>::GetAt(INT_PTR nIndex) const
{
	ASSERT(nIndex >= 0 && nIndex < m_nSize);
	if(nIndex >= 0 && nIndex < m_nSize)
		return m_pData[nIndex];
	AfxThrowInvalidArgException();
}
template<class TYPE, class ARG_TYPE>
AFX_INLINE void CArray<TYPE, ARG_TYPE>::SetAt(INT_PTR nIndex, ARG_TYPE newElement)
{
	ASSERT(nIndex >= 0 && nIndex < m_nSize);
	if(nIndex >= 0 && nIndex < m_nSize)
		m_pData[nIndex] = newElement;
	else
		AfxThrowInvalidArgException();
}
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CArray<TYPE, ARG_TYPE>::ElementAt(INT_PTR nIndex) const
{
	ASSERT(nIndex >= 0 && nIndex < m_nSize);
	if(nIndex >= 0 && nIndex < m_nSize)
		return m_pData[nIndex];
	AfxThrowInvalidArgException();
}
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CArray<TYPE, ARG_TYPE>::ElementAt(INT_PTR nIndex)
{
	ASSERT(nIndex >= 0 && nIndex < m_nSize);
	if(nIndex >= 0 && nIndex < m_nSize)
		return m_pData[nIndex];
	AfxThrowInvalidArgException();
}
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE* CArray<TYPE, ARG_TYPE>::GetData() const
	{ return (const TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE* CArray<TYPE, ARG_TYPE>::GetData()
	{ return (TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE INT_PTR CArray<TYPE, ARG_TYPE>::Add(ARG_TYPE newElement)
	{ INT_PTR nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CArray<TYPE, ARG_TYPE>::operator[](INT_PTR nIndex) const
	{ return GetAt(nIndex); }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CArray<TYPE, ARG_TYPE>::operator[](INT_PTR nIndex)
	{ return ElementAt(nIndex); }

/*============================================================================*/
// CArray<TYPE, ARG_TYPE> out-of-line functions

template<class TYPE, class ARG_TYPE>
CArray<TYPE, ARG_TYPE>::CArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template<class TYPE, class ARG_TYPE>
CArray<TYPE, ARG_TYPE>::~CArray()
{
	ASSERT_VALID(this);

	if (m_pData != NULL)
	{
		for( int i = 0; i < m_nSize; i++ )
			(m_pData + i)->~TYPE();
		delete[] (BYTE*)m_pData;
	}
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::SetSize(INT_PTR nNewSize, INT_PTR nGrowBy)
{
	ASSERT_VALID(this);
	ASSERT(nNewSize >= 0);

	if(nNewSize < 0 )
		AfxThrowInvalidArgException();

	if (nGrowBy >= 0)
		m_nGrowBy = nGrowBy;  // set new size

	if (nNewSize == 0)
	{
		// shrink to nothing
		if (m_pData != NULL)
		{
			for( int i = 0; i < m_nSize; i++ )
				(m_pData + i)->~TYPE();
			delete[] (BYTE*)m_pData;
			m_pData = NULL;
		}
		m_nSize = m_nMaxSize = 0;
	}
	else if (m_pData == NULL)
	{
		// create buffer big enough to hold number of requested elements or
		// m_nGrowBy elements, whichever is larger.
#ifdef SIZE_T_MAX
		ASSERT(nNewSize <= SIZE_T_MAX/sizeof(TYPE));    // no overflow
#endif
		size_t nAllocSize = __max(nNewSize, m_nGrowBy);
		m_pData = (TYPE*) new BYTE[(size_t)nAllocSize * sizeof(TYPE)];
		memset((void*)m_pData, 0, (size_t)nAllocSize * sizeof(TYPE));
		for( int i = 0; i < nNewSize; i++ )
#pragma push_macro("new")
#undef new
			::new( (void*)( m_pData + i ) ) TYPE;
#pragma pop_macro("new")
		m_nSize = nNewSize;
		m_nMaxSize = nAllocSize;
	}
	else if (nNewSize <= m_nMaxSize)
	{
		// it fits
		if (nNewSize > m_nSize)
		{
			// initialize the new elements
			memset((void*)(m_pData + m_nSize), 0, (size_t)(nNewSize-m_nSize) * sizeof(TYPE));
			for( int i = 0; i < nNewSize-m_nSize; i++ )
#pragma push_macro("new")
#undef new
				::new( (void*)( m_pData + m_nSize + i ) ) TYPE;
#pragma pop_macro("new")
		}
		else if (m_nSize > nNewSize)
		{
			// destroy the old elements
			for( int i = 0; i < m_nSize-nNewSize; i++ )
				(m_pData + nNewSize + i)->~TYPE();
		}
		m_nSize = nNewSize;
	}
	else
	{
		// otherwise, grow array
		nGrowBy = m_nGrowBy;
		if (nGrowBy == 0)
		{
			// heuristically determine growth when nGrowBy == 0
			//  (this avoids heap fragmentation in many situations)
			nGrowBy = m_nSize / 8;
			nGrowBy = (nGrowBy < 4) ? 4 : ((nGrowBy > 1024) ? 1024 : nGrowBy);
		}
		INT_PTR nNewMax;
		if (nNewSize < m_nMaxSize + nGrowBy)
			nNewMax = m_nMaxSize + nGrowBy;  // granularity
		else
			nNewMax = nNewSize;  // no slush

		ASSERT(nNewMax >= m_nMaxSize);  // no wrap around

		if(nNewMax  < m_nMaxSize)
			AfxThrowInvalidArgException();

#ifdef SIZE_T_MAX
		ASSERT(nNewMax <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
		TYPE* pNewData = (TYPE*) new BYTE[(size_t)nNewMax * sizeof(TYPE)];

		// copy new data from old
		memcpy_s(pNewData, (size_t)nNewMax * sizeof(TYPE),
			m_pData, (size_t)m_nSize * sizeof(TYPE));

		// construct remaining elements
		ASSERT(nNewSize > m_nSize);
		memset((void*)(pNewData + m_nSize), 0, (size_t)(nNewSize-m_nSize) * sizeof(TYPE));
		for( int i = 0; i < nNewSize-m_nSize; i++ )
#pragma push_macro("new")
#undef new
			::new( (void*)( pNewData + m_nSize + i ) ) TYPE;
#pragma pop_macro("new")

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nSize = nNewSize;
		m_nMaxSize = nNewMax;
	}
}

template<class TYPE, class ARG_TYPE>
INT_PTR CArray<TYPE, ARG_TYPE>::Append(const CArray& src)
{
	ASSERT_VALID(this);
	ASSERT(this != &src);   // cannot append to itself

	if(this == &src)
		AfxThrowInvalidArgException();

	INT_PTR nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);
	CopyElements<TYPE>(m_pData + nOldSize, src.m_pData, src.m_nSize);
	return nOldSize;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::Copy(const CArray& src)
{
	ASSERT_VALID(this);
	ASSERT(this != &src);   // cannot append to itself

	if(this != &src)
	{
		SetSize(src.m_nSize);
		CopyElements<TYPE>(m_pData, src.m_pData, src.m_nSize);
	}
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::FreeExtra()
{
	ASSERT_VALID(this);

	if (m_nSize != m_nMaxSize)
	{
		// shrink to desired size
#ifdef SIZE_T_MAX
		ASSERT(m_nSize <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
		TYPE* pNewData = NULL;
		if (m_nSize != 0)
		{
			pNewData = (TYPE*) new BYTE[m_nSize * sizeof(TYPE)];
			// copy new data from old
			memcpy_s(pNewData, m_nSize * sizeof(TYPE),
				m_pData, m_nSize * sizeof(TYPE));
		}

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::SetAtGrow(INT_PTR nIndex, ARG_TYPE newElement)
{
	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);

	if(nIndex < 0)
		AfxThrowInvalidArgException();

	if (nIndex >= m_nSize)
		SetSize(nIndex+1, -1);
	m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::InsertAt(INT_PTR nIndex, ARG_TYPE newElement, INT_PTR nCount /*=1*/)
{
	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);    // will expand to meet need
	ASSERT(nCount > 0);     // zero or negative size not allowed

	if(nIndex < 0 || nCount <= 0)
		AfxThrowInvalidArgException();

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		SetSize(nIndex + nCount, -1);   // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		INT_PTR nOldSize = m_nSize;
		SetSize(m_nSize + nCount, -1);  // grow it to new size
		// destroy intial data before copying over it
		for( int i = 0; i < nCount; i++ )
			(m_pData + nOldSize + i)->~TYPE();
		// shift old data up to fill gap
		memmove_s(m_pData + nIndex + nCount, (nOldSize-nIndex) * sizeof(TYPE),
			m_pData + nIndex, (nOldSize-nIndex) * sizeof(TYPE));

		// re-init slots we copied from
		memset((void*)(m_pData + nIndex), 0, (size_t)nCount * sizeof(TYPE));
		for( int i = 0; i < nCount; i++ )
#pragma push_macro("new")
#undef new
			::new( (void*)( m_pData + nIndex + i ) ) TYPE;
#pragma pop_macro("new")
	}

	// insert new value in the gap
	ASSERT(nIndex + nCount <= m_nSize);
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::RemoveAt(INT_PTR nIndex, INT_PTR nCount)
{
	ASSERT_VALID(this);
	ASSERT(nIndex >= 0);
	ASSERT(nCount >= 0);
	INT_PTR nUpperBound = nIndex + nCount;
	ASSERT(nUpperBound <= m_nSize && nUpperBound >= nIndex && nUpperBound >= nCount);

	if(nIndex < 0 || nCount < 0 || (nUpperBound > m_nSize) || (nUpperBound < nIndex) || (nUpperBound < nCount))
		AfxThrowInvalidArgException();

	// just remove a range
	INT_PTR nMoveCount = m_nSize - (nUpperBound);
	for( int i = 0; i < nCount; i++ )
		(m_pData + nIndex + i)->~TYPE();
	if (nMoveCount)
	{
		memmove_s(m_pData + nIndex, (size_t)nMoveCount * sizeof(TYPE),
			m_pData + nUpperBound, (size_t)nMoveCount * sizeof(TYPE));
	}
	m_nSize -= nCount;
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::InsertAt(INT_PTR nStartIndex, CArray* pNewArray)
{
	ASSERT_VALID(this);
	ASSERT(pNewArray != NULL);
	ASSERT_VALID(pNewArray);
	ASSERT(nStartIndex >= 0);

	if(pNewArray == NULL || nStartIndex < 0)
		AfxThrowInvalidArgException();

	if (pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (INT_PTR i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}


template<class TYPE>
void AFXAPI SerializeElements(CArchive& ar, TYPE* pElements, INT_PTR nCount)
{
	ENSURE(nCount == 0 || pElements != NULL);
	ASSERT(nCount == 0 ||
		AfxIsValidAddress(pElements, (size_t)nCount * sizeof(TYPE)));

	// default is bit-wise read/write
	if (ar.IsStoring())
	{
		TYPE* pData;
		UINT_PTR nElementsLeft;

		nElementsLeft = nCount;
		pData = pElements;
		while( nElementsLeft > 0 )
		{
			UINT nElementsToWrite;

			nElementsToWrite = UINT(__min(nElementsLeft, INT_MAX/sizeof(TYPE)));
			ar.Write(pData, nElementsToWrite*sizeof(TYPE));
			nElementsLeft -= nElementsToWrite;
			pData += nElementsToWrite;
		}
	}
	else
	{
		TYPE* pData;
		UINT_PTR nElementsLeft;

		nElementsLeft = nCount;
		pData = pElements;
		while( nElementsLeft > 0 )
		{
			UINT nElementsToRead;

			nElementsToRead = UINT(__min(nElementsLeft, INT_MAX/sizeof(TYPE)));
			ar.EnsureRead(pData, nElementsToRead*sizeof(TYPE));
			nElementsLeft -= nElementsToRead;
			pData += nElementsToRead;
		}
	}
}


template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CObject::Serialize(ar);
	if (ar.IsStoring())
	{
		ar.WriteCount(m_nSize);
	}
	else
	{
		DWORD_PTR nOldSize = ar.ReadCount();
		SetSize(nOldSize, -1);
	}
	SerializeElements<TYPE>(ar, m_pData, m_nSize);
}

#ifdef _DEBUG
template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);

	dc << "with " << m_nSize << " elements";
	if (dc.GetDepth() > 0)
	{
		dc << "\n";
		DumpElements<TYPE>(dc, m_pData, m_nSize);
	}

	dc << "\n";
}

template<class TYPE, class ARG_TYPE>
void CArray<TYPE, ARG_TYPE>::AssertValid() const
{
	CObject::AssertValid();

	if (m_pData == NULL)
	{
		ASSERT(m_nSize == 0);
		ASSERT(m_nMaxSize == 0);
	}
	else
	{
		ASSERT(m_nSize >= 0);
		ASSERT(m_nMaxSize >= 0);
		ASSERT(m_nSize <= m_nMaxSize);
		ASSERT(AfxIsValidAddress(m_pData, m_nMaxSize * sizeof(TYPE)));
	}
}
#endif //_DEBUG

/*============================================================================*/
// Basic types
// abstract iteration position
struct __POSITION {};
typedef __POSITION *POSITION;


#pragma warning(push)
#pragma warning(disable:4324)
struct CPlex
{
	CPlex* pNext;
	// BYTE data[maxNum*elementSize];

	void* data() { return this+1; }

	static CPlex* Create(CPlex*& head, UINT_PTR nMax, UINT_PTR cbElement);
			// like 'calloc' but no zero fill
			// may throw memory exceptions

	void FreeDataChain();       // free this one and links
};
#pragma warning(pop)


/*============================================================================*/
// CList<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE = const TYPE&>
class CList : public CObject
{
protected:
	struct CNode
	{
		CNode* pNext;
		CNode* pPrev;
		TYPE data;
	};
public:
// Construction
	explicit CList(INT_PTR nBlockSize = 10);

// Attributes (head and tail)
	// count of elements
	INT_PTR GetCount() const;
	INT_PTR GetSize() const;
	BOOL IsEmpty() const;

	// peek at head or tail
	TYPE& GetHead();
	const TYPE& GetHead() const;
	TYPE& GetTail();
	const TYPE& GetTail() const;

// Operations
	// get head or tail (and remove it) - don't call on empty list !
	TYPE RemoveHead();
	TYPE RemoveTail();

	// add before head or after tail
	POSITION AddHead(ARG_TYPE newElement);
	POSITION AddTail(ARG_TYPE newElement);

	// add another list of elements before head or after tail
	void AddHead(CList* pNewList);
	void AddTail(CList* pNewList);

	// remove all elements
	void RemoveAll();

	// iteration
	POSITION GetHeadPosition() const;
	POSITION GetTailPosition() const;
	TYPE& GetNext(POSITION& rPosition); // return *Position++
	const TYPE& GetNext(POSITION& rPosition) const; // return *Position++
	TYPE& GetPrev(POSITION& rPosition); // return *Position--
	const TYPE& GetPrev(POSITION& rPosition) const; // return *Position--

	// getting/modifying an element at a given position
	TYPE& GetAt(POSITION position);
	const TYPE& GetAt(POSITION position) const;
	void SetAt(POSITION pos, ARG_TYPE newElement);
	void RemoveAt(POSITION position);

	// inserting before or after a given position
	POSITION InsertBefore(POSITION position, ARG_TYPE newElement);
	POSITION InsertAfter(POSITION position, ARG_TYPE newElement);

	// helper functions (note: O(n) speed)
	POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL) const;
		// defaults to starting at the HEAD, return NULL if not found
	POSITION FindIndex(INT_PTR nIndex) const;
		// get the 'nIndex'th element (may return NULL)

// Implementation
protected:
	CNode* m_pNodeHead;
	CNode* m_pNodeTail;
	INT_PTR m_nCount;
	CNode* m_pNodeFree;
	struct CPlex* m_pBlocks;
	INT_PTR m_nBlockSize;

	CNode* NewNode(CNode*, CNode*);
	void FreeNode(CNode*);

public:
	~CList();
	void Serialize(CArchive&) override;
#ifdef _DEBUG
	void Dump(CDumpContext&) const override;
	void AssertValid() const override;
#endif
};

/*============================================================================*/
// CList<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
AFX_INLINE INT_PTR CList<TYPE, ARG_TYPE>::GetCount() const
	{ return m_nCount; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE INT_PTR CList<TYPE, ARG_TYPE>::GetSize() const
	{ return m_nCount; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE BOOL CList<TYPE, ARG_TYPE>::IsEmpty() const
	{ return m_nCount == 0; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CList<TYPE, ARG_TYPE>::GetHead()
	{ ENSURE(m_pNodeHead != NULL);
		return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CList<TYPE, ARG_TYPE>::GetHead() const
	{ ENSURE(m_pNodeHead != NULL);
		return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CList<TYPE, ARG_TYPE>::GetTail()
	{ ENSURE(m_pNodeTail != NULL);
		return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CList<TYPE, ARG_TYPE>::GetTail() const
	{ ENSURE(m_pNodeTail != NULL);
		return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE POSITION CList<TYPE, ARG_TYPE>::GetHeadPosition() const
	{ return (POSITION) m_pNodeHead; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE POSITION CList<TYPE, ARG_TYPE>::GetTailPosition() const
	{ return (POSITION) m_pNodeTail; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CList<TYPE, ARG_TYPE>::GetNext(POSITION& rPosition) // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CList<TYPE, ARG_TYPE>::GetNext(POSITION& rPosition) const // return *Position++
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		rPosition = (POSITION) pNode->pNext;
		return pNode->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CList<TYPE, ARG_TYPE>::GetPrev(POSITION& rPosition) // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CList<TYPE, ARG_TYPE>::GetPrev(POSITION& rPosition) const // return *Position--
	{ CNode* pNode = (CNode*) rPosition;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		rPosition = (POSITION) pNode->pPrev;
		return pNode->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE TYPE& CList<TYPE, ARG_TYPE>::GetAt(POSITION position)
	{ CNode* pNode = (CNode*) position;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		return pNode->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE const TYPE& CList<TYPE, ARG_TYPE>::GetAt(POSITION position) const
	{ CNode* pNode = (CNode*) position;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		return pNode->data; }
template<class TYPE, class ARG_TYPE>
AFX_INLINE void CList<TYPE, ARG_TYPE>::SetAt(POSITION pos, ARG_TYPE newElement)
	{ CNode* pNode = (CNode*) pos;
		ASSERT(AfxIsValidAddress(pNode, sizeof(CNode)));
		pNode->data = newElement; }

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::CList(INT_PTR nBlockSize)
{
	ASSERT(nBlockSize > 0);

	m_nCount = 0;
	m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
	m_pBlocks = NULL;
	m_nBlockSize = nBlockSize;
}

template<class TYPE, class ARG_TYPE>
void CList<TYPE, ARG_TYPE>::RemoveAll()
{
	ASSERT_VALID(this);

	// destroy elements
	CNode* pNode;
	for (pNode = m_pNodeHead; pNode != NULL; pNode = pNode->pNext)
		pNode->data.~TYPE();

	m_nCount = 0;
	m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
	m_pBlocks->FreeDataChain();
	m_pBlocks = NULL;
}

template<class TYPE, class ARG_TYPE>
CList<TYPE, ARG_TYPE>::~CList()
{
	RemoveAll();
	ASSERT(m_nCount == 0);
}
