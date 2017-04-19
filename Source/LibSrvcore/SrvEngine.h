#ifndef __SRV_ENGINE_H_
#define __SRV_ENGINE_H_

#include "csCommon.h"
#include "NetIncludes.h"

class zLogger;
class zProperties;
class zCond;
class zLock;
class zMutex;
class zMutex_scope_lock;
class zRWLock;
class zRWLock_scope_rdlock;
class zRWLock_scope_wrlock;
class zThread;
class zThreadGroup;
class zTCPTask;
class zTCPTaskPool;
class zSyncThread;
class zRecycleThread;
class zCheckconnectThread;
class zCheckwaitThread;
class zTCPClientTaskThread;
class zTimeTick;
class zTaskTimer;
class zRSA;
class zUtility;
class zTextCVS;

namespace H
{
	/**
	* \brief ��Ϸʱ��
	*
	*/
	extern volatile QWORD qwGameTime;

	/**
	* \brief ��־ָ��
	*
	*/
	extern zLogger *logger;

	/**
	* \brief ��ȡȫ�ֱ���������
	*
	*/
	extern zProperties global;

	/**
	* \brief ʱ�仺����
	*
	*/
	extern zTimeTick* timeTick;

	/**
	* \brief ����������
	*
	*/
	extern zRSA* rsa;
};

template <typename Container>
inline void stringtok(Container &container, std::string const &in,
	const char * const delimiters = " \t\n",
	const int deep = 0)
{
	const std::string::size_type len = in.length();
	std::string::size_type i = 0;
	int count = 0;

	while (i < len)
	{
		i = in.find_first_not_of(delimiters, i);
		if (i == std::string::npos)
			return;   // nothing left

					  // find the end of the token
		std::string::size_type j = in.find_first_of(delimiters, i);

		count++;
		// push token
		if (j == std::string::npos
			|| (deep > 0 && count > deep)) {
			container.push_back(in.substr(i));
			return;
		}
		else
			container.push_back(in.substr(i, j - i));

		// set up for next loop
		i = j + 1;
	}
}

//�ֶ����ù��캯�����������ڴ�
template<class _T1>
inline  void constructInPlace(_T1  *_Ptr)
{
	new (static_cast<void*>(_Ptr)) _T1();
}
/// �����䳤ָ��
#define BUFFER_CMD(cmd,name,len) char buffer##name[len];\
	cmd *name=(cmd *)buffer##name;memset(buffer##name,0,len);constructInPlace(name);

#define MAX_USERDATASIZE 64000

template <typename T>
class SingletonBase
{
public:
	SingletonBase() {}
	virtual ~SingletonBase() {}
	static T& getInstance()
	{
		assert(instance);
		return *instance;
	}
	static void newInstance()
	{
		SAFE_DELETE(instance);
		instance = new T();
	}
	static void delInstance()
	{
		SAFE_DELETE(instance);
	}
protected:
	static T* instance;
private:
	SingletonBase(const SingletonBase&);
	SingletonBase & operator= (const SingletonBase &);
};
template <typename T> T* SingletonBase<T>::instance = NULL;

using namespace std;

/**
* \brief ʹ���е�������ÿ������캯���͸�ֵ����
*
*/
class zNoncopyable
{

protected:

	/**
	* \brief ȱʡ���캯��
	*
	*/
	zNoncopyable() {};

	/**
	* \brief ȱʡ��������
	*
	*/
	~zNoncopyable() {};

private:

	/**
	* \brief �������캯����û��ʵ�֣����õ���
	*
	*/
	zNoncopyable(const zNoncopyable&);

	/**
	* \brief ��ֵ�������ţ�û��ʵ�֣����õ���
	*
	*/
	const zNoncopyable & operator= (const zNoncopyable &);

};

struct odds_t
{
	DWORD upNum;
	DWORD downNum;
};

//�������min~max֮������֣�����min��max
int randBetween(int min, int max);

//��ȡ����֮�ļ���
bool selectByOdds(const DWORD upNum, const DWORD downNum);

//��ȡ����֮���ļ���
bool selectByt_Odds(const odds_t &odds);

//��ȡ�ٷ�֮�ļ���
bool selectByPercent(const DWORD percent);

//��ȡ���֮�ļ���
bool selectByTenTh(const DWORD tenth);


//��ȡʮ���֮�ļ���
bool selectByLakh(const DWORD lakh);

//��ȡ�ڷ�֮֮�ļ���
bool selectByOneHM(const DWORD lakh);



#pragma pack(push,1)

typedef UNORDERED_SET<void*> UsingBlocksSet;

// �Զ���չ�ڴ��(�̰߳�ȫ) 
class MemPool
{
public:
	MemPool(bool bInit, uint32 nSizeBlock, uint32 nNumPerSlice = 256)
		: m_bInit(bInit), m_nBlockSize(nSizeBlock), m_nBlockNumPerSlice(nNumPerSlice), m_nSliceSize(nSizeBlock * nNumPerSlice)
	{
		// ��ʼ���������κ��ڴ� 
	}

	virtual ~MemPool()
	{
		// �ͷ�������ڴ�Ƭ 
		while (!m_staMemSlices.empty())
		{
			void* pSlice = m_staMemSlices.top();
			free(pSlice);
			m_staMemSlices.pop();
		}
	}

protected:
	virtual void* Create()
	{
		if (m_staBlocks.empty())
		{
			if (!enlarge())
			{
				return 0;
			}
		}

		void* pNewBlock = m_staBlocks.top();
		m_staBlocks.pop();
		m_setUsingBlocks.insert(pNewBlock);

		return pNewBlock;
	}

	virtual bool Destroy(void* pAllocatedBlock)
	{
		UsingBlocksSet::iterator iter = m_setUsingBlocks.find(pAllocatedBlock);
		if (iter != m_setUsingBlocks.end())
		{
			m_staBlocks.push(pAllocatedBlock);
			m_setUsingBlocks.erase(iter);

			return true;
		}
		else
		{
			return false;
		}
	}

private:

	// �����ڴ�Ƭ�������ڴ�� 
	bool enlarge()
	{
		// �ڴ�Ƭָ�� 
		void* pNewSlice = 0;

		// �����ڴ�Ƭ 
		if (m_bInit)
		{// ��ʼ��0 
			pNewSlice = calloc(m_nBlockNumPerSlice, m_nBlockSize);
		}
		else
		{// δ��ʼ�� 
			pNewSlice = malloc(m_nSliceSize);
		}

		if (!pNewSlice)
		{
			return false;
		}

		// �����ڴ�Ƭ 
		m_staMemSlices.push(pNewSlice);

		// �����ڴ�� 
		int8* pBlock = reinterpret_cast<int8*>(pNewSlice);
		for (uint32 i = 0; i < m_nBlockNumPerSlice; ++i)
		{
			m_staBlocks.push(pBlock + m_nBlockSize * i);
		}

		return true;
	}

protected:
	UsingBlocksSet		m_setUsingBlocks;		// ʹ���е��ڴ�� 

private:
	const bool			m_bInit;					// �Ƿ��ʼ��������ڴ� 
	const uint32		m_nBlockSize;				// �ڴ���С 
	const uint32		m_nBlockNumPerSlice;		// �ڴ�Ƭ���ڴ������ 
	const uint32		m_nSliceSize;				// �ڴ�Ƭ��С 
	std::stack<void*>	m_staMemSlices;				// �ڴ�Ƭ 
	std::stack<void*>	m_staBlocks;				// �ڴ������ 
};

// �����ڴ�ʹ��״̬ 
struct ObjMemInfo
{
	bool		bUsing;			// ����ʹ�� 
	uint64		nMark;			// �ڴ���(ͬһ���ڴ�������ͬ����ʱ��ǲ�ͬ������Ϊ0,ĿǰΪ�ڴ�ʹ�ô���) 
};

// ʹ�ù�����Ϣ���ڴ�� 
class ManageInfoUsingBlocks
{
public:
	// ��ȡ�ڴ�ʹ�ñ��,����0Ϊ��ȡʧ�� 
	template<typename T>
	static uint64 GetMark(T* pAllocatedObj)
	{
		if (HaveManageInfo(pAllocatedObj))
		{
			ObjMemInfo& rInfo = *reinterpret_cast<ObjMemInfo*>(pAllocatedObj + sizeof(T));

			return rInfo.nMark;
		}

		return 0;
	}

	// �������ʹ���е��ڴ������Ƿ�һ�� 
	template<typename T>
	static bool CheckMark(T* pAllocatedObj, uint64 nMark)
	{
		if (HaveManageInfo(pAllocatedObj))
		{
			ObjMemInfo& rInfo = *reinterpret_cast<ObjMemInfo*>(pAllocatedObj + sizeof(T));

			return rInfo.bUsing && !(rInfo.nMark ^ nMark);
		}

		return false;
	}

public:
	static UsingBlocksSet s_setBlocks;	// ʹ���к�������Ϣ���ڴ�� 

private:
	// ����ڴ��Ƿ񱻹��� 
	static bool HaveManageInfo(void* pAllocatedObj)
	{
		return s_setBlocks.find(pAllocatedObj) != s_setBlocks.end();
	}
};


// �����(����ʵ��CreateObj�ſ�ʹ��) 
template<typename T>
class zObjPool : protected MemPool
{
public:
	zObjPool(bool bIsManageInfo = false, uint32 nNumPerSlice = 256)
		: MemPool(bIsManageInfo, (bIsManageInfo ? (sizeof(T) + sizeof(ObjMemInfo)) : sizeof(T)), nNumPerSlice), m_bManageInfo(bIsManageInfo)
	{
	}

	virtual ~zObjPool()
	{
		// ��֤���ͷ��ڴ�Ƭ֮ǰ,��������ʹ�õ��ڴ���϶������������ 
		for (UsingBlocksSet::iterator iter = m_setUsingBlocks.begin();
			iter != m_setUsingBlocks.end(); ++iter)
		{
			T* pT = reinterpret_cast<T*>(*iter);
			pT->~T();
		}
	}

	// T* CreateObj(...)
	T* CreateObj()
	{
		if (void* pMem = Create())
		{
			return new(pMem)T();
		}
		return 0;
	}

	template<typename PT1>
	T* CreateObj(PT1 param1)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1);
		}
		return 0;
	}

	template<typename PT1, typename PT2>
	T* CreateObj(PT1 param1, PT2 param2)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2);
		}
		return 0;
	}

	template<typename PT1, typename PT2, typename PT3>
	T* CreateObj(PT1 param1, PT2 param2, PT3 param3)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2, param3);
		}
		return 0;
	}

	template<typename PT1, typename PT2, typename PT3, typename PT4>
	T* CreateObj(PT1 param1, PT2 param2, PT3 param3, PT4 param4)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2, param3, param4);
		}
		return 0;
	}

	template<typename PT1, typename PT2, typename PT3, typename PT4, typename PT5>
	T* CreateObj(PT1 param1, PT2 param2, PT3 param3, PT4 param4, PT5 param5)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2, param3, param4, param5);
		}
		return 0;
	}

	template<typename PT1, typename PT2, typename PT3, typename PT4, typename PT5, typename PT6>
	T* CreateObj(PT1 param1, PT2 param2, PT3 param3, PT4 param4, PT5 param5, PT6 param6)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2, param3, param4, param5, param6);
		}
		return 0;
	}

	template<typename PT1, typename PT2, typename PT3, typename PT4, typename PT5, typename PT6, typename PT7>
	T* CreateObj(PT1 param1, PT2 param2, PT3 param3, PT4 param4, PT5 param5, PT6 param6, PT7 param7)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2, param3, param4, param5, param6, param7);
		}
		return 0;
	}

	template<typename PT1, typename PT2, typename PT3, typename PT4, typename PT5, typename PT6, typename PT7, typename PT8>
	T* CreateObj(PT1 param1, PT2 param2, PT3 param3, PT4 param4, PT5 param5, PT6 param6, PT7 param7, PT8 param8)
	{
		if (void* pMem = Create())
		{
			return new(pMem)T(param1, param2, param3, param4, param5, param6, param7, param8);
		}
		return 0;
	}

	bool DestroyObj(T* pAllocatedObj)
	{
		if (Destroy(pAllocatedObj))
		{
			pAllocatedObj->~T();

			return true;
		}
		else
		{
			return false;
		}
	}

protected:
	virtual void* Create()
	{
		if (uint8* pNewBlock = reinterpret_cast<uint8*>(MemPool::Create()))
		{
			if (m_bManageInfo)
			{
				ObjMemInfo& rInfo = *reinterpret_cast<ObjMemInfo*>(pNewBlock + sizeof(T));
				rInfo.bUsing = true;
				++rInfo.nMark;				// ʹ�ñ������Ϊ0 

				ManageInfoUsingBlocks::s_setBlocks.insert(pNewBlock);
			}

			return pNewBlock;
		}
		else
		{
			return 0;
		}
	}

	virtual bool Destroy(void* pAllocatedBlock)
	{
		if (MemPool::Destroy(pAllocatedBlock))
		{
			if (m_bManageInfo)
			{
				UsingBlocksSet::iterator iter = ManageInfoUsingBlocks::s_setBlocks.find(pAllocatedBlock);
				if (iter != ManageInfoUsingBlocks::s_setBlocks.end())
				{
					uint8* pBlock = reinterpret_cast<uint8*>(pAllocatedBlock);
					ObjMemInfo& rInfo = *reinterpret_cast<ObjMemInfo*>(pBlock + sizeof(T));
					rInfo.bUsing = false;

					ManageInfoUsingBlocks::s_setBlocks.erase(iter);
				}
			}

			return true;
		}
		else
		{
			return false;
		}
	}

private:
	bool						m_bManageInfo;				// �Ƿ����ڴ������Ϣ 
};

template<typename T>
class ObjPoolFactory
{
protected:
	static zObjPool<T> objpool;
};

template< typename T >
zObjPool<T> ObjPoolFactory< T >::objpool;

// ����ع����ӿ�(���ڲ�����˭����˭���ٵ����) 
struct IObjPoolManagement
{
public:
	virtual void recycle() = 0;
};



#pragma pack(pop)


/**
* \brief �ٽ�������װ��ϵͳ�ٽ�����������ʹ��ϵͳ�ٽ���ʱ����Ҫ�ֹ���ʼ���������ٽ�������Ĳ���
*
*/
class zMutex : private zNoncopyable
{

	friend class zCond;

public:

	//	/**
	//	* \brief ���캯��������һ���ٽ�������
	//	*
	//	*/
	//	zMutex() 
	//	{
	//		InitializeCriticalSection(&m_critical);
	//	}
	//
	//	/**
	//	* \brief ��������������һ���ٽ�������
	//	*
	//	*/
	//	~zMutex()
	//	{
	//		DeleteCriticalSection(&m_critical);
	//	}
	//
	//	/**
	//	* \brief ����һ���ٽ���
	//	*
	//	*/
	//	inline void lock()
	//	{
	////		Zebra::logger->debug("Locking - %0.8X - %s(%u)", (DWORD)this, file,line );
	//		EnterCriticalSection(&m_critical);
	////		Zebra::logger->debug("Locked - %0.8X - %s(%u)", (DWORD)this, file,line );
	//	}
	//
	//	/**
	//	* \brief ����һ���ٽ���
	//	*
	//	*/
	//	inline void unlock()
	//	{
	////		Zebra::logger->debug("UnLock - %0.8X - %s(%u)", (DWORD)this, file,line );
	//		LeaveCriticalSection(&m_critical);
	//	}
	//
	//private:
	//
	//	CRITICAL_SECTION    m_critical; // ϵͳ�ٽ���
	/**
	* \brief ���캯��������һ�����������
	*
	*/
	zMutex()
	{
		//m_hMutex = CreateMutex(NULL, FALSE, NULL);
	}

	/**
	* \brief ��������������һ�����������
	*
	*/
	~zMutex()
	{
		//CloseHandle(m_hMutex);
	}

