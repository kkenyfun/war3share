#include "rudp_log_macro.h"

#ifndef DISABLE_RUDPLOG
	#ifdef _DEBUG
		SingleLogStream rudp_log("rudpdll", BaseLogStream::debug);
	#else
		SingleLogStream rudp_log("rudpdll", BaseLogStream::general);
	#endif
#endif