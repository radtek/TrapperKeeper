// HttpSocket.h

#ifndef HTTP_SOCKET_H
#define HTTP_SOCKET_H

#include "TEventSocket.h"
#include "NoiseModuleThreadStatusData.h"

class NoiseSockets;

class HttpSocket : public TEventSocket
{
public:
	HttpSocket();
	~HttpSocket();
	void InitParent(NoiseSockets *sockets);

	bool Attach(SOCKET hSocket);

	int Close();
	void OnConnect(int error_code){};
	void OnReceive(int error_code);
	void OnClose(int error_code);

	NoiseModuleThreadStatusData ReportStatus();

protected:
	void SocketDataSent(unsigned int len);
	void SocketDataReceived(char *data,unsigned int len);
	void SomeSocketDataReceived(char *data,unsigned int data_len,unsigned int new_len,unsigned int max_len);


private:
	NoiseSockets *p_sockets;

	unsigned int m_file_len;
	unsigned int m_file_num_sent;

	void SendFrames();

	NoiseModuleThreadStatusData m_status;

	CTime m_last_time_i_sent_stuff;

	unsigned int ReturnRemoteIPAddress();
	string ReturnRemoteIPAddressString();

	bool CheckConnectionData(unsigned int ip,string filename);
	void SetConnectionData(unsigned int ip,string filename);

	unsigned int BitScramble(unsigned int val);
	unsigned int BitUnScramble(unsigned int val);

	unsigned int m_rand_offset;

	unsigned char m_buf[4096];
	unsigned int m_buf_offset;
	bool m_connection_keep_alive;
	CFile m_file;
	bool m_file_is_opened;
};

#endif // HTTP_SOCKET_H