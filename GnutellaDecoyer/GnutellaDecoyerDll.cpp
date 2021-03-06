#include "StdAfx.h"
#include "GnutellaDecoyerDll.h"
#include "Iphlpapi.h"	// for GetAdaptersInfo (also need Iphlpapi.lib)
#include "Psapi.h"	// for GetProcessMemoryInfo()
#include "RegistryKeys.h"

GnutellaDecoyerDll::GnutellaDecoyerDll(void)
{
}

//
//
//
GnutellaDecoyerDll::~GnutellaDecoyerDll(void)
{
}

//
//
//
void GnutellaDecoyerDll::DllInitialize()
{

}

//
//
//
void GnutellaDecoyerDll::DllUnInitialize()
{
	WSocket::Cleanup();
	m_connection_manager.KillModules();	// hopefully avoids crash on closing
	m_noise_manager.KillModules();	// hopefully avoids crash on closing
	ClosePerformanceCounters();
	m_dlg.DestroyWindow();
	m_file_sharing_manager.TerminateThread();

}

//
//
//
void GnutellaDecoyerDll::DllStart()
{
	WSocket::Startup();
	RegistryKeys::GetModuleCounts(&m_min_module_count,&m_max_module_count);
	m_dlg.Create(IDD_GNUTELLA_DECOYER_DIALOG,CWnd::GetDesktopWindow());
	m_dlg.InitParent(this);
	InitPerformanceCounters();
	m_dlg.SetTimer(1,1000,0);
	m_dlg.SetTimer(2,60*1000,0);
	m_dlg.SetTimer(3,5*60*1000,0);

	m_connection_manager.InitParent(this);
	//m_connection_manager.SetFileSharingManager(&m_file_sharing_manager);
	m_file_sharing_manager.InitParent(this,m_dlg.GetSafeHwnd());
	m_noise_manager.InitParent(this);
	m_noise_manager.SetFileSharingManager(&m_file_sharing_manager);
	m_decoy_syncher.Register(this, "Gnutella_Decoys");
	m_decoy_syncher.InitParent(this,0);
	m_host_cache_syncher.Register(this, "GnutellaHostCache");
	m_host_cache_syncher.InitParent(this,1);
	m_noise_manager.StartServer();

}

//
//
//
void GnutellaDecoyerDll::DllShowGUI()
{
	m_dlg.ShowWindow(SW_NORMAL);
	m_dlg.BringWindowToTop();
}

//
//
//
//
//
//
int GnutellaDecoyerDll::AlterModuleCounts(int min,int max)
{
	// Check for special data queying calls
	if(min==max)
	{
		if(min==-1)	// if min and max are -1, return the min value
		{
			return m_min_module_count;
		}
		if(min==1)	// if min and max are 1, return the max value
		{
			return m_max_module_count;
		}
	}
	
	m_min_module_count+=min;
	m_max_module_count+=max;

	// If changing the max, it can't go below the min
	if(max!=0)
	{
		if(m_max_module_count<m_min_module_count)
		{
			m_max_module_count=m_min_module_count;
		}
	}
	else	// If changing the min, it will increase the max if the min is greater than the max
	{
		if(m_min_module_count>m_max_module_count)
		{
			m_max_module_count=m_min_module_count;
		}
	}

	// Limit both the min and max module counts to fall within 0 and 100
	if(m_min_module_count<0)
	{
		m_min_module_count=0;
	}
	
	else if(m_min_module_count>200)
	{
		m_min_module_count=200;
	}
	
	if(m_max_module_count<0)
	{
		m_max_module_count=0;
	}
	
	else if(m_max_module_count>200)
	{
		m_max_module_count=200;
	}
	

	// Check to see if we need to delete some mods due to a decrease in the max
	m_connection_manager.LimitModuleCount(m_max_module_count);

	// Store in the registry
	RegistryKeys::SetModuleCounts(m_min_module_count,m_max_module_count);

	// If the max was changed, return the new max, else return the new min
	if(max!=0)
	{
		return m_max_module_count;
	}
	else
	{
		return m_min_module_count;
	}
}

