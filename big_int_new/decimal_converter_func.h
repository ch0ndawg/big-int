/*
 *  decimal_converter_func.h
 *  big_int
 *
 *  Created by Chris on 7/20/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <map>
#include "multithread_radix_convert.h" 
class decimal_converter_func : public ordinary_radix_converter {
public:
	typedef std::map<unsigned int,std::string> results_map;
#ifdef MULTITHREAD
protected:
	typedef std::list< bi_persistent_info > info_queue;
private:
	bi_thread_mutex results_mutex;
	info_queue the_info;
#endif
public:
	decimal_converter_func(const big_int &bi);
	static const double LOGBASE;
	enum {	THE_BASE = 10, MIN_REC_SIZE =2048 };
	virtual int get_base() const { return THE_BASE;}
	virtual size_t min_rec_size() const { return MIN_REC_SIZE * LOGBASE; }
	
	virtual void operator()(const big_int &bi, unsigned int digit_group_id,
					unsigned int expected_size,bool is_first_group);
#ifdef MULTITHREAD
	virtual void start_thread( const  bi_persistent_info  &p)
	{ the_info.push_back(p); }
	void join_all();
#endif
	virtual ~decimal_converter_func() {}
};
