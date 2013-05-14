//WorkerGuy.cpp : implementation file
//

#include "stdafx.h"
#include "DistributedKazaaCollectorDll.h"
#include "WorkerGuy.h"
#include "DatFileEntry.h"

//#include "DBInterface.h"
//#include "ThreadData.h"
//#include "ProcessInfo.h"

/*
#define _CRTDBG_MAP_ALLOC		// For memory leak detection
#include <stdlib.h>
#include <crtdbg.h>
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WorkerGuy

BEGIN_MESSAGE_MAP(WorkerGuy, CWnd)
	//{{AFX_MSG_MAP(WorkerGuy)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// Constructor
WorkerGuy::WorkerGuy()
{
	m_number_of_projects_searched = 0;

	p_parent = NULL;
	m_last_project = false;

	m_state = 1; // Kazaa not started.  Each time the worker does it's last project, it closes kazaa, then
				 // will request on be fired up, and will need to request a free handle from
				 // the manager.

	// Open all the desktops...
	m_desk1 = OpenDesktop("Desktop1",DF_ALLOWOTHERACCOUNTHOOK,FALSE,GENERIC_ALL);
	m_desk2 = OpenDesktop("Desktop2",DF_ALLOWOTHERACCOUNTHOOK,FALSE,GENERIC_ALL);
	m_desk3 = OpenDesktop("Desktop3",DF_ALLOWOTHERACCOUNTHOOK,FALSE,GENERIC_ALL);

	Clear();
}

WorkerGuy::~WorkerGuy()
{
	Clear();

	CloseDesktop(m_desk1);
	CloseDesktop(m_desk2);
	CloseDesktop(m_desk3);
}

//
//
//
void WorkerGuy::InitParent(DistributedKazaaCollectorDll *parent)
{
	p_parent = parent;
}

void WorkerGuy::Clear()
{
	m_state=1;
	p_kazaa = NULL;
	m_kza_hwnd=NULL;
	m_process_id=NULL;
	m_process_handle=NULL;

	m_search_header_hwnd=NULL;
	m_search_tree_hwnd=NULL;
	m_search_now_button_hwnd=NULL;
	m_search_more_button_hwnd=NULL;
	m_search_more_auto_check_hwnd=NULL;
	m_search_edit_hwnd=NULL;
	m_search_video_radio_button_hwnd=NULL;
	m_search_audio_radio_button_hwnd=NULL;
	m_search_status_static_hwnd=NULL;

	m_first_hit = true;

	m_no_connect = 0;
	m_last_state_change = CTime::GetCurrentTime();
}

void WorkerGuy::SetId(int id)
{
	m_id = id;
}

//////////////////////////////////////////
//
// WorkerGuy trigger
//	Used to be OnTimer()
//
////
//void WorkerGuy::OnTimer(UINT nIDEvent)
void WorkerGuy::Trigger()
{
	if (::IsWindow(m_kza_hwnd) == 0)	// does our kazaa window even exist?
	{
		p_parent->Log("Trigger fired, NO KAZAA.  Freeing worker");
		CloseKaZaA();
		return;
	}

//	if (p_parent->m_minimize)
//		::CloseWindow(m_kza_hwnd);

	// On any given iteration we want this to be our day:
	char today[4096];
	CTime now=CTime::GetCurrentTime();
	sprintf(today,"%04u-%02u-%02u",now.GetYear(),now.GetMonth(),now.GetDay());

	// Location for raw data dump... localhost or the funnel-down database:
	CString db_ip = "38.119.64.69";
	CString db_login = "onsystems";
	CString db_pass = "ebertsux37";
	CString db_name = "mediadefender";

	// Marketing DB will be Timmy until determined otherwise: (for OtherArtists, and SearchedMoreIps tables)
	CString marketing_db_ip = "38.119.66.31";
	CString marketing_db_login = "root";
	CString marketing_db_pass = "ebertsux37";
	CString marketing_db_name = "mediadefender";

	// Watchdog check:
	CTimeSpan tspan = CTime::GetCurrentTime() - m_last_state_change;
	if (tspan.GetMinutes() > 10)
	{
		TRACE("WATCHDOG FIRED\n");
		p_parent->m_dlg.Log("WATCHDOG WARNING - 10 minutes passed without a state change. Resetting.");

		// Actually close the kazaa...
		::PostMessage(m_kza_hwnd, WM_COMMAND, 0x00008066,0);

		CloseKaZaA();
		return;
	}

	switch(m_state)
	{
		case 1:	// Make sure we're clean, and get a new kazaa (new supernode)
		{
			// Do we have a new kazaa? If yes, state change...
			if (m_kza_hwnd != NULL)
			{	
//				p_parent->m_dlg.Log("1 - Got new Kazaa, changing to state 2");
				m_state++;
				m_last_state_change = CTime::GetCurrentTime();
			}
			else
			{
				p_parent->m_dlg.Log("1 - No Kazaa Window Handle - Killing Timer");
				
//				p_parent->m_dlg.KillTimer(0);
//				p_parent->m_searching = false;
				CloseKaZaA();
				return;
			}

			break;

		}

		// We can come directly to this state if we don't want to reconnect, perhaps we just want to
		// get a new project.
		case 2:
		{
			// Check to see if it is connected
			if(IsConnected())
			{
//				p_parent->m_dlg.Log("2 - KaZaA Connected");
				m_state++;
				m_last_state_change = CTime::GetCurrentTime();
			}
			else
			{
				TRACE("Not connected...\n");
				m_last_state_change = CTime::GetCurrentTime();

				// Check to see if Kazaa is hosed, probably the supernode key in the registry
				m_no_connect++;
				if (m_no_connect > 20)
				{
//					p_parent->m_dlg.Log("State 2 - Not Connecting - Killing Timer");
//					p_parent->m_dlg.KillTimer(0);
//					p_parent->m_searching = false;
					CloseKaZaA();
					return;
				}
				
				// what is this for?
//				if (p_kazaa == NULL)
//				{
//					p_kazaa = p_parent->GetFreeKazaa(m_id);
//					if (p_kazaa != NULL)
//						m_kza_hwnd = p_kazaa->GetSafeHwnd();
//				}

			}
			break;
		}

		case 3:	// Press New Search (clear everything out, get ready for searching)
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			// New Search
			NewSearch();

			p_parent->m_dlg.Log("1 - Clicked New Search");

			m_state++;
			m_last_state_change = CTime::GetCurrentTime();
			break;
		}

		case 4:	// Find windows
		{
			TRACE("worker %u in state %u\n", m_id, m_state);

			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("2 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			FindWindows();

			if(AreWindowsFound())
			{
				p_parent->m_dlg.Log("2 - All Windows Found");
				m_state++;
				m_last_state_change = CTime::GetCurrentTime();
			}
			break;
		}
		case 5:	// Press SearchBackStatic() before doing the new search
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("3 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			// If the search back static is visible, click on it before performing the new search
			if(PressSearchBackStatic())
			{
				p_parent->m_dlg.Log("3 - Pressing Search Back Static");
			}
			else
			{
				p_parent->m_dlg.Log("3 - NOT Pressing Search Back Static");
			}

			m_state++;
			m_last_state_change = CTime::GetCurrentTime();
			break;
		}

		case 6:	// Press a radio button (Audio, Video or Software)
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("4 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}	


			// ***
			//
			//	Use either GetNextProjectSerial()  OR  GetNextProject() - NEVER BOTH =)
			//
			// ***

/*
			// Spreads the projects across all kazaas, and sets last_project = true for all workers when the last project is reached.
			m_current_project = p_parent->GetNextProjectSerial();

			if (m_last_project && (strcmp(m_current_project.m_project_name.c_str(), "")==0) )
			{
				// This would mean we reached the last project, and the *LAST* project was no good.
				m_last_project = false;
				m_current_project.Clear();

				p_parent->m_dlg.Log("6 - Last Project - Killing Timer");

				p_parent->m_dlg.KillTimer(0);
				p_parent->m_searching = false;

				m_state = 99;
				break;
			}
*/
			// Each worker gets every project, then resets when it reaches the end.
			m_current_project = p_parent->GetNextProject(m_current_project, this);

			CString search_string = m_current_project.m_supply_keywords.m_search_string.c_str();

			// Check to see if we need to restrict this search to a specific file type
			if(m_current_project.m_search_type == ProjectKeywords::search_type::audio)
			{
				p_parent->m_dlg.Log("4 - Pressing AUDIO Radio Button");
				PressAudioRadioButton();
			}
			else if(m_current_project.m_search_type == ProjectKeywords::search_type::video)
			{
				// If there are ZERO tracks, then this is a movie.
				p_parent->m_dlg.Log("4 - Pressing VIDEO Radio Button");
				PressVideoRadioButton();
			}
			else if(m_current_project.m_search_type == ProjectKeywords::search_type::software)
			{
				p_parent->m_dlg.Log("4 - Pressing SOFTWARE Radio Button");
				PressSoftwareRadioButton();
			}
			else
			{
				// Default case
				p_parent->m_dlg.Log("4 - Searching will be for EVERYTHING");
			}

			m_state++;
			m_last_state_change = CTime::GetCurrentTime();
			break;
		}

		case 7:	// Search options?
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("5 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			// Check to see if a there needs to be a search option instead of just the general search by file type
			if(m_current_project.m_search_type != ProjectKeywords::search_type::none)
			{
				// Press the More Search Options static to expand the more specific search terms
				p_parent->m_dlg.Log("5 - Pressing More Search Options Static");
				PressMoreSearchOptionsStatic();
			}
			else
			{
				p_parent->m_dlg.Log("5 - Searching will be without any Search Options");
			}

			m_state++;
			m_last_state_change = CTime::GetCurrentTime();
			break;
		}

		case 8:	// Click option (Title, Artist, Album, Size, etc...)
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("6 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			// Check to see if we are going to be searching with a search option
			bool do_artist_search = false;
			if (do_artist_search)
//			if( strlen(m_current_project.m_artist_name.c_str()) > 1)
			{
				// Press the More Search Options Artist Button to expand the more specific search terms
				p_parent->m_dlg.Log("6 - Pressing More Search Options Artist Button");
				PressMoreSearchOptionsArtistButton();
			}
			else 
			{
				p_parent->m_dlg.Log("6 - No Options Selected");
			}

			m_state++;
			m_last_state_change = CTime::GetCurrentTime();
			break;
		}

		case 9:		// Type the Artist search string, or just regular search
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("7 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			char search_string[1024];
			memset(search_string, 0, sizeof(search_string));
			strcpy(search_string, m_current_project.m_supply_keywords.m_search_string.c_str());
			strupr(search_string);	// make uppercase

			// Check to see if we are going to be searching with a search option
			char msg[1024];

			bool do_artist_search = false;
//			if(strlen(m_current_project.m_artist_name.c_str()) > 1)
			if (do_artist_search)
			{
				sprintf(msg,"%u - Typing Artist Search String : %s",m_state, search_string);
				p_parent->m_dlg.Log(msg);
				TypeArtistSearchString(search_string);
			}
			else
			{
				sprintf(msg,"%u - Typing Search String : %s", m_state, search_string);
				p_parent->m_dlg.Log(msg);
				TypeSearchString(search_string);
			}

			m_state++;
			m_last_state_change = CTime::GetCurrentTime();
			break;
		}

		case 10:	// Press search now button
		{
			// Make sure we are connected
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("10 - NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			// Check to make sure Auto Search More is DISABLED
			if (IsAutoSearchEnabled())
			{
				ClickAutoSearchCheck();
			}

			// re-initialize
			m_search_more_count=0;

			p_parent->m_dlg.Log("10 - Pressing Search Now Button");
			SearchNow();
			m_no_connect = 0;
			m_state++;
			m_last_state_change = CTime::GetCurrentTime();

			// Slow Down the timer for the search more's
//			p_parent->m_dlg.KillTimer(0);
//			p_parent->m_dlg.SetTimer(0, 1000 * m_search_more_interval, NULL);

			m_last_search_more = CTime::GetCurrentTime();

			break;
		}

		case 11:	// Pressing search more
		{
			CTimeSpan ts;
			ts = CTime::GetCurrentTime() - m_last_search_more;
			if (ts.GetTotalSeconds() < m_search_more_interval)
				break;
	
			// Make sure we are connected
			if(IsConnected()==false)
			{
				m_no_connect++;
				if (m_no_connect > 20)
				{
					p_parent->m_dlg.Log("9 - Disconnected In Search More");
					CloseKaZaA();
					m_state=99;
					return;
				}


				// Speed the timer back up
//				p_parent->m_dlg.KillTimer(0);
//				p_parent->m_dlg.SetTimer(0, 300, NULL);
				break;

			}

			// Search More Limit - set by user in the GUI
			unsigned int search_more_limit = p_parent->m_search_more_count;

			if(IsSearchMoreEnabled())
			{
				if((++m_search_more_count)>search_more_limit)
				{
					m_state++;
					m_last_state_change = CTime::GetCurrentTime();
				
					// Speed the timer back up
//					p_parent->m_dlg.KillTimer(0);
//					p_parent->m_dlg.SetTimer(0, 300, NULL);
				}
				else
				{
					char msg[1024];
					sprintf(msg,"9 - Pressing Search More Button : %u / %u",m_search_more_count,search_more_limit);
					p_parent->m_dlg.Log(msg);

					SearchMore();
					m_no_connect = 0;
					m_last_state_change = CTime::GetCurrentTime();
					m_last_search_more = CTime::GetCurrentTime();
				}
			}
			break;
		}

		case 12:	// Extract search results
		{
			// *****
			// **
			// *    It may not be necessary to be connected when extracting the results...
			// **
			// *****
			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("10 - Would extract, but... NOT CONNECTED - RESETTING");
				CloseKaZaA();
				return;
			}

			p_parent->m_dlg.Log("10 - Extracting Search Results");

			// Get the process_id for this Kazaa window, extract search results from its TreeView.
			GetWindowThreadProcessId(m_kza_hwnd,&m_process_id);
			vector<SearchResult> results=ExtractSearchResults();

			// Check for required and kill words here...
			// Remove any items that have the kill words in them, and don't have the required words
			vector<SearchResult> filtered;
			for(unsigned int i=0;i<results.size();i++)
			{
				CString search_str;
				search_str = results[i].m_filename.c_str();
				search_str.Append(results[i].m_title.c_str());
				search_str.Append(results[i].m_album.c_str());
				search_str.Append(results[i].m_artist.c_str());
				search_str.MakeLower();

				// Check for keywords, and record track number
				bool has_all_keywords = false;

				if (m_current_project.m_supply_keywords.v_keywords.size() == 0)
				{
					has_all_keywords = true;
				}
				else
				{
					for (unsigned int track_num=0; track_num < m_current_project.m_supply_keywords.v_keywords.size(); track_num++)	// tracks
					{	
						UINT num_words = (UINT)m_current_project.m_supply_keywords.v_keywords[track_num].v_keywords.size();
						int found_count = 0;

						for (UINT k=0; k < num_words; k++)	// keywords in track
						{
							CString keyword = m_current_project.m_supply_keywords.v_keywords[track_num].v_keywords[k];
							CString filename = results[i].m_filename.c_str();
							keyword.MakeLower();
							filename.MakeLower();
							if ( (strstr(filename, keyword) != NULL) /*&& ( (strstr(search_str, m_current_project.m_project_name.c_str())!=NULL) ||  strstr(search_str, m_current_project.m_supply_keywords.m_search_string.c_str())!=NULL)*/ )
							{
								found_count++;
								if (found_count == num_words)
								{
									has_all_keywords = true;
									int this_track_num = m_current_project.m_supply_keywords.v_keywords[track_num].m_track;
									results[i].m_track_number = this_track_num;	// tracks are not zero based, so + 1
								}

							}
						}
					}
				}

				// Check for killwords
				bool has_a_killword = false;
				for (unsigned j=0; j < m_current_project.m_supply_keywords.v_killwords.size(); j++)
				{	
					CString killword = m_current_project.m_supply_keywords.v_killwords[j].v_keywords[0];
					killword.MakeLower();

					if (strstr(search_str, killword) != NULL)
					{
						has_a_killword = true;
						break;
					}
				}

				if (!has_a_killword && has_all_keywords)
					filtered.push_back(results[i]);
			}

			results=filtered;

/* This is taken care of in the processing dll now
			// We only want to mark projects for swarming if they are REGULAR swarming projects, 'uber catalog'
			// and SPECIAL projects get processed and added to the hashcount table independently of the m_poisoning_enabled flag
			bool regular_swarm = false;
			CString owner = m_current_project.m_owner.c_str();
			owner.MakeLower();
			if (m_current_project.m_fasttrack_swarming_enabled &&		// must be a swarming project
				m_current_project.v_tracks_hash_counts.size() == 0 &&	// not a 'SPECIAL' project, so, size() == 0
				strstr(owner, "uber catalog") == NULL &&				// not an 'UBER CATALOG' project, so, == NULL
				strstr(owner, "monthly catalog") == NULL )				// not a 'MONTHLY CATALOG' project
			{
				regular_swarm = true;
			}
*/
			//
			// This section of code is for actaully recording the supply
			//
			ThreadData *data = new ThreadData;	// This be freed by the thread / function
			data->v_results = results;
			data->m_hwnd = NULL;
			data->m_project = m_current_project.m_project_name.c_str();
			data->m_table_name = "";			// used to be "raw" now it is the project name and is created in LogResults()
			data->m_worker_id = m_id;
			data->m_db_name = db_name;
			data->m_db_pass = db_pass;
			data->m_db_ip = db_ip;
			data->m_db_login = db_login;
//			data->m_swarming = regular_swarm;

			// *****
			// ***   Insert the raw data
			// *****
			if (results.size() > 0)
				LogResults(data);
			
			// The project list update
			p_parent->IncProjectSearch(m_current_project.m_project_name.c_str(),(int)results.size());
			m_number_of_projects_searched++;

			// The total search results update
			char str[2048];
			p_parent->m_search_returns += (long)results.size();
			ltoa(p_parent->m_search_returns, str, 10);
			p_parent->m_dlg.GetDlgItem(IDC_STATIC_SEARCH_RETURNS)->SetWindowText(str);

			// Total searches update
			p_parent->m_total_searches++;
			itoa(p_parent->m_total_searches, str, 10);
			p_parent->m_dlg.GetDlgItem(IDC_STATIC_TOTAL_SEARCHES)->SetWindowText(str);


/*	
					***																	  ***
					***		Processing is done by the KazaaSupplyProcessor plug-in now	  ***
					***																	  ***

			// Are we already processing the raw data table?
			if (p_parent->m_processing == false)
			{
				p_parent->m_db.OpenConnection(m_current_project.m_ip, m_current_project.m_login, m_current_project.m_pass, m_current_project.m_db);

				// Do we see raw data from yesterday?  and can we get the lock?
				int ret;
				ret = p_parent->m_db.NeedToProcess(m_current_project.m_table);

				if (ret == 0)	// we got the lock and there is old stuff
				{
					ThreadData *pdata = new ThreadData;
					pdata->m_hwnd = p_parent->m_dlg_hwnd;
					pdata->m_table_name = m_current_project.m_table;
					pdata->m_db_name = m_current_project.m_db;
					pdata->m_db_pass = m_current_project.m_pass;
					pdata->m_db_ip = m_current_project.m_ip;
					pdata->m_db_login = m_current_project.m_login;

					::PostMessage(p_parent->m_dlg_hwnd,WM_PROCESS_RAW_DATA, (WPARAM)pdata, (LPARAM)0); // Start the processing - opens a new database connection
				}
				else if (ret == 1)
				{
					p_parent->m_dlg.Log("12 - *** Could not get 'processing_lock' from database ***");
				}

				p_parent->m_db.CloseConnection();	// the temporary lock will be released when we close this connection, then the thread will get it.
			}

			// end of raw data recording & processing
*/

			if (m_current_project.m_find_more)

			{
				// Find the first non-zero -lan ip- and do a find more from user if we haven't seen it before (this week)
				unsigned int ip=0;
				HTREEITEM item=NULL;

				p_parent->m_db.OpenConnection(marketing_db_ip, marketing_db_login, marketing_db_pass, marketing_db_name);
				for(i=0; i < results.size(); i++)
				{
					ip=results[i].m_ip;
					if((ip!=0)) // && (((ip>>24)&0xFF)!=10) && ((((ip>>24)&0xFF)!=192)&&(((ip>>1192)&0xFF)!=168))
					{
						// Check to see if we've found more from this user yet
						bool found=false;

						// ip to string for database
						char char_ip[32];
						sprintf(char_ip, "%u.%u.%u.%u",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip>>0)&0xFF);

						if (p_parent->m_db.IsNewIp(m_current_project.m_project_name.c_str(), char_ip, today))
						{
							item=results[i].m_hTreeItem;
							break;
						}
					} // end if
				} // enf for
				p_parent->m_db.CloseConnection();

				// Check to see that we found an item
				if((ip==0) || (item==NULL))
				{
					p_parent->m_dlg.Log("12 - Could not find a new user to find more from");

					if (m_last_project)
					{
						m_state=99;
					}
					else
					{
						m_state = 2;
						m_last_state_change = CTime::GetCurrentTime();
					}
					break;
				}

				char msg[1024];
				memset(msg, 0, sizeof(msg));

				sprintf(msg,"10 - Find More From User - %s - %u.%u.%u.%u",results[i].m_user.c_str(),(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip>>0)&0xFF);
				p_parent->m_dlg.Log(msg);
				FindMoreFromSameUser(item);
				m_state++;
				m_last_state_change = CTime::GetCurrentTime();
			}
			else	// We DO NOT want to find more, start over...
			{
				if (m_last_project)
				{
					m_state = 99;
				}
				else
				{
					m_state = 3;	// don't close, just get a new project
					m_last_state_change = CTime::GetCurrentTime();
				}
			}	
			break;
		}

		case 13:	// Get more from same user
		{
 			TRACE("worker %u in state %u\n", m_id, m_state);

			// Make sure we have a kazaa window
			if (::IsWindow(m_kza_hwnd) == 0)
//			if(IsConnected()==false)
			{
				p_parent->m_dlg.Log("11 - NO WINDOW TO EXTRACT FROM - RESETTING");
				CloseKaZaA();
				return;
			}

			if(IsSearching() == true) // we're not done searching yet... wait.
				break;

			// Extract the find more from same user data
			p_parent->m_dlg.Log("11 - Extracting (Find More From User) Results");
			vector<SearchResult> results=ExtractSearchResults();

			// Check to see if nothing came back
			if(results.size()==0)
			{
				p_parent->m_dlg.Log("11 - Search Finished Without Any Result");
			}
			else
			{

				//
				// This is FIND MORE DATA, a.k.a. for the Marketing Intelligence Database
				//
				ThreadData *data = new ThreadData; // This be freed by the thread
				data->v_results = results;
				data->m_hwnd = NULL;
				data->m_project = m_current_project.m_project_name.c_str();
				data->m_table_name = "OtherArtists";
				data->m_worker_id = m_id;
				data->m_db_name = marketing_db_name;
				data->m_db_pass = marketing_db_pass;
				data->m_db_ip = marketing_db_ip;
				data->m_db_login = marketing_db_login;
				data->m_week_number = m_week_number;	// This may be unnecessary now...

				// FIND MORE DISABLED TEMPORARILY 
				//
				// to turn on, get the FIND MORE DUMP procedure from KazaaSupplyTakerDlg.cpp in the KazaaSupplyTaker project.
				// Start a new thread to insert the find more results
//				::PostMessage(p_parent->m_dlg_hwnd,WM_FIND_MORE_DUMP, (WPARAM)data,(LPARAM) 0);
				
				SendFindMore(data);

				// ip to string for database
				char char_ip[32];
				unsigned int ip=results[0].m_ip;
				int file_count = (int)results.size();

				sprintf(char_ip, "%u.%u.%u.%u",(ip>>24)&0xFF,(ip>>16)&0xFF,(ip>>8)&0xFF,(ip>>0)&0xFF);
				p_parent->m_db.OpenConnection(marketing_db_ip, marketing_db_login, marketing_db_pass, marketing_db_name);
				p_parent->m_db.UpdateFileCount(m_current_project.m_project_name.c_str(), char_ip, today, file_count);
				p_parent->m_db.CloseConnection();
			}

			if (m_last_project)
			{
				m_state = 99;
			}
			else
			{
				m_state = 2;	// don't close, just get a new project
				m_last_state_change = CTime::GetCurrentTime();
			}

			break;
		}
		case 99:	// Closing case
		{
			TRACE("worker %u in state %u\n", m_id, m_state);

			Clear();

			if (m_last_project)
			{
				p_parent->Log("Last Project - Freeing that worker guy");
				p_parent->FreeWorker(this);
				return;
/*
				p_parent->m_dlg.KillTimer(0);
				p_parent->m_searching = false;
				m_last_project = false;

				// We may need to get rid of this... so all projects get searched equally...
				m_current_project.Clear();  // that was the last project, so start over...
*/
			}
			else
			{
				p_parent->m_dlg.Log("State 99 - Doing next project?");
			}

			break;
		}

		default:
		{
			p_parent->m_dlg.Log("FATAL ERROR - IN DEFAULT CASE IN SWITCH - INVALID STATE");
			break;
		}
	}
}

