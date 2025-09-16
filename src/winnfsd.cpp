#include "Socket.h"
#include "RPCServer.h"
#include "PortmapProg.h"
#include "NFSProg.h"
#include "MountProg.h"
#include "ServerSocket.h"
#include "DatagramSocket.h"
#include <stdio.h>
#include <direct.h>
#include <iostream>
#include <fstream>
#include <vector>

#define SOCKET_NUM 3
enum
{
    PORTMAP_PORT = 111,
    MOUNT_PORT = 1058,
    NFS_PORT = 2049
};
enum
{
    PROG_PORTMAP = 100000,
    PROG_NFS = 100003,
    PROG_MOUNT = 100005
};

static unsigned int g_nUID, g_nGID;
static bool g_bLogOn;
static char *g_sFileName;
static CRPCServer g_RPCServer;
static CPortmapProg g_PortmapProg;
static CNFSProg g_NFSProg;
static CMountProg g_MountProg;
static unsigned short g_nNFSPort, g_nMountPort, g_nPortmapPort;
static HANDLE g_hTerminateEvent = NULL;

static void printUsage(char *pExe)
{
    printf("\n");
    printf("Usage: %s [-id <uid> <gid>] [-log on | off] [-pathFile <file>]\n", pExe);
    printf("[-mountPort <port>] [-nfsPort <port>] [-portmapPort <port>] [-addr <ip>] [export path] [alias path]\n\n");
    printf("At least a file or a path is needed\n");
    printf("For example:\n");
    printf("On Windows> %s d:\\work\n", pExe);
    printf("On Linux> mount -t nfs 192.168.12.34:/d/work mount\n\n");
    printf("For another example:\n");
    printf("On Windows> %s d:\\work /exports\n", pExe);
    printf("On Linux> mount -t nfs 192.168.12.34:/exports\n\n");
    printf("Another example where WinNFSd is only bound to a specific interface:\n");
    printf("On Windows> %s -addr 192.168.12.34 d:\\work /exports\n", pExe);
    printf("On Linux> mount - t nfs 192.168.12.34: / exports\n\n");
    printf("Use \".\" to export the current directory (works also for -filePath):\n");
    printf("On Windows> %s . /exports\n", pExe);
}

static void printLine(void)
{
    printf("=====================================================\n");
}

static void printAbout(void)
{
    printLine();
    printf("WinNFSd {{VERSION}} [{{HASH}}]\n");
    printf("Network File System server for Windows\n");
    printf("Copyright (C) 2005 Ming-Yang Kao\n");
    printf("Edited in 2011 by ZeWaren\n");
    printf("Edited in 2013 by Alexander Schneider (Jankowfsky AG)\n");
	printf("Edited in 2014 2015 by Yann Schepens\n");
	printf("Edited in 2016 by Peter Philipp (Cando Image GmbH), Marc Harding\n");
    printLine();
}

static void mountPaths(std::vector<std::vector<std::string>> paths)
{
	size_t i;
	size_t numberOfElements = paths.size();

	for (i = 0; i < numberOfElements; i++) {
		char *pPath = (char*)paths[i][0].c_str();
		char *pPathAlias = (char*)paths[i][1].c_str();
		g_MountProg.Export(pPath, pPathAlias);  //export path for mount
	}
}

BOOL WINAPI ConsoleTerminateHandler(DWORD ctrlType)
{
    switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            if (g_hTerminateEvent) SetEvent(g_hTerminateEvent);
            return TRUE;
        default:
            return FALSE;
    }
}

static void waitForTerminateEvent()
{
    g_hTerminateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetConsoleCtrlHandler(ConsoleTerminateHandler, TRUE);

    printf("Press Ctrl+C or close the console window to terminate...\n");
    WaitForSingleObject(g_hTerminateEvent, INFINITE);

    SetConsoleCtrlHandler(ConsoleTerminateHandler, FALSE);
    CloseHandle(g_hTerminateEvent);
    g_hTerminateEvent = NULL;
}

