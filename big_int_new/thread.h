/*
 *  thread.h
 *  big_int
 *
 *  Created by Chris on 7/11/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#include <boost/shared_ptr.hpp>
#include <memory>
#ifdef USE_BOOST_THREADS__
#include <boost/thread/thread.hpp>
#include <boost/ref.hpp>

class bi_thread {
private:
	boost::thread impl;
public:
	// template constructor
	template<class Callable>
	bi_thread(Callable &T) : impl(boost::ref(T)) {}
	void join() { impl.join(); }
	void detach() { impl.detach(); }
	void interrupt() { impl.interrupt(); }
	// ~bi_thread() // = default;
};

// basic mutex class which encapsulates the pthread mutex
typedef boost::mutex bi_thread_mutex;

// RAII locking class
typedef boost::mutex::scoped_lock bi_mutex_lock; 
#else

/* if Boost is not available, define a wrapper class for POSIX threads.
	Because we do not have need for most of the more sophisticated thread functions
	(interruptibility and so forth)
*/

#include <pthread.h>
#include <typeinfo>
#include "big_int.h"

namespace {
	// dummy function to adapt our interface to that of Pthreads
	template<class Callable> // any callable thing
	void *f00(void *ptrargs) // ha ha, the classic function name
	{
		// invoke its operator() with no args (this also allows functions of type void (*)())
		// note that we must explicitly dereference it, if it is a function object
		
		try {
			(*reinterpret_cast<Callable *>(ptrargs))();
		}
		// inter-thread exception handling
		catch (big_int::exception &e) {
			return e.clone();
		}
		catch (std::exception &) {
		
		}
		return NULL;
	}
};

class bi_thread {
	pthread_t the_thread;
	bool joinable;
	bi_thread(const bi_thread &);//disallow copying
	bi_thread& operator=(const bi_thread &);
public:
	// template constructor:  initialize with anything that has operator() (including genuine functions)
	template<class Callable> 
	bi_thread(Callable &T) : the_thread(), joinable(true)
	// pass the address of the func object to pthread_create; it gets used as the arguments to the dummy function
	{ pthread_create(&the_thread, NULL, f00<Callable>, reinterpret_cast<void *>(&T)); }
	void detach() { joinable = false; pthread_detach(the_thread);}
	void join() { 
		if (joinable) {
			void *p = NULL;
			pthread_join(the_thread, &p); 
			joinable = false; 
			// if e points to a valid exception -- THIS IS VERY BAD
			std::auto_ptr<big_int::exception> e(static_cast<big_int::exception *>(p));
			if (p) e->repropagate();
		} 
	}
	void interrupt() {} // do nothing for now
	//~bi_thread() { detach(); }
};


// basic mutex class which encapsulates the pthread mutex
class bi_thread_mutex {
	pthread_mutex_t the_mutex;
public:
	bi_thread_mutex() : the_mutex() { pthread_mutex_init(&the_mutex, NULL); }
	~bi_thread_mutex() { pthread_mutex_destroy(&the_mutex); }
	friend class bi_mutex_lock;
};

// RAII locking class
class bi_mutex_lock {
	bi_thread_mutex &bi_mutex;
	bi_mutex_lock(const bi_mutex_lock&);
	bi_mutex_lock& operator=(const bi_mutex_lock&);
public:
	bi_mutex_lock(bi_thread_mutex &the_mutex) : bi_mutex(the_mutex) { pthread_mutex_lock(&bi_mutex.the_mutex); }
	~bi_mutex_lock() { pthread_mutex_unlock(&bi_mutex.the_mutex); }  
};
#endif

// our version of thread group. We do not use boost::threadgroup even for the boost case
// because that disallows a dynamic join_all(): allowing the container to change as we wait for the join
/*
class bi_thread_group {
	std::vector< bi_thread * > bi_thread_vec;
public:
	// bi_thread_group(); // default
	void join_all() { 
		for (int i=0; i < bi_thread_vec.size(); i++) {
			bi_thread_vec[i]->join();
		}
	}
	void add_thread(bi_thread* bi_thread_ptr)
	{ bi_thread_vec.push_back(bi_thread_ptr); }
	
	template <class Callable>
	void create_thread(Callable &T)
	{ bi_thread_vec.push_back(new bi_thread(T)); }
	
	~bi_thread_group() {
		for (int i=0; i < bi_thread_vec.size(); i++)
			delete bi_thread_vec[i];
	}
};
*/