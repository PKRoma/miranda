#ifdef __cplusplus
extern "C" {
#endif

void InitCacheAsync();
void UninitCacheAsync();
void gtaRenewText(HANDLE hContact);
int gtaAddRequest(struct ClcData *dat,struct ClcContact *contact,HANDLE hContact);

#ifdef __cplusplus
};
#endif