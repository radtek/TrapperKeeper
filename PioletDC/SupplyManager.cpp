// SupplyManager.cpp

#include "stdafx.h"
#include "SupplyManager.h"
#include "ConnectionManager.h"
#include "PioletDCDll.h"

#include "imagehlp.h"	// for MakeSureDirectoryPathExists

//
//
//
SupplyManager::SupplyManager()
{
	m_minute_counter = 0;
	//m_second_counter = 0;
	//m_5_mins_passed = false;

//	ReadSupplyFromFiles();
}

//
//
//
SupplyManager::~SupplyManager()
{
/*	for(UINT i=0;i<v_thread_data.size();i++)
	{
		delete v_thread_data[i];
	}
	v_thread_data.clear();
*/
}

//
//
//
void SupplyManager::InitParent(PioletDCDll* parent)
{
	p_parent = parent;

//	p_parent->UpdateSupplyProjects(v_supply_projects);	// update the gui with the supply projects read from file
}

//
//
//
void SupplyManager::InitConnectionManager(ConnectionManager* cm)
{
	p_connection_manager = cm;
}

//
//
//
void SupplyManager::UpdateProjects(vector<SupplyProject> &supply_projects)
{
	UINT i, j;

	vector<SupplyProject> new_supply_projects;

	//find existing project within the updated projects and update it.  If not found, delete the project
	for(i=0; i<v_supply_projects.size(); i++)
	{
		bool project_found = false;
		for(j=0; j<supply_projects.size(); j++)
		{
			if(strcmp(v_supply_projects[i].m_name.c_str(), supply_projects[j].m_name.c_str())==NULL)
			{
				project_found = true;
				v_supply_projects[i].m_interval = supply_projects[j].m_interval;
				//v_supply_projects[i].m_uber_dist_enabled = supply_projects[j].m_uber_dist_enabled;
				new_supply_projects.push_back(v_supply_projects[i]);
				break;
			}
		}
		
		//project not found, delete it
		/*if(project_found == false)
		{
			v_supply_projects[i].DeleteSupplyFile();
		}
		*/
	}
	for(i=0; i<supply_projects.size(); i++)
	{
		bool project_found = false;


		for(j=0; j<v_supply_projects.size(); j++)
		{
			if(strcmp(supply_projects[i].m_name.c_str(), v_supply_projects[j].m_name.c_str())==NULL)
			{
				project_found = true;
/* =>_<=

				supply_projects[i].v_dist_entries=v_supply_projects[j].v_dist_entries;
				supply_projects[i].v_spoof_entries=v_supply_projects[j].v_spoof_entries;
				supply_projects[i].m_num_spoofs=v_supply_projects[j].m_num_spoofs;
*/
				break;
			}
		}
		if(project_found==false)
		{
/* =>_<=
			CFile archive_file;
	
			char new_filename[128];
			strcpy(new_filename,"Supply Files\\");
			strcat(new_filename, supply_projects[i].m_name.c_str());
			strcat(new_filename, ".arc");

			BOOL open = archive_file.Open(new_filename, CFile::modeRead|CFile::typeBinary);
			if(open==TRUE)
			{
				SupplyProject archived_project;
				
				char * buffer =  new char[(UINT)archive_file.GetLength()];

				archive_file.Read(buffer, (UINT)archive_file.GetLength());

				archived_project.ReadFromBuffer(buffer);

				
				supply_projects[i].v_spoof_entries = archived_project.v_spoof_entries;

				supply_projects[i].m_num_spoofs = 0;

				for(UINT j=0; j<supply_projects[i].v_spoof_entries.size(); j++)
				{
					supply_projects[i].m_num_spoofs += supply_projects[i].v_spoof_entries[j].m_num_spoofs;
				}

//				p_dist_manager->ProcessSupplyProject(supply_projects[i]);

				archive_file.Close();
				delete [] buffer;

				// Rename the arc file to sup
				string sup_filename="Supply Files\\";
				sup_filename+=supply_projects[i].m_name.c_str();
				sup_filename+=".sup";
				CFile::Rename(new_filename,sup_filename.c_str());
			}
*/			new_supply_projects.push_back(supply_projects[i]);
		}
	}

	v_supply_projects = new_supply_projects;

#ifdef _DEBUG
/*
	// *&* TEMP KLUDGE - to make the supply offset 2 minute(s)
	for(i=0; i<v_supply_projects.size(); i++)
	{
		v_supply_projects[i].m_interval=2*v_supply_projects.size();
	}
*/
#endif // _DEBUG

	// Calculate the supply project offsets based on the intervals
	for(i=0; i<(UINT)v_supply_projects.size(); i++)
	{
		// Find the number of projects with this same interval, so that the offsets will be multiples of interval / # projects with that interval
		int interval_count=0;
		int interval_index=0;
		for(j=0;j<v_supply_projects.size();j++)
		{
			if(v_supply_projects[j].m_interval==v_supply_projects[i].m_interval)
			{
				// Save what the index of this interval is when counting all of the projects with this interval
				if(i==j)
				{
					interval_index=interval_count;
				}

				interval_count++;
			}
		}

		// offset = interval index * # minutes / # projects with this interval
		UINT offset = interval_index*(v_supply_projects[i].m_interval*60/interval_count);
		offset /= 60;
		v_supply_projects[i].m_offset = offset;

		// Write out the project supply file with the new offset
//		WriteProjectSupplyToFile(v_supply_projects[i].m_name);
	}

//	p_parent->UpdateSupplyProjects(v_supply_projects);	// update the gui
	p_connection_manager->ProjectSupplyUpdated(NULL);	// update the sockets (all the projects)

/* =>_<=
	for(i=0; i<supply_projects.size();i++)
	{
		p_dist_manager->ProcessSupplyProject(supply_projects[i]);
	}
=>_<= */
}

