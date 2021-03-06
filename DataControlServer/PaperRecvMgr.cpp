#include "PaperRecvMgr.h"


CPaperRecvMgr::CPaperRecvMgr()
: m_pTcpServer(NULL)
, m_bStop(true)
{
}

CPaperRecvMgr::~CPaperRecvMgr()
{
	StopWork();
}

void CPaperRecvMgr::OnAccept(ITcpContext* pTcpContext)
{
	CPaperUser* pUser = new CPaperUser(pTcpContext, m_listPaperUser);
	if (!pUser)
	{
		pTcpContext->ReleaseConnections();
		return;
	}

	m_listPaperUser.AddUser(pUser);

	std::cout << "new file sender connected. Current connecter: " << m_listPaperUser.m_UserList.size() << std::endl;

	//Ͷ�ݽ��ղ���
	pUser->OnRead(NULL, 0);

}
bool CPaperRecvMgr::StartWork(const char* pLocalIP, WORD wPort)
{
	if (!m_pTcpServer)
	{
		m_pTcpServer = CreateTcpServer(*this, pLocalIP, wPort);
	}

	if (!m_pTcpServer)
	{
		return false;
	}
	m_bStop = false;

	return true;
}

void CPaperRecvMgr::StopWork(void)
{
	if (m_bStop)
	{
		return;
	}
	m_bStop = true;

	if (m_pTcpServer)
	{
		m_pTcpServer->ReleaseConnections();
	}
	m_listPaperUser.ClearUser();
}