	/**
	* \brief ����һ��������
	*
	*/
	inline void lock()
	{
		//if (WaitForSingleObject(m_hMutex, 10000) == WAIT_TIMEOUT)
		//{
		//	char szName[MAX_PATH];
		//	GetModuleFileName(NULL, szName, sizeof(szName));
		//	::MessageBox(NULL, "����������", szName, MB_ICONERROR);
		//}
	}

	/**
	* \brief ����һ��������
	*
	*/
	inline void unlock()
	{
		//ReleaseMutex(m_hMutex);
	}

private:

	//HANDLE m_hMutex;    /**< ϵͳ������ */

};

/**
* \brief Wrapper
* �����ڸ��Ӻ���������ʹ��
*/
class zMutex_scope_lock : private zNoncopyable
{

public:

	/**
	* \brief ���캯��
	* ��������lock����
	* \param m ��������
	*/
	zMutex_scope_lock(zMutex &m) : mlock(m)
	{
		mlock.lock();
	}

	/**
	* \brief ��������
	* ��������unlock����
	*/
	~zMutex_scope_lock()
	{
		mlock.unlock();
	}

private:

	/**
	* \brief ��������
	*/
	zMutex &mlock;

};

/**
* \brief ʱ�䶨��
*
*
*/
/**
* \brief ��ʵʱ����,��timeval�ṹ�򵥷�װ,�ṩһЩ����ʱ�亯��
* ʱ�侫�Ⱦ�ȷ�����룬
* ����timeval��man gettimeofday
*/
class zRTime
{

private:

	/**
	* \brief ��ʵʱ�任��Ϊ����
	*
	*/
	QWORD _msecs;

	/**
	* \brief �õ���ǰ��ʵʱ��
	*
	* \return ��ʵʱ�䣬��λ����
	*/
	QWORD _now()
	{
		QWORD retval = 0LL;
		struct tm tv;
		getLocalTime(tv, NULL);
		retval = tv.tm_sec;
		return retval;
	}

	/**
	* \brief �õ���ǰ��ʵʱ���ӳٺ��ʱ��
	* \param delay �ӳ٣�����Ϊ��������λ����
	*/
	void nowByDelay(int delay)
	{
		_msecs = _now();
		addDelay(delay);
	}

public:

	/**
	* \brief ���캯��
	*
	* \param delay ���������ʱ�����ʱ����λ����
	*/
	zRTime(const int delay = 0)
	{
		nowByDelay(delay);
	}

	/**
	* \brief �������캯��
	*
	* \param rt ����������
	*/
	zRTime(const zRTime &rt)
	{
		_msecs = rt._msecs;
	}

	/**
	* \brief ��ȡ��ǰʱ��
	*
	*/
	void now()
	{
		_msecs = _now();
	}

	/**
	* \brief ��������
	*
	* \return ����
	*/
	DWORD sec() const
	{
		return _msecs / 1000;
	}

	/**
	* \brief ���غ�����
	*
	* \return ������
	*/
	DWORD msec() const
	{
		return _msecs % 1000;
	}

	/**
	* \brief �����ܹ��ĺ�����
	*
	* \return �ܹ��ĺ�����
	*/
	QWORD msecs() const
	{
		return _msecs;
	}

	/**
	* \brief �����ܹ��ĺ�����
	*
	* \return �ܹ��ĺ�����
	*/
	void setmsecs(QWORD data)
	{
		_msecs = data;
	}

	/**
	* \brief ���ӳ�ƫ����
	*
	* \param delay �ӳ٣�����Ϊ��������λ����
	*/
	void addDelay(int delay)
	{
		_msecs += delay;
	}

	/**
	* \brief ����=�������
	*
	* \param rt ����������
	* \return ��������
	*/
	zRTime & operator= (const zRTime &rt)
	{
		_msecs = rt._msecs;
		return *this;
	}

	/**
	* \brief �ع�+������
	*
	*/
	const zRTime & operator+ (const zRTime &rt)
	{
		_msecs += rt._msecs;
		return *this;
	}

	/**
	* \brief �ع�-������
	*
	*/
	const zRTime & operator- (const zRTime &rt)
	{
		_msecs -= rt._msecs;
		return *this;
	}

	/**
	* \brief �ع�>���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator > (const zRTime &rt) const
	{
		return _msecs > rt._msecs;
	}

	/**
	* \brief �ع�>=���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator >= (const zRTime &rt) const
	{
		return _msecs >= rt._msecs;
	}

	/**
	* \brief �ع�<���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator < (const zRTime &rt) const
	{
		return _msecs < rt._msecs;
	}

	/**
	* \brief �ع�<=���������Ƚ�zRTime�ṹ��С
	*
	*/
	bool operator <= (const zRTime &rt) const
	{
		return _msecs <= rt._msecs;
	}

	/**
	* \brief �ع�==���������Ƚ�zRTime�ṹ�Ƿ����
	*
	*/
	bool operator == (const zRTime &rt) const
	{
		return _msecs == rt._msecs;
	}

	/**
	* \brief ��ʱ�����ŵ�ʱ�䣬��λ����
	* \param rt ��ǰʱ��
	* \return ��ʱ�����ŵ�ʱ�䣬��λ����
	*/
	QWORD elapse(const zRTime &rt) const
	{
		if (rt._msecs > _msecs)
			return (rt._msecs - _msecs);
		else
			return 0LL;
	}

	static std::string & getLocalTZ(std::string & s);
	static void getLocalTime(struct tm & tv1, time_t timValue)
	{
		timValue += 8 * 60 * 60;
		tv1 = *gmtime(&timValue);
	}

};

/**
* \brief ʱ����,��struct tm�ṹ�򵥷�װ
*/

class zTime
{

public:

	/**
	* \brief ���캯��
	*/
	zTime()
	{
		time(&secs);
		zRTime::getLocalTime(tv, secs);
	}

	/**
	* \brief �������캯��
	*/
	zTime(const zTime &ct)
	{
		secs = ct.secs;
		zRTime::getLocalTime(tv, secs);
	}

	/**
	* \brief ��ȡ��ǰʱ��
	*/
	void now()
	{
		time(&secs);
		zRTime::getLocalTime(tv, secs);
	}

	/**
	* \brief ���ش洢��ʱ��
	* \return ʱ�䣬��
	*/
	time_t sec() const
	{
		return secs;
	}

	/**
	* \brief ����=�������
	* \param rt ����������
	* \return ��������
	*/
	zTime & operator= (const zTime &rt)
	{
		secs = rt.secs;
		return *this;
	}

	/**
	* \brief �ع�+������
	*/
	const zTime & operator+ (const zTime &rt)
	{
		secs += rt.secs;
		return *this;
	}

	/**
	* \brief �ع�-������
	*/
	const zTime & operator- (const zTime &rt)
	{
		secs -= rt.secs;
		return *this;
	}

	/**
	* \brief �ع�-������
	*/
	const zTime & operator-= (const time_t s)
	{
		secs -= s;
		return *this;
	}

	/**
	* \brief �ع�>���������Ƚ�zTime�ṹ��С
	*/
	bool operator > (const zTime &rt) const
	{
		return secs > rt.secs;
	}

	/**
	* \brief �ع�>=���������Ƚ�zTime�ṹ��С
	*/
	bool operator >= (const zTime &rt) const
	{
		return secs >= rt.secs;
	}

	/**
	* \brief �ع�<���������Ƚ�zTime�ṹ��С
	*/
	bool operator < (const zTime &rt) const
	{
		return secs < rt.secs;
	}

	/**
	* \brief �ع�<=���������Ƚ�zTime�ṹ��С
	*/
	bool operator <= (const zTime &rt) const
	{
		return secs <= rt.secs;
	}

	/**
	* \brief �ع�==���������Ƚ�zTime�ṹ�Ƿ����
	*/
	bool operator == (const zTime &rt) const
	{
		return secs == rt.secs;
	}

	/**
	* \brief ��ʱ�����ŵ�ʱ�䣬��λ��
	* \param rt ��ǰʱ��
	* \return ��ʱ�����ŵ�ʱ�䣬��λ��
	*/
	time_t elapse(const zTime &rt) const
	{
		if (rt.secs > secs)
			return (rt.secs - secs);
		else
			return 0;
	}

	/**
	* \brief ��ʱ�����ŵ�ʱ�䣬��λ��
	* \return ��ʱ�����ŵ�ʱ�䣬��λ��
	*/
	time_t elapse() const
	{
		zTime rt;
		return (rt.secs - secs);
	}

	/**
	* \brief �õ���ǰ���ӣ���Χ0-59��
	*
	* \return
	*/
	int getSec()
	{
		return tv.tm_sec;
	}

	/**
	* \brief �õ���ǰ���ӣ���Χ0-59��
	*
	* \return
	*/
	int getMin()
	{
		return tv.tm_min;
	}

	/**
	* \brief �õ���ǰСʱ����Χ0-23��
	*
	* \return
	*/
	int getHour()
	{
		return tv.tm_hour;
	}

	/**
	* \brief �õ���������Χ1-31
	*
	* \return
	*/
	int getMDay()
	{
		return tv.tm_mday;
	}

	/**
	* \brief �õ���ǰ���ڼ�����Χ1-7
	*
	* \return
	*/
	int getWDay()
	{
		return tv.tm_wday;
	}

	/**
	* \brief �õ���ǰ�·ݣ���Χ1-12
	*
	* \return
	*/
	int getMonth()
	{
		return tv.tm_mon + 1;
	}

	/**
	* \brief �õ���ǰ���
	*
	* \return
	*/
	int getYear()
	{
		return tv.tm_year + 1900;
	}

private:

	/**
	* \brief �洢ʱ�䣬��λ��
	*/
	time_t secs;

	/**
	* \brief tm�ṹ���������
	*/
	struct tm tv;


};

class Timer
{
public:
	Timer(const float how_long, const int delay = 0) : _long((int)(how_long * 1000)), _timer(delay * 1000)
	{

	}
	Timer(const float how_long, const zRTime cur) : _long((int)(how_long * 1000)), _timer(cur)
	{
		_timer.addDelay(_long);
	}
	void next(const zRTime &cur)
	{
		_timer = cur;
		_timer.addDelay(_long);
	}
	bool operator() (const zRTime& current)
	{
		if (_timer <= current) {
			_timer = current;
			_timer.addDelay(_long);
			return true;
		}

		return false;
	}
private:
	int _long;
	zRTime _timer;
};

/**
* \brief �������������
*/
#pragma pack(1)


/**
* \brief ��װ��ϵͳ��д����ʹ����Ҫ�򵥣�ʡȥ���ֹ���ʼ��������ϵͳ��д���Ĺ�������Щ�����������ɹ��캯���������������Զ����
*
*/
class zRWLock : private zNoncopyable
{

public:
	/**
	* \brief ���캯�������ڴ���һ����д��
	*
	*/
	zRWLock()
	{
		//m_hMutex = CreateMutex(NULL, FALSE, NULL);
	}

	/**
	* \brief ������������������һ����д��
	*
	*/
	~zRWLock()
	{
		//CloseHandle(m_hMutex);
	}

	/**
	* \brief �Զ�д�����ж���������
	*
	*/
	inline void rdlock()
	{
		//WaitForSingleObject(m_hMutex, INFINITE);
	};

	/**
	* \brief �Զ�д������д��������
	*
	*/
	inline void wrlock()
	{
		//WaitForSingleObject(m_hMutex, INFINITE);
	}

	/**
	* \brief �Զ�д�����н�������
	*
	*/
	inline void unlock()
	{
		//ReleaseMutex(m_hMutex);
	}

private:

	//HANDLE m_hMutex;    /**< ϵͳ��д�� */

};

template <typename V>
class Parse
{
public:
	V* operator () (const std::string& down, const std::string& separator_down)
	{
		std::string::size_type pos = 0;
		if ((pos = down.find(separator_down)) != std::string::npos) {

			std::string first_element = down.substr(0, pos);
			std::string second_element = down.substr(pos + separator_down.length());
			return new V(first_element, second_element);
		}

		return NULL;
	}
};

template <typename V>
class Parse3
{
public:
	V* operator () (const std::string& down, const std::string& separator_down)
	{
		std::string::size_type pos = 0;
		if ((pos = down.find(separator_down)) != std::string::npos) {

			std::string first_element = down.substr(0, pos);
			std::string::size_type npos = 0;
			if ((npos = down.find(separator_down, pos + separator_down.length())) != std::string::npos) {
				std::string second_element = down.substr(pos + separator_down.length(), npos - pos);
				std::string third_element = down.substr(npos + separator_down.length());
				return new V(first_element, second_element, third_element);
			}
		}

		return NULL;
	}
};


/**
* \brief  �ָ��ɶ����ָ����ָ����ַ���
* \param list ���ָ����ַ���
* \param dest �洢�ָ���������������ض�������Ҫ��
* \param separator_up һ���ָ���
* \param separator_down �����ָ���
*/
template <template <typename> class P = Parse>
class Split
{
public:

	template <typename T>
	void operator() (const std::string& list, T& dest, const std::string& separator_up = ";", const std::string& separator_down = ",")
	{
		typedef typename T::value_type value_type;
		typedef typename T::pointer pointer;

		std::string::size_type lpos = 0;
		std::string::size_type pos = 0;
		P<value_type> p;


		while ((lpos = list.find(separator_up, pos)) != std::string::npos) {
			/*
			std::string down = list.substr(pos,lpos - pos);
			std::string::size_type dpos = 0;
			if  ( (dpos = down.find(separator_down)) != std::string::npos ) {

			std::string first_element = down.substr(0,dpos);
			std::string second_element = down.substr(dpos+separator_down.length());
			dest.push_back(typename T::value_type(first_element,second_element));
			}
			pos = lpos+1;
			*/
			std::string down = list.substr(pos, lpos - pos);
			pointer v = p(down, separator_down);
			if (v) {
				dest.push_back(*v);
				SAFE_DELETE(v);
			}
			pos = lpos + 1;
		}

		std::string down = list.substr(pos, lpos - pos);
		pointer v = p(down, separator_down);
		if (v) {
			dest.push_back(*v);
			SAFE_DELETE(v);
		}
	}
};


/**
* \brief zUniqueIDģ��
* ��ģ��ʵ����ΨһID������������֤�̰߳�ȫ��
* �����ø��ֳ��ȵ��޷���������ΪID��
*/
template <class T>
class zUniqueID :private zNoncopyable
{
private:
	zMutex mutex;
	//std::list<T,__pool_alloc<T> > ids;
	std::list<T> ids;
	T maxID;
	T minID;
	T curMaxID;
	void init(T min, T max)
	{
		minID = min;
		maxID = max;
		curMaxID = minID;
	}

public:
	/**
	* \brief Ĭ�Ϲ��캯��
	* ��ʼIDΪ1�������ЧIDΪ(T)-2,��ЧIDΪ(T)-1
	*/
	zUniqueID()
	{
		init(1, (T)-1);
	}

	/**
	* \brief ���캯��
	* �û��Զ�����ʼID�������ЧIDΪ(T)-2,��ЧIDΪ(T)-1
	* \param startID �û��Զ������ʼID
	*/
	zUniqueID(T startID)
	{
		init(startID, (T)-1);
	}

	/**
	* \brief ���캯��
	* �û��Զ�����ʼID���������ЧID,�����ЧIDΪ�����ЧID-1
	* \param startID �û��Զ������ʼID
	* \param endID �û��Զ���������ЧID
	*/
	zUniqueID(T startID, T endID)
	{
		init(startID, endID);
	}

	/**
	* \brief ��������
	* �����ѷ����ID�ڴ档
	*/
	~zUniqueID()
	{
		//mutex.lock();
		ids.clear();
		//mutex.unlock();
	}

	/**
	* \brief �õ������ЧID
	* \return ���������ЧID
	*/
	T invalid()
	{
		return maxID;
	}