//
//
//
/*
void SupplyManager::OnHeartbeat()
{
	for(UINT i=0; i<v_supply_projects.size(); i++)
	{
/* not needed if we calculate supply right before doing another search
		if(v_supply_projects[i].GUIDIsNull()==false)
		{
			v_supply_projects[i].m_calculate_supply_counter++;
		}
*/
/*		if(m_minute_counter % v_supply_projects[i].m_interval==v_supply_projects[i].m_offset)
		{
			p_parent->StopGatheringProjectData(v_supply_projects[i].m_name);
			v_supply_projects[i].m_query_guid = p_connection_manager->PerformProjectSupplyQuery((char *)v_supply_projects[i].m_name.c_str());
			/*
			// Stop gathering data for the previous search...since we are going to start a new one...and process the data (if there is a search)
			bool data_processing=false;
			if(v_supply_projects[i].GUIDIsNull()==false)
			{
				data_processing=true;
				CalculateProjectSupply(v_supply_projects[i].m_name);

				p_parent->StopGatheringProjectData(v_supply_projects[i].m_name);
			}
			
			// Do a new search for this project (queue it until the supply processing thread has returned) only if there's data processing
			if(data_processing)
			{
				SupplyQuery supply_query;

				// Create the GUID and make it look like we are a new gnutella client
				CoCreateGuid(&supply_query.m_guid);
				unsigned char *ptr=(unsigned char *)&supply_query.m_guid;
				ptr[8]=0xFF;
				ptr[15]=0x00;;
				
				supply_query.m_project_name=v_supply_projects[i].m_name;
				v_cached_supply_queries.push_back(supply_query);

				v_supply_projects[i].m_query_guid=supply_query.m_guid;
			}
			else	// there's no data processing, so just do the query
			{
				v_supply_projects[i].m_query_guid = p_connection_manager->PerformProjectSupplyQuery((char *)v_supply_projects[i].m_name.c_str());
//				v_supply_projects[i].m_calculate_supply_counter = 0;
			}

			// Create thread data object
			SupplyManagerThreadData *data=new SupplyManagerThreadData;
			data->m_hwnd=p_parent->m_dlg.GetSafeHwnd();
			data->m_project_name=v_supply_projects[i].m_name;
			data->m_guid=v_supply_projects[i].m_query_guid;
			data->m_start_time=CTime::GetCurrentTime();	// we are starting to take data for this project
			v_thread_data.push_back(data);
			*/