//
//
//
void GnutellaDecoyerDll::InitPerformanceCounters()
{
	UINT timer_ret=1;

	int i;

	char iai_buf[2048];
	DWORD iai_buf_len=2048;
	IP_ADAPTER_INFO *iai=(IP_ADAPTER_INFO *)iai_buf;

	GetAdaptersInfo(iai,&iai_buf_len);

	// Remove (,) and / from the description of the interface
	while(strchr(iai->Description,'(')!=NULL)
	{
		*strchr(iai->Description,'(')='[';
	}
	while(strchr(iai->Description,')')!=NULL)
	{
		*strchr(iai->Description,')')=']';
	}
	while(strchr(iai->Description,'/')!=NULL)
	{
		*strchr(iai->Description,'/')='_';
	}
	while(strchr(iai->Description,'#')!=NULL)
	{
		*strchr(iai->Description,'#')='_';
	}

	CString network_adaptor = iai->Description;
	network_adaptor.Trim();
	m_keynames[0]="\\Processor(0)\\% Processor Time";
	m_keynames[1]="\\Network Interface(";
	m_keynames[1]+=network_adaptor;
	m_keynames[1]+=")\\Bytes Total/sec";
	m_keynames[2]="\\Network Interface(";
	m_keynames[2]+=network_adaptor;
	m_keynames[2]+=")\\Current Bandwidth";

    m_pdh=0;

    // Create the pdh query
    if(PdhOpenQuery(NULL,0,&m_pdh))
	{
		//MessageBox(NULL,"Error opening pdh query","Error",MB_OK);
		return;
	}

    // ADD A COUNTER TO THE QUERY
	for(i=0;i<3;i++)
	{
		PDH_STATUS error=PdhAddCounter(m_pdh,m_keynames[i].c_str(),NULL,&m_pdh_counters[i]);

		if(error)
		{
			//MessageBox(NULL,"Error adding counter to the pdh query","Error",MB_OK);
			return;
		}
	}
}

//
//
//
void GnutellaDecoyerDll::OnTimer(UINT nIDEvent)
{
	UINT timer_ret=1;
	m_dlg.KillTimer(nIDEvent);
	switch(nIDEvent)
	{
		case 1:	// 1 second
		{
			CheckPerformanceCounters();
			m_connection_manager.TimerHasFired();
			//
			UINT spoof_size,temp_size,hash_size;
			m_file_sharing_manager.GetSpoofEntriesSizes(spoof_size, temp_size,hash_size);
			m_dlg.UpdateSpoofSize(spoof_size,temp_size,hash_size);
			m_dlg.SetTimer(nIDEvent,1*1000,0);
			break;
		}
		case 2:	// 1 minute heartbeat
		{
			m_connection_manager.OnHeartbeat();	// write out the host cache to file
			


			m_file_sharing_manager.OnOneMinuteTimer();
			m_dlg.SetTimer(nIDEvent,60*1000,0);
			break;
		}
		case 3: // 5 minute timer
		{
			CheckModuleConnections();
			m_dlg.SetTimer(nIDEvent,5*60*1000,0);
			break;
		}
		case 4:
		{
			m_noise_manager.StartServer();
			break;
		}
		case 5:
		{
			m_noise_manager.ClearIdleModules();
			m_dlg.SetTimer(nIDEvent,30*1000,0);
			break;
		}
		//case 6:
		//{
		//	m_dlg.SetTimer(nIDEvent,5*1000,0);
		//	break;
		//}
	}
}

