#ifndef SVC_H__
#define SVC_H__ 1

#include <ctype.h>
#include <aim.h>

#define OSCAR_CONTACT_KEY "_sn"

/* find a hContact given a normalised screen name, 
	if 'add' is set, then the contact is added if it didnt exist */
HANDLE Miranda_FindContact(char * sn, int add);

/* given a hContact, return the screename/#icq associated with it, 
returns 1 on success, 0 on failure */
int Miranda_ReverseFindContact(HANDLE hContact, char * sn, size_t cch);

/* visit every contact under the protocol and set 'Status' to offline */
void Miranda_SetContactsOffline(void);

/* takes sn and strips all <= ' ' characters, cch has to be >= 1 otherwise the
function returns 0 _after_ normalisation */
int Miranda_Normalise(char * sn, char * buf, size_t cch);

/* given a libfaim ICQ status, returns a miranda ID_STATUS_* */
unsigned int Miranda_TransformStatus(unsigned int status);

#endif // SVC_H__


