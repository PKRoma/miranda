//
// Simple DLL wraper for paintlib (www.painlib.de)
// This functions will allow loading paintlib supported images
// into a windows 32-bit RGBA DIB section.
//
// Cookbook intructions:
// 1) Call ImgNewDecoder to create a new image decoder
// 2) Call ImgNewDIBFromFile to load an image from a file
// 3) Call ImgGetHandle for retrieving an std. HBITMAP handle and a pointer to bitmap bits
// 4) Use the bitmap at will
// 5) Call ImgDeleteDIBSection to delete the image (also delete the handle returned by ImgGetHandle)
// 6) Call ImgDeleteDecoder
//
//
// NOTE: All functions return 0 on success.
//
// Created by Rui Godinho Lopes <ruiglopes@yahoo.com>
//
// THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED. USE IT AT YOUT OWN RISK. THE AUTHOR ACCEPTS NO
// LIABILITY FOR ANY DATA DAMAGE/LOSS THAT THIS PRODUCT MAY CAUSE.
//

#ifndef IMGDECODER_H_INC
#define IMGDECODER_H_INC

//DWORD  ImgNewDecoder(void * /*out*/*ppDecoder);
//DWORD  ImgDeleteDecoder(void */*in*/pDecoder);
//DWORD  ImgNewDIBFromFile(LPVOID /*in*/pDecoder, LPCTSTR /*in*/pFileName, LPVOID /*out*/*pImg);
//DWORD  ImgDeleteDIBSection(LPVOID /*in*/pImg);
//DWORD  ImgGetHandle(LPVOID /*in*/pImg, HBITMAP /*out*/*pBitmap, LPVOID /*out*/*ppDIBBits);

#endif