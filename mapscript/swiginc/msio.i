/* $Id$ */

/* copied from mapio.h to avoid to much #ifdef swigging */

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
