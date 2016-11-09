
#include <Poco/Util/ServerApplication.h>
#include "NetOperatorDll.h"
#include "VCSDef.h"
#include "NetCmdMgr.h"


int		g_nExitFlag = 0;
CLog	g_Log;
LIST_FILEINFO g_lFileInfo;

class VCS : public Poco::Util::ServerApplication
{
protected:
	void initialize(Poco::Util::Application& self)
	{
		Application::ArgVec argv = self.argv();
		Poco::Path path(argv[0]);
		//		chdir(path.parent().toString().c_str());
		//		loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
	}

	void uninitialize()
	{
		ServerApplication::uninitialize();
	}

	void InitFileList()
	{
		std::string strVerFilePath = CMyCodeConvert::Gb2312ToUtf8(SysSet.m_strFileDir);
		Poco::DirectoryIterator it(strVerFilePath);
		Poco::DirectoryIterator end;
		while (it != end)
		{
			Poco::Path p(it->path());
			if (it->isFile())
			{
				pST_FILEINFO pFileInfo = new ST_FILEINFO;
				g_lFileInfo.push_back(pFileInfo);

				pFileInfo->strFileName = CMyCodeConvert::Utf8ToGb2312(p.getFileName());
				pFileInfo->strFilePath = CMyCodeConvert::Utf8ToGb2312(p.toString());
				pFileInfo->strMd5 = calcFileMd5(p.toString());
			}
			else if (it->isDirectory())
			{
				Poco::DirectoryIterator it2(p.toString());
				Poco::DirectoryIterator end2;
				while (it2 != end2)
				{
					Poco::Path p2(it2->path());
					if (it2->isFile())
					{
						pST_FILEINFO pFileInfo = new ST_FILEINFO;
						g_lFileInfo.push_back(pFileInfo);

						pFileInfo->strFileName = CMyCodeConvert::Utf8ToGb2312(p2.getFileName());
						pFileInfo->strFilePath = CMyCodeConvert::Utf8ToGb2312(p2.toString());
						pFileInfo->strMd5 = calcFileMd5(p2.toString());
					}
					it2++;
				}
			}
			it++;
		}
	}

	int main(const std::vector < std::string > & args)
	{
		Poco::Net::HTTPStreamFactory::registerFactory();

		std::cout << "�汾���Ʒ�������������..." << std::endl;
		std::string strCurrentPath = config().getString("application.dir");
		std::string strLogPath = strCurrentPath + "VCS.Log";
		std::string strDllLogPath = CMyCodeConvert::Utf8ToGb2312(strCurrentPath) + "VCS_Dll.Log";
		std::string strConfigPath = strCurrentPath + "VCS-config.ini";

		g_Log.SetFileName(strLogPath);
		SetLogFileName((char*)strDllLogPath.c_str());
		SysSet.Load(strConfigPath);
		SysSet.m_strCurrentDir = strCurrentPath;

#ifdef POCO_OS_FAMILY_WINDOWS
		char szTitle[150] = { 0 };
		sprintf(szTitle, "%s <%s:%d>", SOFT_VERSION, SysSet.m_sLocalIP.c_str(), SysSet.m_nCmdPort);
		std::wstring wstrTitle;
		Poco::UnicodeConverter::toUTF16(szTitle, wstrTitle);
		SetConsoleTitle(wstrTitle.c_str());
#endif

		InitFileList();

		CNetCmdMgr* m_pNetCmdMgr = new CNetCmdMgr;

		bool bResult = m_pNetCmdMgr->StartWork((char*)SysSet.m_sLocalIP.c_str(), SysSet.m_nCmdPort);
		if (bResult)
			g_Log.LogOut("StartCmdChannel success.");
		else
			g_Log.LogOut("StartCmdChannel fail.");

		std::cout << "�汾���Ʒ������������" << std::endl;
		waitForTerminationRequest();
		g_nExitFlag = 1; 
		if (m_pNetCmdMgr)
		{
			m_pNetCmdMgr->StopWork();
			delete m_pNetCmdMgr;
			m_pNetCmdMgr = NULL;
		}

		LIST_FILEINFO::iterator it = g_lFileInfo.begin();
		for (; it != g_lFileInfo.end();)
		{
			pST_FILEINFO pFileInfo = *it;
			SAFE_RELEASE(pFileInfo);
			it = g_lFileInfo.erase(it);
		}

		g_Log.LogOut("StopCmdChannel complete.");
		return 0;
	}
};

POCO_SERVER_MAIN(VCS);