//
//
//
bool WorkerGuy::IsSearchNowEnabled()
{
	if((::IsWindowVisible(m_search_now_button_hwnd)==false) || (::IsWindowEnabled(m_search_now_button_hwnd)==false))
	{
		return false;
	}
	else
	{
        return true;
	}
}

//
//
//
void WorkerGuy::FindMoreFromSameUser(HTREEITEM item)
{

//	::SendMessage(m_search_tree_hwnd,TVM_SELECTITEM,TVGN_CARET,(LPARAM)item);	
	// Select the item (and scroll it into view) so I can get the rect
	if(::SendMessageTimeout(m_search_tree_hwnd,TVM_SELECTITEM,(WPARAM)TVGN_CARET,(LPARAM)item,SMTO_ABORTIFHUNG||SMTO_BLOCK,5000,NULL) == 0)
	{
		p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::FindMoreFromSameUser() #1 - KAZAA IS HUNG");
		return;
	}

	// Get the item's rect so that I can click on it
	CRect rect;
	memset(&rect,0,sizeof(CRect));
	void *ptr=AllocateMemory(sizeof(CRect));
	*((HTREEITEM*)&rect)=item;
	WriteMemory(ptr,(char *)&rect,sizeof(CRect));

//	::SendMessage(m_search_tree_hwnd,TVM_GETITEMRECT,(WPARAM)TRUE,(LPARAM)ptr);
	if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETITEMRECT,(WPARAM)TRUE,(LPARAM)ptr,SMTO_ABORTIFHUNG||SMTO_BLOCK,5000,NULL) == 0)
	{
		p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::FindMoreFromSameUser() #2 - KAZAA IS HUNG");
		return;
	}

	ReadMemory(ptr,(char *)&rect,sizeof(CRect));
	FreeMemory(ptr);

	// Click on the item
	DWORD pos=(rect.left)+(rect.top<<16);
	::PostMessage(m_search_tree_hwnd,WM_LBUTTONDOWN,(WPARAM)MK_LBUTTON,(LPARAM)pos);

	::PostMessage(m_search_tree_hwnd,WM_LBUTTONUP,(WPARAM)0,(LPARAM)pos);

	// *** This Sleep() IS necessary ***
	Sleep(1000);	// keeps kazaa from kicking its ass...

	// Find more from this user
	::PostMessage(m_kza_hwnd,WM_COMMAND,0x000080dc,0);		
}

