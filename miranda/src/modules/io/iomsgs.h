
#ifndef IOMSGS_H__
#define IOMSGS_H__ 1

#define WM_MIO_BASE (WM_USER+0x100)

#define WM_MIO_DUMMY WM_MIO_BASE+0


/*
	wParam: 0
	lParam: 0
	Affect: IO system is internally requesting that the thread release any resources, including MIOCTX* and
		transport objects, free itself from the context list and shut down.
*/
#define WM_MIO_CLOSEHANDLE WM_MIO_BASE+1

#endif /* IOMSGS_H__ */