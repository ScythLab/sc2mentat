#pragma once
#include <windows.h>
#include <string>

class Base
{
};

#define LABEL_ADDR(var, lbl) { __asm lea eax, lbl	__asm mov var, eax }

#define TIME_MAX_LEN 8
#define TIME_MAX_SECONDS (100 * 3600 - 1)

#define SafeRelease(pInterface) if(pInterface != NULL) {pInterface->Release(); pInterface=NULL;}
#define SafeDelete(pObject) if(pObject != NULL) {delete pObject; pObject=NULL;}
#define SafeVectorDelete(pVector) { for (size_t i = 0; i < pVector.size(); i++) delete pVector[i]; pVector.clear(); }
#define ListDelete(pList) { for (size_t i = 0; i < pList.size(); i++) { void* value = pList.front(); pList.pop_front(); delete value; } pList.clear(); }


void changeFileExt(LPWSTR lpFilename, SIZE_T size, LPCWSTR newExt);
void extractPath(LPWSTR fullFileName, SIZE_T size);
int timeToSecond(LPCSTR time);
char* secondsToTime(int sec, char* buff);
char* strTrim(char* str);
char* createTrimString(LPCSTR str);
char* removeSpaces(char* str);
bool strToKey(LPCSTR text, UINT* virtKey, UINT* keyMod);
LPWSTR strToWStr(LPCSTR pInStr);
void* makeTrampoline(void* objThis, void* meth);
void freeTrompoline(void** trompoline);
LPWSTR includeTrailingPathDelimiter(LPWSTR str);
LPSTR includeTrailingPathDelimiter(LPSTR str);

namespace std
{
	string format(LPCSTR format, ...);
}
