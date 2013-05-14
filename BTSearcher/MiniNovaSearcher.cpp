#include "StdAfx.h"
#include "mininovasearcher.h"
#include "mininovaparser.h"

MiniNovaSearcher::MiniNovaSearcher(void)
{
	parser = new MiniNovaParser(list, NULL);
}

MiniNovaSearcher::~MiniNovaSearcher(void)
{
}




void MiniNovaSearcher::FormURLs()
{
	v_urls.clear();
	char url[1024+1];
	Log("IsoHuntSearcher::FormURLs()");
	for(unsigned int i = 0; i < (*v_project_data).size(); i++) {
		ProjectData pd = (*v_project_data)[i];
		// http://www.mininova.org/search/?search=batman+begins
		strcpy(url, "http://www.mininova.org/search/?search=");
		vector<string> items = GetSearchString(pd.m_search_strings);
		for(unsigned int j=0; j < items.size(); j++) {
			if(j!=0) strcat(url, "+");
			strcat(url, items[j].c_str());
		}
		strcat(url, "&pg=");
		v_urls.push_back(url);
	}
}

