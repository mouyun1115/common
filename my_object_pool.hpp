/*********************************************************** 
* Date: 2016-07-08 
* 
* Author: 牟韵 
* 
* Email: mouyun1115@163.com 
* 
* Module: 对象池 
* 
* Brief: 
* 
* Note: 
* 
* CodePage: Pure UTF-8 
************************************************************/ 
#ifndef	__MY_OBJECT_POOL_HPP_BY_MOUYUN_2014_10_27__
#define	__MY_OBJECT_POOL_HPP_BY_MOUYUN_2014_10_27__

#include <type_traits>
#include <list>
#include <mutex>

namespace my_module_space
{
    template<typename T>
    class ClearFunctionSensor
    {
    public:
        template<typename U, void (U::*)()> class Matcher;
        template<typename U> static char helper(Matcher<U, &U::clear>*);
        template<typename U> static int helper(...);
        enum { value = sizeof(helper<T>(nullptr)) == sizeof(char) };
    };

    template <bool> class ClearImpl;
    template<> class ClearImpl<true>
    {
    public:
        template<typename T> inline static void clear(T *p_t) {p_t->clear();}
    };
    template<> class ClearImpl<false>
    {
    public:
        template<typename T> inline static void clear(T *p_t) {}
    };

    template<typename T> inline void clear_object(T *p_t)
    {
        ClearImpl<ClearFunctionSensor<T>::value>::clear(p_t);
    }

    template <typename T>
    class ObjectPool
    {
    public:
        ObjectPool() : m_max_holding_count(8) {} /// < default max holding count is 8

        ~ObjectPool()
        {
            m_lock.lock();
            typename std::list<T*>::iterator itr = m_obj_list.begin();
            while (itr != m_obj_list.end())
            {
                delete *itr;
                ++itr;
            }
            m_obj_list.clear();
            m_lock.unlock();
        }

    private:
        ObjectPool(const ObjectPool&) = delete;
        ObjectPool(ObjectPool&&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;

    private:
        std::list<T*> m_obj_list;
        std::mutex m_lock;
        int m_max_holding_count; /// < when push(), delete or hold, determined by this var

    public:
        T* pop()
        {
            T *p_obj = nullptr;

            std::lock_guard<std::mutex> lg(m_lock);

            if (m_obj_list.empty())
            {
                try {p_obj = new T();} catch(...){} /// < pool is empty, new object
            }
            else
            {
                p_obj = m_obj_list.front();
                m_obj_list.pop_front();
            }

            return p_obj;
        }

        void push(T *p_obj)
        {
            if (p_obj == nullptr)
            {
                return;
            }

            std::lock_guard<std::mutex> lg(m_lock);

            if (static_cast<int>(m_obj_list.size()) < m_max_holding_count)
            {
                clear_object(p_obj); /// < using the fucking template meta programming
                m_obj_list.push_back(p_obj);
            }
            else
            {
                delete p_obj; /// < pool is full, delete object
            }
        }

        inline int get_max_holding_count() const
        {
            return m_max_holding_count;
        }

        void set_max_holding_count(const int count)
        {
            std::lock_guard<std::mutex> lg(m_lock);

            m_max_holding_count = count;

            int current_size = static_cast<int>(m_obj_list.size()); 
            while (current_size-- > m_max_holding_count)
            {
                delete m_obj_list.front();
                m_obj_list.pop_front();
            }
        }
    };


    /// < 请在CPP文件中定义此宏 
#define DEFINE_OBJECT_POOL(OBJECT_TYPE,POOL_NAME) class POOL_NAME {\
private:static my_module_space::ObjectPool<OBJECT_TYPE> m_pool;\
public:static OBJECT_TYPE* pop() { return m_pool.pop(); }\
public:static void push(OBJECT_TYPE *p_obj) { m_pool.push(p_obj); }\
public:static std::shared_ptr<OBJECT_TYPE> pop_sp() { return std::shared_ptr<OBJECT_TYPE>(pop(), push);}\
public:static int get_max_holding_count() { return m_pool.get_max_holding_count(); }\
public:static void set_max_holding_count(const int _count) { m_pool.set_max_holding_count(_count);}};\
my_module_space::ObjectPool<OBJECT_TYPE> POOL_NAME::m_pool;

}

#endif
