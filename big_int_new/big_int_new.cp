/*
 *  big_int_new.cp
 *  big_int_new
 *
 *  Created by Chris on 11/2/12.
 *  Copyright (c) 2012 Chris. All rights reserved.
 *
 */

#include <iostream>
#include "big_int_new.h"
#include "big_int_newPriv.h"

void big_int_new::HelloWorld(const char * s)
{
	 big_int_newPriv *theObj = new big_int_newPriv;
	 theObj->HelloWorldPriv(s);
	 delete theObj;
};

void big_int_newPriv::HelloWorldPriv(const char * s) 
{
	std::cout << s << std::endl;
};

