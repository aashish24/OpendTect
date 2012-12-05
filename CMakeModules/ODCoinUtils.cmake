#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

SET(COINDIR "" CACHE PATH "COIN Location" )

MACRO(OD_SETUP_COIN)
    IF ( (NOT DEFINED COINDIR) OR COINDIR STREQUAL "" )
	MESSAGE( FATAL_ERROR "COINDIR not set")
    ENDIF()

    FIND_PACKAGE( OpenGL )

    IF(WIN32)
	FIND_LIBRARY(COINLIB NAMES Coin3 PATHS ${COINDIR}/lib REQUIRED )
    ELSE()
	FIND_LIBRARY(COINLIB NAMES Coin PATHS ${COINDIR}/lib REQUIRED )
    ENDIF()

    IF(WIN32)
	FIND_LIBRARY(OD_SIMVOLEON_LIBRARY NAMES SimVoleon2
		     PATHS ${COINDIR}/lib REQUIRED )
    ELSE()
	SET(TMPVAR ${CMAKE_FIND_LIBRARY_SUFFIXES})
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${OD_STATIC_EXTENSION})
	FIND_LIBRARY(OD_SIMVOLEON_LIBRARY NAMES SimVoleon
		     PATHS ${COINDIR}/lib REQUIRED )
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ${TMPVAR})
    ENDIF()

    IF ( OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	INSTALL ( FILES ${COINLIB} 
		  DESTINATION ${OD_EXEC_INSTALL_PATH} )

	IF ( WIN32 )
	    INSTALL ( FILES ${OD_SIMVOLEON_LIBRARY} DESTINATION
		      ${OD_EXEC_INSTALL_PATH} )
	ENDIF()
    ENDIF()


    IF(OD_USECOIN)
	IF ( OD_EXTRA_COINFLAGS )
	    add_definitions( ${OD_EXTRA_COINFLAGS} )
	ENDIF( OD_EXTRA_COINFLAGS )

	LIST(APPEND OD_MODULE_INCLUDESYSPATH ${COINDIR}/include )
	SET(OD_COIN_LIBS ${COINLIB} ${OPENGL_gl_LIBRARY} )
    ENDIF()

    LIST(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_COIN_LIBS} )
ENDMACRO(OD_SETUP_COIN)
