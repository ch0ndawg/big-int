/*
 *  multithread_radix_convert.h
 *  big_int
 * 
 *	Here we give a fancy O(n log n) and parallel algorithm to convert a big_int into a string. 
 *	What is needed is a function (or function object) that does the base case. The base case is
 *	defined by the constant MIN_REC_SIZE below which, recursion is terminated.
 *
 *	This algorithm operates roughly as follows: recall that it is very easy to convert between
 *	radices (radixes?) which are powers of some smaller space: simply regroup the digits.
 *	If the base is B^n, for example, then groups of n base-B digits, starting from the *right*,
 *	form the base-B^n expansion.
 *
 *	Now what we do here is: compute an extremely high power of the base we wish to convert to,
 *	of magnitude approx. the square root of our number. Then the usual radix conversion algorithm
 *	splits our number into two very large "digits," which we represent as smaller big_ints. But we've
 *	still got work to do, because we still have to convert these large "digits" into something 
 *	representable. But these two digits can be processed independently! So we split each of the two
 *	"digits" again into two smaller "digits," in different threads, and so on and so forth.
 *
 *	Until, of course, we hit bottom--the base case, which does ordinary single-digit by single-digit
 *	radix conversion by brute force. When the functions return, the strings are glued together.
 *	The size of the base case is chosen as a compromise between the overhead incurred by both
 *	threading and recursion, which is guaranteed to exist by the Intermediate Value Theorem,
 *	since for very large N, the O(N log N) nature of this algorithm will come to be dominated by
 *	the brute-force digit-by-digit algorithm, which is O(N^2).
 *
 *  Created by Chris on 7/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>
#include <list>
#include <map>
#include "ordinary_radix_converter.h"
#ifdef MULTITHREAD
#include "thread.h"
extern bi_thread_mutex cout_mutex;
typedef boost::shared_ptr<const bi_ldiv_t> smart_quotrem_ptr;
#endif

class fancy_radix_converter {
private:
	// state data, essentially the "parameters" of the function (e.g. the extensive use of const references)
	big_int::exception *e;
	const big_int &num;
	const size_t expected_size; // expected size - for padding with zeros
	ordinary_radix_converter &converter_func;
	unsigned int digit_group_id; // thread id
	bool is_first_group;
public:
	// construction for a recursive invocation
	fancy_radix_converter(const big_int &bi, size_t ee,  ordinary_radix_converter &o, int i, bool f ) 
	:  e(0),num(bi), expected_size(ee), 
	converter_func(o), digit_group_id(i), is_first_group(f) { }
	
	// initial construction via the ordinary radix converter
	fancy_radix_converter(const big_int &bi, ordinary_radix_converter &o) 
	:  e(0), num(bi), expected_size(o.size()), 
	converter_func(o), digit_group_id(1), is_first_group(true) { }
	
	void f00(); // the real work
	void operator()();
	big_int::exception * get_exception() const { return e; }
};
#ifdef MULTITHREAD
typedef boost::shared_ptr<bi_thread> smart_thread_ptr; 

// structure that keeps track of essential thread information allocated on the heap
struct bi_persistent_info {
	// use smart pointers
	
	// referencing radix converter (used to store a final or exceptional state)
	boost::shared_ptr<fancy_radix_converter> frc;
	// intermediate result: quotient and remainder
	smart_quotrem_ptr quotrem;
	
	// the thread reference
	smart_thread_ptr the_thread;
	bi_persistent_info(const boost::shared_ptr<fancy_radix_converter > &frc_,
					const smart_quotrem_ptr &quotrem_) :   
					frc(frc_), quotrem(quotrem_), the_thread(new bi_thread(*frc)) {}
	//~bi_persistent_info()  {} // default -- decrements the reference count of each shared ptr
};
#endif