//
//
//
bool WorkerGuy::IsSearching()
{

	HWND child = ::GetWindow(m_kza_hwnd, GW_CHILD);

	while(child!=NULL)
	{
		char buf[1024];
		memset(buf,0,sizeof(buf));
		GetClassName(child,buf,sizeof(buf));
		if(strcmp(buf,"msctls_statusbar32")==0)
		{
			// If the third item in the status bar is "Not sharing any files" then we are connected. (+1 for NULL)
			DWORD size;
			memset(buf, 0, sizeof(buf));
			if(::SendMessageTimeout(child,SB_GETTEXT,(WPARAM)0,(LPARAM)buf,SMTO_ABORTIFHUNG||SMTO_BLOCK,5000,&size) == 0)
			{
				p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::IsSearching() #1 - KAZAA IS HUNG");
				return false;
			}
			else
			{
				// Are we still searching? [strlen() of "Performing search, please wait..."]
				if (size == 33)
					return true;
				else
					return false;
			}
		}
		else
		{
			child = ::GetWindow(child, GW_HWNDNEXT);
		}
	}

	ASSERT(1 == 2);	// we should never get here.
	return false;
}

//
//
//
void WorkerGuy::ReadInPreviouslyFoundHosts()
{
	v_find_more_hosts.clear();

	// Load previously found hosts
	CFile file;
	if(file.Open("out\\hosts.dat",CFile::typeBinary|CFile::modeRead|CFile::shareDenyNone)==FALSE)
	{
		p_parent->m_dlg.Log("Error opening hosts.dat");
	}
	else
	{
		unsigned int ip;
		while(file.Read(&ip,sizeof(unsigned int))==sizeof(unsigned int))
		{
			v_find_more_hosts.push_back(ip);
		}
		file.Close();
		char msg[1024];
		sprintf(msg,"Read in %u previously found hosts.",v_find_more_hosts.size());
		p_parent->m_dlg.Log(msg);
	}
}

