// RUdpDll.cpp : Defines the exported functions for the DLL application.
//

#include "session_ctrl.h"
#include "rudp_log_macro.h"
#pragma comment(lib, "winmm.lib")

rudp_callback*			g_pCallback =NULL;

//导出方法
rudp_module*  create_rudp_module( rudp_callback* pCallback, Inet_Addr sockAddr[], uint16_t nAddrCount, uint32_t nMaxConnectionCount )
{
	RUDP_DEBUG ( "CreateUdpIOCP addr" << nAddrCount <<  "maxconnection" << nMaxConnectionCount );

	CSessionCtrl* pSessionCtrl= new CSessionCtrl( pCallback );
	if( pSessionCtrl == NULL )
	{
		return NULL;
	}
	//nMaxConnectionCount=6000;
	if( pSessionCtrl->initService( sockAddr, nAddrCount, nMaxConnectionCount ) != 0 )
	{
		delete pSessionCtrl;
		pSessionCtrl= NULL;
	}
	g_pCallback = pCallback;
	return (rudp_module*)pSessionCtrl;
}


void close_rudp_module( rudp_module* module )
{
	CSessionCtrl* p= (CSessionCtrl*)module;
	if( p )
	{
		delete p;
	}
	return;
}
