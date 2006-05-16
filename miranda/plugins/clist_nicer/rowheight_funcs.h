#ifndef __ROWHEIGHT_FUNCS_H__
# define __ROWHEIGHT_FUNCS_H__

#define ROW_SPACE_BEETWEEN_LINES 0
#define ICON_HEIGHT 16



BOOL RowHeights_Initialize(struct ClcData *dat);
void RowHeights_Free(struct ClcData *dat);
void RowHeights_Clear(struct ClcData *dat);

BOOL RowHeights_Alloc(struct ClcData *dat, int size);

// Calc and store max row height
int RowHeights_GetMaxRowHeight(struct ClcData *dat, HWND hwnd);

// Calc and store row height
int __forceinline RowHeights_GetRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, int item, DWORD style);

// Calc and store row height for all itens in the list
void RowHeights_CalcRowHeights(struct ClcData *dat, HWND hwnd);

// Calc item top Y (using stored data)
int RowHeights_GetItemTopY(struct ClcData *dat, int item);

// Calc item bottom Y (using stored data)
int RowHeights_GetItemBottomY(struct ClcData *dat, int item);

// Calc total height of rows (using stored data)
int RowHeights_GetTotalHeight(struct ClcData *dat);

// Return the line that pos_y is at or -1 (using stored data). Y start at 0
int RowHeights_HitTest(struct ClcData *dat, int pos_y);

// Returns the height of the chosen row
int RowHeights_GetHeight(struct ClcData *dat, int item);

// returns the height for a floating contact
int RowHeights_GetFloatingRowHeight(struct ClcData *dat, HWND hwnd, struct ClcContact *contact, DWORD dwFlags);

#endif // __ROWHEIGHT_FUNCS_H__
