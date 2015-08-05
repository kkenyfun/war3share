#pragma once

namespace signalfd
{
	typedef int (*fn_signal)(int sig);
#if defined(WIN32)
	inline bool initialize(fn_signal on_signal) { return false; }
#else
	bool initialize(fn_signal on_signal);
#endif
}