	/**
	* \brief �������ID�Ƿ񱻷����ȥ
	* \return �������ȥ����true,��ЧID��δ����ID����false
	*/
	bool hasAssigned(T testid)
	{
		//mutex.lock();
		if (testid<maxID && testid >= minID)
		{
			typename std::list<T/*,__pool_alloc<T>*/ >::iterator iter = ids.begin();
			for (; iter != ids.end(); iter++)
			{
				if (*iter == testid)
				{
					mutex.unlock();
					return false;
				}
			}
			/*
			for(int i=0,n=ids.size() ;i<n;i++)
			{
			if (ids[i]==testid)
			{
			mutex.unlock();
			return false;
			}
			}
			// */
			mutex.unlock();
			return true;
		}
		//mutex.unlock();
		return false;
	}

	/**
	* \brief �õ�һ��ΨһID
	* \return ����һ��ΨһID��������������ЧID���ȱ�ʾ����ID���ѱ��ã��޿���ID��
	*/
	T get()
	{
		T ret;
		mutex.lock();
		if (maxID>curMaxID)
		{
			ret = curMaxID;
			curMaxID++;
		}
		else
			ret = maxID;
		if (ret == maxID && !ids.empty())
		{
			ret = ids.back();
			ids.pop_back();
		}
		mutex.unlock();
		return ret;
	}

	/**
	* \brief һ�εõ����ID����ЩID�������ڵ�,���Ҳ��ر��Ż�ȥ
	* \param size Ҫ�����ID����
	* \param count ʵ�ʷ���ID�ĸ���
	* \return ���ص�һ��ID��������������ЧID���ȱ�ʾ����ID���ѱ��ã��޿���ID��
	*/
	T get(int size, int & count)
	{
		T ret;
		mutex.lock();
		if (maxID>curMaxID)
		{
			count = (maxID - curMaxID)>size ? size : (maxID - curMaxID);
			ret = curMaxID;
			curMaxID += count;
		}
		else
		{
			count = 0;
			ret = maxID;
		}
		mutex.unlock();
		return ret;
	}

	/**
	* \brief ��ID�Ż�ID�أ��Ա��´�ʹ�á�
	*
	* �Żص�ID��������get�����õ��ġ����Ҳ��ܱ�֤�Żص�ID,û�б������߳�ʹ�á�
	* �����û�Ҫ�Լ���֤����ʹ�õ�ID���ᱻ�Ż�ȥ���������ID�ظ�����
	* \param id ��get�õ���ID.
	*/
	void put(T id)
	{
		mutex.lock();
		if (id<maxID && id >= minID)
		{
			bool hasID = false;
			typename std::list<T/*,__pool_alloc<T> */>::iterator iter = ids.begin();
			for (; iter != ids.end(); iter++)
			{
				if (*iter == id)
				{
					hasID = true;
					break;
				}
			}
			/*
			for(int i=0,n=ids.size() ;i<n;i++)
			{
			if (ids[i]==id)
			{
			hasID=true;
			break;
			}
			}
			// */
			if (!hasID) ids.push_front(id);
			//if (!hasID) ids.insert(ids.begin(),id);
			//if (!hasID) ids.push_back(id);
		}
		//mutex.unlock();
	}
};

typedef zUniqueID<QWORD> zUniqueDWORDID;

#pragma pack(1)

struct zEntryC
{
	/**
	* \brief entry������ID����ͬ���͵�Entry���ܻ��ظ�,��ʱ����ʵ�ִ�ID����entry
	*/
	QWORD id;
	/**
	* \brief entry����ʱid,������ʵ��EntryManagerʱ����֤����Ψһ
	*/
	QWORD tempid;
	/**
	* \brief entry�����֣���ͬ���͵�Entry���ܻ��ظ�,��ʱ����ʵ�ִ����ֲ���entry
	*/
	char name[MAX_NAMESIZE + 1];
	zEntryC()
	{
		id = 0;
		tempid = 0;
		bzero(name, sizeof(name));
	};
};


#define CreateEntryC(TempName) \
struct zEntryC##TempName \
{\
	QWORD id;\
	QWORD TempName;\
	char name[MAX_NAMESIZE + 1];\
	zEntryC##TempName()\
	{\
		id = 0;\
		TempName = 0;\
		bzero(name, sizeof(name));\
	};\
};


/**
* \brief �ص�������ģ��
*/
template <typename T, typename RTValue = bool>
struct zEntryCallback
{
	virtual RTValue exec(T *e) = 0;
	virtual ~zEntryCallback() {};
};


struct zEntry :public zEntryC, private zNoncopyable
{
	virtual ~zEntry() {};
	zEntry() :zEntryC()
	{
	};
};

#define CreateEntry(TempName) \
struct zEntry##TempName :public zEntryC##TempName, private zNoncopyable \
{\
	virtual ~zEntry##TempName() {};\
	zEntry##TempName() :zEntryC##TempName()\
	{\
	};\
};

/**
* \brief keyֵ��ֵ�Ƚ�,Ŀǰ֧�� (DWORD,char *)����������
*/
template <class keyT>
struct my_key_equal : public std::binary_function<keyT, keyT, bool>
{
	inline bool operator()(const keyT s1, const keyT s2) const;
};

/**
* \brief ģ��ƫ�ػ�
* ���ַ������бȽ�
*/
template<>
inline bool my_key_equal<const char *>::operator()(const char * s1, const char * s2) const
{
	return strcmp(s1, s2) == 0;
}

/**
* \brief ģ��ƫ�ػ�
* ���������бȽ�
*/
template<>
inline bool my_key_equal<DWORD>::operator()(const DWORD s1, const DWORD s2) const
{
	return s1 == s2;
}

/**
* \brief ����ͰHash����ģ��,���̰߳�ȫ
*
* Ŀǰ֧������key����(DWORD,char *),value���Ͳ�������,��������Ҫ��copy�ġ�
* \param keyT key����(DWORD,char *)
* \param valueT value����
*/
template <class keyT, class valueT>
class LimitHash :private zNoncopyable
{
protected:

	/**
	* \brief hash_map����
	*/
	//typedef hash_map<keyT,valueT,hash<keyT>,my_key_equal<keyT> > hashmap;
	typedef UNORDERED_MAP<keyT, valueT> hashmap;
	typedef typename hashmap::iterator iter;
	typedef typename hashmap::const_iterator const_iter;
	hashmap ets;

	/**
	* \brief �������ݣ����ԭ��������ͬkeyֵ�����ݣ�ԭ�����ݽ��ᱻ�滻
	* \param key keyֵ
	* \param value Ҫ���������
	* \return �ɹ�����true�����򷵻�false
	*/
	inline bool insert(const keyT &key, valueT &value)
	{
		ets[key] = value;
		return true;
	}

	/**
	* \brief ����keyֵ���Ҳ��õ�����
	* \param key ҪѰ�ҵ�keyֵ
	* \param value ���ؽ��������˴�,δ�ҵ�������ı��ֵ
	* \return ���ҵ�����true��δ�ҵ�����false
	*/
	inline bool find(const keyT &key, valueT &value) const
	{
		const_iter it = ets.find(key);
		if (it != ets.end())
		{
			value = it->second;
			return true;
		}
		else
			return false;
	}

	/**
	* \brief ���Ҳ��õ�һ������
	* \param value ���ؽ��������˴�,δ�ҵ�������ı��ֵ
	* \return ���ҵ�����true��δ�ҵ�����false
	*/
	inline bool findOne(valueT &value) const
	{
		if (!ets.empty())
		{
			value = ets.begin()->second;
			return true;
		}
		return false;
	}

	/**
	* \brief ���캯��
	*
	*/
	LimitHash()
	{
	}

	/**
	* \brief ��������,�����������
	*/
	~LimitHash()
	{
		clear();
	}

	/**
	* \brief �Ƴ�����
	* \param key Ҫ�Ƴ���keyֵ
	*/
	inline void remove(const keyT &key)
	{
		ets.erase(key);
	}

	/**
	* \brief �����������
	*/
	inline void clear()
	{
		ets.clear();
	}

	/**
	* \brief ͳ�����ݸ���
	*/
	inline DWORD size() const
	{
		return ets.size();
	}

	/**
	* \brief �ж������Ƿ�Ϊ��
	*/
	inline bool empty() const
	{
		return ets.empty();
	}
};

/**
* \brief ����ͰMultiHash����ģ��,���̰߳�ȫ
*
* Ŀǰ֧������key����(DWORD,char *),value���Ͳ�������,��������Ҫ��copy�ġ�
* \param keyT key����(DWORD,char *)
* \param valueT value����
*/
template <class keyT, class valueT>
class MultiHash :private zNoncopyable
{
protected:

	/**
	* \brief hash_multimap����
	*/
	//typedef hash_multimap<keyT,valueT,hash<keyT>,my_key_equal<keyT> > hashmap;
	typedef UNORDERED_MULTIMAP<keyT, valueT> hashmap;
	typedef typename hashmap::iterator iter;
	typedef typename hashmap::const_iterator const_iter;
	hashmap ets;

	/**
	* \brief �������ݣ����ԭ��������ͬkeyֵ�����ݣ�ԭ�����ݽ��ᱻ�滻
	* \param key keyֵ
	* \param value Ҫ���������
	* \return �ɹ�����true�����򷵻�false
	*/
	inline bool insert(const keyT &key, valueT &value)
	{
		//if(ets.find(key) == ets.end())
		ets.insert(std::pair<keyT, valueT>(key, value));
		return true;
	}

	/**
	* \brief ���캯��
	*
	*/
	MultiHash()
	{
	}

	/**
	* \brief ��������,�����������
	*/
	~MultiHash()
	{
		clear();
	}

	/**
	* \brief �����������
	*/
	inline void clear()
	{
		ets.clear();
	}

	/**
	* \brief ͳ�����ݸ���
	*/
	inline DWORD size() const
	{
		return ets.size();
	}

	/**
	* \brief �ж������Ƿ�Ϊ��
	*/
	inline bool empty() const
	{
		return ets.empty();
	}
};

/**
* \brief Entry����ʱIDΪkeyֵ��ָ����������Ҫ�̳�ʹ��
*/

class zEntryTempID :public LimitHash<QWORD, zEntry *>
{
protected:

	zEntryTempID() {}
	virtual ~zEntryTempID() {}

	/**
	* \brief ��Entry����������,tempid�ظ�����ʧ��
	* \param e Ҫ�����Entry
	* \return �ɹ�����true,���򷵻�false
	*/
	inline bool push(zEntry* e)
	{
		if (e != NULL && getUniqeID(e->tempid))
		{
			zEntry *temp;
			if (!find(e->tempid, temp))
			{
				if (insert(e->tempid, e))
					return true;
			}
			putUniqeID(e->tempid);
		}
		return false;
	}

	/**
	* \brief �Ƴ�Entry
	* \param e Ҫ�Ƴ���Entry
	*/
	inline void remove(zEntry* e)
	{
		if (e != NULL)
		{
			putUniqeID(e->tempid);
			LimitHash<QWORD, zEntry *>::remove(e->tempid);
		}
	}

	/**
	* \brief ͨ����ʱID�õ�Entry
	* \param tempid Ҫ�õ�Entry����ʱID
	* \return ����Entryָ��,δ�ҵ�����NULL
	*/
	inline zEntry* getEntryByTempID(const QWORD tempid) const
	{
		zEntry *ret = NULL;
		LimitHash<QWORD, zEntry *>::find(tempid, ret);
		return ret;
	}

	/**
	* \brief �õ�һ����ʱID
	* \param tempid ���Ҫ�õ�����ʱID
	* \return �õ�����true,���򷵻�false
	*/
	virtual bool getUniqeID(QWORD &tempid) = 0;
	/**
	* \brief �Ż�һ����ʱID
	* \param tempid Ҫ�Żص���ʱID
	*/
	virtual void putUniqeID(const QWORD &tempid) = 0;
};