/*		}
/*
		// Check to see if the amount of time that we've been taking supply >= interval - (interval / # projects)
		// If so, then do that calculation for supply.  (This is because the next supply to be taken will be this one again
		if(v_supply_projects[i].m_calculate_supply_counter >= (v_supply_projects[i].m_interval - (v_supply_projects[i].m_interval/v_supply_projects.size())))
		{
			string proj_name = v_supply_projects[i].m_name;
			CalculateProjectSupply(proj_name);
		}
*/
/*	}
	m_minute_counter++;	// increment it for next time for the next heartbeat
}
*/
//
//
//
void SupplyManager::OnOneMinuteTimer()
{
/*
	if(m_second_counter>=300 || m_5_mins_passed) //won't take any supply data until 5 mins after the program starts up
	{
		if(!m_5_mins_passed)
		{
			m_second_counter=0;
			m_5_mins_passed=true;
		}
*/
		for(UINT i=0; i<v_supply_projects.size(); i++)
		{
			if(m_minute_counter % (v_supply_projects[i].m_interval) == v_supply_projects[i].m_offset)
			{
				//p_parent->StopGatheringProjectData(v_supply_projects[i].m_name);
				p_connection_manager->PerformProjectSupplyQuery((char *)v_supply_projects[i].m_name.c_str());
			}
		}
//	}
	m_minute_counter++;
}