//
//
//
void WorkerGuy::WriteOutPreviouslyFoundHosts()
{
	CFile file;
	file.Open("out\\hosts.dat",CFile::modeCreate|CFile::typeBinary|CFile::modeWrite|CFile::shareDenyNone);
	for(unsigned int i=0;i<v_find_more_hosts.size();i++)
	{
		file.Write(&v_find_more_hosts[i],sizeof(unsigned int));
	}
	file.Close();
}

//
//
//
vector<string> WorkerGuy::ExtractColumns()
{
	unsigned int i;
	vector<string> columns;

	// Get the number of columns in the search return
//	unsigned int num_columns=(unsigned int)::SendMessage(m_search_header_hwnd,HDM_GETITEMCOUNT,0,0);
	DWORD num_columns;
	::SendMessageTimeout(m_search_header_hwnd,HDM_GETITEMCOUNT,(WPARAM)0,(LPARAM)0,SMTO_ABORTIFHUNG||SMTO_BLOCK,5000,&num_columns);
	if(num_columns == 0)
	{
		p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractColumns() #1 - KAZAA IS HUNG");
		return columns;
	}

	// Read the column titles
	char buf[sizeof(HDITEM)+1024];
	char *ptr=(char *)AllocateMemory(sizeof(buf));

	for(i=0;i<num_columns;i++)
	{
		memset(buf,0,sizeof(buf));
		HDITEM *hditem=(HDITEM *)buf;

		hditem->mask=HDI_TEXT;
		hditem->cchTextMax=1024;
		hditem->pszText=ptr+sizeof(HDITEM);

		WriteMemory(ptr,buf,sizeof(buf));
//		::SendMessage(m_search_header_hwnd,HDM_GETITEM,(WPARAM)i,(LPARAM)ptr);	
		if(::SendMessageTimeout(m_search_tree_hwnd,HDM_GETITEM,(WPARAM)i,(LPARAM)ptr,SMTO_ABORTIFHUNG||SMTO_BLOCK,5000,NULL) == 0)
		{
			p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::FindMoreFromSameUser() #1 - KAZAA IS HUNG");
			columns.clear();
			return columns;
		}

		ReadMemory(ptr,buf,sizeof(buf));

		columns.push_back(&buf[sizeof(HDITEM)]);
	}

	FreeMemory(ptr);

	return columns;
}