#define CreateEntryTempID(TempName) \
class zEntryTempID##TempName :public LimitHash<QWORD, zEntry##TempName *>\
{\
protected:\
	zEntryTempID##TempName() {}\
	virtual ~zEntryTempID##TempName() {}\
	inline bool push(zEntry##TempName* e)\
	{\
		if (e != NULL && getUniqeID(e->##TempName))\
		{\
			zEntry##TempName *temp;\
			if (!find(e->##TempName, temp))\
			{\
				if (insert(e->##TempName, e))\
					return true;\
			}\
			putUniqeID(e->##TempName);\
		}\
		return false;\
	}\
	inline void remove(zEntry##TempName* e)\
	{\
		if (e != NULL)\
		{\
			putUniqeID(e->##TempName);\
			LimitHash<QWORD, zEntry##TempName *>::remove(e->##TempName);\
		}\
	}\
	inline zEntry##TempName* getEntryBy##TempName(const QWORD TempName) const\
	{\
		zEntry##TempName *ret = NULL;\
		LimitHash<QWORD, zEntry##TempName *>::find(TempName, ret);\
		return ret;\
	}\
	virtual bool getUniqeID(QWORD &##TempName) = 0;\
	virtual void putUniqeID(const QWORD &##TempName) = 0;\
};

/**
* \brief Entry��IDΪkeyֵ��ָ����������Ҫ�̳�ʹ��
*/
class zEntryID :public LimitHash<QWORD, zEntry *>
{
protected:
	/**
	* \brief ��Entry����������
	* \param e Ҫ�����Entry
	* \return �ɹ�����true,���򷵻�false
	*/
	inline bool push(zEntry * &e)
	{
		zEntry *temp;
		if (!find(e->id, temp))
			return insert(e->id, e);
		else
			return false;
	}

	/**
	* \brief �Ƴ�Entry
	* \param e Ҫ�Ƴ���Entry
	*/
	inline void remove(zEntry * e)
	{
		if (e != NULL)
		{
			LimitHash<QWORD, zEntry *>::remove(e->id);
		}
	}

	/**
	* \brief ͨ��ID�õ�Entry
	* \param id Ҫ�õ�Entry��ID
	* \return ����Entryָ��,δ�ҵ�����NULL
	*/
	inline zEntry * getEntryByID(const QWORD id) const
	{
		zEntry *ret = NULL;
		LimitHash<QWORD, zEntry *>::find(id, ret);
		return ret;
	}
};

#define CreateEntryID(TempName) \
class zEntryID##TempName :public LimitHash<QWORD, zEntry##TempName *>\
{\
protected:\
	inline bool push(zEntry##TempName * &e)\
	{\
		zEntry##TempName *temp;\
		if (!find(e->id, temp))\
			return insert(e->id, e);\
		else\
			return false;\
	}\
	inline void remove(zEntry##TempName * e)\
	{\
		if (e != NULL)\
		{\
			LimitHash<QWORD, zEntry##TempName *>::remove(e->id);\
		}\
	}\
	inline zEntry##TempName * getEntryByID(const QWORD id) const\
	{\
		zEntry##TempName *ret = NULL;\
		LimitHash<QWORD, zEntry##TempName *>::find(id, ret);\
		return ret;\
	}\
};

/**
* \brief Entry������Ϊkeyֵ��ָ����������Ҫ�̳�ʹ��
*/
class zEntryName :public LimitHash<std::string, zEntry *>
{
protected:
	/**
	* \brief ��Entry����������,�������������ͬkeyֵ������ʧ��
	* \param e Ҫ�����Entry
	* \return �ɹ�����true,���򷵻�false
	*/
	inline bool push(zEntry * &e)
	{
		zEntry *temp;
		if (!find(std::string(e->name), temp))
			return insert(std::string(e->name), e);
		else
			return false;
	}

	/**
	* \brief �Ƴ�Entry
	* \param e Ҫ�Ƴ���Entry
	*/
	inline void remove(zEntry * e)
	{
		if (e != NULL)
		{
			LimitHash<std::string, zEntry *>::remove(std::string(e->name));
		}
	}

	/**
	* \brief ͨ�����ֵõ�Entry
	* \param name Ҫ�õ�Entry������
	* \return ����Entryָ��,δ�ҵ�����NULL
	*/
	inline zEntry * getEntryByName(const char * name) const
	{
		zEntry *ret = NULL;
		LimitHash<std::string, zEntry *>::find(std::string(name), ret);
		return ret;
	}

	/**
	* \brief ͨ�����ֵõ�Entry
	* \param name Ҫ�õ�Entry������
	* \return ����Entryָ��,δ�ҵ�����NULL
	*/
	inline zEntry * getEntryByName(const std::string  &name) const
	{
		return getEntryByName(name.c_str());
	}
};

#define CreateEntryName(TempName) \
class zEntryName##TempName :public LimitHash<std::string, zEntry##TempName *>\
{\
protected:\
	inline bool push(zEntry##TempName * &e)\
	{\
		zEntry##TempName *temp;\
		if (!find(std::string(e->name), temp))\
			return insert(std::string(e->name), e);\
		else\
			return false;\
	}\
	inline void remove(zEntry##TempName * e)\
	{\
		if (e != NULL)\
		{\
			LimitHash<std::string, zEntry##TempName *>::remove(std::string(e->name));\
		}\
	}\
	inline zEntry##TempName * getEntryByName(const char * name) const\
	{\
		zEntry##TempName *ret = NULL;\
		LimitHash<std::string, zEntry##TempName *>::find(std::string(name), ret);\
		return ret;\
	}\
	inline zEntry##TempName * getEntryByName(const std::string  &name) const\
	{\
		return getEntryByName(name.c_str());\
	}\
};

/**
* \brief Entry������Ϊkeyֵ��ָ����������Ҫ�̳�ʹ��
*/
class zMultiEntryName :public MultiHash</*const char **/std::string, zEntry *>
{
protected:
	/**
	* \brief ��Entry����������,�������������ͬkeyֵ������ʧ��
	* \param e Ҫ�����Entry
	* \return �ɹ�����true,���򷵻�false
	*/
	inline bool push(zEntry * &e)
	{
		return insert(std::string(e->name), e);
	}

	/**
	* \brief ��Entry���������Ƴ�
	* \param e ��Ҫ�Ƴ���Entry
	*/
	inline void remove(zEntry * &e)
	{
		pair<iter, iter> its = ets.equal_range(std::string(e->name));
		for (iter it = its.first; it != its.second; it++)
		{
			if (it->second == e)
			{
				ets.erase(it);
				return;
			}
		}
	}

	/**
	* \brief ����keyֵ���Ҳ��õ�����
	* \param name ҪѰ�ҵ�nameֵ
	* \param e ���ؽ��������˴�,δ�ҵ�������ı��ֵ
	* \param r ����ж���ƥ�䣬�Ƿ����ѡ��
	* \return ���ҵ�����true��δ�ҵ�����false
	*/
	inline bool find(const char * &name, zEntry * &e, const bool r = false) const
	{
		int rd = ets.count(std::string(name));
		if (rd > 0)
		{
			int mrd = 0, j = 0;
			if (r)
				randBetween(0, rd - 1);
			pair<const_iter, const_iter> its = ets.equal_range(std::string(name));
			for (const_iter it = its.first; it != its.second && j < rd; it++, j++)
			{
				if (mrd == j)
				{
					e = it->second;
					return true;
				}
			}
		}
		return false;
	}

};

template<int i>
class zEntryNone
{
protected:
	inline bool push(zEntry * &e) { return true; }
	inline void remove(zEntry * &e) { }
	inline void clear() {}
};

#define CreateEntryNone(TempName) \
template<int i>\
class zEntryNone##TempName\
{\
protected:\
	inline bool push(zEntry##TempName * &e) { return true; }\
	inline void remove(zEntry##TempName * &e) { }\
	inline void clear() {}\
};

template<int i>
class zEntryNoneQ
{
protected:
	inline bool push(zEntry * &e) { return true; }
	inline void remove(zEntry * &e) { }
	inline void clear() {}
};

/**
* \brief Entry�����ӿ�,��<code>zEntryMgr::execEveryEntry</code>ʹ��
*/
template <class YourEntry>
struct execEntry
{
	virtual bool exec(YourEntry *entry) = 0;
	virtual ~execEntry() {}
};

/**
* \brief Entryɾ�������ӿ�,��<code>zEntryMgr::removeEntry_if</code>ʹ��
*/
template <class YourEntry>
struct removeEntry_Pred
{
	/**
	* \brief ��ɾ����entry�洢������
	*/
	std::vector<YourEntry *> removed;
	/**
	* \brief �����Ƿ�Ҫɾ����entry,��Ҫʵ��
	* \param Ҫ�����Ե�entry
	*/
	virtual bool isIt(YourEntry *entry) = 0;
	/**
	* \brief ��������
	*/
	virtual ~removeEntry_Pred() {}
};

/**
* \brief Entry�������ӿ�,�û�Ӧ�ø��ݲ�ͬʹ������̳���
*/

template<typename e1, typename e2 = zEntryNone<1>, typename e3 = zEntryNone<2> >
class zEntryMgr :protected e1, protected e2, protected e3
{
protected:

	//unsigned long count;

	/**
	* \brief ����Entry,�����ظ�������Entry����ʧ��
	* \param e �����ӵ� Entryָ��
	* \return �ɹ�����true�����򷵻�false
	*/
	inline bool addEntry(zEntry * e)
	{

		if (NULL == e)
			return false;
		//++count;
		// unsigned long t = count;

		//if( 765 == count)
		//{
		//Zebra::logger->error("%u\n",count);
		//fprintf(stderr,"%u\n",count);
		//  }
		if (e1::push(e))
		{
			//zEntry *ee = e1::getEntryByName(e->name); 
			if (e2::push(e))
			{

				if (e3::push(e))
					return true;
				else
				{
					e2::remove(e);
					e1::remove(e);
				}
			}
			else
				e1::remove(e);
		}
		return false;
	}

	/**
	* \brief ɾ��Entry
	* \param e ��ɾ����Entryָ��
	*/
	inline void removeEntry(zEntry * e)
	{
		e1::remove(e);
		e2::remove(e);
		e3::remove(e);
	}


	zEntryMgr() { }
	/**
	* \brief ����������
	*/
	~zEntryMgr() { };

	/**
	* \brief ͳ�ƹ�������Entry�ĸ���
	* \return ����Entry����
	*/
	inline int size() const
	{
		return e1::size();
	}

	/**
	* \brief �ж������Ƿ�Ϊ��
	*/
	inline bool empty() const
	{
		return e1::empty();
	}

	/**
	* \brief �������Entry
	*/
	inline void clear()
	{
		e1::clear();
		e2::clear();
		e3::clear();
	}

	/**
	* \brief ��ÿ��Entry���д���
	* ������ĳ��Entry����falseʱ������ϴ�������
	* \param eee �����ӿ�
	* \return ���ȫ��ִ����Ϸ���true,���򷵻�false
	*/
	template <class YourEntry>
	inline bool execEveryEntry(execEntry<YourEntry> &eee)
	{
		typedef typename e1::iter my_iter;
		for (my_iter it = e1::ets.begin(); it != e1::ets.end(); it++)
		{
			if (!eee.exec((YourEntry *)it->second))
				return false;
		}
		return true;
	}

	/**
	* \brief ɾ������������Entry
	* \param pred ���������ӿ�
	*/
	template <class YourEntry>
	inline void removeEntry_if(removeEntry_Pred<YourEntry> &pred)
	{
		typedef typename e1::iter my_iter;
		my_iter it = e1::ets.begin();
		while (it != e1::ets.end())
		{
			if (pred.isIt((YourEntry *)it->second))
			{
				pred.removed.push_back((YourEntry *)it->second);
			}
			it++;
		}

		for (DWORD i = 0; i<pred.removed.size(); i++)
		{
			removeEntry(pred.removed[i]);
		}
	}
};

#define CreateEntryManager(TempName) \
template<typename e1, typename e2 = zEntryNone##TempName<1>, typename e3 = zEntryNone##TempName<2> > \
class zEntryMgr##TempName :protected e1, protected e2, protected e3 \
{\
protected:\
	inline bool addEntry(zEntry##TempName * e)\
	{\
		if (NULL == e)\
			return false;\
		if (e1::push(e))\
		{\
			if (e2::push(e))\
			{\
				if (e3::push(e))\
					return true;\
				else\
				{\
					e2::remove(e);\
					e1::remove(e);\
				}\
			}\
			else\
				e1::remove(e);\
		}\
		return false;\
	}\
	inline void removeEntry(zEntry##TempName * e)\
	{\
		e1::remove(e);\
		e2::remove(e);\
		e3::remove(e);\
	}\
	zEntryMgr##TempName() { }\
	~zEntryMgr##TempName() { };\
	inline int size() const\
	{\
		return e1::size();\
	}\
	inline bool empty() const\
	{\
		return e1::empty();\
	}\
	inline void clear()\
	{\
		e1::clear();\
		e2::clear();\
		e3::clear();\
	}\
	template <class YourEntry>\
	inline bool execEveryEntry(execEntry<YourEntry> &eee)\
	{\
		typedef typename e1::iter my_iter;\
		for (my_iter it = e1::ets.begin(); it != e1::ets.end(); it++)\
		{\
			if (!eee.exec((YourEntry *)it->second))\
				return false;\
		}\
		return true;\
	}\
	template <class YourEntry>\
	inline void removeEntry_if(removeEntry_Pred<YourEntry> &pred)\
	{\
		typedef typename e1::iter my_iter;\
		my_iter it = e1::ets.begin();\
		while (it != e1::ets.end())\
		{\
			if (pred.isIt((YourEntry *)it->second))\
			{\
				pred.removed.push_back((YourEntry *)it->second);\
			}\
			it++;\
		}\
		for (DWORD i = 0; i<pred.removed.size(); i++)\
		{\
			removeEntry(pred.removed[i]);\
		}\
	}\
};


template <class TData>
class zDatabaseCallBack
{
public:
	virtual bool exec(TData *entry) = 0;
	virtual ~zDatabaseCallBack() {};
};

typedef zEntryMgr< zEntryID, zMultiEntryName> zDataManager;
template <class TData, class TBase>
class  zDataBM :public zDataManager
{
private:
	static zDataBM<TData, TBase> *me;
	zRWLock rwlock;
	zDataBM()
	{

	}

	class deleteEvery :public zDatabaseCallBack<TData>
	{
		bool exec(TData *entry)
		{
			delete entry;
			return true;
		}
	};

	~zDataBM()
	{
		deleteEvery temp;
		execAll(temp);
		rwlock.wrlock();
		clear();
		rwlock.unlock();
	}

	zEntry* getEntryByID(DWORD id)
	{
		zEntry* ret = NULL;
		zEntryID::find(id, ret);
		return ret;
	}

	void removeEntryByID(DWORD id)
	{
		zEntry * ret = NULL;
		if (zEntryID::find(id, ret))
			removeEntry(ret);
	}

	zEntry* getEntryByName(const char * name)
	{
		zEntry * ret = NULL;
		zMultiEntryName::find(name, ret, true);
		return ret;
	}

	void removeEntryByName(const char * name)
	{
		zEntry * ret = NULL;
		if (zMultiEntryName::find(name, ret))
			removeEntry(ret);
	}


public:

	static zDataBM & getMe()
	{
		if (me == NULL)
			me = new zDataBM();
		return *me;
	}

	static void delMe()
	{
		SAFE_DELETE(me);
	}

	bool refresh(TBase &base)
	{
		static DWORD id = base.getUniqueID();
		TData *o = (TData *)getEntryByID(base.getUniqueID());
		if (o == NULL)
		{
			o = new TData();
			if (o == NULL)
			{
				printf("�޷������ڴ�\n");
				return false;
			}
			o->fill(base);
			if (!zDataManager::addEntry(o))
			{
				printf("����Entry����(%ld)(id=%ld,name=%s)\n", base.field0, o->id, o->name);
				SAFE_DELETE(o);
				return false;
			}
		}
		else
		{
			o->fill(base);
			//���µ�������hash�е�λ�ã�������ʹ���Ƹı�Ҳ���Բ�ѯ��
			zMultiEntryName::remove((zEntry * &)o);
			zMultiEntryName::push((zEntry * &)o);
		}
		return true;
	}

	TData *get(DWORD dataid)
	{
		rwlock.rdlock();
		TData *ret = (TData *)getEntryByID(dataid);
		rwlock.unlock();
		return ret;
	}

	TData *get(const char *name)
	{
		rwlock.rdlock();
		TData *ret = (TData *)getEntryByName(name);
		rwlock.unlock();
		return ret;
	}

	void execAll(zDatabaseCallBack<TData> &base)
	{
		rwlock.rdlock();
		for (zEntryID::hashmap::iterator it = zEntryID::ets.begin(); it != zEntryID::ets.end(); it++)
		{
			if (!base.exec((TData *)it->second))
			{
				rwlock.unlock();
				return;
			}
		}
		rwlock.unlock();
	}

	void listAll()
	{
		class listevery :public zDatabaseCallBack<TData>
		{
		public:
			int i;
			listevery()
			{
				i = 0;
			}
			bool exec(TData *zEntry)
			{
				i++;
				printf("%ld\t%s\n", zEntry->id, zEntry->name);
				return true;
			}
		};
		listevery le;
		execAll(le);
		printf("Total %d\n", le.i);
	}
};





struct aTypeS {
	aTypeS()
	{
		byValue[0] = 0;
		byValue[1] = 0;
	}
	union {
		struct {
			BYTE byAType;
			BYTE byAction;
		};
		BYTE byValue[2];
	};
};

struct CarryObject
{
	DWORD id;
	int   rate;
	int   minnum;
	int   maxnum;
	CarryObject()
	{
		id = 0;
		rate = 0;
		minnum = 0;
		maxnum = 0;
	}
};

typedef std::vector<CarryObject> NpcLostObject;

struct NpcCarryObject : private zNoncopyable
{
	NpcCarryObject() {};
	bool set(const char *objects)
	{
		bool retval = true;
		//mlock.lock();
		cov.clear();
		if (strcmp(objects, "0"))
		{
			std::vector<std::string> obs;
			stringtok(obs, objects, ";");
			for (std::vector<std::string>::const_iterator it = obs.begin(); it != obs.end(); it++)
			{
				std::vector<std::string> rt;
				stringtok(rt, *it, ":");
				if (3 == rt.size())
				{
					CarryObject co;
					co.id = atoi(rt[0].c_str());
					co.rate = atoi(rt[1].c_str());
					std::vector<std::string> nu;
					stringtok(nu, rt[2], "-");
					if (2 == nu.size())
					{
						co.minnum = atoi(nu[0].c_str());
						co.maxnum = atoi(nu[1].c_str());
						cov.push_back(co);
					}
					else
						retval = false;
				}
				else
					retval = false;
			}
		}
		//mlock.unlock();
		return retval;
	}

	/**
	* \brief ��Ʒ���䴦��
	* \param nlo npcЯ����Ʒ����
	* \param value �����ʴ��۱�
	* \param value1 ����������
	* \param value2 ���ӵ���������
	*/
	void lost(NpcLostObject &nlo, int value, int value1, int value2, int vcharm, int vlucky, int player_level, int DropRate, int DropRateLevel)
	{
		//mlock.lock();
		if (vcharm>1000) vcharm = 1000;
		if (vlucky>1000) vlucky = 1000;
		for (std::vector<CarryObject>::const_iterator it = cov.begin(); it != cov.end(); it++)
		{
			//Zebra::logger->debug("%u,%u,%u,%u",(*it).id,(*it).rate,(*it).minnum,(*it).maxnum);
			switch ((*it).id)
			{
			case 665:
			{
				int vrate = (int)(((*it).rate / value)*(1 + value1 / 100.0f)*(1 + value2 / 100.0f)*(1 + vcharm / 1000.0f)*(1 + vlucky / 1000.0f));
				/*if (selectByTenTh(vrate))
				{
				nlo.push_back(*it);
				}*/
			}
			break;
			default:
			{
				int vrate = (int)(((*it).rate / value)*(1 + value1 / 100.0f)*(1 + vcharm / 1000.0f)*(1 + vlucky / 1000.0f));
				if (player_level <= DropRateLevel)
				{
					/*if (selectByTenTh(vrate * DropRate))
					{
					nlo.push_back(*it);
					}*/
				}
				else
				{
					/*if (selectByTenTh(vrate))
					{
					nlo.push_back(*it);
					}*/
				}
			}
			break;
			}
		}
		//mlock.unlock();
	}
	/**
	* \brief ȫ����Ʒ���䴦��
	* \param nlo npcЯ����Ʒ����
	* \param value �����ʴ��۱�
	* \param value1 ����������
	* \param value2 ���ӵ���������
	*/
	void lostAll(NpcLostObject &nlo)
	{
		for (std::vector<CarryObject>::const_iterator it = cov.begin(); it != cov.end(); it++)
		{
			nlo.push_back(*it);
		}
	}

	/**
	* \brief װ����Ʒȫ�����䴦��(�̹�ר��)
	* \param nlo npcЯ����Ʒ����
	* \param value �����ʴ��۱�
	* \param value1 ����������
	* \param value2 ���ӵ���������
	*/
	void lostGreen(NpcLostObject &nlo, int value = 1, int value1 = 0, int value2 = 0, int vcharm = 0, int vlucky = 0);
private:
	std::vector<CarryObject> cov;
	//zMutex mlock;
};


#pragma pack()

///npcʹ��һ�����ܵ�����
struct npcSkill
{
	DWORD id;///����id
	int needLevel;///����id
	int rate;///ʹ�ü���
	int coefficient;///����ϵ��

	npcSkill() :id(0), needLevel(0), rate(0), coefficient(0) {}
	npcSkill(const npcSkill &skill)
	{
		id = skill.id;
		needLevel = skill.needLevel;
		rate = skill.rate;
		coefficient = skill.coefficient;
	}
	npcSkill& operator = (const npcSkill &skill)
	{
		id = skill.id;
		needLevel = skill.needLevel;
		rate = skill.rate;
		coefficient = skill.coefficient;
		return *this;
	}
};

struct npcRecover
{
	DWORD start;
	BYTE type;
	DWORD num;

	npcRecover()
	{
		start = 0;
		type = 0;
		num = 0;
	}

	void parse(const char * str)
	{
		if (!str) return;

		std::vector<std::string> vec;

		vec.clear();
		stringtok(vec, str, ":");
		if (3 == vec.size())
		{
			start = atoi(vec[0].c_str());
			type = atoi(vec[1].c_str());
			num = atoi(vec[2].c_str());
		}
	}
};




/**
* \brief A*Ѱ·�㷨
*/
/**
* \brief A*Ѱ·�㷨ģ��
* ����step��ʾ������radius��ʾ�����뾶
*/
template <int step = 1, int radius = 12>
class zAStar
{

private:

	/**
	* \brief ·�������
	*/
	struct zPathPoint
	{
		/**
		* \brief ����
		*/
		zPos pos;
		/**
		* \brief ��ǰ����
		*/
		int cc;
		/**
		* \brief ·����һ�����ָ��
		*/
		zPathPoint *father;
	};

	/**
	* \brief ·��ͷ
	*/
	struct zPathQueue
	{
		/**
		* \brief ·���ڵ�ͷָ��
		*/
		zPathPoint *node;
		/**
		* \brief ·�����ľ���
		*/
		int cost;
		/**
		* \brief ���캯��
		* \param node ��ʼ����·���ڵ�ͷָ��
		* \param cost ��ǰ���ľ���
		*/
		zPathQueue(zPathPoint *node, int cost)
		{
			this->node = node;
			this->cost = cost;
		}
		/**
		* \brief �������캯��
		* \param queue ��������Դ����
		*/
		zPathQueue(const zPathQueue &queue)
		{
			node = queue.node;
			cost = queue.cost;
		}
		/**
		* \brief ��ֵ��������
		* \param queue ����ֵ��Դ����
		* \return ���ؽṹ������
		*/
		zPathQueue & operator= (const zPathQueue &queue)
		{
			node = queue.node;
			cost = queue.cost;
			return *this;
		}
	};

	/**
	* \brief ��������·��������
	*/
	typedef std::list<zPathQueue> zPathQueueHead;
	typedef typename zPathQueueHead::iterator iterator;
	typedef typename zPathQueueHead::reference reference;

	/**
	* \brief ���ۺ���
	* \param midPos �м���ʱ�����
	* \param endPos ���������
	* \return �����������֮��ľ���
	*/
	int judge(const zPos &midPos, const zPos &endPos)
	{
		int distance = abs((long)(midPos.x - endPos.x)) + abs((long)(midPos.y - endPos.y));
		return distance;
	}

	/**
	* \brief ����·������
	* \param queueHead ·������ͷ
	* \param pPoint ��·���ڵ����ӵ�·����
	* \param currentCost ·���Ĺ������
	*/
	void enter_queue(zPathQueueHead &queueHead, zPathPoint *pPoint, int currentCost)
	{
		zPathQueue pNew(pPoint, currentCost);
		if (!queueHead.empty())
		{
			for (iterator it = queueHead.begin(); it != queueHead.end(); it++)
			{
				//���а�cost��С�����˳������
				if ((*it).cost > currentCost)
				{
					queueHead.insert(it, pNew);
					return;
				}
			}
		}
		queueHead.push_back(pNew);
	}

	/**
	* \brief ��·�������е����������
	* \param queueHead ·������ͷ
	* \return ���������·��
	*/
	zPathPoint *exit_queue(zPathQueueHead &queueHead)
	{
		zPathPoint *ret = NULL;
		if (!queueHead.empty())
		{
			reference ref = queueHead.front();
			ret = ref.node;
			queueHead.pop_front();
		}
		return ret;
	}

public:

	/**
	* \brief Ѱ·�������ж��м���Ƿ�ɴ�Ŀ�ĵ�
	*
	*  return (scene->zPosShortRange(tempPos,destPos,radius)
	*      && (!scene->checkBlock(tempPos) //Ŀ���ɴ����������Ŀ���
	*        || tempPos == destPos));
	*
	* \param tempPos Ѱ·���̵��м��
	* \param destPos Ŀ�ĵ�����
	* \param radius Ѱ·��Χ��������Χ����ΪĿ�ĵز��ɴ�
	* \return �����Ƿ�ɵ���Ŀ�ĵ�
	*/
	virtual bool moveable(const zPos &tempPos, const zPos &destPos, const int _radius = radius) = 0;
	/**
	* \brief �����ĳһ�������ƶ�
	* \param direct ����
	* \param step ��ʾ����
	* \return �ƶ��Ƿ�ɹ�
	*/
	virtual bool move(const int direct, const int _step = step) = 0;
	/**
	* \brief ʹ�����ĳһ�����ƶ�
	* ��Ѱ·�㷨���ƶ�
	* \param srcPos �������
	* \param destPos Ŀ�ĵ�����
	* \return �ƶ��Ƿ�ɹ�
	*/
	bool gotoFindPath(const zPos &srcPos, const zPos &destPos);
	/**
	* \brief Npc��ĳһ�����ƶ�
	* \param srcPos �������
	* \param destPos Ŀ�ĵ�����
	* \return �ƶ��Ƿ�ɹ�
	*/
	bool goTo(const zPos &srcPos, const zPos &destPos);
	/**
	* \brief Npc�����ĳһ�������ƶ�
	* \param direct �������
	* \return �ƶ��Ƿ�ɹ�
	*/
	bool shiftMove(const int direct);

};

template<int step, int radius>
bool zAStar<step, radius>::gotoFindPath(const zPos &srcPos, const zPos &destPos)
{
	//DisMap����destPosΪ���ĵı߳�Ϊ2 * radius + 1 ��������
	const int width = (2 * radius + 1);
	const int height = (2 * radius + 1);
	const int MaxNum = width * height;
	//������·�������ʼ��Ϊ���ֵ
	std::vector<int> pDisMap(MaxNum, MaxNum);
	std::vector<zPathPoint> stack(MaxNum * 8 + 1);//�ڶ�ջ�з����ڴ�
	zPathQueueHead queueHead;

	//�ӿ�ʼ������м���
	zPathPoint *root = &stack[MaxNum * 8];
	root->pos = srcPos;
	root->cc = 0;
	root->father = NULL;
	enter_queue(queueHead, root, root->cc + judge(root->pos, destPos));

	int Count = 0;
	//�������,ѭ������MaxNum�������
	while (Count < MaxNum)
	{
		root = exit_queue(queueHead);
		if (NULL == root)
		{
			//Ŀ��㲻�ɴ�
			return false;
		}

		if (root->pos == destPos)
		{
			//�ҵ�����Ŀ�ĵص�·��
			break;
		}

		const zAdjust adjust[8] =
		{
			{ 1 * step,0 * step },
			{ 0 * step,-1 * step },
			{ 0 * step,1 * step },
			{ -1 * step,0 * step },
			{ 1 * step,-1 * step },
			{ -1 * step,-1 * step },
			{ -1 * step,1 * step },
			{ 1 * step,1 * step }
		};
		for (int i = 0; i < 8; i++)
		{
			//�ֱ����Χ8�������м���·��
			bool bCanWalk = true;
			zPos tempPos = root->pos;
			tempPos += adjust[i];

			if (moveable(tempPos, destPos))
			{
				//��·�����л���
				zPathPoint *p = root;
				while (p)
				{
					if (p->pos == tempPos)
					{
						//����������Ѿ��ڻ���·���У�������ǰ��
						bCanWalk = false;
						break;
					}
					p = p->father;
				}

				//���·�����ݳɹ�����ʾ������ǿ����ߵ�
				if (bCanWalk)
				{
					int cost = root->cc + 1;
					int index = (tempPos.y - destPos.y + radius) * width + (tempPos.x - destPos.x + radius);
					if (index >= 0
						&& index < MaxNum
						&& cost < pDisMap[index])
					{
						//����·�����ϴμ����·����Ҫ�̣���Ҫ���뵽���·��������
						pDisMap[index] = cost;
						zPathPoint *pNewEntry = &stack[Count * 8 + i];
						pNewEntry->pos = tempPos;
						pNewEntry->cc = cost;
						pNewEntry->father = root;
						enter_queue(queueHead, pNewEntry, pNewEntry->cc + judge(pNewEntry->pos, destPos));
					}
				}
			}
		}

		Count++;
	}

	if (Count < MaxNum)
	{
		//����·����PointHead��,��ֻ��һ��
		while (root)
		{
			//�����ڶ����ڵ�
			if (root->father != NULL
				&& root->father->father == NULL)
			{
				return move(srcPos.getDirect(root->pos));
			}

			root = root->father;
		}
	}

	return false;
}

template<int step, int radius>
inline bool zAStar<step, radius>::goTo(const zPos &srcPos, const zPos &destPos)
{
	int direct = srcPos.getDirect(destPos);

	if (!move(direct)) {
		int r = randBetween(0, 1);
		int deep = 0;
		while (deep < 3) {
			switch (r) {
			case 0://˳ʱ��
				direct++;
				break;
			case 1://��ʱ��
				direct += 7;
				break;
			}
			direct %= 8;
			if (move(direct))
				return true;
			deep++;
		}
	}

	return false;
}

template<int step, int radius>
inline bool zAStar<step, radius>::shiftMove(const int direct)
{
	return move(direct);
}


/**
* \brief ����������
*/
typedef DWORD zPosI;

/**
* \brief �������ŵ�����
*
*/
typedef std::vector<zPosI> zPosIVector;
typedef std::vector<zPos> zPosVector;

typedef std::list<WORD> SceneEntryStateList;


//class zSceneEntryIndex;
/**
* \brief �������������������,NPC,������������Ʒ��
*
* ����������
*
* 1.����������
* 2.�����赲
*/
struct zSceneEntry :public zEntry
{
	//friend class zSceneEntryIndex;
public:

	unsigned short dupIndex;
	/**
	* \brief �������
	*/
	enum SceneEntryType
	{
		SceneEntry_Player,/**< ��ҽ�ɫ*/
		SceneEntry_NPC,  /**< NPC*/
		SceneEntry_Build,/**< ����*/
		SceneEntry_Object,/**< ������Ʒ*/
		SceneEntry_Pet,  /**< ����*/
		SceneEntry_MAX
	};
	/**
	* \brief ���״̬
	*
	*/
	enum SceneEntryState
	{
		SceneEntry_Normal,  /**< ��ͨ״̬ */
		SceneEntry_Death,  /**< ����״̬ */
		SceneEntry_Hide      /**< ����״̬ */
	};
	/**
	* \brief ����ת��
	* \param screenWH ��ͼ���͸�
	* \param pos Դ����
	* \param posi Ŀ������
	*/
	static void zPos2zPosI(const zPos &screenWH, const zPos &pos, zPosI &posi)
	{
		posi = ((screenWH.x + SCREEN_WIDTH - 1) / SCREEN_WIDTH) * (pos.y / SCREEN_HEIGHT) + (pos.x / SCREEN_WIDTH);
	}
protected:
	/**
	* \brief ���캯��
	*/
	zSceneEntry(SceneEntryType type, const SceneEntryState state = SceneEntry_Normal) :sceneentrytype(type), sceneentrystate(state)
	{
		bzero(byState, sizeof(byState));
		dir = _DIR_DOWN;
		inserted = false;
		dupIndex = 0;
	}

	/**
	* \brief ����
	*/
	zPos pos;
	/**
	* \brief ������
	*/
	zPosI posi;
	/**
	* \brief ����
	*
	*/
	BYTE dir;

	zPos lastPos1;
	zPos lastPos2;

private:
	/**
	* \brief �������
	*/
	const SceneEntryType sceneentrytype;
	/**
	* \brief ���״̬
	*
	*/
	SceneEntryState sceneentrystate;
	/**
	* \brief ����Ƿ��ڳ�����
	*/
	bool inserted;

	/**
	* \brief �����������
	* \param screenWH �����Ŀ���
	* \param newPos �����������
	* \return ���곬���������߷���false,���򷵻�true
	*/
	bool setPos(const zPos &screenWH, const zPos &newPos)
	{
		if (screenWH.x>newPos.x && screenWH.y>newPos.y)
		{
			pos = newPos;
			zPos2zPosI(screenWH, newPos, posi);
			return true;
		}
		else
			return false;
	}

private:
	/**
	* \brief ���״̬����ħ������ص�
	* ����״̬����ۿ��Ա��ֵģ�����ĳ��״̬�ͻ��˾Ϳ�����һ�ַ�ʽ������
	* ��ϸ��״̬�μ�Command.h��
	*/
	BYTE byState[(Cmd::MAX_STATE + 7) / 8];
protected:
	SceneEntryStateList stateList;
public:
	/**
	* \brief ������״̬
	* \param state ���λ��
	* \return ״̬����
	*/
	inline BYTE full_UState(WORD *state)
	{
		BYTE ret = stateList.size();
		SceneEntryStateList::iterator iter = stateList.begin();
		for (int i = 0; i < ret; i++)
		{
			state[i] = *iter;
			iter++;
		}
		return ret;
	}
	/**
	* \brief ����������״̬
	* \param state ���λ��
	*/
	inline void full_all_UState(void *state, DWORD maxSize)
	{
		bcopy(byState, state, sizeof(byState), maxSize);
	}

	/**
	* \brief �õ��������
	* \return �������
	*/
	inline const zPos &getPos() const
	{
		return pos;
	}

	/**
	* \brief �õ�����ղŵ�����
	* \return �������
	*/
	inline const zPos &getOldPos1() const
	{
		return lastPos1;
	}

	/**
	* \brief �õ�����ղŵ�����
	* \return �������
	*/
	inline const zPos &getOldPos2() const
	{
		return lastPos2;
	}

	/**
	* \brief �õ����������
	* \return ���������
	*/
	inline const zPosI &getPosI() const
	{
		return posi;
	}
	/**
	* \brief ��������Ƿ��ڳ�����
	* \return ����ڳ����з���true,���򷵻�false
	*/
	inline bool hasInScene() const
	{
		return inserted;
	}

	/**
	* \brief �õ��������
	* \return �������
	*/
	inline const SceneEntryType & getType() const
	{
		return sceneentrytype;
	}

	/**
	* \brief ��ȡ���״̬
	* \return ״̬
	*/
	inline const SceneEntryState & getState() const
	{
		return sceneentrystate;
	}

	/**
	* \brief �������״̬
	* \param state ��Ҫ���õ�״̬
	*/
	void setState(const SceneEntryState & state)
	{
		sceneentrystate = state;
	}

	/**
	* \brief ��ȡ����
	* \return ����
	*/
	inline const BYTE getDir() const
	{
		return dir % 8;
	}

	/**
	* \brief ���÷���
	* \param dir ����
	*/
	void setDir(const BYTE dir)
	{
		this->dir = dir % 8;
	}

	/**
	* \brief ���ĳ��״̬�Ƿ�����
	* \param state ������״̬
	* \return ����״̬�Ƿ��Ѿ�����
	*/
	inline bool issetUState(const int state) const
	{
		return Cmd::isset_state(byState, state);
	}

	/**
	* \brief ����ĳ��״̬
	* \param state �����õ�״̬
	* \return ����Ѿ����ø�״̬����false,���򷵻�true
	*/
	inline bool setUState(const int state)
	{
		if (!issetUState(state))
		{
			stateList.push_back(state);
			Cmd::set_state(byState, state);
			return true;
		}
		return false;
	}

	/**
	* \brief ���ĳ��״̬
	* \param state �������״̬
	* \return ����Ѿ����ø�״̬����true,���򷵻�false
	*/
	inline bool clearUState(const int state)
	{
		Cmd::clear_state(byState, state);
		SceneEntryStateList::iterator iter = stateList.begin();
		for (; iter != stateList.end(); ++iter)
		{
			if (*iter == state)
			{
				stateList.erase(iter);
				return true;
			}
		}
		return false;
	}
};

/**
* \brief ��������������
*/
enum enumSceneRunningState {
	SCENE_RUNNINGSTATE_NORMAL,//��������
	SCENE_RUNNINGSTATE_UNLOAD,//����ж��
	SCENE_RUNNINGSTATE_REMOVE,//����ж��
};
/**
* \brief ����������Ϣ����
*/
struct zScene :public zEntry
{
private:
	DWORD running_state;
public:
	zScene() :running_state(SCENE_RUNNINGSTATE_NORMAL) {}
	DWORD getRunningState() const
	{
		return running_state;
	}
	DWORD setRunningState(DWORD set)
	{
		running_state = set;
		return running_state;
	}
};

/**
* \brief ����������
*
* �����ֺ���ʱID����,û��ID��������Ϊ���������ظ�
*/
class zSceneManager :public zEntryMgr<zEntryID, zEntryTempID, zEntryName >
{
protected:
	/**
	* \brief ���ʹ������Ļ�����
	*/
	zRWLock rwlock;

	zScene * getSceneByName(const char * name)
	{
		rwlock.rdlock();
		zScene *ret = (zScene *)getEntryByName(name);
		rwlock.unlock();
		return ret;
	}



	zScene * getSceneByID(DWORD id)
	{
		rwlock.rdlock();
		zScene *ret = (zScene *)getEntryByID(id);
		rwlock.unlock();
		return ret;
	}

	zScene * getSceneByTempID(DWORD tempid)
	{
		rwlock.rdlock();
		zScene *ret = (zScene *)getEntryByTempID(tempid);
		rwlock.unlock();
		return ret;
	}

	template <class YourSceneEntry>
	bool execEveryScene(execEntry<YourSceneEntry> &exec)
	{
		rwlock.rdlock();
		bool ret = execEveryEntry<>(exec);
		rwlock.unlock();
		return ret;
	}

	/**
	* \brief �Ƴ����������Ľ�ɫ
	* \param pred ��������
	*/
	template <class YourSceneEntry>
	void removeScene_if(removeEntry_Pred<YourSceneEntry> &pred)
	{
		rwlock.wrlock();
		removeEntry_if<>(pred);
		rwlock.unlock();
	}

public:
	/**
	* \brief ���캯��
	*/
	zSceneManager()
	{
	}

	/**
	* \brief ��������
	*/
	virtual ~zSceneManager()
	{
		clear();
	}

};



/**
* \brief ��ɫ����������
*/
/**
* \brief ��ɫ������,�д�����
*/
struct zUser :public zSceneEntry
{
	zUser() :zSceneEntry(SceneEntry_Player)
	{
	}
	void lock()
	{
		//Zebra::logger->debug("lockuser");
		mlock.lock();
	}

	void unlock()
	{
		//Zebra::logger->debug("unlockuser");
		mlock.unlock();
	}

private:
	zMutex mlock;
};


/**
* \brief ��ɫ������
* uid,sessionid,name
* ʵ����ID����ʱID�����ֵ�����,������Щֵ�����ظ�
*/
class zUserManager :public zEntryMgr< zEntryID, zEntryTempID, zEntryName >
{
protected:
	/**
	* \brief ���������ʻ�����
	*/
	zRWLock rwlock;

public:
	/**
	* \brief ���캯��
	*/
	zUserManager()
	{
	}

	/**
	* \brief ��������
	*/
	virtual ~zUserManager()
	{
		clear();
	}

	/**
	* \brief ���ݽ�ɫ���ֵõ���ɫ
	* \param name ��ɫ����
	* \return ��ɫָ��,�������NULL��ʾû�ҵ���ɫ
	*/
	zUser * getUserByName(const char * name)
	{
		rwlock.rdlock();
		zUser *ret = (zUser *)getEntryByName(name);
		rwlock.unlock();
		return ret;
	}

	/**
	* \brief ���ݽ�ɫID�õ���ɫ
	* \param id ��ɫID
	* \return ��ɫָ��,�������NULL��ʾû�ҵ���ɫ
	*/
	zUser * getUserByID(QWORD id)
	{
		rwlock.rdlock();
		zUser *ret = (zUser *)getEntryByID(id);
		rwlock.unlock();
		return ret;
	}

	/**
	* \brief ���ݽ�ɫ��ʱID�õ���ɫ
	* \param tempid ��ɫ��ʱID
	* \return ��ɫָ��,�������NULL��ʾû�ҵ���ɫ
	*/
	zUser * getUserBySessID(QWORD sessid)
	{
		rwlock.rdlock();
		zUser *ret = (zUser *)getEntryByTempID(sessid);
		rwlock.unlock();
		return ret;
	}

	/**
	* \brief ���ӽ�ɫ
	* \param user ��ɫ
	* \return �����Ƿ�ɹ�
	*/
	bool addUser(zSceneEntry *user)
	{
		rwlock.wrlock();
		bool ret = addEntry((zEntry *)user);
		rwlock.unlock();
		return ret;
	}

	/**
	* \brief �Ƴ���ɫ
	* \param user ��ɫ
	*/
	void removeUser(zSceneEntry *user)
	{
		rwlock.wrlock();
		removeEntry((zEntry *)user);
		rwlock.unlock();
	}

	/**
	* \brief �Ƴ����������Ľ�ɫ
	* \param pred ��������
	*/
	template <class YourUserEntry>
	void removeUser_if(removeEntry_Pred<YourUserEntry> &pred)
	{
		rwlock.wrlock();
		removeEntry_if<>(pred);
		rwlock.unlock();
	}

	/**
	* \brief ��ÿ���û�ִ��
	* \param exec ִ�нӿ�
	*/
	template <class YourUserEntry>
	bool execEveryUser(execEntry<YourUserEntry> &exec)
	{
		rwlock.rdlock();
		bool ret = execEveryEntry<>(exec);
		rwlock.unlock();
		return ret;
	}
};

class zLock
{
public:

	/**
	* \brief ���캯��������һ���ٽ�������
	*
	*/
	zLock()
	{
		//InitializeCriticalSection(&m_critical);
	}

	/**
	* \brief ��������������һ���ٽ�������
	*
	*/
	~zLock()
	{
		//DeleteCriticalSection(&m_critical);
	}

	/**
	* \brief ����һ���ٽ���
	*
	*/
	inline void lock()
	{
		//EnterCriticalSection(&m_critical);
	}

	/**
	* \brief ����һ���ٽ���
	*
	*/
	inline void unlock()
	{
		//LeaveCriticalSection(&m_critical);
	}

private:

	//CRITICAL_SECTION    m_critical; // ϵͳ�ٽ���
};


/**
* \brief Zebra��Ŀ����־�ࡣ
*
* Ŀǰʵ��������д��־��ʽ��������̨�������ļ���
*
* Ĭ����־������#DEBUG
*
* ����Ϊ�̰߳�ȫ�ࡣ
*/
class zLogger
{
public:
	/**
	* \brief zLevel�����˼�����־�ȼ�
	*
	* ������log4cxx�ṩ�ı�׼��־�ȼ���Ϊ��־�ȼ��⣬���Զ�������Ϸ��־�ȼ�.
	*
	* ������־�ȼ���ϵΪ #OFF> #FATAL> #ERROR> #WARN> #INFO> #DEBUG> #ALL
	*
	*/

	/**
	* \brief Zebra��Ŀ��֧����־�ȼ����ֶ���
	*/
	typedef enum
	{
		/**
		* \brief ��zLogger�ȼ�����ΪOFF�����򲻻�����κ���־
		*/
		LEVEL_OFF = INT_MAX,

		/**
		* \brief ��zLogger�ȼ�����ΪFATAL��ֻ���FATAL�ȼ�����־
		*
		* �������������Ѿ��޷��ṩ�����ķ����ܡ�
		*/
		LEVEL_FATAL = 50000,

		/**
		* \brief ��zLogger�ȼ�����ΪERROR��ֻ������ڵ��ڴ˵ȼ�����־
		*
		* ���󣬿��ܲ����ṩĳ�ַ��񣬵����Ա�֤������ȷ���С�
		*/
		LEVEL_ERROR = 40000,

		/**
		* \brief ��zLogger�ȼ�����ΪWARN��ֻ������ڵ��ڴ˵ȼ�����־
		*
		* ���棬ĳЩ�ط���Ҫ����ע�⣬����û�������ļ�����������Ĭ��ѡ�����ʹ�á�
		*/
		LEVEL_WARN = 30000,

		/**
		* \brief ��zLogger�ȼ�����ΪINFO��ֻ������ڵ��ڴ˵ȼ�����־
		*
		* ��Ϣ���ṩһ����Ϣ��¼��������һЩ����״̬�ļ�¼��
		*/
		LEVEL_INFO = 20000,

		/**
		* \brief ��zLogger�ȼ�����ΪDEBUG��������еȼ�����־
		*/
		LEVEL_DEBUG = 10000,

		/**
		* \brief ��zLogger�ȼ�����ΪALL��������еȼ�����־
		*/
		LEVEL_ALL = INT_MIN
	}zLevel;

	zLogger(char *name = "Zebra");
	~zLogger();

	const std::string& getName();
	void removeConsoleLog();
	void addLocalFileLog(const std::string &file);

	void setLevel(const zLevel level);
	void setLevel(const std::string &level);

	void logtext(const zLevel level, const char * text);
	void logva(const zLevel level, const char * pattern, va_list vp);
	void log(const zLevel level, const char * pattern, ...);

	void debug(const char * pattern, ...);
	void debug16(const char* info, const BYTE* pData, int Datasize);
	void error(const char * pattern, ...);
	void info(const char * pattern, ...);
	void fatal(const char * pattern, ...);
	void warn(const char * pattern, ...);

private:
	zLock msgMut;

	zLevel      m_level;
	FILE        *fp_console;
	FILE        *fp_file;
	int         m_day;
	std::string m_name;
	std::string m_file;
};


/**
* \brief ���Թ������������������Թؼ��ֺ�ֵ��ʹ���ַ����������ؼ��ֲ����ִ�Сд
*
*/
class zProperties
{

public:

	/**
	* \brief ��ȡһ������ֵ
	*
	* \param key �ؼ���
	* \return ������ؼ��ֶ�Ӧ������ֵ
	*/
	const std::string &getProperty(const std::string &key)
	{
		return properties[key];
	}

	/**
	* \brief ����һ������
	*
	* \param key �ؼ���
	* \param value �ؼ��ֶ�Ӧ������
	*/
	void setProperty(const std::string &key, const std::string &value)
	{
		properties[key] = value;
	}

	/**
	* \brief ���ز�������������ؼ��ֶ�Ӧ������ֵ
	*
	* \param key �ؼ���
	* \return ����ֵ
	*/
	std::string & operator[] (const std::string &key)
	{
		//fprintf(stderr,"properties operator[%s]\n",key.c_str());
		return properties[key];
	}

	/**
	* \brief ����洢����������ֵ
	*
	*/
	void dump(std::ostream &out)
	{
		property_hashtype::const_iterator it;
		for (it = properties.begin(); it != properties.end(); it++)
			out << it->first << " = " << it->second << std::endl;
	}

	DWORD parseCmdLine(const std::string &cmdLine);
	DWORD parseCmdLine(const char *cmdLine);

protected:

	/**
	* \brief hash����
	*
	*/
	/*struct key_hash : public std::unary_function<const std::string,size_t>
	{
	size_t operator()(const std::string &x) const
	{
	std::string s = x;
	hash<string> H;
	// _Hash<string> H;
	//ת���ַ���ΪСд
	to_lower(s);
	//tolower(s);
	//return H(s);
	return 0;
	}
	};*/
	/**
	* \brief �ж������ַ����Ƿ����
	*
	*/
	/*struct key_equal : public std::binary_function<const std::string,const std::string,bool>
	{
	bool operator()(const std::string &s1,const std::string &s2) const
	{
	// return strcasecmp(s1.c_str(),s2.c_str()) == 0;
	return (s1==s2);
	}
	};*/

	/**
	* \brief �ַ�����hash_map
	*
	*/
	//typedef hash_map<std::string,std::string,key_hash,key_equal> property_hashtype;

	//typedef hash_map<std::string,std::string,key_hash,key_equal> property_hashtype;
	typedef UNORDERED_MAP<std::string, std::string> property_hashtype;

	property_hashtype properties;      /**< �������Եļ�ֵ�� */

};


/**
* \brief �����ļ�����������
*/
/**
* \brief zXMLParser����
*
* ��Ҫ�ṩ�˽ڵ�����,�������Եĵõ�.
*/
class zXMLParser
{
public:
	zXMLParser();
	~zXMLParser();

	bool initFile(const std::string &xmlFile);
	bool initFile(const char *xmlFile);
	bool initStr(const std::string &xmlStr);
	bool initStr(const char *xmlStr);
	bool init();
	void final();
	std::string & dump(std::string & s, bool format = false);
	std::string & dump(xmlNodePtr dumpNode, std::string & s, bool head = true);
	xmlNodePtr getRootNode(const char *rootName);
	xmlNodePtr getChildNode(const xmlNodePtr parent, const char *childName);
	xmlNodePtr getNextNode(const xmlNodePtr node, const char *nextName);
	DWORD getChildNodeNum(const xmlNodePtr parent, const char *childName);

	xmlNodePtr newRootNode(const char *rootName);
	xmlNodePtr newChildNode(const xmlNodePtr parent, const char *childName, const char *content);
	bool newNodeProp(const xmlNodePtr node, const char *propName, const char *prop);

	bool getNodePropNum(const xmlNodePtr node, const char *propName, void *prop, int propSize);
	bool getNodePropStr(const xmlNodePtr node, const char *propName, void *prop, int propSize);
	bool getNodePropStr(const xmlNodePtr node, const char *propName, std::string &prop);
	bool getNodeContentNum(const xmlNodePtr node, void *content, int contentSize);
	bool getNodeContentStr(const xmlNodePtr node, void *content, int contentSize);
	bool getNodeContentStr(const xmlNodePtr node, std::string &content);
	bool getNodeContentStr(const xmlNodePtr node, std::string &content, bool head);
private:
	BYTE *charConv(BYTE *in, const char *fromEncoding, const char *toEncoding);
	xmlDocPtr doc;
};

using namespace boost::gregorian;
using namespace boost::posix_time;

/*

ʱ�ӵĵδ����������룬daystart,weekstart,monthstart��
��ȷ���룬��Ҫ���ڵĶ�ʱ������update����������

*/

class zTimeTick
{
public:
	zTimeTick();
	~zTimeTick();

	void update();

	inline int32 now() const
	{
		return nowtime;
	}

	inline int64 getMilliTime()
	{
		return MilliTime();
	}

	inline int32 getDayStart() const
	{
		return daystart;
	}

	inline int32 getDayYmd() const
	{
		return dayymd;
	}

	inline int32 getWeekStart() const
	{
		return weekstart;
	}

	inline int32 getWeekYmd() const
	{
		return weekymd;
	}

private:

	//  ���ϵͳʱ��� 
	inline uint32 GetCurrentSeconds()
	{
		time_duration time_from_epoch = second_clock::universal_time() - epochStd;
		return time_from_epoch.total_seconds();
	}

	// ���ϵͳʱ���(΢��)
	inline uint64 GetCurrentMircoseconds()
	{
		time_duration time_from_epoch = microsec_clock::universal_time() - epochStd;
		return time_from_epoch.total_microseconds();
	}

	// ���ʱ������룩
	inline int32 NowTime()
	{
		time_duration time_from_epoch = second_clock::universal_time() - epochStd;
		return static_cast<int32>(time_from_epoch.total_seconds());
	}

	// ���ʱ��������룩
	inline int64 MilliTime()
	{
		time_duration time_from_epoch = microsec_clock::universal_time() - epochStd;
		return static_cast<int64>(time_from_epoch.total_milliseconds());
	}

	// ��ý���0��ʱ��
	inline int32 TodayStart()
	{
		ptime nowt = microsec_clock::universal_time();
		time_duration time_from_epoch = (nowt - epochStd);
		return time_from_epoch.total_seconds() - (nowt.time_of_day().total_seconds() + beijingDuration.total_seconds()) % 86400;
	}

	// ���ʱ�����΢�룩
	inline int64 MicroTime()
	{
		time_duration time_from_epoch = microsec_clock::universal_time() - epochStd;
		return static_cast<int64>(time_from_epoch.total_microseconds());
	}

	// ���������СʱΪ��Ԫ��ʱ�仺�� 
	void checkHourUnit();

private:

	/* 1970��1��1�� 01:00:00����׼ʱ����㣩 */
	ptime epochStd;

	/* 1970��1��1�� 08:00:00������ʱ����㣩�����ڼ���������֮������Ҫ�ô�ֵ����� */
	time_duration beijingDuration;

private:

	int32 nowtime;
	int32 daystart;
	int32 dayymd;
	int32 weekstart;
	int32 weekymd;

};

/*
*
*	Detail: ���ò������ģʽ����Ϊÿһ�ֵĶ�ʱ������һ����
*
*
*  Created by hzd 2015-6-4
*
*/


/*
*
*	Detail: �첽��ʱ��
*
* Copyright (c) Created by hzd 2015-6-4.All rights reserved
*
*/

typedef boost::function<void(const zTaskTimer* timer)> zTaskTimerHandler;

class zTaskTimer : public io_service
{
public:

	// Detail: �ص�ʱ��(����)���������ص�����
	zTaskTimer(int32 interval, int32 maxTimes, zTaskTimerHandler pHander);

	// 
	~zTaskTimer();

	// ��ʼ����
	void start();

	// ֹͣ�����һ�λص���
	void cancel();

	// ʱ���ж� 
	bool is1Sec() const;
	bool is2Sec() const;
	bool is3Sec() const;
	bool is5Sec() const;
	bool is1Min() const;
	bool is5Min() const;
	bool is1Hour() const;

	// ��ʱ���������ж�
	bool isNewHour() const;
	bool isNewDay() const;
	bool isNewWeek() const; //����Ϊһ�ܵĿ�ʼ 
	bool isNewMonth() const;
	bool isNewYear() const;

private:

	void doStart();
	void doHandler();

	void diyTimeCache();
	void checkCancel();

private:

	int32 m_Interval;	// ÿ�μ��ʱ��(����) 
	int32 m_MaxTimes;   // �����ô���(0���޴Σ�1��) 
	int32 m_HadTimes;   // �Ѿ����ô��� 
	int64 m_execTime;	// �Ѿ�ִ�е�ʱ�� 

	zTaskTimerHandler	m_fHandler;  // �ص����� 
	deadline_timer		m_ctimer;

	boost::thread  m_cServiceThread;		// ��ʱ���߳�  

	bool b1Sec;
	bool b2Sec;
	bool b3Sec;
	bool b5Sec;
	bool b1Min;
	bool b5Min;
	bool b1Hour;

	bool bNewHour;
	bool bNewDay;
	bool bNewWeek;
	bool bNewMonth;
	bool bNewYear;

};


/**
* \brief �����˷������Ŀ�ܻ���
*
* ���з���������ʵ����Ҫ�̳�����࣬���Ҳ��������ж��ٸ����࣬�������л���ֻ��һ�����ʵ��<br>
* ֻҪ������ʹ��Singleton���ģʽʵ�־Ϳ�����
*
*/
class zService : private zNoncopyable
{
public:
	/**
	* \brief ����������
	*
	*/
	virtual ~zService() { inst = NULL; };

	/**
	* \brief ���¶�ȡ�����ļ���ΪHUP�źŵĴ�������
	*
	* ȱʡʲô���鶼���ɣ�ֻ�Ǽ����һ��������Ϣ�����������������ɵ�����
	*
	*/
	virtual void reloadConfig()
	{
	}

	/**
	* \brief �ж���ѭ���Ƿ����
	*
	* �������true�����������ص�
	*
	* \return ��ѭ���Ƿ����
	*/
	bool isTerminate() const
	{
		return terminate;
	}

	/**
	* \brief ������ѭ����Ҳ���ǽ������ص�����
	*
	*/
	void Terminate()
	{
		terminate = true;
	}

	void main();

	/**
	* \brief ���ط����ʵ��ָ��
	*
	* \return �����ʵ��ָ��
	*/
	static zService *getInst()
	{
		return inst;
	}


	virtual bool init() = 0;

	/**
	* \brief ȷ�Ϸ�������ʼ���ɹ��������������ص�����
	*
	* \return ȷ���Ƿ�ɹ�
	*/
	virtual bool validate()
	{
		return true;
	}

	/**
	* \brief �����������ص���������Ҫ���ڼ�������˿ڣ��������false���������򣬷���true����ִ�з���
	*
	* \return �ص��Ƿ�ɹ�
	*/
	virtual bool run() = 0;

	/**
	* \brief �������������򣬻�����Դ�����麯����������Ҫʵ���������
	*
	*/
	virtual void finaly() = 0;

public:

	static zService *inst;	/**< ���Ψһʵ��ָ�룬���������࣬��ʼ��Ϊ��ָ�� */

	zProperties env;        /**< �洢��ǰ����ϵͳ�Ļ������� */
	std::string name;       /**< �������� */
	bool terminate;         /**< ���������� */

protected:

	/**
	* \brief ���캯��
	*/
	zService(const std::string &_name) : name(_name)
	{
		inst = this;
		terminate = false;
	}
};

// ��Ϸ��
struct GameZone_t
{
	int32 gameID;
	int32 zoneID;
};

// ͨ����
struct GeneralZone_t
{
	int8 type;
	int8 actID;
	int32 flatID;
	GameZone_t zone;
};

/*
 * ·��ͷ��Ϣ������zone,ȥ��zone
 * ģ����̫������Ϣ,���ṹΪ[fromZone,toZone,prootol][GOTOdata/SENDTOdata]
 * GOTOΪ������Э�飬gotoServerZone���ڵ���
 * SENDTOΪת�����Э��,sendToServerZone���ڵ���
 */
struct GeneralRoute_t
{
	GeneralZone_t fromZone;
	GeneralZone_t toZone;
	BYTE formType;	//������:0Դ������,1���
	QWORD formID;	//��ԴID
	BYTE toType;	//��������:0������,1���
	QWORD toID;		//����ID
	BYTE routeCount;//ת������
	BYTE dirType;	//ָ������0��ǰ,1����
	BYTE errType;	//0��,1ʧ�ܷ���,2�������
	bool needFailBack()
	{
		return errType & 0x01;
	}
	bool needUppack()
	{
		return errType & 0x02;
	}
};


/*
 * �����������ܻ���
 *
 */
class zNetSerivce : public zService
{
public:

	virtual ~zNetSerivce() { };

	// ����Ϣ����ĳ��
	void sendToServerZone(GeneralRoute_t route, const NetMsgSS* cmd,int32 len); // SЭ��
	void sendToServerZoneUser(GeneralRoute_t route,int64 toUserID,const NetMsgSS* cmd, int32 len); //cЭ��

	// ���Լ�ת�Ƶ�ĳ����¼(��������,ĳ������)
	void gotoServerZone(GeneralRoute_t route);

protected:

	zNetSerivce(const std::string &_name) : zService(_name)
	{

	}

	void sendToServerZone(GeneralRoute_t route, std::pair<int32, int64> toPair, const NetMsgSS* cmd, int32 len);

private:

	// ƽ̨����
	int32 flatID;	//ƽ̨ID
	int32 flatZone;	//����ID

	// ������ ƽ̨id=>��ID(��id=>�Ϸ�ID)
	std::map<int32, int32> flatZonelist;
	std::map<int32, int32> gameZoneList;
};

/*
*	���ܡ�������
*/
class zEncrypt
{
	// Э��ӽ���ʹ�� 
public:
	// ��������Կ(Ĭ��32λ)
	static void getRandKey(char* outKey, int32 length = 32);

	// ���ӽ���
	static int32 xorCode(uint32 nRandNum, const char* inKey, char *pSrc, uint32 nSrcLen);

	// ��[x,y]ȡ�������
	static int32 randBetween(int x,int y);

};

/*

����C++�汾MD5�������кܶ�C++�汾��MD5������ͬ�İ汾�㷨���в�ͬ
��C#,Java,Php������ͨ��һ�ף����Ըð汾ʵ��Ҳ������һ��

ʹ�÷���
unsigned char a[] = "a";
char md5str[50] = { 0 };
EncryptMD5str(md5str, a, 1);
printf("MD5 String is %s\n", md5str);

*/

#ifdef __cplusplus
extern "C" {
#endif
	/*������������������������������������������������.
	| md5 encrypt function
	.������������������������������������������������*/
	void EncryptMD5(unsigned char *output, unsigned char *input);
	void EncryptMD5str(char *output, unsigned char *input, int len);

#ifdef __cplusplus
}
#endif

class zUUID
{

	//twitter snowflake �㷨
	//64     63--------------22-----------12------0
	//����λ |    41λʱ��   | 10λ������| 12λ������|
public:
	zUUID();
	~zUUID();
	uint64_t get_time();

	void set_epoch(uint64_t epoch);
	void set_machine(int32_t machine);

	int64_t generate();
private:
	uint64_t epoch_;
	uint64_t time_;
	uint64_t machine_;
	int32_t sequence_;

};


/*
 * ���ӹ����� 
 */ 

CreateEntryC(serivceid);
CreateEntry(serivceid);
CreateEntryTempID(serivceid);
CreateEntryID(serivceid);
CreateEntryNone(serivceid);
CreateEntryName(serivceid);
CreateEntryManager(serivceid);

CreateEntryC(serverid);
CreateEntry(serverid);
CreateEntryTempID(serverid);
CreateEntryID(serverid);
CreateEntryNone(serverid);
CreateEntryName(serverid);
CreateEntryManager(serverid);

CreateEntryC(sessid);
CreateEntry(sessid);
CreateEntryTempID(sessid);
CreateEntryID(sessid);
CreateEntryNone(sessid);
CreateEntryName(sessid);
CreateEntryManager(sessid);

CreateEntryC(mapid);
CreateEntry(mapid);
CreateEntryTempID(mapid);
CreateEntryID(mapid);
CreateEntryNone(mapid);
CreateEntryName(mapid);
CreateEntryManager(mapid);

CreateEntryC(uid);
CreateEntry(uid);
CreateEntryTempID(uid);
CreateEntryID(uid);
CreateEntryNone(uid);
CreateEntryName(uid);
CreateEntryManager(uid);


// ����ȡ��serversession��clientsession
#define EncryptDef "1234567890ABCDEFGHIJKLMNOPQSRT\0"

// id = socketid,tempid = serivceid

class zSession : public zEntryserivceid
{
public:
	zSession(NetSocket* _socket):socket(_socket)
	{
		initEncrypt();
	}

	~zSession()
	{
		
	}

	void sendMsg(const void* msg, int32 size)
	{
		if (socket)
		{
			socket->ParkMsg((const NetMsgSS*)msg, size);
			socket->SendMsg();
		}
	}

	void exist()
	{
		if (socket)
		{
			socket->OnEventColse();
		}
	}

	enum ServerType
	{
		SERVER_TYPE_NULL,
		SERVER_TYPE_SERVER,
		SERVER_TYPE_CLIENT
	};

	enum SessionType
	{
		FOR_NULL,
		FOR_FEP,
		FOR_LOGIN,
		FOR_WORLD,
		FOR_SCENE,
		FOR_DP,
		FOR_PLAYER,
	};

	void setServerType(ServerType _serverType)
	{
		serverType = _serverType;
	}

	void setSessionType(SessionType _sessionType)
	{
		sessionType = _sessionType;
	}

	void setServerID(int32 _serverID)
	{
		serverid = _serverID;
	}

	void initEncrypt()
	{
		memcpy(encrypt, EncryptDef, MAX_NAMESIZE);
	}

public:

	NetSocket* socket;
	ServerType serverType;// 1 ������,2�ͻ��� 
	SessionType sessionType;

	// ����������;�����ֶ� 
	int32 serverid;
	int32 status;
	int32 lastasktime;		// �ϴη���ʱ�� 
	int32 lastaskprotocol;	// �ϴη���Э��
	char encrypt[MAX_NAMESIZE + 1];

	
};

/*
*
*  Detail: �����ⲿ���������ӹ���
*
*/
class zSessionMgr : protected zEntryMgrserivceid<zEntryIDserivceid>
{
protected:
	bool getUniqeID(QWORD& tempid);
	void putUniqeID(const QWORD& tempid);

public:
	typedef PNetServerEnterHandler NetMsgEnter;
	typedef PNetServerOnMsgHandler NetMsgOn;
	typedef PNetServerExistHandler NetMsgExit;
	zSessionMgr();
	~zSessionMgr();

	bool bind(int32 id, const char* ip, int16 port,int16 maxConnects,NetMsgEnter fEnter, NetMsgOn fMsg, NetMsgExit fExit);
	zSession* connect(int32 id, const char* ip, int16 port, NetMsgEnter fEnter, NetMsgOn fMsg, NetMsgExit fExit);
	zSession* CreateObj(NetSocket* socket)
	{
		return sessionobjpool.CreateObj(socket);
	}

	void DestoryObj(zSession* obj)
	{
		sessionobjpool.DestroyObj(obj);
	}

	bool add(zSession* conn)
	{
		return zEntryMgrserivceid::addEntry(conn);
	}

	void remove(zSession* conn)
	{
		zEntryMgrserivceid::removeEntry(conn);
	}

	zSession* get(QWORD id)
	{
		return (zSession*)zEntryMgrserivceid::getEntryByID(id);
	}

	// ����ĳ��������Session 
	zSession* getByServerID(zSession::SessionType fun, int32 serverid);

	zSession* getWs(int32 serverid = 3001);
	zSession* getLs(int32 serverid = 2001);
	zSession* getFep(int32 serverid = 1001);
	zSession* getSs(int32 serverid = 4001);
	zSession* getDp(int32 serverid = 5001);
	bool sendMsg(int64 sessid, const NetMsgSS* pMsg, int32 nSize);
	bool sendToWs(const NetMsgSS* pMsg,int32 nSize);
	bool sendToDp(const NetMsgSS* pMsg, int32 nSize);
	bool sendToLs(const NetMsgSS* pMsg, int32 nSize);
	bool sendToSs(int32 serverid,const NetMsgSS* pMsg, int32 nSize);
	bool sendToFep(int32 serverid, const NetMsgSS* pMsg, int32 nSize);

	template <class YourSessionEntry>
	bool execEveryConn(execEntry<YourSessionEntry> &exec)
	{
		return execEveryEntry<>(exec);;
	}

	void updateIO(const zTaskTimer* timer);

	void updateServerIO(const zTaskTimer* timer);

	void updateClientIO(const zTaskTimer* timer);

	int32 CountConnected()
	{
		return zEntryMgrserivceid::size();
	}

private:

	zObjPool<NetServer>  serverobjpool;
	zObjPool<NetClient>  clientobjpool;
	zObjPool<zSession>	sessionobjpool;

	// �ȴ����뵽used�б��� 
	std::vector<NetServer*> serverListAdd;
	std::vector<NetClient*> clientListAdd;

	// ����ʹ���е��б�
	std::vector<NetServer*> serverListUsed;
	std::vector<NetClient*> clientListUsed;

};

enum EServerStatus
{
	E_SERVER_STATUS_INIT = 0,
	E_SERVER_STATUS_CONNETED,
	E_SERVER_STATUS_LOGINED,
	E_SERVER_STATUS_OK,
};

typedef boost::function<void(zSession*, const NetMsgSS*, int32 nSize)> HandleFunc;

// Э�����Ӧ�ĵ��ú��� 
struct MsgFunc
{
	MsgFunc(int32 _packSize, const HandleFunc& _handlerFun):packSize(_packSize),handlerFun(_handlerFun)
	{
	}
	MsgFunc(const MsgFunc& info)
	{
		operator=(info);
	}
	const MsgFunc& operator=(const MsgFunc& info)
	{
		memcpy(this, &info, sizeof(MsgFunc));
		return *this;
	}
	int32 packSize;		// ���ݴ�С 
	HandleFunc handlerFun;	// �������� 
};

typedef	map< uint32, MsgFunc > MsgFuncMap;

class zMsgHandler
{
public:
	virtual void OnNetMsgEnter(NetSocket& rSocket) = 0;
	virtual void OnNetMsg(NetSocket& rSocket, NetMsgSS* pMsg, int32 nSize) = 0;
	virtual void OnNetMsgExit(NetSocket& rSocket) = 0;

protected:
	zMsgHandler();
	~zMsgHandler();
	const MsgFunc* GetMsgHandler(uint32 protocol);
	bool RegisterMessage(uint32 msgIdx, int32 packSize, const HandleFunc& handlerFun);

private:

	MsgFuncMap msgFuncs;

};


/*
 * ������ע����Ϣ
 * id=>serverid,tempid=>sessid
 */
class zServerReg : public zEntrysessid
{
public:
	zServerReg()
	{

	}
public:
	// ������Ϣ���糡���� 
	// 
};


class zServerRegMgr : protected zEntryMgrsessid<zEntryIDsessid, zEntryTempIDsessid>
{
public:
	zServerRegMgr();
	~zServerRegMgr();
	bool getUniqeID(QWORD& tempid);
	void putUniqeID(const QWORD& tempid);
	zServerReg* CreateObj();
	void DestroyObj(zServerReg* obj);

	bool add(zServerReg* obj);
	void remove(zServerReg* obj);
	zServerReg* get(int64 id);
	zServerReg* getBySessID(int64 id);

	template <class YourSessionEntry>
	bool execEveryServer(execEntry<YourSessionEntry> &exec)
	{
		return execEveryEntry<>(exec);;
	}

private:

	zObjPool<zServerReg> objpool;

};


class zSerivceCfgMgr
{
public:

	struct Serivce
	{
		Serivce()
		{
			id = remoteid = port = maxConnects = broadcast = 0;
		}

		int32 id;
		std::string name;
		std::string fun;
		int32 remoteid;
		std::string ip;
		int16 port;
		int16 maxConnects;
		std::string user;
		std::string passwd;
		std::string value;
		int32 broadcast;

	};

	struct Server
	{
		Server()
		{
			id = recvsrvlist = broadcastlist = 0;
		}

		zSession::SessionType getSessType() const
		{
			if (stricmp(name.c_str(), "gatewayserver") == 0)
			{
				return zSession::FOR_FEP;
			}
			else if (stricmp(name.c_str(), "loginserver") == 0)
			{
				return zSession::FOR_LOGIN;
			}
			else if (stricmp(name.c_str(), "worldserver") == 0)
			{
				return zSession::FOR_WORLD;
			}
			else if (stricmp(name.c_str(), "sceneserver") == 0)
			{
				return zSession::FOR_SCENE;
			}
			else if (stricmp(name.c_str(), "dbserver") == 0)
			{
				return zSession::FOR_DP;
			}
			else
			{
				return zSession::FOR_NULL;
			}
		}

		int32 id;
		string name;
		int32 recvsrvlist;
		int32 broadcastlist;
		std::map<int32, Serivce> serivces;
	};

	zSerivceCfgMgr();
	~zSerivceCfgMgr();

	bool loadConfig(const std::string& filename);
	const std::map<int32, Server>& getServer() const;
	const Server* getServer(int32 serverid) const;
	const Serivce* getSerivce(int32 serverid, int32 serivceid) const;

private:

	std::map<int32, Server > allServers;

};


/*
 * RSA���ܽ���	
 *
 */
class zRSA
{
public:
	zRSA(const char* _pubfilename,const char* _prifilename,const char* _password);
	~zRSA();
	bool generateKey();
	std::string bio_read_privateKey(std::string data);
	std::string bio_read_publicKey(std::string data);
	void encryptFile(std::string inputfile, std::string outputfile);
	void decryptFile(std::string inputfile, std::string outputfile);
	int testEncodeUncode();

	const char* getPubfilename() const;
	const char* getPrifilename() const;
	const char* getPassword() const;

private:

	char pubfilename[MAX_NAMESIZE + 1];
	char prifilename[MAX_NAMESIZE + 1];
	char password[MAX_NAMESIZE + 1];

};

/*
 * ���ù�����	
 */
class zUtility
{
public:
	inline static int RangedRand(int nMin, int nMax)
	{
		return static_cast<int>(nMin + rand() * 1.f / (RAND_MAX + 1) * (nMax - nMin));
	}

	inline static int GBKToUTF8(const char * lpGBKStr, char * lpUTF8Str, int nUTF8StrLen)
	{
		wchar_t * lpUnicodeStr = NULL;
		int nRetLen = 0;

		if (!lpGBKStr)  // ���GBK�ַ���ΪNULL������˳�  
			return 0;

		nRetLen = ::MultiByteToWideChar(CP_ACP, 0, (char *)lpGBKStr, -1, NULL, NULL);  // ��ȡת����Unicode���������Ҫ���ַ��ռ䳤�� 
		lpUnicodeStr = new WCHAR[nRetLen + 1];  // ΪUnicode�ַ����ռ� 
		nRetLen = ::MultiByteToWideChar(CP_ACP, 0, (char *)lpGBKStr, -1, lpUnicodeStr, nRetLen);  // ת����Unicode���� 
		if (!nRetLen)  // ת��ʧ��������˳�
			return 0;

		nRetLen = ::WideCharToMultiByte(CP_UTF8, 0, lpUnicodeStr, -1, NULL, 0, NULL, NULL);  // ��ȡת����UTF8���������Ҫ���ַ��ռ䳤�� 

		if (!lpUTF8Str)  // ���������Ϊ���򷵻�ת������Ҫ�Ŀռ��С 
		{
			if (lpUnicodeStr)
				delete[]lpUnicodeStr;
			return nRetLen;
		}

		if (nUTF8StrLen < nRetLen)  // ���������������Ȳ������˳� 
		{
			if (lpUnicodeStr)
				delete[]lpUnicodeStr;
			return 0;
		}

		nRetLen = ::WideCharToMultiByte(CP_UTF8, 0, lpUnicodeStr, -1, (char *)lpUTF8Str, nUTF8StrLen, NULL, NULL);  // ת����UTF8���� 

		if (lpUnicodeStr)
			delete[]lpUnicodeStr;

		return nRetLen;
	}

	inline static char* Utf8ToGBK(const char* strUtf8)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, NULL, 0);
		unsigned short * wszGBK = new unsigned short[len + 1];
		memset(wszGBK, 0, len * 2 + 2);
		MultiByteToWideChar(CP_UTF8, 0, strUtf8, -1, (LPWSTR)wszGBK, len);
		len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wszGBK, -1, NULL, 0, NULL, NULL);
		char *szGBK = new char[len + 1];
		memset(szGBK, 0, len + 1);
		WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wszGBK, -1, (LPSTR)szGBK, len, NULL, NULL);
		return szGBK;
	}

	inline static int CalcGroupCount(int count, int max_pile_count)
	{
		if (max_pile_count == 0)
		{
			return 1;
		}
		int group_count = count / max_pile_count;
		if (count % max_pile_count != 0)
		{
			++group_count;
		}
		return group_count;
	}

	/*
	*	��Map��Valueֵ��ֵ
	*/
	template <typename MapType>
	inline void AddValueInMap(MapType& map_obj, typename MapType::key_type key, typename MapType::mapped_type value)
	{
		typename MapType::iterator iter = map_obj.find(key);
		if (iter != map_obj.end())
		{
			iter->second += value;
		}
		else
		{
			map_obj.insert(std::make_pair(key, value));
		}
	}

	/*
	*	���Map��Valueָ�룬���Key�����ڣ��򷵻�NULL
	*/
	template <typename MapType>
	inline typename MapType::mapped_type* GetValPtrFromMap(MapType& map_obj, typename MapType::key_type& key)
	{
		typename MapType::iterator iter = map_obj.find(key);
		return iter == map_obj.end() ? NULL : &iter->second;
	}

	/*
	* ��Map�в���һ��Key����������򷵻�false���ɹ��򷵻�true
	*/
	template <typename MapType>
	inline bool InsertToMap(MapType& map_obj, typename MapType::key_type& key, typename MapType::mapped_type& value)
	{
		typename MapType::iterator iter = map_obj.find(key);
		if (iter != map_obj.end())
			return false;
		map_obj.insert(std::make_pair(key, value));
		return true;
	}

	/*
	*   ֵ����һ��Ϊָ��
	*	���Map��Valueָ�룬���Key�����ڣ��򷵻�NULL
	*/
	template <typename MapType>
	inline typename MapType::mapped_type GetValFromMap(MapType& map_obj, typename MapType::key_type key)
	{
		typename MapType::iterator iter = map_obj.find(key);
		return iter == map_obj.end() ? NULL : iter->second;
	}

	/*
	*	����һ�������е�һ��ֵ��Ϊelem���±�
	*/
	template <typename T, unsigned int count>
	inline unsigned int FindFirstNotOf(const T(&arr)[count], const T& elem)
	{
		unsigned int index = 0;
		for (unsigned int i = 0; i < count; ++i)
		{
			if (elem != arr[i])
			{
				index = i;
				break;
			}
		}
		return index;
	}

	/*
	*	����һ�������е�һ��ֵΪelem���±�
	*/
	template <typename T, unsigned int count>
	inline unsigned int FindFirstOf(const T(&arr)[count], const T& elem)
	{
		unsigned int index = 0;
		for (unsigned int i = 0; i < count; ++i)
		{
			if (elem == arr[i])
			{
				index = i;
				break;
			}
		}
		return index;
	}


#ifdef TWP_SNPRINTF
# undef	TWP_SNPRINTF
#endif // TWP_SNPRINTF
#if defined(linux) || defined(__linux) || defined(__linux__)
#  define TWP_SNPRINTF snprintf
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#  define TWP_SNPRINTF(str, size, fmt, ...) _snprintf_s(str, size, (size) - 1, fmt, ##__VA_ARGS__)
#endif

	/**
	* \brief ���ַ�������tokenת��Ϊ����ַ���
	*
	* ������ʹ�����ӳ���
	*    <pre>
	*    std::list<string> ls;
	*    stringtok (ls," this  \t is\t\n  a test  ");
	*    for(std::list<string>const_iterator i = ls.begin(); i != ls.end(); ++i)
	*        std::cerr << ':' << (*i) << ":\n";
	*     </pre>
	*
	* \param container ���������ڴ���ַ���
	* \param in �����ַ���
	* \param delimiters �ָ�����
	* \param deep ��ȣ��ָ����ȣ�ȱʡû������
	*/
	template <typename Container>
	inline void my_stringtok(Container &container, std::string const &in,
		const char * const delimiters = " \t\n",
		const int deep = 0)
	{
		const std::string::size_type len = in.length();
		std::string::size_type i = 0;
		int count = 0;

		while (i < len)
		{
			i = in.find_first_not_of(delimiters, i);
			if (i == std::string::npos)
				return;   // nothing left

						  // find the end of the token
			std::string::size_type j = in.find_first_of(delimiters, i);

			count++;
			// push token
			if (j == std::string::npos
				|| (deep > 0 && count > deep)) {
				container.push_back(in.substr(i));
				return;
			}
			else
				container.push_back(in.substr(i, j - i));

			// set up for next loop
			i = j + 1;
		}
	}

	template<typename T1, typename T2, typename T3> struct my_streble
	{
		T1 first;
		T2 second;
		T3 third;
		my_streble() :first(), second(), third() {}
		my_streble(const T1& t1, const T2& t2, const T3& t3) :first(t1), second(t2), third(t3) {}
		template<typename U1, typename U2, typename U3> my_streble(const my_streble<U1, U2, U3>& _streble) : first(_streble.first), second(_streble.second), third(_streble.third) {}
	};
	template<typename T1, typename T2, typename T3> inline static my_streble<T1, T2, T3> make_my_streble(T1 x, T2 y, T3 z)
	{
		return my_streble<T1, T2, T3>(x, y, z);
	}

};


/*
 * ����ָ���ָ����������Դ�ļ�
 * �� ExcelתΪCVS��txt�ļ�
 */
class zTextCVS
{
public:
	zTextCVS(const std::string& strFileName, uint32 nFlagsRow = 4, uint32 nDataStartRow = 6);

	~zTextCVS();

	/*
	* ��ȡ��ǰ�к���ĵ�nLines��
	*/
	bool NextRow(int32 nLines = 1);

	/*
	* ������ݿ�
	*/
	bool GetInt8(int8& o_nValue);
	bool GetUInt8(uint8& o_nValue);
	bool GetInt16(int16& o_nValue);
	bool GetUInt16(uint16& o_nValue);
	bool GetInt32(int32& o_nValue);
	bool GetUInt32(uint32& o_nValue);
	bool GetInt64(int64& o_nValue);
	bool GetFloat32(float32& o_fValue);
	bool GetString(std::string& o_strValue);
	const char*	GetCString();

	// �������� 
	int32 UseColCount()
	{
		return m_nUseColCount;
	}

	// �Ƿ���� 
	bool Valid()
	{
		return m_bValid;
	}

private:
	/*
	* ����Դ�ļ�
	*/
	bool Load(const char* cstrFileName);

	/*
	* ��ʼ����־��
	*/
	bool ReadFlagsRow();

	/*
	* ����һ������
	*/
	void AnalyseLine(const std::string& i_strLine, std::vector<std::string>& o_vecCol);

private:
	int32						m_nFlagsRow;		// ��־��(1:��������ȡ,2:��ȡ,3:��ȡ) 
	int32						m_nDataStartRow;	// ������ʼ�� 
	uint32						m_nCurrRowIndex;	// ��ǰ��ȡ������[0,xx) 

	bool						m_bValid;			// �ļ��Ƿ���� 
	std::ifstream				m_cFileStream;		// �ļ������� 
	std::vector<bool>			m_vecFlags;			// �п���״̬ 

	std::string					m_strLine;			// ��ǰ��ȡ�� 
	std::vector<std::string>	m_vecCol;			// ��ǰ��ȡ�и���ֵ 
	uint32						m_nCol;				// ��ǰ��ȡ�������� 
	int32						m_nUseColCount;		// ���������� 
};



#endif


