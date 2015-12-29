/*
 *  multithread_radix_convert.cpp
 *  big_int
 *
 *  Created by Chris on 7/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
 
#include "multithread_radix_convert.h"

// the actual multithreaded recursive function
void fancy_radix_converter::f00()
{
	// base case
	if (expected_size <= converter_func.min_rec_size() )
	{   
		//std::cout << "Base Case encountered in thread " << digit_group_id << "...\n";
#ifdef USE_BOOST_THREADS__
		boost::this_thread::interruption_point();
#endif
		converter_func(num, digit_group_id, expected_size, is_first_group);
		
		return;
	}
	
	// expected_size is the length of the resultant digit string for which this call has control over
	size_t new_expected_size = expected_size>>1; // divide 
	
	// find its two "digits"
#ifdef MULTITHREAD
	typedef boost::shared_ptr<fancy_radix_converter > smart_frc_ptr;
	smart_quotrem_ptr the_result (
		// lookup the powers in the table with entry new_expected_size
		new bi_ldiv_t(num, converter_func.get_power(new_expected_size))
	);
	// if the thread needs to be canceled
#ifdef USE_BOOST_THREADS__
	boost::this_thread::interruption_point();
#endif
	bool need_to_skip_zeros = is_first_group && (the_result->q == 0);
#else
	bi_ldiv_t the_result(num, converter_func.get_power(new_expected_size));
	bool need_to_skip_zeros = is_first_group && (the_result.q == 0);
#endif
	
	unsigned int newid = digit_group_id << 1;
	//if (newid > 48 ) throw big_int::overflow_exception();
	// if this is the first group, and the quotient is zero, i.e. the zeros are leading, then we need to skip them
	
	//  skip computation of first group if we need to skip zeros
	if (!need_to_skip_zeros) {
		// otherwise, do the recursion; it will assemble the requisite placeholder-zeros
		
		// create another fancy radix converter on the heap, because we need local objects to persist after this thread returns
		// (notation is ewww, i know)
		#ifdef MULTITHREAD
		smart_frc_ptr left_digit ( 
			new fancy_radix_converter(the_result->q, new_expected_size,
			converter_func, newid++, is_first_group )
		);
		// spawn a new thread to calculate the left "digit"
		converter_func.start_thread(bi_persistent_info(left_digit,the_result));
		#else
		fancy_radix_converter left_digit(the_result.q, new_expected_size,
										 converter_func, newid++, is_first_group );
		left_digit.f00();
		#endif
	}
	 
	// unlike the FFT, this thread does not have any obligations to stay running when the child threads are running
	#ifdef MULTITHREAD
	smart_frc_ptr right_digit( 
		new fancy_radix_converter (the_result->r, new_expected_size,
		converter_func, newid, need_to_skip_zeros ) // if we needed to skip zeros, then the right digit is the new first group
	);
	// start the thread
	converter_func.start_thread(bi_persistent_info(right_digit,the_result));
	#else
	// single-threaded operation: recursively call ourselves, in a closure-like way
	fancy_radix_converter right_digit(the_result.r, new_expected_size,
									   converter_func, newid, need_to_skip_zeros );
	right_digit.f00();
	#endif
}

void fancy_radix_converter::operator()()
{
		// invocation; this actually is the exception handler
		try {
			f00();
		}
		catch (big_int::exception &ex) {
			e = ex.clone(); // polymorphic copy
			//bi_mutex_lock l(cout_mutex);
			//std::cerr << "Thread #" << digit_group_id << " threw an exception:\n" << e->what()<< "\n";
		} // thread interrupted exception is passed to global pthread function
	}
