/*
dbRW

Copyright (c) 2005-2007 Robert Rainwater

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
*/
#ifndef DBRW_SQL_H
#define DBRW_SQL_H

void sql_init();
void sql_destroy();
void sql_prepare_add(char **text, sqlite3_stmt **stmts, int len);
void sql_prepare_stmts();
int sql_step(sqlite3_stmt *stmt);
int sql_reset(sqlite3_stmt *stmt);
int sql_exec(sqlite3 *sql, const char *query);
int sql_open(const char *path, sqlite3 **sql);
int sql_close(sqlite3 *sql);
int sql_prepare(sqlite3 *sql, const char *query, sqlite3_stmt **stmt);
int sql_finalize(sqlite3_stmt *stmt);

#endif