//
//
//
vector<SearchResult> WorkerGuy::ExtractSearchResults()
{
	unsigned int i;

	LRESULT res;
	vector<SearchResult> results;

	// Extract the column strings and figure out the header types
	vector<string> column_strings=ExtractColumns();
//	vector<int> columns=SearchResult::ProcessColumnHeaders(column_strings);	

	// If num column_strings < 10 then there were no results (used to be columns.size())
	if(column_strings.size()<10)
	{
		return results;
	}
	
	// Get the root item of the tree

	// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
	if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,(WPARAM)TVGN_ROOT,(LPARAM)0,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
	{
		p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #1 - KAZAA IS HUNG");
		return results;
	}
	HTREEITEM root=(HTREEITEM)res;

	HTREEITEM item=root;

	char buf[sizeof(TVITEM)+1024];
	char *ptr=(char *)AllocateMemory(sizeof(buf));

	bool is_child=false;
	while(1)
	{
		bool skip = false;

		// Expand this item - not needed
//		res=::SendMessage(m_search_tree_hwnd,TVM_EXPAND,(WPARAM)TVE_EXPAND,(LPARAM)item);

		memset(buf,0,sizeof(buf));

		// Get item data
		TVITEM *tv=(TVITEM *)buf;
		tv->mask=TVIF_HANDLE|TVIF_TEXT|TVIF_IMAGE|TVIF_PARAM|TVIF_STATE;
		tv->hItem=item;
		tv->cchTextMax=sizeof(buf)-sizeof(TVITEM);
		tv->pszText=ptr+sizeof(TVITEM);	// points after the TVITEM structure
		tv->state=0;
		tv->stateMask=TVIS_SELECTED;

		WriteMemory(ptr,buf,sizeof(TVITEM));

		// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
		if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETITEM,(WPARAM)0,(LPARAM)ptr,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
		{
			p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #2 - KAZAA IS HUNG");
			break;
		}
		ReadMemory(ptr,buf,sizeof(TVITEM));

		// Make a copy of the TVITEM
		TVITEM tv2;
		memcpy(&tv2,buf,sizeof(TVITEM));
		memset(buf,0,sizeof(buf));

		// Read the item string
		ReadMemory((void *)tv2.lParam,buf,sizeof(buf));

		unsigned int num_read_1=0;
		unsigned int num_read_2=0;
		unsigned int num_read_3=0;
		char buf1[sizeof(buf)];
		char buf2[sizeof(buf)];
		char buf3[sizeof(buf)];
		memset(buf1,0,sizeof(buf1));
		memset(buf2,0,sizeof(buf2));
		memset(buf3,0,sizeof(buf3));

		// Get the text (0xb6 is the separator)
		char text[1024];
		memset(text,0,sizeof(text));
		void *addr;
		memcpy(&addr,buf,sizeof(void *));
		num_read_1=ReadMemory(addr,text,sizeof(text));

		if (num_read_1 == 0)
			skip = true;

		memcpy(buf1,buf,sizeof(buf1));

		// Check to see what kind of item this is so that we know how to get the ip and port
		bool is_parent=false;
		bool has_children=false;

		// See if this item is a parent (it has no parent)

		// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
		if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_PARENT,(LPARAM)item,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
		{
			p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #3 - KAZAA IS HUNG");
			break;
		}
		if(res==0)
		{
			is_parent=true;
			
			// See if this item has children

			// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
			if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_CHILD,(LPARAM)item,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
			{
				p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #4 - KAZAA IS HUNG");
				break;
			}
			if(res!=0)
			{
				has_children=true;
			}
		}

		// Based on what we know about this item, get the ip and port
		SearchResult result;
		result.m_hTreeItem=item;

		char username[69];
		char filename[256];

		unsigned int num_meta=0;

		// If it is a parent with no children, then get the ip and port one way
		unsigned int offset=0;
		if(is_parent && (has_children==false))
		{
			offset=0x0c;
			memcpy(&addr,&buf[0x11],4);
			num_read_2=ReadMemory(addr,buf,sizeof(buf));

			if (num_read_2 == 0)
				skip = true;

			memcpy(buf2,buf,sizeof(buf2));

			// See if it is audio or video
			result.m_media_type=*((unsigned int *)&buf[0x08]);
		}
		else if(is_parent==false)
		{
			// First read in the parent to see if it is audio or video
			char av[sizeof(buf)];
			memcpy(&addr,&buf[0x11],4);
			num_read_2=ReadMemory(addr,av,sizeof(av));
			memcpy(buf2,buf,sizeof(buf2));
			result.m_media_type=*((unsigned int *)&av[0x08]);

			// Now get the stuff for the children
			memcpy(&addr,&buf[0x15],4);
			num_read_3=ReadMemory(addr,buf,sizeof(buf));

			if (num_read_3 == 0)
				skip = true;

			memcpy(buf3,buf,sizeof(buf3));
		}

		// Save this search result item, unless it is the top of a group
		if((is_parent && has_children)==false)
		{
			// Filename
			memcpy(&addr,&buf[0x04+offset],4);
			memset(filename,0,sizeof(filename));
			ReadMemory(addr,filename,sizeof(filename));
			result.m_filename=filename;

			// Username
			memcpy(&addr,&buf[0x18+offset],4);
			memset(username,0,sizeof(username));
			ReadMemory(addr,username,sizeof(username));
			result.m_user=username;

			// Copy the exact size in bytes
			memcpy(&result.m_size,&buf[0x08+offset],4);

			// Copy ip and port
			memcpy(&result.m_ip,&buf[0x10+offset],4);
			memcpy(&result.m_port,&buf[0x10+4+offset],4);

			// Get the number of metadata things
			memcpy(&num_meta,&buf[0x21+offset],4);

			// Get to the pointers
			memcpy(&addr,&buf[0x25+offset],4);
			ReadMemory(addr,buf,sizeof(buf));

			if(num_meta>50)
			{
				// This is bad...skip = true already because ReadMemory() should have returned 0
				skip = true; // just in case
			}

			if (skip==false)
			{
				// Find the correct type of data pointer
				char *ptr=buf;
				for(i=0;i<num_meta;i++)
				{
					unsigned int type=*((unsigned int *)ptr);
					unsigned int len=*((unsigned int *)(ptr+8));
					ptr+=4;
					
					if(type==3)	// Hash
					{
						memcpy(&addr,ptr,4);	// md5 hash
						ReadMemory(addr,result.m_hash,sizeof(result.m_hash));
					}				
					else if(type==8)	// Album
					{
						char *mem=new char[len+1];
						memset(mem,0,len);
						memcpy(&addr,ptr,4);	// album
						ReadMemory(addr,mem,len);
						result.m_album=mem;
						delete [] mem;
					}
					else if(type==4)	// Title
					{
						char *mem=new char[len+1];
						memset(mem,0,len);
						memcpy(&addr,ptr,4);	// album
						ReadMemory(addr,mem,len);
						result.m_title=mem;
						delete [] mem;
					}
					else if(type==6)	// Artist
					{
						char *mem=new char[len+1];
						memset(mem,0,len);
						memcpy(&addr,ptr,4);	// album
						ReadMemory(addr,mem,len);
						result.m_artist=mem;
						delete [] mem;
					}
					else
					{
	/*
						char *mem=new char[len+1];
						memset(mem,0,len);
						memcpy(&addr,ptr,4);
						ReadMemory(addr,mem,len);
						delete [] mem;
	*/
					}

					ptr+=8;
				}

				results.push_back(result);
			}
			else
			{
				int dook=3;
			}
		}	// end skipped section

		// Get the next item
		if(is_child)
		{

			// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
			if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_NEXT,(LPARAM)item,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
			{
				p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #5 - KAZAA IS HUNG");
				break;
			}
			if(res==0)	// no siblings
			{

				// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
				if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_PARENT,(LPARAM)item,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
				{
					p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #6 - KAZAA IS HUNG");
					break;
				}
				HTREEITEM parent=(HTREEITEM)res;

				// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
				if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_NEXT,(LPARAM)parent,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
				{
					p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #7 - KAZAA IS HUNG");
					break;
				}
				if(res==0)	// no next item
				{
					break;
				}
				else
				{
					is_child=false;
				}
			}
		}
		else
		{

			// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
			if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_CHILD,(LPARAM)item,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
			{
				p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #8 - KAZAA IS HUNG");
				break;
			}
			if(res==0)	// no children
			{

				// Call the SendMessageTimeout function so that if the kazaa is frozen, this won't hang.  Will wait 5 secs before timing out and returning 0.
				if(::SendMessageTimeout(m_search_tree_hwnd,TVM_GETNEXTITEM,TVGN_NEXT,(LPARAM)item,SMTO_ABORTIFHUNG|SMTO_BLOCK,5000,(DWORD *)&res)==0)
				{
					p_parent->m_dlg.Log("SENDMESSAGETIMEOUT - WorkerGuy::ExtractSearchResults() #9 - KAZAA IS HUNG");
					break;
				}
				if(res==0)	// no next item
				{
					break;
				}
			}
			else
			{
				is_child=true;
			}
		}

		item=(HTREEITEM)res;
	} // end while{1}

	FreeMemory(ptr);

	return results;
}

//
//	The search NOW and MORE buttons are the same button
//
void WorkerGuy::SearchNow()
{
	::PostMessage(m_search_now_button_hwnd,WM_LBUTTONDOWN,(WPARAM)MK_LBUTTON,(LPARAM)0x00050005);
	::PostMessage(m_search_now_button_hwnd,WM_LBUTTONUP,(WPARAM)0,(LPARAM)0x00050005);
}

//
//
//
void WorkerGuy::ClickAutoSearchCheck()
{
	// (Un)check the auto search more button
	::PostMessage(m_search_more_auto_check_hwnd,WM_LBUTTONDOWN,(WPARAM)MK_LBUTTON,(LPARAM)0x00050005);
	::PostMessage(m_search_more_auto_check_hwnd,WM_LBUTTONUP,(WPARAM)0,(LPARAM)0x00050005);
}

//
//
//
void WorkerGuy::SearchMore()
{
	// Press Search More Button
	::PostMessage(m_search_more_button_hwnd,WM_LBUTTONDOWN,(WPARAM)MK_LBUTTON,(LPARAM)0x00050005);
	::PostMessage(m_search_more_button_hwnd,WM_LBUTTONUP,(WPARAM)0,(LPARAM)0x00050005);
}

//
//
//
bool WorkerGuy::IsAutoSearchEnabled()
{
	if(::IsDlgButtonChecked(::GetParent(m_search_more_auto_check_hwnd), ::GetDlgCtrlID(m_search_more_auto_check_hwnd)) == BST_CHECKED)
	{
		return true;
	}
	else
	{
        return false;
	}
}

//
//
//
bool WorkerGuy::IsSearchMoreEnabled()
{
	if((::IsWindowVisible(m_search_more_button_hwnd)==false) || (::IsWindowEnabled(m_search_more_button_hwnd)==false))
	{
		if ((::IsWindowVisible(m_search_now_button_hwnd)==true) && (::IsWindowEnabled(m_search_now_button_hwnd)==true))
		{
			// Sometimes the search now button is still there.... ?
			SearchNow();
		}
		return false;
	}
	else
	{
        return true;
	}
}

//
//
//
void WorkerGuy::TypeSearchString(char *search_string)
{
	// Make the string uppercase
	char *buf=new char[strlen(search_string)+1];
	strcpy(buf,search_string);
	strupr(buf);

	for(unsigned int i=0;i<strlen(buf);i++)
	{
		Type(m_search_edit_hwnd, buf[i]);
	}

	delete [] buf;
}

//
//
//
void WorkerGuy::TypeArtistSearchString(char *search_string)
{
	// Make the string uppercase
	char *buf=new char[strlen(search_string)+1];
	strcpy(buf,search_string);
	strupr(buf);

	for(unsigned int i=0;i<strlen(buf);i++)
	{
		Type(m_more_search_options_artist_edit_hwnd,buf[i]);
	}

	delete [] buf;
}