//
//
//
void GnutellaDecoyerDll::CheckPerformanceCounters()
{
	UINT i;

    // Collect the pdh query data
	if(PdhCollectQueryData(m_pdh))
	{
//		MessageBox(NULL,"Error collecting pdh query data","Error",MB_OK);
//		MessageBox(NULL,m_keynames[1].c_str(),"On this network adaptor",MB_OK);
		//return;
	}

	// Check the processor usage counter
	PDH_FMT_COUNTERVALUE value;
    PDH_STATUS error;

	error=PdhGetFormattedCounterValue(m_pdh_counters[0],PDH_FMT_LONG,0,&value);
	if(error)
	{
//		MessageBox(NULL,"Error formatting pdh counter data","Error",MB_OK);
//		MessageBox(NULL,m_keynames[1].c_str(),"On this network adaptor",MB_OK);
//		m_dlg.SetWindowText("Error formatting pdh coutner data");
		return;
	}

	m_proc_percent_usage=value.longValue;

	// If less than 90 % kick off another mod if there is less than min count of them
	if(/*(m_proc_percent_usage<90) & */(m_connection_manager.ReturnModCount()<(UINT)m_min_module_count))
	{
		m_connection_manager.AddModule();

		char log[1024];
		sprintf(log,"Module Added : (Current Count : %u) : (Proc % Usage : %u%%) : (Min Mod Count : %u)",
			m_connection_manager.ReturnModCount(),m_proc_percent_usage,m_min_module_count);
		m_dlg.Log(log);
	}

	// Check the bandwidth counter
	error=PdhGetFormattedCounterValue(m_pdh_counters[1],PDH_FMT_LONG,0,&value);
	if(error)
	{
//		MessageBox(NULL,"Error formatting bandwidth counter data","Error",MB_OK);
//		MessageBox(NULL,m_keynames[1].c_str(),"On this network adaptor",MB_OK);
//		m_dlg.SetWindowText("Error formatting pdh coutner data");
		return;
	}

	m_bandwidth_in_bytes=value.longValue;

	// Check the current bandwidth in bits per second
	error=PdhGetFormattedCounterValue(m_pdh_counters[2],PDH_FMT_LONG,0,&value);
	if(error)
	{
//		MessageBox(NULL,"Error formatting bandwidth in bits per second data","Error",MB_OK);
//		MessageBox(NULL,m_keynames[1].c_str(),"On this network adaptor",MB_OK);
		return;
	}

	m_current_bandwdith_in_bits_per_second=value.longValue;	// this should be 100 Mbps
	
	double bps=m_bandwidth_in_bytes*8;

	// Measure the processes mem usage
	HANDLE hHandle;
	DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),GetCurrentProcess(),&hHandle,0,FALSE,DUPLICATE_SAME_ACCESS);
	
	PROCESS_MEMORY_COUNTERS pmc;
	pmc.cb=sizeof(PROCESS_MEMORY_COUNTERS);
	GetProcessMemoryInfo(hHandle,&pmc,sizeof(PROCESS_MEMORY_COUNTERS));

	CloseHandle(hHandle);

	m_mem_usage=(UINT)pmc.WorkingSetSize;	// in bytes

	// Put commas in the mem measurement value
	char mem_buf[1024];
	string mem;
	_itoa(m_mem_usage/1024,mem_buf,10);
	strrev(mem_buf);
	for(i=0;i<strlen(mem_buf);i++)
	{
		if((i>0) && (i%3==0))
		{
			mem+=",";
		}
		
		mem+=mem_buf[i];
	}
	strcpy(mem_buf,mem.c_str());
	strrev(mem_buf);
	
	// Display the status
	char buf[1024];
	sprintf(buf,"[ %u %% ] - [ %.2f KB/sec - %.2f %% ] - [ %s KB ]",m_proc_percent_usage,((double)m_bandwidth_in_bytes)/1024,
		(100*bps)/m_current_bandwdith_in_bits_per_second,mem_buf);
	m_dlg.GetDlgItem(IDC_Computer_Status_Static)->SetWindowText(buf);
}

//
//
//
void GnutellaDecoyerDll::ClosePerformanceCounters()
{
	 PdhCloseQuery(m_pdh);
}