/* =>_<=
//
//
//
void SupplyManager::QueryHitsReceived(char * project_name, vector<QueryHit> &hits)
{

	for(UINT i=0; i<v_thread_data.size(); i++)
	{
		if(strcmp(v_thread_data[i]->m_project_name.c_str(),project_name)==0)
		{
			for(UINT j=0; j<hits.size(); j++)
			{
				if(v_thread_data[i]->m_guid==hits[j].m_guid)
				{
					v_thread_data[i]->v_query_hits.push_back(hits[j]);
					v_thread_data[i]->v_ips.push_back(hits[j].m_ip);	// add the ip of this query hit to the thread data for the reverse dns
				}
			}

			break;
		}
	}

}

//
//
//
/*
UINT CalculateProjectSupplyThreadProc(LPVOID pParam)
{
	SupplyManagerThreadData *data=(SupplyManagerThreadData *)pParam;

	srand((unsigned int)time(NULL));	// seed random number generator for this thread

	for(UINT i=0; i<data->v_query_hits.size(); i++)
	{
		bool entry_found = false;
		for(UINT j=0; j<data->v_supply_entries.size(); j++)
		{
			if(strcmp(data->v_query_hits[i].Filename(),data->v_supply_entries[j].Filename())==NULL)
			{
				entry_found = true;

				bool size_found = false;
				for(UINT k=0; k<data->v_supply_entries[j].v_entry_sizes.size(); k++)
				{
					if(data->v_query_hits[i].m_file_size == data->v_supply_entries[j].v_entry_sizes[k].m_entry_size)
					{
						size_found = true;

						bool info_found = false;
						for(UINT l=0; l<data->v_supply_entries[j].v_entry_sizes[k].v_entry_infos.size(); l++)
						{
							if(strcmp(data->v_query_hits[i].Info(),data->v_supply_entries[j].v_entry_sizes[k].v_entry_infos[l].Info())==NULL)
							{
								info_found = true;

								data->v_supply_entries[j].v_entry_sizes[k].v_entry_infos[l].m_num_spoofs++;


								SupplyEntryLocation location;
								location.m_index = data->v_query_hits[i].m_file_index;
								location.m_ip = data->v_query_hits[i].m_ip;
								location.m_port = data->v_query_hits[i].m_port;
								location.m_speed = data->v_query_hits[i].m_speed;

								data->v_supply_entries[j].v_entry_sizes[k].v_entry_infos[l].v_entry_locations.push_back(location);

								break;
							}

						}

						if(info_found == false)
						{
							SupplyEntryLocation location;
							location.m_index = data->v_query_hits[i].m_file_index;
							location.m_ip = data->v_query_hits[i].m_ip;
							location.m_port = data->v_query_hits[i].m_port;
							location.m_speed = data->v_query_hits[i].m_speed;

							SupplyEntryInfo info;
							info.Info(data->v_query_hits[i].Info());
							info.m_num_spoofs = 1;
							info.v_entry_locations.push_back(location);

							data->v_supply_entries[j].v_entry_sizes[k].v_entry_infos.push_back(info);
						}
						break;
					}
				}

				if(size_found == false)
				{
					SupplyEntryLocation location;
					location.m_index = data->v_query_hits[i].m_file_index;
					location.m_ip = data->v_query_hits[i].m_ip;
					location.m_port = data->v_query_hits[i].m_port;
					location.m_speed = data->v_query_hits[i].m_speed;

					SupplyEntryInfo info;
					info.Info(data->v_query_hits[i].Info());
					info.m_num_spoofs = 1;
					info.v_entry_locations.push_back(location);

					SupplyEntrySize size;
					size.m_entry_size = data->v_query_hits[i].m_file_size;
					size.v_entry_infos.push_back(info);

					data->v_supply_entries[j].v_entry_sizes.push_back(size);
				}

				break;
			}
		}

		if(entry_found==false)
		{
			SupplyEntryLocation location;
			location.m_index = data->v_query_hits[i].m_file_index;
			location.m_ip = data->v_query_hits[i].m_ip;
			location.m_port = data->v_query_hits[i].m_port;
			location.m_speed = data->v_query_hits[i].m_speed;

			SupplyEntryInfo info;
			info.Info(data->v_query_hits[i].Info());
			info.m_num_spoofs= 1;
			info.v_entry_locations.push_back(location);

			SupplyEntrySize size;
			size.m_entry_size = data->v_query_hits[i].m_file_size;
			size.v_entry_infos.push_back(info);

			SupplyEntry new_entry;
			new_entry.Filename(data->v_query_hits[i].Filename());
			new_entry.v_entry_sizes.push_back(size);\

			data->v_supply_entries.push_back(new_entry);
		}
	}

	// Extract the ips that we are going to want to reverse DNS
	unsigned int count=(UINT)data->v_ips.size();
	
	// Check to see if count/500 == 0 or count/500==1 (in which case rand() % 0 === NaN and rand() % 1 === 0)
	if(count/500 < 2)
	{
		// They all pass
		data->v_ips_to_reverse_dns=data->v_ips;
	}
	else
	{
		// Only some of them will pass (<1000 of them)
		for(i=0;i<(UINT)count;i++)
		{
			if(rand() % (count/500) == 0)
			{
				data->v_ips_to_reverse_dns.push_back(data->v_ips[i]);
			}
		}
	}

	// Tell the supply manager that the data is ready
	PostMessage(data->m_hwnd,WM_SUPPLY_MANAGER_THREAD_DATA_READY,(WPARAM)data,(LPARAM)0);
	
	return 0;
}

//
//
//
void SupplyManager::CalculateProjectSupply(string &project_name)
{
	// Find the thread data object for this project and start a thread, passing thread ptr to data and removing ptr from the vector
	vector<SupplyManagerThreadData *>::iterator data_iter=v_thread_data.begin();
	while(data_iter!=v_thread_data.end())
	{
		if(strcmp((*data_iter)->m_project_name.c_str(),project_name.c_str())==0)
		{
			p_parent->m_log_window_manager.Log("Supply Manager : ",0x0000C0C0);	// dirty yellow
			p_parent->m_log_window_manager.Log("Starting CalculateProjectSupply Thread : ");
			p_parent->m_log_window_manager.Log((char *)project_name.c_str(),0x00FF0000,true);	// bold blue
			p_parent->m_log_window_manager.Log("\n");

			// We are done gathering data for this project
			(*data_iter)->m_end_time=CTime::GetCurrentTime();

			AfxBeginThread(CalculateProjectSupplyThreadProc,(LPVOID)(*data_iter),THREAD_PRIORITY_LOWEST);
			v_thread_data.erase(data_iter);
			break;
		}

		data_iter++;
	}
}

//
//
//
void SupplyManager::SupplyManagerThreadDataReady(WPARAM wparam,LPARAM lparam)
{
/* =>_<=
	SupplyManagerThreadData *data=(SupplyManagerThreadData *)wparam;

	// Find the correct SupplyProject object and replace it's SupplyEntry vector
	for(UINT i=0;i<v_supply_projects.size();i++)
	{
		if(strcmp(v_supply_projects[i].m_name.c_str(),data->m_project_name.c_str())==0)
		{
			v_supply_projects[i].v_spoof_entries=data->v_supply_entries;
			break;
		}
	}

	char msg[32];
	p_parent->m_log_window_manager.Log("Supply Manager : ",0x0000C0C0);	// dirty yellow
	p_parent->m_log_window_manager.Log("CalculateProjectSupply Thread Data Ready : ");
	p_parent->m_log_window_manager.Log((char *)data->m_project_name.c_str(),0x00FF0000,true);	// bold blue
	sprintf(msg," : %u ",data->v_supply_entries.size());
	p_parent->m_log_window_manager.Log(msg,0,true);	// bold black
	p_parent->m_log_window_manager.Log("Supply Entries\n");


	// Tell the reverse dns manager to perform the reverse dns of the reverse dns ips
	ReverseDNSData reverse_dns_data;
	reverse_dns_data.m_project=data->m_project_name;
	reverse_dns_data.v_ips=data->v_ips_to_reverse_dns;
	reverse_dns_data.m_start_time=data->m_start_time;
	reverse_dns_data.m_end_time=data->m_end_time;
	p_reverse_dns_manager->PerformReverseDNS(reverse_dns_data);
	
	// The dist manager will put dists into v_dist_entries
	p_dist_manager->ProjectSupplyUpdated(data->m_project_name);
	delete data;	// free memory


	// Perform the next query for this project, popping it off of the queue
	SupplyQuery query=*v_cached_supply_queries.begin();
	v_cached_supply_queries.erase(v_cached_supply_queries.begin());

	p_connection_manager->PerformProjectSupplyQuery((char *)query.m_project_name.c_str(),&query.m_guid,false);
}
=>_<= */

