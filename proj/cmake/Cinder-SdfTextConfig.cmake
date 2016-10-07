if( NOT TARGET Cinder-SdfText )

	get_filename_component( CINDER_SDFTEXT_SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../src" ABSOLUTE )
	get_filename_component( CINDER_SDFTEXT_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../include" ABSOLUTE )
	get_filename_component( CINDER_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE )

	list( APPEND CINDER_SDFTEXT_SOURCES 
			"${CINDER_SDFTEXT_SOURCE_PATH}/cinder/gl/SdfText.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/cinder/gl/SdfTextMesh.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/Bitmap.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/Contour.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/edge-coloring.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/equation-solver.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/EdgeHolder.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/edge-segments.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/render-sdf.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/save-bmp.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/Shape.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/shape-description.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/SignedDistance.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/core/Vector2.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/msdfgen.cpp"
			"${CINDER_SDFTEXT_SOURCE_PATH}/msdfgen/util.cpp"
		)

	if( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
		set_source_files_properties( ${CINDER_SDFTEXT_SOURCES} PROPERTIES COMPILE_FLAGS "-x objective-c++" )
	endif()

	add_library( Cinder-SdfText "${CINDER_SDFTEXT_SOURCES}" )

	target_include_directories( Cinder-SdfText PUBLIC "${CINDER_SDFTEXT_INCLUDE_PATH}" )
	target_include_directories( Cinder-SdfText PUBLIC "${CINDER_SDFTEXT_INCLUDE_PATH}/cinder" )
	target_include_directories( Cinder-SdfText PUBLIC "${CINDER_SDFTEXT_INCLUDE_PATH}/msdfgen" )
	target_include_directories( Cinder-SdfText PRIVATE BEFORE "${CINDER_PATH}/include" )
	target_include_directories( Cinder-SdfText PRIVATE BEFORE "${CINDER_PATH}/include/freetype" )

	target_compile_options( Cinder-SdfText PRIVATE "-std=c++11" )
 	target_compile_definitions( Cinder-SdfText PRIVATE "-DMSDFGEN_USE_CPP11" )


	if( NOT TARGET cinder )
		include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
		find_package( cinder REQUIRED PATHS
			"${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
			"$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
	endif()

	target_link_libraries( Cinder-SdfText PRIVATE cinder )

endif()
