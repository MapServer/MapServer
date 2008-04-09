/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Definitions for MapServer IO redirection capability.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 * Note:
 * copied from mapio.h to avoid to much #ifdef swigging
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/


/* $Id$ */

void msIO_resetHandlers(void);
void msIO_installStdoutToBuffer(void);
void msIO_installStdinFromBuffer(void);
const char *msIO_stripStdoutBufferContentType(void);

/* mapscript only extensions */

const char *msIO_getStdoutBufferString(void);
gdBuffer msIO_getStdoutBufferBytes(void);

%{

const char *msIO_getStdoutBufferString() {
    msIOContext *ctx = msIO_getHandler( (FILE *) "stdout" );
    msIOBuffer  *buf;

    if( ctx == NULL || ctx->write_channel == MS_FALSE 
        || strcmp(ctx->label,"buffer") != 0 )
    {
	msSetError( MS_MISCERR, "Can't identify msIO buffer.",
                    "msIO_getStdoutBufferString" );
	return "";
    }

    buf = (msIOBuffer *) ctx->cbData;

    /* write one zero byte and backtrack if it isn't already there */
    if( buf->data_len == 0 || buf->data[buf->data_offset] != '\0' ) {
        msIO_bufferWrite( buf, "", 1 );
	buf->data_offset--;
    }

    return (const char *) (buf->data);
}

gdBuffer msIO_getStdoutBufferBytes() {
    msIOContext *ctx = msIO_getHandler( (FILE *) "stdout" );
    msIOBuffer  *buf;
    gdBuffer     gdBuf;

    if( ctx == NULL || ctx->write_channel == MS_FALSE 
        || strcmp(ctx->label,"buffer") != 0 )
    {
	msSetError( MS_MISCERR, "Can't identify msIO buffer.",
                    "msIO_getStdoutBufferString" );
	gdBuf.data = (unsigned char*)"";
	gdBuf.size = 0;
	gdBuf.owns_data = MS_FALSE;
	return gdBuf;
    }

    buf = (msIOBuffer *) ctx->cbData;

    gdBuf.data = buf->data;
    gdBuf.size = buf->data_offset;
    gdBuf.owns_data = MS_FALSE;

    /* we are seizing ownership of the buffer contents */
    buf->data_offset = 0;
    buf->data_len = 0;
    buf->data = NULL;

    return gdBuf;
}

%}