static void start(std::vector<std::vector<std::string>> paths)
{
	int i;
	CDatagramSocket DatagramSockets[SOCKET_NUM];
	CServerSocket ServerSockets[SOCKET_NUM];
	bool bSuccess;
	hostent *localHost;
    
    int nPortMapPort = g_nPortmapPort > 0 ? g_nPortmapPort : PORTMAP_PORT;
    int nNFSPort = g_nNFSPort > 0 ? g_nNFSPort : NFS_PORT;
    int nMountPort = g_nMountPort > 0 ? g_nMountPort : MOUNT_PORT;

	g_PortmapProg.Set(PROG_MOUNT, nMountPort);  //map port for mount
	g_PortmapProg.Set(PROG_NFS, nNFSPort);  //map port for nfs
	g_NFSProg.SetUserID(g_nUID, g_nGID);  //set uid and gid of files

	mountPaths(paths);

	g_RPCServer.Set(PROG_PORTMAP, &g_PortmapProg);  //program for portmap
	g_RPCServer.Set(PROG_NFS, &g_NFSProg);  //program for nfs
	g_RPCServer.Set(PROG_MOUNT, &g_MountProg);  //program for mount
	g_RPCServer.SetLogOn(g_bLogOn);

	for (i = 0; i < SOCKET_NUM; i++) {
		DatagramSockets[i].SetListener(&g_RPCServer);
		ServerSockets[i].SetListener(&g_RPCServer);
	}

	bSuccess = false;

	if (ServerSockets[0].Open(nPortMapPort, 3) && DatagramSockets[0].Open(nPortMapPort)) { //start portmap daemon
		printf("Portmap daemon started\n");

		if (ServerSockets[1].Open(nNFSPort, 10) && DatagramSockets[1].Open(nNFSPort)) { //start nfs daemon
			printf("NFS daemon started\n");

			if (ServerSockets[2].Open(nMountPort, 3) && DatagramSockets[2].Open(nMountPort)) { //start mount daemon
				printf("Mount daemon started\n");
				bSuccess = true;  //all daemon started
			} else {
				printf("Mount daemon starts failed (check if port 1058 is not already in use ;) ).\n");
			}
		} else {
			printf("NFS daemon starts failed.\n");
		}
	} else {
		printf("Portmap daemon starts failed.\n");
	}

	if (bSuccess) {
		localHost = gethostbyname("");
		printf("Listening on %s\n", g_sInAddr);  //local address
		waitForTerminateEvent();
	}

	for (i = 0; i < SOCKET_NUM; i++) {
		DatagramSockets[i].Close();
		ServerSockets[i].Close();
	}
}

int main(int argc, char *argv[])
{
    std::vector<std::vector<std::string>> pPaths;
    char *pPath = NULL;
	bool pathFile = false;

    WSADATA wsaData;

    printAbout();

    if (argc < 2) {
        pPath = strrchr(argv[0], '\\');
        pPath = pPath == NULL ? argv[0] : pPath + 1;
        printUsage(pPath);
        return 1;
    }

    g_nUID = g_nGID = 0;
    g_bLogOn = true;
    g_sFileName = NULL;
	g_sInAddr = (char*)"0.0.0.0";

    for (int i = 1; i < argc; i++) {//parse parameters
        if (_stricmp(argv[i], "-id") == 0) {
            g_nUID = atoi(argv[++i]);
            g_nGID = atoi(argv[++i]);
        } else if (_stricmp(argv[i], "-log") == 0) {
            g_bLogOn = _stricmp(argv[++i], "off") != 0;
        } else if (_stricmp(argv[i], "-nfsPort") == 0) {
            g_nNFSPort = atoi(argv[++i]);
        } else if (_stricmp(argv[i], "-portmapPort") == 0) {
            g_nPortmapPort = atoi(argv[++i]);
        } else if (_stricmp(argv[i], "-mountPort") == 0) {
            g_nMountPort = atoi(argv[++i]);
        } else if (_stricmp(argv[i], "-addr") == 0) {
			g_sInAddr = argv[++i];
		} else if (_stricmp(argv[i], "-pathFile") == 0) {
            g_sFileName = argv[++i];

			if (g_MountProg.SetPathFile(g_sFileName) == false) {
                printf("Can't open file %s.\n", g_sFileName);
                return 1;
			} else {
				g_MountProg.Refresh();
				pathFile = true;
			}
        } else if (i == argc - 2) {
            pPath = argv[argc - 2];  //path is before the last parameter

            char *pCurPathAlias = argv[argc - 1]; //path alias is the last parameter

            if (pPath != NULL || pCurPathAlias != NULL) {
                std::vector<std::string> pCurPaths;
                pCurPaths.push_back(std::string(pPath));
                pCurPaths.push_back(std::string(pCurPathAlias));
                pPaths.push_back(pCurPaths);
            }

            break;
        } else if (i == argc - 1) {
            char *pPath = argv[argc - 1];  //path is the last parameter

            if (pPath != NULL) {
                char curPathAlias[MAXPATHLEN];
                strcpy_s(curPathAlias, pPath);
                char *pCurPathAlias = curPathAlias;

                std::vector<std::string> pCurPaths;
                pCurPaths.push_back(std::string(pPath));
                pCurPaths.push_back(std::string(pCurPathAlias));
                pPaths.push_back(pCurPaths);
            }

            break;
        }
    }

    HWND console = GetConsoleWindow();

    if (g_bLogOn == false && IsWindow(console)) {
        ShowWindow(console, SW_HIDE); // hides the window
    }

	if (pPaths.size() <= 0 && !pathFile) {
        printf("No paths to mount\n");
        return 1;
    }

    WSAStartup(0x0101, &wsaData);
    start(pPaths);
    WSACleanup();

    return 0;
}