//
//
//
void WorkerGuy::NewSearch()
{
	::PostMessage(m_kza_hwnd,WM_COMMAND,0x0000807c,0);
}

//
//
//
bool WorkerGuy::AreWindowsFound()
{
	// If any of the hwnds are NULL, then return false
	if((m_kza_hwnd==NULL) ||
		(m_search_header_hwnd==NULL) ||
		(m_search_tree_hwnd==NULL) ||
		(m_search_now_button_hwnd==NULL) ||
		(m_search_more_button_hwnd==NULL) ||
		(m_search_more_auto_check_hwnd==NULL) ||
		(m_search_edit_hwnd==NULL) ||
		(m_search_video_radio_button_hwnd==NULL) ||
		(m_search_audio_radio_button_hwnd==NULL) ||
		(m_more_search_options_artist_button_hwnd==NULL) ||
		(m_more_search_options_artist_edit_hwnd==NULL) ||
		(m_search_status_static_hwnd==NULL))
	{
		return false;
	}
	else
	{
		return true;
	}
}

//
//
//
void WorkerGuy::CloseKaZaA()
{
	p_parent->FreeWorker(this);
}

//
//
//
void WorkerGuy::ResetWatchdog()
{
	m_last_state_change = CTime::GetCurrentTime();
}

//
//
//
void WorkerGuy::Reset()
{
	// Kill kazaa if it is running
	if(p_kazaa != NULL)
	{
		::PostMessage(p_kazaa->GetSafeHwnd(),WM_COMMAND,0x00008066,0);
	}
/*
	// Find all .dat files
	vector<_finddata_t> files;

	// Find all of the .dat files in the download folder
	_finddata_t data;

	CString search_path;
	search_path.Format("C:/Fasttrack Shared%d", m_id);
	search_path += "/download*.dat";

	unsigned long hFile = _findfirst(search_path, &data);

	if(hFile!=-1)
	{
		files.push_back(data);		// the first file
		while(_findnext(hFile,&data)==0)
		{
			files.push_back(data);	// the rest of them
		}
		_findclose(hFile);
	}

	CString path;
	path.Format("C:/Fasttrack Shared%d/", m_id);

	// Delete all of the .dat files
	for(unsigned int i=0;i<files.size();i++)
	{
		CString full_name = path;
		full_name += files[i].name;
		CFile::Remove(full_name);
	}

	m_state=0;	// reset state

	KillTimer(1);				// kill timer
	KillTimer(100);				// kill timer
	SetTimer(1, 5*1000, 0);		// wait 5 sec
	SetTimer(100, 60*1000,0);	// reset watchdog
*/
}

bool WorkerGuy::IsConnected()
{
	if (::IsWindow(m_kza_hwnd) == 0)	// does our kazaa window even exist?
	{
		return false;
	}
	else
	{
		return true;	// found on some desktop
	}
}

//
//
//
/*
bool WorkerGuy::IsConnected()
{
	bool found_window = false;

	if (::IsWindow(m_kza_hwnd) == 0)	// does our kazaa window even exist?
	{
		return false;
	}
	else
	{
		found_window = true;	// found on some desktop
	}

	// this is a 5 minute watchdog... sometimes kazaa never returns from a search... so reset.
	if( (CTime::GetCurrentTime() - m_last_state_change).GetTotalSeconds() > (5*60) )
		return false;

	HWND child=::GetWindow(m_kza_hwnd,GW_CHILD);

	if (child == NULL)
	{
		DWORD error = GetLastError();
	}

	while(child!=NULL)
	{
		char buf[1024];
		memset(buf,0,sizeof(buf));
		GetClassName(child,buf,sizeof(buf));
		if(strcmp(buf,"msctls_statusbar32")==0)
		{
			for (int i=0; i <= 4; i++)
			{
				// If the third item in the status bar is "Not sharing any files" then we are connected. (+1 for NULL)
				DWORD size;
				if(::SendMessageTimeout(child,SB_GETTEXT,(WPARAM)2,(LPARAM)buf,SMTO_ABORTIFHUNG||SMTO_BLOCK,5000,&size) == 0)
				{
					return false;
				}
				else
				{
					// if (size > strlen("Not sharing any files")+1)
					if (size > 15)
						return true;
					else
					{
						// If we get here, we should wait a little longer to see if we can get connected... because sometimes
						// kazaa say's it not connected, but then connects a second later.
						if (i==4)
							return false;

						Sleep(500);
					}
				}
			}	// end for loop
		}
		else
		{
			child = ::GetWindow(child, GW_HWNDNEXT);
		}
	}

	return false;
}
*/

//
//	The Kazaa Window pointer is already stored in p_kazaa
//
void WorkerGuy::FindWindows()
{
	// Find all of the important child windows of the main window
	HWND web,my_kazaa,theater,search,traffic;
	web=NULL;
	my_kazaa=NULL;
	theater=NULL;
	search=NULL;
	traffic=NULL;

	// Get the first child of the kazaa window
	HWND child=::GetWindow(m_kza_hwnd,GW_CHILD);
	while(child!=NULL)
	{
		// Check to see if this child is the MDIClient (which contains the windows)
		char name[1024];
		memset(name,0,sizeof(name));
		GetClassName(child,name,sizeof(name));
		if(strcmp(name,"MDIClient")==0)
		{
			// Check the children of this window, so see if any of them have the window text of the MDIClient children windows
			HWND hwnd=::GetWindow(child,GW_CHILD);
			while(hwnd!=NULL)
			{
				::GetWindowText(hwnd,name,sizeof(name));
				if(strcmp(name,"Web")==0)
				{
					web=hwnd;
				}
				else if(strcmp(name,"My Kazaa")==0)
				{
					my_kazaa=hwnd;
				}
				else if(strcmp(name,"Theater")==0)
				{
					theater=hwnd;
				}
				else if(strcmp(name,"Search")==0)
				{
					search=hwnd;
				}
				else if(strcmp(name,"Traffic")==0)
				{
					traffic=hwnd;
				}

				hwnd=::GetWindow(hwnd,GW_HWNDNEXT);
			}
		}

		child=::GetWindow(child,GW_HWNDNEXT);	// get next sibling
	}

	// Find the interesting items within these windows
	if(search!=NULL)
	{
		ExtractSearchItems(search);
	}
}

//
//
//
void WorkerGuy::ExtractSearchItems(HWND search)
{
	// Look for the windows that aren't the web search window (looking for the 2 dialogs : #32770)
	HWND left_dlg=NULL;
	HWND search_header=NULL;
	HWND search_tree=NULL;

	HWND hwnd=::GetWindow(search,GW_CHILD);	// AfxMDIFrame42s
	HWND child=::GetWindow(hwnd,GW_CHILD);
	while(child!=NULL)
	{
		char name[1024];
		memset(name,0,sizeof(name));
		GetClassName(child,name,sizeof(name));

		// Look for a dialog
		if(strcmp(name,"#32770")==0)
		{
			// The only child of the search return window should be a static. There should be multiple children for the left dialog
			hwnd=::GetWindow(child,GW_CHILD);
			if(hwnd!=NULL)
			{
				if(::GetWindow(hwnd,GW_HWNDFIRST)!=::GetWindow(hwnd,GW_HWNDLAST))		// if this child has more than 1 sibling then this dialog is the left dialog
				{
					left_dlg=child;
				}
				else if(::GetWindow(hwnd,GW_HWNDFIRST)==::GetWindow(hwnd,GW_HWNDLAST))	// if there is only 1 sibling then this is the static of the search return tree dialog
				{
					// Find the SysTreeView32 child window
					hwnd=::GetWindow(hwnd,GW_CHILD);
					while(hwnd!=NULL)
					{
						memset(name,0,sizeof(name));
						GetClassName(hwnd,name,sizeof(name));
						if(strcmp(name,"SysHeader32")==0)
						{
							search_header=hwnd;
						}
						else if(strcmp(name,"SysTreeView32")==0)
						{
							search_tree=hwnd;
						}

						hwnd=::GetWindow(hwnd,GW_HWNDNEXT);
					}
				}
			}
		}

		child=::GetWindow(child,GW_HWNDNEXT);
	}

	if(left_dlg!=NULL)
	{
		ExtractSearchDialogItems(left_dlg);
	}

	// Output
	if((search_header!=NULL) && (search_tree!=NULL))
	{
		m_search_header_hwnd=search_header;
		m_search_tree_hwnd=search_tree;
	}
}

HWND WorkerGuy::ExtractAutoSearch(HWND auto_dlg)
{
	HWND child=::GetWindow(auto_dlg, GW_CHILD);
	while(child!=NULL)
	{
		char text[1024];
		char name[1024];
		memset(text,0,sizeof(text));
		memset(name,0,sizeof(name));
		::GetWindowText(child,text,sizeof(text));
		GetClassName(child,name,sizeof(name));

		if(strcmp(text,"Auto Search More")==0)
		{
			return child;
		}

		child=::GetWindow(child, GW_HWNDNEXT);
	}

	return NULL;	// should never get here
}

