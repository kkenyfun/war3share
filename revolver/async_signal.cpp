#include "async_signal.h"

#if !defined(WIN32)

#include "base_namespace.h"
#include "base_event_handler.h"
#include "base_reactor_instance.h"

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <signal.h>
#include <unistd.h>

using namespace BASEOBJECT;

namespace signalfd
{
	int pipefd[2];

	int readfd()
	{
		return pipefd[0];
	}

	int read(char* buf, size_t buflen)
	{
		return ::read(readfd(), buf, buflen);
	}

	void signal_handler(int sig)
	{
		int save_errno = errno;
		int msg = sig;
		write(pipefd[1], (char*)&msg, 1);
		errno = save_errno;
	}

	void listen(int sig)
	{
		signal(sig, signal_handler);
	}

	class event_t
		: public CEventHandler
	{
	public:
		void install(fn_signal on_signal)
		{
			on_signal_ = on_signal;
			REACTOR_INSTANCE()->register_handler(this, MASK_READ);
		}

		BASE_HANDLER get_handle() const
		{
			return (BASE_HANDLER)readfd();
		}
	
		int32_t handle_input(BASE_HANDLER handle)
		{
			char signals[1024];
			int rc = read(signals, sizeof(signals));
			if (rc > 0)
			{
				for (int i = 0; i < rc; ++i)
				{
					on_signal_(signals[i]);
				}
			}
			return 0;
		}

		fn_signal on_signal_;
	};
	event_t event;

	bool initialize(fn_signal on_signal)
	{
		pipefd[0] = -1;
		pipefd[1] = -1;
		if (pipe(pipefd)) {
			return false;
		}
		listen(SIGTERM);
		event.install(on_signal);
		return true;
	}
}
#endif