//
//
//
void GnutellaDecoyerDll::CheckModuleConnections()
{
	UINT i;

	int mod_count=m_connection_manager.ReturnModCount();

	// Check to see that the mod count is at least as big as the min.  If not, let it get to that point
	if(mod_count<m_min_module_count)
	{
		return;
	}

	// If all of modules are all below 30 connected sockets, remove a module if we are above the min module count
	if(mod_count>m_min_module_count)
	{
		bool remove=true;
		for(i=0;i<v_mod_status.size();i++)
		{
			if(v_mod_status[i].m_connected_socket_count>=30)
			{
				remove=false;
				break;
			}
		}

		if(remove)
		{
			char log[1024];
			sprintf(log,"Module Removed : (Current Count : %u) : (Connected Socket Counts :",mod_count-1);
			
			char val[32];
			for(i=0;i<v_mod_status.size();i++)
			{
				strcat(log," ");
				_itoa(v_mod_status[i].m_connected_socket_count,val,10);
				strcat(log,val);
			}
			strcat(log,")\n");
			
			m_dlg.Log(log);
			
			// Remove the module
			m_connection_manager.LimitModuleCount(mod_count-1);
			m_dlg.ModuleCountHasChanged(mod_count-1);

			// Clear the status vector so that only the remaining modules will have status members
			v_mod_status.clear();

			return;	// if we just removed a mod, we ain't gonna add one
		}
	}

	// If all of the modules are all greater than 40 connected sockets, add a module if we are below the max mod count
	if(mod_count<m_max_module_count)
	{
		bool add=true;
		int not_enough_count=0;
		for(i=0;i<v_mod_status.size();i++)
		{
			if(v_mod_status[i].m_connected_socket_count<40)
			{
				add=false;
				not_enough_count++;
			}
		}

		if(add)
		{
			m_connection_manager.AddModule();

			char log[1024];
			sprintf(log,"Module Added : (Current Count : %u) : (Proc % Usage : %u%%) : (Min Mod Count : %u)",
				m_connection_manager.ReturnModCount(),m_proc_percent_usage,m_min_module_count);
			m_dlg.Log(log);
		}
		else
		{
			// Log why we didn't add a module (not all of the modules were connected to at least 40 modules)
			char log[1024];
			sprintf(log,"Module NOT Added : %u / %u modules were not connected to at least 40 Supernodes",not_enough_count,v_mod_status.size());
			m_dlg.Log(log);
		}
	}
}

//
//
//
void GnutellaDecoyerDll::EnableVendorCounts(BOOL enable)
{
	m_connection_manager.m_vendor_counts_enabled = (bool)enable;
}

//
//
//
void GnutellaDecoyerDll::ReportConnectionStatus(ConnectionModuleStatusData &status)
{
	vector<ConnectionModuleStatusData>::iterator data_iter=v_mod_status.begin();
	while(data_iter!=v_mod_status.end())
	{
		if(data_iter->m_mod==status.m_mod)
		{
			v_mod_status.erase(data_iter);
			break;
		}

		data_iter++;
	}

	v_mod_status.push_back(status);

	// Display the status on the dialog
	m_dlg.ReportConnectionStatus(status);//,&v_mod_status);
}

//
//
//
void GnutellaDecoyerDll::Log(const char* log)
{
	m_dlg.Log(log);
}

//
//
//
void GnutellaDecoyerDll::VendorCountsReady(vector<VendorCount>& vendor_counts)
{
	m_dlg.UpdateVendorCounts(vendor_counts);
}

//
//
//
void GnutellaDecoyerDll::ShowSocketStatus(const char* msg)
{
	m_dlg.GetDlgItem(IDC_Connection_Status_Static)->SetWindowText(msg);
}

//
//
//
void GnutellaDecoyerDll::InitFileSharingManagerThreadData(WPARAM wparam,LPARAM lparam)
{
	m_file_sharing_manager.InitThreadData(wparam,lparam);
}

//
//
//
void GnutellaDecoyerDll::FileSharingManagerUpdateSharedFilesDone()
{
	m_file_sharing_manager.FileSharingManagerUpdateSharedFilesDone();
}

//
//
//
void GnutellaDecoyerDll::RebuildSharingDB()
{
	m_file_sharing_manager.RebuildSharedFilesList();
}

//
//
//
void GnutellaDecoyerDll::DecoySupplySynched()
{
	m_file_sharing_manager.RebuildSharedFilesList();
}

//
//
//
void GnutellaDecoyerDll::NewShareFileList(vector<QueryHitResult> share_files,hash_set<FileSizeAndHash,FileSizeAndHashHash> hashes)
{
	m_connection_manager.NewShareFileList(share_files);
}

//
//
//
int GnutellaDecoyerDll::GetModCount()
{
	int count=0;
	count=m_noise_manager.GetModCount();
	return count;
}

//
//
//
void GnutellaDecoyerDll::DisableSharing()
{
	m_noise_manager.DisableSharing();
//	m_file_sharing_manager.DisableSharing();
}

//
//
//
void GnutellaDecoyerDll::EnableSharing()
{
	m_noise_manager.EnableSharing();
//	m_file_sharing_manager.RebuildSharedFilesList();
}

//
//
//
void GnutellaDecoyerDll::ReportNoiseManagerStatus(vector<NoiseModuleThreadStatusData> &status)
{
	m_dlg.ReportNoiseManagerStatus(status);
}