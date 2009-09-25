/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

$Id$

*/

#ifndef __SENDQUEUE_H
#define __SENDQUEUE_H

#define TIMERID_MSGSEND      100
#define TIMERID_TYPE         3
#define TIMERID_AWAYMSG      4
#define TIMEOUT_TYPEOFF      10000      // send type off after 10 seconds of inactivity
#define SB_CHAR_WIDTH        45

#if defined(_UNICODE)
    #define SEND_FLAGS PREF_UNICODE
#else
    #define SEND_FLAGS 0
#endif

/*
 * send flags
 */

#define	SENDJOBS_MAX_SENDS 100

struct SendJob {
	HANDLE  hSendId;
	char    *sendBuffer;
	int     dwLen;        // actual buffer length (checked for reallocs()
	int     sendCount;
	HANDLE  hOwner;
	HWND    hwndOwner;
	unsigned int iStatus;
	TCHAR   szErrorMsg[128];
	DWORD   dwFlags;
	int     iAcksNeeded;
	HANDLE  hEventSplit;
	int     chunkSize;
	DWORD   dwTime;
};

class SendLaterJob {
public:
	SendLaterJob();
	~SendLaterJob();
	char	szId[20];									// database key name (time stamp of original send)
	HANDLE	hContact;									// original contact where the message has been assigned
	HANDLE  hTargetContact;								// *real* contact (can be different for metacontacts, e.g).
	HANDLE	hProcess;									// returned from the protocols sending service. needed to find it in the ACK handler
	time_t	lastSent;									// time at which the delivery was initiated. used to handle timeouts
	char	*sendBuffer;								// utf-8 send buffer
	PBYTE	pBuf;										// conventional send buffer (for non-utf8 protocols)
	DWORD	dwFlags;
	int		iSendCount;									// # of times we tried to send it...
	bool	fSuccess;
};

class SendQueue {
public:
	enum {
			NR_SENDJOBS = 30,
			SQ_ERROR = 2,
			SQ_INPROGRESS = 1,
			SQ_UNDEFINED = 0
		 };

	SendQueue()
	{
		ZeroMemory(m_jobs, (sizeof(SendJob) * NR_SENDJOBS));
		m_currentIndex = 0;
	}

	void	inc() { m_currentIndex++; }
	void	dec() { m_currentIndex--; }
	void	operator++() { m_currentIndex++; }
	void	operator--() { m_currentIndex--; }

	~SendQueue()
	{
		for(int i = 0; i < NR_SENDJOBS; i++) {
			if(m_jobs[i].sendBuffer)
				free(m_jobs[i].sendBuffer);
		}
	}

	SendJob *getJobByIndex(const int index) { return(&m_jobs[index]); }

	void 	TSAPI clearJob					(const int index);
	int 	TSAPI findNextFailed			(const _MessageWindowData *dat) const;
	void 	TSAPI handleError				(_MessageWindowData *dat, const int iEntry) const;
	int 	TSAPI addTo						(_MessageWindowData *dat, const int iLen, int dwFlags);
	int 	TSAPI sendQueued				(_MessageWindowData *dat, const int iEntry);
	void 	TSAPI checkQueue 		 		(const _MessageWindowData *dat) const;
	void 	TSAPI logError					(const _MessageWindowData *dat, int iSendJobIndex,
											 const TCHAR *szErrMsg) const;
	void 	TSAPI recallFailed				(const _MessageWindowData *dat, int iEntry) const;
	void 	TSAPI showErrorControls			(_MessageWindowData *dat, const int showCmd) const;
	int 	TSAPI ackMessage				(_MessageWindowData *dat, WPARAM wParam, LPARAM lParam);
	int		TSAPI sendLater					(int iIndex, _MessageWindowData *dat, bool fAddHeader = true, HANDLE hContact = 0);
	/*
	 * static members
	 */
#if defined(_UNICODE)
	static	int TSAPI RTL_Detect				(const wchar_t *pszwText);
#endif
	static	char* TSAPI MsgServiceName			(const HANDLE hContact, const _MessageWindowData *dat, int isUnicode);
	static  int   TSAPI GetProtoIconFromList	(const char *szProto, int iStatus);
	static  LRESULT TSAPI WarnPendingJobs		(unsigned int uNrMessages);
	static	void  TSAPI NotifyDeliveryFailure	(const _MessageWindowData *dat);
	static	void  TSAPI UpdateSaveAndSendButton	(_MessageWindowData *dat);
	static	void  TSAPI EnableSending			(const _MessageWindowData *dat, const int iMode);
private:
	SendJob		m_jobs[NR_SENDJOBS];
	int			m_currentIndex;
};

extern SendQueue *sendQueue;

int  TSAPI ActivateExistingTab	(ContainerWindowData *pContainer, HWND hwndChild);
void TSAPI ShowMultipleControls	(const HWND hwndDlg, const UINT * controls, int cControls, int state);
void TSAPI HandleIconFeedback		(_MessageWindowData *dat, HICON iIcon);

#endif /* __SENDQUEUE_H */
