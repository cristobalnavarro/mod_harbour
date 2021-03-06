#include "precomp.h"

extern "C" {
	typedef int ( * PHB_APACHE )( void * pRequestRec, void * pAPRPuts,
		const char * szFileName, const char * szArgs, const char * szMethod, const char * szUserIP,
		void * pHeadersIn, void * pHeadersOut,
		void * pHeadersInCount, void * pHeadersInKey, void * pHeadersInVal,
		void * pPostPairsCount, void * pPostPairsKey, void * pPostPairsVal,
		void * pHeadersOutCount, void * pHeadersOutSet, void * pSetContentType,
		void * pApacheGetenv, void * pAPBody, long lAPRemaining );

	static IHttpContext * _pHttpContext = NULL;

	char * GetErrorMessage( DWORD dwLastError )
	{
		LPVOID lpMsgBuf;

		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			dwLastError,
			MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // Default language
			( LPTSTR ) &lpMsgBuf,
			0,
			NULL );

		return ( char * ) lpMsgBuf;
		LocalFree( lpMsgBuf );
	}

	int ap_rputs( const char * szText, IHttpContext * pHttpContext )
	{
		HTTP_DATA_CHUNK dataChunk;
		PCSTR pszText = ( PCSTR ) pHttpContext->AllocateRequestMemory( strlen( szText ) + 1 );
		IHttpResponse * pHttpResponse = pHttpContext->GetResponse();

		if( pszText )
		{
			strcpy( ( char * ) pszText, szText );

			dataChunk.DataChunkType = HttpDataChunkFromMemory;
			dataChunk.FromMemory.pBuffer = ( PVOID ) pszText;
			dataChunk.FromMemory.BufferLength = ( ULONG ) strlen( pszText );

			return pHttpResponse->WriteEntityChunkByReference( &dataChunk, -1 );
		}
		else
			return 0;
	}


	int ap_headers_out_count( void )
	{
		return 0;
	}

	const char * ap_headers_in_key( int iKey )
	{
		return "";
	}

	const char * ap_headers_in_val( int iKey )
	{
		return "";
	}

	int ap_post_pairs_count( void )
	{
		return 0;
	}

	const char * ap_post_pairs_key( int iKey )
	{
		return "";
	}

	const char * ap_post_pairs_val( int iKey )
	{
		return "";
	}

	int ap_headers_in_count( void )
	{
		return 0;
	}

	void ap_headers_out_set( const char * szKey, const char * szValue )
	{
	}

	void ap_set_contenttype( const char * szContentType )
	{
		IHttpResponse * pHttpResponse = _pHttpContext->GetResponse();

		pHttpResponse->SetHeader( "Content-Type", szContentType, strlen( szContentType ), true );
	}

	const char * ap_getenv( const char * szVarName )
	{
		PCSTR rawBuffer = NULL;
		DWORD rawLength = 0;

		_pHttpContext->GetServerVariable( szVarName, &rawBuffer, &rawLength );

		return rawBuffer;
	}

	const char * ap_args( void )
	{
		return ap_getenv( "QUERY_STRING" );
	}

	const char * ap_body( void )
	{
       DWORD bytesRead = 0;
       int totalBytesRead = 0;
       int bytesToRead = atoi( ap_getenv( "CONTENT_LENGTH" ) ), iSize;
       IHttpRequest * request = _pHttpContext->GetRequest();
       char * buffer = ( char * ) _pHttpContext->AllocateRequestMemory( bytesToRead );
       BOOL bCompletionPending = false;

      iSize = bytesToRead;
       
		if( buffer )
		{
			while( bytesToRead > 0 )
			{
				request->ReadEntityBody( ( char * ) ( buffer + bytesRead ), bytesToRead, false, &bytesRead, &bCompletionPending );

				if( ! bytesRead )
					break;

				bytesToRead -= bytesRead;
			}

         * ( buffer + iSize ) = 0;
		}

		return buffer;	
	}
}

static long lAPRemaining = 0;

REQUEST_NOTIFICATION_STATUS CMyHttpModule::OnAcquireRequestState( IN IHttpContext * pHttpContext,
									                  							IN OUT IHttpEventProvider * pProvider )
{
	const char * szPathInfo;

	_pHttpContext = pHttpContext;
	szPathInfo = ap_getenv( "PATH_INFO" );

	if( strstr( szPathInfo, ".prg" ) || strstr( szPathInfo, ".hrb" ) )
	{
		HMODULE lib_harbour = LoadLibrary( "c:\\Windows\\SysWOW64\\inetsrv\\libharbour.dll" );

		ap_set_contenttype( "text/html" );

		if( lib_harbour == NULL )
		{
			char * szErrorMessage = GetErrorMessage( GetLastError() );
			ap_rputs( "c:\\Windows\\SysWOW64\\inetsrv\\libharbour.dll - ", pHttpContext );
			ap_rputs( szErrorMessage, pHttpContext );
			LocalFree( ( void * ) szErrorMessage );
		}
		else
		{
			PHB_APACHE _hb_apache = ( PHB_APACHE ) GetProcAddress( lib_harbour, "hb_apache" );
			char szPath[ 512 ];

			strcpy( szPath, ap_getenv( "APPL_PHYSICAL_PATH" ) );
			strcat( szPath, szPathInfo + 1 );
         while( strchr( szPath, '\\' ) )
            * strchr( szPath, '\\' ) = '/';

			if( _hb_apache != NULL )
			{
 				_hb_apache( pHttpContext, ap_rputs, szPath, ap_args(), 
					        ap_getenv( "REQUEST_METHOD" ), ap_getenv( "REMOTE_ADDR" ),
							  NULL, NULL,
							  ( void * ) ap_headers_in_count, ( void * ) ap_headers_in_key, ( void * ) ap_headers_in_val,
							  ( void * ) ap_post_pairs_count, ( void * ) ap_post_pairs_key, ( void * ) ap_post_pairs_val,
							  ( void * ) ap_headers_out_count, ( void * ) ap_headers_out_set, ( void * ) ap_set_contenttype,
							  ( void * ) ap_getenv, ( void * ) ap_body, lAPRemaining );
 			}
			
			if( _hb_apache == NULL )
				ap_rputs( "can't find hb_apache()", pHttpContext );
		}

		if( lib_harbour )
			FreeLibrary( lib_harbour );
		
		return RQ_NOTIFICATION_FINISH_REQUEST;
	}
	else
       return RQ_NOTIFICATION_CONTINUE;
}