//
//
//
/*
void SupplyManager::ProjectSupplySorted(string &project_name)
{
	WriteProjectSupplyToFile(project_name);										// *&* This (might) lock up the gui
	p_parent->UpdateSupplyProjects(v_supply_projects);							// update the gui
	p_connection_manager->ProjectSupplyUpdated((char *)project_name.c_str());	// update the sockets
}
*/

//
//
//
/*
void SupplyManager::WriteProjectSupplyToFile(string &project_name)
{
	UINT i;
	for(i=0; i<v_supply_projects.size(); i++)
	{
		if(strcmp(v_supply_projects[i].m_name.c_str(), project_name.c_str())==NULL)
		{
			int buf_len = v_supply_projects[i].GetBufferLength();

			char * buf = new char[buf_len];

			v_supply_projects[i].WriteToBuffer(buf);

			CFile project_supply_file;
			char filename[128];
			strcpy(filename, "Supply Files\\");
			BOOL ret=MakeSureDirectoryPathExists(filename);

			if(ret==TRUE)
			{
				strcat(filename, project_name.c_str());
				strcat(filename, ".sup");
				BOOL open = project_supply_file.Open(filename, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary);

				if(open==TRUE)
				{
					project_supply_file.Write(buf, buf_len);
					project_supply_file.Close();
				}
			}

			delete [] buf;

			break;
		}
	}
}
*/
//
//
//
/*
void SupplyManager::ReadSupplyFromFiles()
{
	char *folder="Supply Files\\";
	string path;
	
	v_supply_projects.clear();

	WIN32_FIND_DATA file_data;
	path=folder;
	path+="*.sup";
	HANDLE search_handle = ::FindFirstFile(path.c_str(), &file_data);
	BOOL found = FALSE;

	if (search_handle!=INVALID_HANDLE_VALUE)
	{
		found = TRUE;
	}

	while(found == TRUE)
	{
		CFile supply_data_file;
		path=folder;
		path+=file_data.cFileName;
		BOOL open = supply_data_file.Open(path.c_str(),CFile::typeBinary|CFile::modeRead|CFile::shareDenyNone);

		if(open==TRUE)
		{
			unsigned char * supply_data =  new unsigned char[(UINT)supply_data_file.GetLength()];

			supply_data_file.Read(supply_data, (UINT)supply_data_file.GetLength());

			SupplyProject new_supply;
			new_supply.ReadFromBuffer((char*)supply_data);
			
			v_supply_projects.push_back(new_supply);

			supply_data_file.Close();

			delete [] supply_data;
		}

		found = ::FindNextFile(search_handle, &file_data);
	}
	
	::FindClose(search_handle);
}
*/