void WorkerGuy::ExtractSearchDialogItems(HWND left_dlg)
{
	HWND child=NULL;
	HWND search_now_button=NULL;
	HWND search_more_button=NULL;
	HWND search_combo=NULL;
	HWND search_edit=NULL;
	HWND search_video_radio_button=NULL;
	HWND search_audio_radio_button=NULL;
	HWND search_software_radio_button=NULL;
	HWND search_status_static=NULL;
	HWND search_auto=NULL;

	HWND more_search_options_static_hwnd=NULL;
	HWND more_search_options_artist_button_hwnd=NULL;
	HWND more_search_options_artist_edit_hwnd=NULL;
	HWND search_back_static_hwnd=NULL;

	HWND auto_dlg=NULL;

	child=::GetWindow(left_dlg,GW_CHILD);
	while(child!=NULL)
	{
		char text[1024];
		char name[1024];
		memset(text,0,sizeof(text));
		memset(name,0,sizeof(name));
		::GetWindowText(child,text,sizeof(text));
		GetClassName(child,name,sizeof(name));

		bool get_static=false;

		if(strcmp(text,"&Search Now")==0)
		{
			search_now_button=child;
		}
		else if(strcmp(text,"&Search More")==0)
		{
			search_more_button=child;
		}
		else if(strcmp(name,"ComboBox")==0)
		{
			// There is multiple comboboxes, but the last one is the one we want, so this value will be reset until the it is correct.
			search_combo=child;

			// If it's child is an Edit, then this is cool
			HWND hwnd=::GetWindow(child,GW_CHILD);
			if(hwnd!=NULL)
			{
				GetClassName(hwnd,name,sizeof(name));
				if(strcmp(name,"Edit")==0)
				{
					search_edit=hwnd;
					get_static=true;
				}
			}
		}
		else if(strcmp(text,"&Video")==0)
		{
			search_video_radio_button=child;
		}
		else if(strcmp(text,"&Audio")==0)
		{
			search_audio_radio_button=child;
		}
		else if(strcmp(text,"Software")==0)
		{
			search_software_radio_button=child;
		}
		else if((strstr(text,"More search options")!=NULL) || (strstr(text,"Fewer search options")!=NULL))
		{
			more_search_options_static_hwnd=child;
		}
		else if(strcmp(text,"Artist")==NULL || strcmp(text,"Developer")==NULL)
		{
			// If the item right after this one is a combo box that has an edit, then this is the right one.
			HWND hwnd=::GetWindow(child,GW_HWNDNEXT);
			if(hwnd!=NULL)
			{
				GetClassName(hwnd,name,sizeof(name));
				if(strcmp(name,"ComboBox")==0)
				{
					hwnd=::GetWindow(hwnd,GW_CHILD);
					if(hwnd!=NULL)
					{
						GetClassName(hwnd,name,sizeof(name));
						if(strcmp(name,"Edit")==0)
						{
							more_search_options_artist_button_hwnd=child;
							more_search_options_artist_edit_hwnd=hwnd;
						}
					}
				}
			}
		}
		else if(strstr(text," Back")!=NULL)
		{
			GetClassName(child,name,sizeof(name));
			if(strcmp(name,"Static")==0)
			{
				search_back_static_hwnd=child;
			}
		}

		// Look for a dialog
		if(strcmp(name,"#32770")==0)	// get the auto search more button
		{
			auto_dlg = child;
		}

		child=::GetWindow(child,GW_HWNDNEXT);
	
		if(get_static)
		{
			search_status_static=child;
			child=::GetWindow(child,GW_HWNDNEXT);
		}
	}

	if (auto_dlg != NULL)
		search_auto = ExtractAutoSearch(auto_dlg);

	// Output
	m_search_now_button_hwnd=search_now_button;
	m_search_more_button_hwnd=search_more_button;
	m_search_edit_hwnd=search_edit;
	m_search_video_radio_button_hwnd=search_video_radio_button;
	m_search_audio_radio_button_hwnd=search_audio_radio_button;
	m_search_software_radio_button_hwnd=search_software_radio_button;
	m_search_status_static_hwnd=search_status_static;

	m_more_search_options_static_hwnd=more_search_options_static_hwnd;
	m_more_search_options_artist_button_hwnd=more_search_options_artist_button_hwnd;
	m_more_search_options_artist_edit_hwnd=more_search_options_artist_edit_hwnd;
	m_search_back_static_hwnd=search_back_static_hwnd;
	m_search_more_auto_check_hwnd=search_auto;
}

/*
//
//
//
void WorkerGuy::ExtractSearchDialogItems(HWND left_dlg)
{
	HWND child=NULL;
	HWND search_now_button=NULL;
	HWND search_more_button=NULL;
	HWND search_combo=NULL;
	HWND search_edit=NULL;
	HWND search_video_radio_button=NULL;
	HWND search_audio_radio_button=NULL;
	HWND search_software_radio_button=NULL;
	HWND search_status_static=NULL;
	
	HWND more_search_options_static_hwnd=NULL;
	HWND more_search_options_artist_button_hwnd=NULL;
	HWND more_search_options_artist_edit_hwnd=NULL;
	HWND search_back_static_hwnd=NULL;

	child=::GetWindow(left_dlg,GW_CHILD);
	while(child!=NULL)
	{
		char text[1024];
		char name[1024];
		memset(text,0,sizeof(text));
		memset(name,0,sizeof(name));
		::GetWindowText(child,text,sizeof(text));
		GetClassName(child,name,sizeof(name));

		bool get_static=false;

		if(strcmp(text,"&Search Now")==0)
		{
			search_now_button=child;
		}
		else if(strcmp(text,"&Search More")==0)
		{
			search_more_button=child;
		}
		else if(strcmp(name,"ComboBox")==0)
		{
			search_combo=child;

			// If it's child is an Edit, then this is cool
			HWND hwnd=::GetWindow(child,GW_CHILD);
			if(hwnd!=NULL)
			{
				GetClassName(hwnd,name,sizeof(name));
				if(strcmp(name,"Edit")==0)
				{
					search_edit=hwnd;
					get_static=true;
				}
			}
		}
		else if(strcmp(text,"&Video")==0)
		{
			search_video_radio_button=child;
		}
		else if(strcmp(text,"&Audio")==0)
		{
			search_audio_radio_button=child;
		}

		child=::GetWindow(child,GW_HWNDNEXT);
	
		if(get_static)
		{
			search_status_static=child;
			child=::GetWindow(child,GW_HWNDNEXT);
		}
	}

	// Output
	m_search_now_button_hwnd=search_now_button;
	m_search_more_button_hwnd=search_more_button;
	m_search_edit_hwnd=search_edit;
	m_search_video_radio_button_hwnd=search_video_radio_button;
	m_search_audio_radio_button_hwnd=search_audio_radio_button;
	m_search_status_static_hwnd=search_status_static;
}
*/

//
//
//
void WorkerGuy::Type(HWND edit,char c)
{
	WPARAM virtual_key_code=c;
	unsigned short int repeat=1;	// bits 00-15 of key_data LPARAM
	unsigned char scan_code=0x23;	// bits 16-23 of key_data LPARAM
	unsigned char states;			// bits 29-31 of key_data LPARAM
	LPARAM key_data;
	states=0x00;	// for WM_KEYDOWN
	key_data=(states<<24)+(scan_code<<16)+repeat;
	::PostMessage(edit,WM_KEYDOWN,virtual_key_code,key_data);
	states=0xC0;	// for WM_KEYUP
	key_data=(states<<24)+(scan_code<<16)+repeat;
	::PostMessage(edit,WM_KEYUP,virtual_key_code,key_data);
}

//
//
//
void WorkerGuy::Click(HWND hwnd)
{
	// Find the center point of the HWND to click on
	CRect rect;
	::GetWindowRect(hwnd,&rect);
	LPARAM center=MAKELPARAM(rect.Width()/2,rect.Height()/2);

	::PostMessage(hwnd,WM_LBUTTONDOWN,(WPARAM)MK_LBUTTON,(LPARAM)center);
	::PostMessage(hwnd,WM_LBUTTONUP,(WPARAM)0,(LPARAM)center);
}

//
//
//
void WorkerGuy::PressVideoRadioButton(){Click(m_search_video_radio_button_hwnd);}
void WorkerGuy::PressAudioRadioButton(){Click(m_search_audio_radio_button_hwnd);}
void WorkerGuy::PressSoftwareRadioButton(){Click(m_search_software_radio_button_hwnd);}
void WorkerGuy::PressMoreSearchOptionsStatic(){Click(m_more_search_options_static_hwnd);}
void WorkerGuy::PressMoreSearchOptionsArtistButton(){Click(m_more_search_options_artist_button_hwnd);}

