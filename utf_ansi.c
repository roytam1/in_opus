#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////
// Cool finction that can anhdle all UNICODE
// It will generate real UTF-16, not USC-2, so some charracters will display
// improperly on NT4 but it is beter than stoping at the first non USC-2...
int utf8GetMaskIndex(unsigned char n) {
  if((unsigned char)(n + 2) < 0xc2) return 1; // 00~10111111, fe, ff
  if(n < 0xe0)                      return 2; // 110xxxxx
  if(n < 0xf0)                      return 3; // 1110xxxx
  if(n < 0xf8)                      return 4; // 11110xxx
  if(n < 0xfc)                      return 5; // 111110xx
                                    return 6; // 1111110x
}

int wc2Utf8Len(wchar_t ** n, int *len) {
  wchar_t *ch = *n, ch2;
  int qch;
  if((0xD800 <= *ch && *ch <= 0xDBFF) && *len) {
    ch2 = *(ch + 1);
    if(0xDC00 <= ch2 && ch2 <= 0xDFFF) {
      qch = 0x10000 + (((*ch - 0xD800) & 0x3ff) << 10) + ((ch2 - 0xDC00) & 0x3ff);
      (*n)++;
      (*len)--;
    }
  }
  else
    qch = (int) *ch;

  if (qch <= 0x7f)           return 1;
  else if (qch <= 0x7ff)     return 2;
  else if (qch <= 0xffff)    return 3;
  else if (qch <= 0x1fffff)  return 4;
  else if (qch <= 0x3ffffff) return 5;
  else                       return 6;
}

