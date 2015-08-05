#ifndef __SWITCH_LOG_MACRO_H_
#define __SWITCH_LOG_MACRO_H_

#include "base_log.h"
#include "base_hex_string.h"
#include "rudp_global.h"

#if !defined WIN32
#	define DISABLE_RUDPLOG
#endif

#ifdef DISABLE_RUDPLOG
#	define RUDP_DEBUG(arg)
#	define RUDP_INFO(arg)
#	define RUDP_WARNING(arg)
#	define RUDP_ERROR(arg)
#	define RUDP_FATAL(arg)
#	define BUFF_DEBUG(arg)
#	define BUFF_INFO(arg)
#	define BUFF_WARNING(arg)
#	define BUFF_ERROR(arg)
#	define BUFF_FATAL(arg)
#	define SET_RUDP_LOG_LEVEL(arg)
#	define SET_BUFF_LOG_LEVEL(arg)
#else

extern SingleLogStream rudp_log;

#define RUDP_DEBUG(arg)\
	if( 0 == g_RudpCfgInfo.nOutputLogFlag )\
	{ \
		DEBUG_TRACE(rudp_log, arg)\
	}\
	else\
	{\
		OUT_PUT_LOG(RUDP_LOG_ALL, arg );\
	}

#define RUDP_INFO(arg)\
	if( 0 == g_RudpCfgInfo.nOutputLogFlag )\
	{ \
		INFO_TRACE(rudp_log, arg)\
	}\
	else\
	{\
		OUT_PUT_LOG(RUDP_LOG_NORMAL, arg );\
	}\

#define RUDP_WARNING(arg)\
	if( 0 == g_RudpCfgInfo.nOutputLogFlag )\
	{ \
		WARNING_TRACE(rudp_log, arg)\
	}\
	else\
	{\
	OUT_PUT_LOG(RUDP_LOG_WARNING, arg );\
	}\

#define RUDP_ERROR(arg)\
	if( 0 == g_RudpCfgInfo.nOutputLogFlag )\
	{ \
		WARNING_TRACE(rudp_log, arg)\
	}\
	else\
	{\
	OUT_PUT_LOG( RUDP_LOG_ERROR , arg);\
	}\

#define SET_RUDP_LOG_LEVEL(arg)\
	rudp_log.set_trace_level(arg)

#endif

#endif