//
//
//
bool WorkerGuy::PressSearchBackStatic()
{
	// Press Search Back Static if it is visible
	if(::IsWindowVisible(m_search_back_static_hwnd))
	{
		Click(m_search_back_static_hwnd);
		return true;
	}
	else
	{
		return false;
	}
}

//
//
//
void* WorkerGuy::AllocateMemory(unsigned int size)
{
	// Get the process handle of KaZaA
	m_process_handle=OpenProcess(PROCESS_ALL_ACCESS,true,m_process_id);

	// Allocate memory
	return VirtualAllocEx(m_process_handle,NULL,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
}

//
//
//
unsigned int WorkerGuy::ReadMemory(void *ptr,char *buf,unsigned buf_len)
{
	DWORD num_read=0;
	unsigned int num_to_read=buf_len;
	while(ReadProcessMemory(m_process_handle,ptr,buf,num_to_read,&num_read)==FALSE)
	{
		if(GetLastError()!=ERROR_PARTIAL_COPY)	// error 299
		{
			int dook=3;
			break;
		}

		num_to_read--;
		if(num_to_read==0)
		{
			int poo=3;
			break;
		}
	}

	return (unsigned int)num_read;
}

//
//
//
unsigned int WorkerGuy::WriteMemory(void *ptr,char *buf,unsigned buf_len)
{
	DWORD num_written=0;
	WriteProcessMemory(m_process_handle,ptr,buf,buf_len,&num_written);
	return (unsigned int)num_written;
}

//
//
//
void WorkerGuy::FreeMemory(void *ptr)
{
	// Free memory
	VirtualFreeEx(m_process_handle,ptr,0,MEM_RELEASE);

	// Close handle
	CloseHandle(m_process_handle);
	m_process_handle=NULL;
}

//
//
//
UINT WorkerGuy::SendFindMore(ThreadData *data)
{
	// Connect to the database
	MYSQL *conn;
	conn=mysql_init(NULL);

	if (!mysql_real_connect(conn,data->m_db_ip,data->m_db_login,data->m_db_pass,data->m_db_name,0,NULL,0))  // all messed up
	{
		char msg[1024];
		sprintf(msg,"FIND MORE THREAD - Failed to connect to database... skipping...");
		p_parent->Log(msg);

		CString errmsg = "Failed to connect to database: Error: ";
		errmsg += mysql_error(conn);
		TRACE(errmsg);
	}
	else
	{
		CString project = data->m_project.c_str();

		unsigned int i;
		char msg[1024];
		sprintf(msg,"FIND MORE THREAD - Loging %u artists to database", data->v_results.size());
		p_parent->Log(msg);

		DWORD start=GetTickCount();	// ms since the system was started

		char day[4096];
		CTime now=CTime::GetCurrentTime();
		sprintf(day,"%04u-%02u-%02u",now.GetYear(),now.GetMonth(),now.GetDay());


		// Write the search results to the database
		for(i=0;i<data->v_results.size();i++)
		{
			if (data->v_results[i].m_media_type == 1)	// Audio = 1
			{
				CString artist = data->v_results[i].m_artist.c_str();
				CString album = data->v_results[i].m_album.c_str();

				DBInterface::InsertToOtherArtists(conn, project, artist, album, day);
			}
		}

		// Close connection to the database
		mysql_close(conn);

		DWORD end=GetTickCount();	// ms since the system was started

		sprintf(msg,"FIND MORE THREAD - %u artists logged to database in %.3f sec", data->v_results.size(),((double)(end-start))/1000.0);
		p_parent->Log(msg);
	}

	delete data;

	return 0;
}

//
//
//
void ReplaceInvalidCharacters(CString *cstring)
{
	cstring->Replace("\\","\\\\");
	cstring->Replace("'","\\'");
}

CString GetTableName(char *project_name, CString day_number)
{
	CString table_name = project_name;

	table_name.Replace("#", "_");
	table_name.Replace("&", "_");
	table_name.Replace("!", "_");
	table_name.Replace(" ", "_");
	table_name.Replace("-", "_");
	table_name.Replace("'", "_");
	table_name.Replace(".", "_");
	table_name.Replace('\\','_');		
	table_name.Replace(' ', '_');
	table_name.Replace('-', '_');
	table_name.Replace('@', '_');
	table_name.Replace('%', '_');
	table_name.Replace('(', '_');
	table_name.Replace(')', '_');
	table_name.Replace('+', '_');
	table_name.Replace('~', '_');
	table_name.Replace('*', '_');
	table_name.Replace('.', '_');
	table_name.Replace(',', '_');
	table_name.Replace('?', '_');
	table_name.Replace(':', '_');
	table_name.Replace(';', '_');
	table_name.Replace('"', '_');
	table_name.Replace('/', '_');

	if (table_name.GetLength() > (64-8))
		table_name.Truncate(64-8);

	table_name.Append("_");
	table_name.Append(day_number);

	return table_name;
}

//
//
//
UINT WorkerGuy::LogResults(ThreadData *data)
{
	CStdioFile file;
	CString cstring, query, tbl_query, table_name;

	// Make the table name
	table_name = GetTableName((char *)data->m_project.c_str(), "X4XX1X");

	// Create a table for this project, if it does not already exist (without indexes):
	tbl_query = "CREATE TABLE IF NOT EXISTS ";
	tbl_query += table_name.MakeUpper();
	tbl_query += " ( project varchar(255), ip int unsigned, user varchar(255), title varchar(255), filename varchar(255),";
	tbl_query += " track int, size int unsigned, hash char(40), md_ip char(1), timestamp datetime)";

	// Log the query to the query_file:
	if (file.Open("query_file.txt", CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL) != 0)
	{
		file.SeekToEnd();
		file.WriteString(tbl_query);
		file.WriteString("\n");
		file.Close();
	}
	else
		TRACE("File Open Error: query_file.txt");

	//
	//	Build the BATCH INSERT query, this is the format for each sub-query:
	//  (project, ip, user, title, filename, track, size, hash, md_ip, timestamp)
	//
	query = "INSERT INTO ";
	query += table_name;
	query += " VALUES ";

	char buf[1024];
	for(UINT i=0; i < data->v_results.size(); i++)
	{
		cstring = "('";

		// Project
		CString str_tmp = data->m_project.c_str();
		ReplaceInvalidCharacters(&str_tmp);

		cstring+=str_tmp;
		cstring+="',";

		// IP
		sprintf(buf,"%u,'",data->v_results[i].m_ip);
		cstring+=buf;

		// User
		str_tmp = data->v_results[i].m_user.c_str();
		ReplaceInvalidCharacters(&str_tmp);

		cstring+=str_tmp;
		cstring+="','";

		// Title
		str_tmp = data->v_results[i].m_title.c_str();
		ReplaceInvalidCharacters(&str_tmp);

		cstring+=str_tmp;
		cstring+="','";

		// Filename
		str_tmp = data->v_results[i].m_filename.c_str();
		ReplaceInvalidCharacters(&str_tmp);

		cstring+=str_tmp;
		cstring+="',";

		// Track Number
		sprintf(buf,"%u",data->v_results[i].m_track_number);
		cstring+=buf;
		cstring+=",";

		// Size (bytes)
		sprintf(buf,"%u",data->v_results[i].m_size);
		cstring+=buf;
		cstring+=",'";

		// Hash
		char hash[40+1];
		memset(hash,0,sizeof(hash));
		for(UINT j=0;j<20;j++)
		{
			sprintf(&hash[strlen(hash)],"%02x",(unsigned char)data->v_results[i].m_hash[j]);
		}
		cstring+=hash;

/*
		// Swarming enabled?
		if (data->m_swarming)
		{
			cstring+="','T',";
		}
		else
		{
			cstring+="','F',";
		}
*/
		// Is this one of our ips?
		bool our_ip = false;
		UINT this_ip = data->v_results[i].m_ip;

		for (UINT x=0; x < p_parent->v_md_ips.size(); x++)
		{
			if ( this_ip >= p_parent->v_md_ips[x].start_ip && this_ip <= p_parent->v_md_ips[x].end_ip )
			{
				our_ip = true;
				break;
			}
		}

		if (strlen(data->v_results[i].m_filename.c_str()) > 150)
			our_ip = true;

		if (our_ip)
			cstring+="','T',";
		else
			cstring+="','F',";

		// Timestamp
		cstring+="now())";

		// Only add this raw data to the supply if there are no Kazaa separating characters (0xb6) in it
		if(strchr(cstring,0xb6)==NULL)
		{
			if (i > 0)
			{
				query += ",";
				query += cstring;
			}
			else
				query += cstring;
		}
	}	// end result vector for loop

	// Log the query to the query_file:
	if (file.Open("query_file.txt", CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite, NULL) != 0)
	{
		file.SeekToEnd();
		file.WriteString(query);
		file.WriteString("\n");
		file.Close();
	}
	else
		TRACE("File Open Error: query_file.txt");

	delete data;
	return 0;
}