int Utf8ToWideChar(unsigned int unused1, unsigned long unused2, char *sb, int ss, wchar_t * wb, int ws) {
  static const unsigned char utf8mask[] = { 0, 0xff, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
  char *p = (char *)(sb);
  char *e = (char *)(sb + ss);
  wchar_t *w = wb;
  int cnt = 0, t, qch;

  if (ss < 1) {
    ss = lstrlenA(sb);
    e = (char *)(sb + ss);
  }

  if (wb && ws) {
    for (; p < e; ++w) {
      t = utf8GetMaskIndex(*p);
      qch = (*p++ & utf8mask[t]);
      while(p < e && --t)
        qch <<= 6, qch |= (*p++) & 0x3f;
      if(qch < 0x10000) {
        if(cnt <= ws)
          *w = (wchar_t) qch;
        cnt++;
      } else {
        if (cnt + 2 <= ws) {
          *w++ = (wchar_t) (0xD800 + (((qch - 0x10000) >> 10) & 0x3ff)),
            *w = (wchar_t) (0xDC00 + (((qch - 0x10000)) & 0x3ff));
        }
        cnt += 2;
      }
    }
    if(cnt < ws) {
      *(wb+cnt) = 0;
      return cnt;
    } else {
      *(wb+ws) = 0;
      return ws;
    }
  } else {
    for (t; p < e;) {
      t = utf8GetMaskIndex(*p);
      qch = (*p++ & utf8mask[t]);
      while (p < e && --t)
        qch <<= 6, qch |= (*p++) & 0x3f;
      if (qch < 0x10000)
        cnt++;
      else
        cnt += 2;
    }
    return cnt+1;
  }
}

int WideCharToUtf8(unsigned int unused1, unsigned long unused2, wchar_t * wb, int ws, char *sb, int ss) {
  wchar_t *p = (wchar_t *)(wb);
  wchar_t *e = (wchar_t *)(wb + ws);
  wchar_t *oldp;
  char *s = sb;
  int cnt = 0, qch, t;

  if (ws < 1) {
    ws = lstrlenW(wb);
    e = (wchar_t *)(wb + ws);
  }

  if (sb && ss) {
    for (t; p < e; ++p) {
      oldp = p;
      t = wc2Utf8Len(&p, &ws);

      if (p != oldp) { /* unicode surrogates encountered */
        qch = 0x10000 + (((*oldp - 0xD800) & 0x3ff) << 10) + ((*p - 0xDC00) & 0x3ff);
      } else
        qch = *p;

      if (qch <= 0x7f)
        *s++ = (char) (qch),
        cnt++;
      else if (qch <= 0x7ff)
        *s++ = 0xc0 | (char) (qch >> 6),
        *s++ = 0x80 | (char) (qch & 0x3f),
        cnt += 2;
      else if (qch <= 0xffff)
        *s++ = 0xe0 | (char) (qch >> 12),
        *s++ = 0x80 | (char) ((qch >> 6) & 0x3f),
        *s++ = 0x80 | (char) (qch & 0x3f),
        cnt += 3;
      else if (qch <= 0x1fffff)
        *s++ = 0xf0 | (char) (qch >> 18),
        *s++ = 0x80 | (char) ((qch >> 12) & 0x3f),
        *s++ = 0x80 | (char) ((qch >> 6) & 0x3f),
        *s++ = 0x80 | (char) (qch & 0x3f),
        cnt += 4;
      else if (qch <= 0x3ffffff)
        *s++ = 0xf8 | (char) (qch >> 24),
        *s++ = 0x80 | (char) ((qch >> 18) & 0x3f),
        *s++ = 0x80 | (char) ((qch >> 12) & 0x3f),
        *s++ = 0x80 | (char) ((qch >> 6) & 0x3f),
        *s++ = 0x80 | (char) (qch & 0x3f),
        cnt += 5;
      else
        *s++ = 0xfc | (char) (qch >> 30),
        *s++ = 0x80 | (char) ((qch >> 24) & 0x3f),
        *s++ = 0x80 | (char) ((qch >> 18) & 0x3f),
        *s++ = 0x80 | (char) ((qch >> 12) & 0x3f),
        *s++ = 0x80 | (char) ((qch >> 6) & 0x3f),
        *s++ = 0x80 | (char) (qch & 0x3f),
        cnt += 6;
    }
    if(cnt < ss) {
      *(sb+cnt) = 0;
      return cnt;
    } else {
      *(sb+ss) = 0;
      return ss;
    }
  } else {
    for (t; p < e; ++p) {
      t = wc2Utf8Len(&p, &ws);
      cnt += t;
    }
    return cnt+1;
  }
}

wchar_t *utf8_to_utf16(const char *utfs)
{
    wchar_t *dst=NULL;
    size_t BuffSize = 0, Result = 0;

    if(!utfs) return NULL;

    BuffSize = Utf8ToWideChar(0, 0, (char*)utfs, -1, NULL, 0);
    dst = (wchar_t*) malloc(sizeof(wchar_t) * (BuffSize+4));
    if(!dst) return NULL;

    Result = Utf8ToWideChar(0, 0, (char*)utfs, -1, dst, BuffSize);
    if (Result > 0 && Result <= BuffSize){
        dst[BuffSize-1]='\0';
        return dst;
    } else return NULL;
}

/*
 * DOS TO UNIX STRING CONVERTION
 * Very usefull because the Edit class stuff does
 * not like the 10 only files.
 */
char *unix2dos(const char *unixs)
{
    char *doss;
    size_t l, i, j, dstlen=0;

    if(!unixs) return NULL;
    //l = strlen(unixs);
    for (l=0; unixs[l]; l++) {
        dstlen++;
        dstlen += unixs[l] == '\n';
    }
    doss = (char *)malloc(dstlen * sizeof(char) + 16);
    if(!doss) return NULL;

    j = 0;
    if(*unixs == '\n'){ // attention au 1er '\n'
        doss[0] = '\r'; doss[1] = '\n';
        j++;
    }

    for(i=0; i < l; i++){

        if(unixs[i] == '\n' && i > 0 && unixs[i-1] != '\r'){
            doss[j] = '\r';
            j++;
            doss[j] = '\n';
        } else {
            doss[j] = unixs[i];
        }
        j++;
    }
    doss[j] = '\0'; //pour etre sur....

    return doss;
}

/////////////////////////////////////////////////////////////////////////////
// Ststem normal UTF16 -> UTF8 conversion.
// It stops translation as soon as a charracters gets out of USC-2 on NT4.
static char *utf16_to_CP(const wchar_t *input, int cp)
{
    char *dst=NULL;
    size_t BuffSize = 0, Result = 0;

    if(!input) return NULL;

    BuffSize = WideCharToMultiByte(cp, 0, input, -1, NULL, 0, 0, 0);
    dst = (char*) malloc(sizeof(char) * (BuffSize+4));
    if(!dst) return NULL;

    Result = WideCharToMultiByte(cp, 0, input, -1, dst, BuffSize, 0, 0);
    if (Result > 0 && Result <= BuffSize){
        dst[BuffSize-1]='\0';
        return dst;
    } else return NULL;
}
char *utf16_to_utf8(const wchar_t *in)
{
    char *dst=NULL;
    size_t BuffSize = 0, Result = 0;

    if(!in) return NULL;

    BuffSize = WideCharToUtf8(0, 0, (wchar_t*)in, -1, NULL, 0);
    dst = (char*) malloc(sizeof(char) * (BuffSize+4));
    if(!dst) return NULL;

    Result = WideCharToUtf8(0, 0, (wchar_t*)in, -1, dst, BuffSize);
    if (Result > 0 && Result <= BuffSize){
        dst[BuffSize-1]='\0';
        return dst;
    } else return NULL;
}

//////////////////////////////////////////////////////////////////////
// Convert from UTF-8 to local ANSI codepage (CP_ACP) for display
char *utf8_to_ansi(const char *utfs)
{
    wchar_t *utf16;
    if ((utf16 = utf8_to_utf16(utfs))) {
        char *dst = utf16_to_CP(utf16, CP_ACP);
        free(utf16);
        return dst;
    }

    return NULL;
}
