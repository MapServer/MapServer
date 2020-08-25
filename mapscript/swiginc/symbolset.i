/* ===========================================================================
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript symbolSetObj extensions
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2020 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================
*/

%extend symbolSetObj 
{

    /// Create new :class:`symbolSetObj` instance. If symbolfile is specified, symbols will be 
    /// loaded from the file. 
    symbolSetObj(const char *symbolfile=NULL) 
    {
        symbolSetObj *symbolset;
        mapObj *temp_map=NULL;
        symbolset = (symbolSetObj *) malloc(sizeof(symbolSetObj));
        msInitSymbolSet(symbolset);
        if (symbolfile) {
            symbolset->filename = msStrdup(symbolfile);
            temp_map = msNewMapObj();
            msLoadSymbolSet(symbolset, temp_map);
            symbolset->map = NULL;
            msFreeMap(temp_map);
        }
        return symbolset;
    }

    ~symbolSetObj() 
    {
        msFreeSymbolSet(self);
        if (self->filename) free(self->filename);
        free(self);
    }

    %newobject getSymbol;
    /// Returns a reference to the symbol at index.
    symbolObj *getSymbol(int i) 
    {
        if (i >= 0 && i < self->numsymbols) {
            symbolObj *s=self->symbol[i];
            MS_REFCNT_INCR(s);
            return s;
        } else
            return NULL;
    }

    %newobject getSymbolByName;
    /// Returns a reference to the symbol named name.
    symbolObj *getSymbolByName(char *symbolname) 
    {
        int i;

        if (!symbolname) return NULL;

        i = msGetSymbolIndex(self, symbolname, MS_TRUE);
        if (i == -1)
            return NULL;
        else {
            MS_REFCNT_INCR(self->symbol[i]);
            return self->symbol[i];
        }
    }

    /// Return the index of the symbol named name or -1 in the case that no such 
    /// symbol is found. 
    int index(char *symbolname) 
    {
        return msGetSymbolIndex(self, symbolname, MS_TRUE);
    }

    /// Add a copy of symbol to the symbolset and return its index. 
    int appendSymbol(symbolObj *symbol) 
    {
        return msAppendSymbol(self, symbol);
    }

    %newobject removeSymbol;
    /// Remove the symbol at index and return a copy of the symbol.
    symbolObj *removeSymbol(int index) 
    {
        symbolObj *s=msRemoveSymbol(self, index);
        if (s!=NULL) {
            MS_REFCNT_INCR(s);
        }
        return s;
    }

    /// Save symbol set to a file. Returns :data:`MS_SUCCESS` or :data:`MS_FAILURE`
    int save(const char *filename) {
        return msSaveSymbolSet(self, filename);
    }

}


