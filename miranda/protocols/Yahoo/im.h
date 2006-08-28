/*
 * $Id: yahoo.h 3629 2006-08-28 19:45:06Z gena01 $
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#ifndef _YAHOO_IM_H_
#define _YAHOO_IM_H_

void ext_yahoo_got_im(int id, const char *me, const char *who, const char *msg, long tm, int stat, int utf8, int buddy_icon);

int YahooSendNudge(WPARAM wParam, LPARAM lParam);
int YahooRecvMessage(WPARAM wParam, LPARAM lParam);
int YahooSendMessageW(WPARAM wParam, LPARAM lParam);
int YahooSendMessage(WPARAM wParam, LPARAM lParam);

#endif
