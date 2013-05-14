#include "StdAfx.h"
#include "dll.h"

Dll::Dll(void)
{
}

//
//
//
Dll::~Dll(void)
{
}

//
//
//
void Dll::AddInterface(Interface* pi)
{
	v_pInterfaces.push_back(pi);
}

//
//
//
void Dll::RemoveInterface(Interface* pi)
{
	vector<Interface*>::iterator iter = v_pInterfaces.begin();
	while(iter!=v_pInterfaces.end())
	{
		if(*(iter) == pi)
		{
			v_pInterfaces.erase(iter);
			break;
		}
		iter++;
	}
}

//
//
//
bool Dll::DllReceivedData(AppID from_app_id, void* input_data, void* output_data)
{
	for(unsigned int i=0; i<v_pInterfaces.size();i++)
	{
		if(v_pInterfaces[i]->InterfaceReceivedData(from_app_id,input_data,output_data))
			return true;
	}
	if(ReceivedDllData(from_app_id,input_data,output_data))
		return true;
	return false;
}

//
//
//
bool Dll::ReceivedDllData(AppID from_app_id,void* input_data,void* output_data)
{
	return false;
}
