#------------------------------------------------------------------------------
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
    CMAKE_POLICY(SET CMP0043 NEW)
ENDIF (COMMAND CMAKE_POLICY)

project( CAVASS )
#------------------------------------------------------------------------------
# set compiler warning level
if (MSVC)
  add_compile_options( /W4 )
elseif (APPLE)
  add_compile_options( -Wall -Wextra -Wpedantic
      -Wno-array-bounds
      -Wno-deprecated-declarations
      -Wno-extra-semi
      -Wno-format
      -Wno-macro-redefined
      -Wno-missing-field-initializers
      -Wno-sizeof-array-decay
      -Wno-overloaded-virtual
      -Wno-parentheses
      -Wno-sign-compare
      -Wno-sometimes-uninitialized
      -Wno-unused-but-set-variable
      -Wno-unused-const-variable
      -Wno-unused-parameter
      -Wno-unused-private-field
      -Wno-unused-result
      -Wno-unused-variable
  )
else()
  add_compile_options( -Wall -Wextra
      -Wno-array-bounds
      -Wno-array-parameter
      -Wno-cast-function-type
      -Wno-deprecated-declarations
      -Wno-format
      -Wno-implicit-fallthrough
      -Wno-misleading-indentation
      -Wno-missing-field-initializers
      -Wno-parentheses
      -Wno-sign-compare
      -Wno-stringop-truncation
      -Wno-unused-but-set-variable
      #-Wno-unused-const-variable
      -Wno-unused-parameter
      -Wno-stringop-overflow
      -Wno-unused-result
      -Wno-unused-variable
  )
endif()
#------------------------------------------------------------------------------
# (optional) Parallel build.
if (NOT BUILD_PARALLEL)
    #if not specified, set default value
    SET (BUILD_PARALLEL Off CACHE STRING
         "Turn parallel build either On or Off." FORCE)
endif (NOT BUILD_PARALLEL)

if (BUILD_PARALLEL STREQUAL "On")
    SET (PARALLEL TRUE)
    ADD_DEFINITIONS( -DPARALLEL )

    if (NOT BUILD_PARALLEL_LIB)
        if (MSVC)
            set (BUILD_PARALLEL_LIB "c:/Program Files/MPICH2/lib" CACHE STRING
                 "Specify location of parallel system library." FORCE)
        else (MSVC)
            set (BUILD_PARALLEL_LIB "" CACHE STRING
                 "Specify location of parallel system library." FORCE)
        endif (MSVC)
    endif (NOT BUILD_PARALLEL_LIB)

    if (NOT BUILD_PARALLEL_INC)
        if (MSVC)
            set (BUILD_PARALLEL_INC "c:/Program Files/MPICH2/include" CACHE STRING
                 "Specify location of parallel system include files." FORCE)
        else (MSVC)
            set (BUILD_PARALLEL_INC "" CACHE STRING
                 "Specify location of parallel system include files." FORCE)
        endif (MSVC)
    endif (NOT BUILD_PARALLEL_INC)
else (BUILD_PARALLEL STREQUAL "On")
    SET (PARALLEL FALSE)
endif (BUILD_PARALLEL STREQUAL "On")
#------------------------------------------------------------------------------
# (optional) Find ITK.
FIND_PACKAGE( ITK )
IF (ITK_FOUND)
    ADD_DEFINITIONS( -DBUILD_WITH_ITK )
ENDIF (ITK_FOUND)

IF (MSVC)
    add_definitions( -D_CRT_SECURE_NO_DEPRECATE )
ENDIF (MSVC)
#------------------------------------------------------------------------------
SET (MAXFLOW_VERSION adjacency_list)

#
#build the 3dviewnix library
#
add_library( 3dviewnix  3dviewnix/LIBRARY/data_interf.c
                        3dviewnix/LIBRARY/globals.c
                        3dviewnix/LIBRARY/overlay.c
                        3dviewnix/LIBRARY/proc_interf.c )

if (MSVC)
    add_custom_command( TARGET 3dviewnix PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/3dviewnix/ ${CAVASS_BINARY_DIR}/Release )
    add_custom_command( TARGET 3dviewnix PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/3dviewnix/ ${CAVASS_BINARY_DIR}/Debug )
else (MSVC)
    add_custom_command( TARGET 3dviewnix PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/3dviewnix/ ${CAVASS_BINARY_DIR} )
endif (MSVC)
#------------------------------------------------------------------------------
#
#build the TIFF library
#
add_library( libtiff  port_data/tif_aux.c port_data/tif_close.c port_data/tif_codec.c port_data/tif_color.c port_data/tif_compress.c port_data/tif_dir.c port_data/tif_dirinfo.c port_data/tif_dirread.c port_data/tif_dirwrite.c port_data/tif_dumpmode.c port_data/tif_error.c port_data/tif_fax3.c port_data/tif_fax3sm.c port_data/tif_flush.c port_data/tif_getimage.c port_data/tif_luv.c port_data/tif_lzw.c port_data/tif_next.c port_data/tif_open.c port_data/tif_packbits.c port_data/tif_predict.c port_data/tif_read.c port_data/tif_strip.c port_data/tif_swab.c port_data/tif_thunder.c port_data/tif_tile.c port_data/tif_unix.c port_data/tif_warning.c port_data/tif_write.c )
#------------------------------------------------------------------------------
#
#build 3dviewnix command line programs
#
include_directories( . 3dviewnix/INCLUDE maxflow/${MAXFLOW_VERSION} )
if (UNIX)
    set( 3DVLIB 3dviewnix -lm )
else (UNIX)
    set( 3DVLIB 3dviewnix )
endif (UNIX)

add_executable( BIM_to_rover  port_data/BIM_to_rover.c )
target_link_libraries( BIM_to_rover  3dviewnix )

add_executable( BS0_TO_BS1  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/BS0_TO_BS1.c 3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/gcode.c )
target_link_libraries( BS0_TO_BS1 ${3DVLIB} )

add_executable( BS1_TO_BS0  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/BS1_TO_BS0.c 3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/gcode.c )
target_link_libraries( BS1_TO_BS0 ${3DVLIB} )

add_executable( IM0_to_pgm port_data/IM0_to_pgm.c )
target_link_libraries( IM0_to_pgm 3dviewnix )

add_executable( LTDT3D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/LTDT3d.cpp )
target_link_libraries( LTDT3D  3dviewnix )

add_executable( MV0_to_ppm port_data/MV0_to_ppm.c )
target_link_libraries( MV0_to_ppm 3dviewnix )

add_executable( MV0_to_tiff port_data/MV0_to_tiff.c )
if (UNIX)
    target_link_libraries( MV0_to_tiff ${3DVLIB} libtiff -lm )
else (UNIX)
    target_link_libraries( MV0_to_tiff ${3DVLIB} libtiff )
endif (UNIX)

add_executable( affine  registration/affine.c  registration/bigden.c registration/matrix.c  registration/biglag.c registration/newuoa.c  registration/newuob.c registration/trsapp.c  registration/update.c )
target_link_libraries( affine ${3DVLIB} )

add_executable( algebra  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/algebra.c )
target_link_libraries( algebra ${3DVLIB} )

add_executable( b_scale_anisotrop_diffus_2D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/b_scale_anisotrop_diffus_2D.c )
target_link_libraries( b_scale_anisotrop_diffus_2D ${3DVLIB} )

add_executable( b_scale_anisotrop_diffus_3D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/b_scale_anisotrop_diffus_3D.c )
target_link_libraries( b_scale_anisotrop_diffus_3D ${3DVLIB} )

add_executable( ball_enhance  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/ball_enhance.c )
target_link_libraries( ball_enhance ${3DVLIB} )

add_executable( bin_mask  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/bin_mask.c )
target_link_libraries( bin_mask  3dviewnix )

add_executable( bin_ops  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/bin_ops.c )
target_link_libraries( bin_ops  3dviewnix )

add_executable( bin_to_MSK  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/bin_to_MSK.c )
target_link_libraries( bin_to_MSK  3dviewnix )

add_executable( bin_volume  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/bin_volume.c )
target_link_libraries( bin_volume ${3DVLIB} )

add_executable( bscale_dilate_2D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/bscale_dilate_2D.c )
target_link_libraries( bscale_dilate_2D ${3DVLIB} )

add_executable( class2f  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/CLASSIFY/class2f.c )
target_link_libraries( class2f  3dviewnix )

add_executable( combine_cine_slices  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/combine_cine_slices.c )
target_link_libraries( combine_cine_slices  3dviewnix )

add_executable( comp_interv  aar/comp_interv.c )

add_executable( copy_pose  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/copy_pose.c )
target_link_libraries( copy_pose  3dviewnix )

add_executable( correct_min_max  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/correct_min_max.c )
target_link_libraries( correct_min_max ${3DVLIB} )

add_executable( create_icon  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/create_icon.c 3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/gcode.c )
target_link_libraries( create_icon ${3DVLIB} )

add_executable( deform  registration/deform.c )
target_link_libraries( deform ${3DVLIB} )

add_executable( direction  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/direction.c )
target_link_libraries( direction  3dviewnix )

add_executable( distance3D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/distance3D.c )
target_link_libraries( distance3D ${3DVLIB} )

add_executable( divide  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/divide.c )
target_link_libraries( divide  3dviewnix )

add_executable( edge_stats  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/edge_stats.c registration/matrix.c )
target_link_libraries( edge_stats  3dviewnix )

add_executable( face_stats  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/face_stats.c registration/matrix.c )
target_link_libraries( face_stats ${3DVLIB} )

add_executable( fcut  port_data/fcut.c )

add_executable( fg_components  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/fg_components.c )
target_link_libraries( fg_components  3dviewnix )

add_executable( fillcontours  port_data/fillcontours.cpp port_data/read_acrnema.cpp )
target_link_libraries( fillcontours  3dviewnix )

add_executable( from_BS2 port_data/from_BS2.c )
target_link_libraries( from_BS2 ${3DVLIB} )

add_executable( fuzz_track_3d  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/fuzz_track_3d.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/chash.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/chash2.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/hheap.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/gqueue.c )
target_link_libraries( fuzz_track_3d ${3DVLIB} )

add_executable( fuzz_track_rel  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/fuzz_track_rel.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/chash.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/chash2.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/hheap.c )
target_link_libraries( fuzz_track_rel ${3DVLIB} )

add_executable( fuzziness  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/fuzziness.c )
target_link_libraries( fuzziness ${3DVLIB} )

add_executable( gaussian2d  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/gaussian2d.c )
target_link_libraries( gaussian2d ${3DVLIB} )

add_executable( gaussian3d  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/gaussian3d.c )
target_link_libraries( gaussian3d ${3DVLIB} )

add_executable( gaussian_3D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/gaussian_3D.c )
target_link_libraries( gaussian_3D ${3DVLIB} )

add_executable( gbscale2_scale  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/VOI/gbscale2_scale.c )
target_link_libraries( gbscale2_scale ${3DVLIB} )

add_executable( gbscale2_std  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/VOI/gbscale2_std.c )
target_link_libraries( gbscale2_std ${3DVLIB} )

add_executable( gbscale2_train  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/VOI/gbscale2_train.c )
target_link_libraries( gbscale2_train ${3DVLIB} )

add_executable( gc  maxflow/gc.cpp maxflow/${MAXFLOW_VERSION}/graph.cpp maxflow/${MAXFLOW_VERSION}/maxflow.cpp )
target_link_libraries( gc  3dviewnix )

add_executable( get_bins  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/get_bins.c registration/matrix.c )
target_link_libraries( get_bins ${3DVLIB} )

add_executable( get_slicenumber  3dviewnix/PROCESS/AIRWAY/get_slicenumber.c )
target_link_libraries( get_slicenumber  3dviewnix )

add_executable( gradient  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/gradient.c )
target_link_libraries( gradient ${3DVLIB} )

add_executable( gradient3d  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/gradient3d.c )
target_link_libraries( gradient3d ${3DVLIB} )

add_executable( gscale  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/gscale.c )
target_link_libraries( gscale ${3DVLIB} )

add_executable( imatron_to_IM0 port_data/imatron_to_IM0.c )
target_link_libraries( imatron_to_IM0 3dviewnix )

add_executable( inhomo_correct_bal_std  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/inhomo_correct_bal_std.c )
target_link_libraries( inhomo_correct_bal_std ${3DVLIB} )

add_executable( inhomo_correct_interac  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/inhomo_correct_interac.c )
target_link_libraries( inhomo_correct_interac ${3DVLIB} )

add_executable( inhomo_correct_whole  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/inhomo_correct_whole.c )
target_link_libraries( inhomo_correct_whole ${3DVLIB} )

add_executable( inhomo_gscale_float  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/inhomo_gscale_float.c )
target_link_libraries( inhomo_gscale_float ${3DVLIB} )

add_executable( intensity_map  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/CLASSIFY/intensity_map.c )
target_link_libraries( intensity_map ${3DVLIB} )

add_executable( invert  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/invert.c )
target_link_libraries( invert  3dviewnix )

add_executable( largest_component  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/largest_component.c )
target_link_libraries( largest_component  3dviewnix )

add_executable( local_maxima  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/local_maxima.c )
target_link_libraries( local_maxima ${3DVLIB} )

add_executable( magnitude  registration/magnitude.c )
target_link_libraries( magnitude ${3DVLIB} )

add_executable( mask2bin  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MASK/mask2bin.c )
target_link_libraries( mask2bin  3dviewnix )

add_executable( mask2grey  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MASK/mask2grey.c )
target_link_libraries( mask2grey  3dviewnix )

add_executable( masked_stats  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/masked_stats.c )
target_link_libraries( masked_stats ${3DVLIB} )

add_executable( matched_reslice  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/matched_reslice.c )
target_link_libraries( matched_reslice  3dviewnix )

add_executable( mean  aar/mean.c )

add_executable( mean_pose  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/mean_pose.c )
if (UNIX)
     target_link_libraries( mean_pose -lm )
else (UNIX)
     target_link_libraries( mean_pose )
endif (UNIX)

add_executable( median2d  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/median2d.c )
target_link_libraries( median2d ${3DVLIB} )

add_executable( median3d  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/median3d.c )
target_link_libraries( median3d ${3DVLIB} )

add_executable( merge  aar/merge.c )
target_link_libraries( merge ${3DVLIB} )

add_executable( merge_surface  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/MERGE_STRUCT/merge_surface.c )
target_link_libraries( merge_surface  3dviewnix )

add_executable( min_xor_threshold  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/min_xor_threshold.c )
target_link_libraries( min_xor_threshold  3dviewnix )

add_executable( min_xor_thresholds  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/min_xor_thresholds.c )
target_link_libraries( min_xor_thresholds  3dviewnix )

add_executable( mipg2dicom  port_data/mipg2dicom.cpp )
target_link_libraries( mipg2dicom  3dviewnix )

add_executable( mmorph  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/mmorph.c )

add_executable( morph  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/morph.c )
target_link_libraries( morph ${3DVLIB} )

add_executable( mrscaleprog  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/mrscaleprog.c )
target_link_libraries( mrscaleprog  3dviewnix )

add_executable( ndclass  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/CLASSIFY/ndclass.c )
target_link_libraries( ndclass  3dviewnix )

add_executable( ndinterpolate  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/INTERPOLATE/ndinterpolate.c )
target_link_libraries( ndinterpolate ${3DVLIB} )

add_executable( ndthreshold  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/THRESHOLD/ndthreshold.c )
target_link_libraries( ndthreshold  3dviewnix )

add_executable( ndvoi  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/VOI/ndvoi.c )
target_link_libraries( ndvoi ${3DVLIB} )

add_executable( nonrigid  registration/nonrigid.c )
target_link_libraries( nonrigid ${3DVLIB} )

add_executable( optimal_threshold  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/THRESHOLD/optimal_threshold.c )
target_link_libraries( optimal_threshold ${3DVLIB} )

add_executable( overlap_peak  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/overlap_peak.c )
target_link_libraries( overlap_peak ${3DVLIB} )

add_executable( relpos 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/relpos.c )
if (UNIX)
    target_link_libraries( relpos -lm )
else (UNIX)
    target_link_libraries( relpos )
endif (UNIX)

add_executable( reslice_proc  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/reslice_proc.c )
target_link_libraries( reslice_proc ${3DVLIB} )

add_executable( rint  aar/rint.c )

add_executable( scale_based_filtering_2D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/scale_based_filtering_2D.c )
target_link_libraries( scale_based_filtering_2D ${3DVLIB} )

add_executable( scale_based_filtering_3D  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/scale_based_filtering_3D.c )
target_link_libraries( scale_based_filtering_3D ${3DVLIB} )

add_executable( screen_params  aar/screen_params.c )
if (UNIX)
    target_link_libraries( screen_params -lm )
else (UNIX)
    target_link_libraries( screen_params )
endif (UNIX)

add_executable( shell  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/CLASSIFY/shell.c 3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/CLASSIFY/gcode.c )
target_link_libraries( shell ${3DVLIB} )

add_executable( sketch  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/sketch.c )
target_link_libraries( sketch ${3DVLIB} )

add_executable( sobel  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/sobel.c )
target_link_libraries( sobel ${3DVLIB} )

add_executable( suv_stats  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/suv_stats.c )
target_link_libraries( suv_stats ${3DVLIB} )

add_executable( texture_image  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/texture_image.c )
target_link_libraries( texture_image ${3DVLIB} )

add_executable( texture_stats  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/texture_stats.c )
target_link_libraries( texture_stats ${3DVLIB} )

add_executable( to_normal  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/to_normal.c 3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/gcode.c )
target_link_libraries( to_normal ${3DVLIB} )

add_executable( toboggan  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FILTER/toboggan.c )
target_link_libraries( toboggan ${3DVLIB} )

add_executable( track_3d  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/track_3d.c 3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/gcode.c )
target_link_libraries( track_3d ${3DVLIB} )

add_executable( track_all  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/track_all.c 3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_NORMAL/gcode.c )
target_link_libraries( track_all ${3DVLIB} )

add_executable( vote  aar/vote.c )

add_executable( xor_min  3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/MISC_OPS/xor_min.c )
target_link_libraries( xor_min  3dviewnix )
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#
#build cavass
#
set( wxWidgets_USE_LIBS base core adv html )
find_package( wxWidgets )
if (wxWidgets_FOUND)

    #include wxWidgets macros
    include( ${wxWidgets_USE_FILE} )
    include_directories( . ./3dviewnix/INCLUDE ./frames ./filter ./render
                         ./port_data ${wxWidgets_INCLUDE_DIRS} )

    # For convenience we define the sources as a variable.
    SET( SRCS
        3dviewnix/PROCESS/PREPROCESS/SCENE_OPERATIONS/FUZZ_TRACK/gqueue.cpp
        CavassData.cpp
        CavassData.h
        ChunkData.h
        #DemonsDialog.cpp
        #DemonsDialog.h
        Dicom.cpp
        Dicom.h
        DicomInfoFrame.h
        FileHistory.cpp
        FileHistory.h
        GaussianTransform.cpp
        Globals.h
        GrayScaleTransform.cpp
        Preferences.cpp
        Preferences.h
        PreferencesDialog.cpp
        PreferencesDialog.h
        ProcessManager.h
        RampTransform.cpp
        #SnakesDialog.cpp
        #SnakesDialog.h
        TrapezoidTransform.cpp
        analyze/AnalyzeStaticCanvas.cpp
        analyze/AnalyzeStaticCanvas.h
        analyze/AnalyzeStaticFrame.cpp
        analyze/AnalyzeStaticFrame.h
        analyze/DensityAlgorithm.cpp
        analyze/DensityCanvas.cpp
        analyze/DensityCanvas.h
        analyze/DensityFrame.cpp
        analyze/DensityFrame.h
        analyze/KinematicsInterCanvas.cpp
        analyze/KinematicsInterCanvas.h
        analyze/KinematicsInterFrame.cpp
        analyze/KinematicsInterFrame.h
        analyze/KinematicsIntraCanvas.cpp
        analyze/KinematicsIntraCanvas.h
        analyze/KinematicsIntraFrame.cpp
        analyze/KinematicsIntraFrame.h
        analyze/ROIStatisticalAlgorithm.cpp
        analyze/ROIStatisticalCanvas.cpp
        analyze/ROIStatisticalCanvas.h
        analyze/ROIStatisticalFrame.cpp
        analyze/ROIStatisticalFrame.h
        analyze/auto_register.cpp
        analyze/build_surf_tree.cpp
        cavass.h
        cv3dv.h
        fft.cpp
        fft.h
        filter/FilterAlgorithms.cpp
        filter/FilterCanvas.cpp
        filter/FilterCanvas.h
        filter/FilterFrame.cpp
        filter/FilterFrame.h
        filter/ITKFilterCanvas.cpp
        filter/ITKFilterCanvas.h
        filter/ITKFilterFrame.cpp
        filter/ITKFilterFrame.h
        frames/AlgebraCanvas.cpp
        frames/AlgebraCanvas.h
        frames/AlgebraFrame.cpp
        frames/AlgebraFrame.h
        frames/CineControls.h
        frames/ClassifyControls.h
        frames/ColorControls.h
        frames/CovarianceControls.h
        frames/CycleCanvas.cpp
        frames/CycleCanvas.h
        frames/CycleData.h
        frames/CycleFrame.cpp
        frames/CycleFrame.h
        frames/CyclePrint.h
        frames/EasyHeaderCanvas.cpp
        frames/EasyHeaderCanvas.h
        frames/EasyHeaderFrame.cpp
        frames/EasyHeaderFrame.h
        frames/ExampleCanvas.cpp
        frames/ExampleCanvas.h
        frames/ExampleFrame.cpp
        frames/ExampleFrame.h
        frames/ExportCanvas.cpp
        frames/ExportCanvas.h
        frames/ExportFrame.cpp
        frames/ExportFrame.h
        frames/FrameControls.h
        frames/FromDicomCanvas.cpp
        frames/FromDicomCanvas.h
        frames/FromDicomFrame.cpp
        frames/FromDicomFrame.h
        frames/FunctionControls.h
        frames/FuzzCompCanvas.cpp
        frames/FuzzCompCanvas.h
        frames/FuzzCompFrame.cpp
        frames/FuzzCompFrame.h
        frames/GrayMapControls.h
        frames/HistoSettingControls.h
        frames/IRFCCanvas.cpp
        frames/IRFCCanvas.h
        frames/IRFCControls.h
        frames/IRFCFrame.cpp
        frames/IRFCFrame.h
        frames/ImportCanvas.cpp
        frames/ImportCanvas.h
        frames/ImportFrame.cpp
        frames/ImportFrame.h
        frames/InformationDialog.cpp
        frames/InformationDialog.h
        frames/InputHistoryDialog.cpp
        frames/InputHistoryDialog.h
        frames/IntenMapCanvas.cpp
        frames/IntenMapCanvas.h
        frames/IntenMapFrame.cpp
        frames/IntenMapFrame.h
        frames/InterpolateCanvas.cpp
        frames/InterpolateCanvas.h
        frames/InterpolateFrame.cpp
        frames/InterpolateFrame.h
        frames/MagnifyControls.h
        frames/MainCanvas.cpp
        frames/MainCanvas.h
        frames/MainFileDropTarget.h
        frames/MainFrame.cpp
        frames/MainFrame.h
        frames/MainSplitter.cpp
        frames/MontageCanvas.cpp
        frames/MontageCanvas.h
        frames/MontageFrame.cpp
        frames/MontageFrame.h
        frames/MontagePrint.h
        frames/OpacityControls.h
        frames/OverlayCanvas.cpp
        frames/OverlayCanvas.h
        frames/OverlayFrame.cpp
        frames/OverlayFrame.h
        frames/OverlayPrint.h
        frames/ParameterControls.h
        frames/RegisterFrame.cpp
        frames/RegisterFrame.h
        frames/RegistrationControls.h
        frames/SaveScreenControls.h
        frames/Segment2dCanvas.cpp
        frames/Segment2dFrame.cpp
        frames/SetFuzzCompIndexControls.h
        frames/SetIndexControls.h
        frames/ShowScreenCanvas.cpp
        frames/ShowScreenCanvas.h
        frames/ShowScreenFrame.cpp
        frames/ShowScreenFrame.h
        frames/SurfOpacityControls.h
        frames/SurfPropertiesControls.h
        frames/SurfSpeedControls.h
        frames/SurfViewCanvas.cpp
        frames/SurfViewCanvas.h
        frames/SurfViewFrame.cpp
        frames/SurfViewFrame.h
        frames/SurfViewPrint.h
        frames/TextControls.h
        frames/ThresholdCanvas.cpp
        frames/ThresholdCanvas.h
        frames/ThresholdControls.h
        frames/ThresholdFrame.cpp
        frames/ThresholdFrame.h
        frames/ThresholdPrint.h
        frames/ToStructureCanvas.cpp
        frames/ToStructureCanvas.h
        frames/ToStructureFrame.cpp
        frames/ToStructureFrame.h
        frames/VOIIOICanvas.cpp
        frames/VOIIOICanvas.h
        frames/VOIIOIFrame.cpp
        frames/VOIIOIFrame.h
        frames/VOIPickSlicesCanvas.cpp
        frames/VOIPickSlicesCanvas.h
        frames/VOIPickSlicesFrame.cpp
        frames/VOIPickSlicesFrame.h
        frames/VOIROICanvas.cpp
        frames/VOIROICanvas.h
        frames/VOIROIFrame.cpp
        frames/VOIROIFrame.h
        frames/VOIStandardlizeCanvas.cpp
        frames/VOIStandardlizeCanvas.h
        frames/VOIStandardlizeFrame.cpp
        frames/VOIStandardlizeFrame.h
        main.cpp
        misc.cpp
        port_data/read_acrnema.cpp
        render/AtoM.cpp
        render/cmput_colrs.cpp
        render/cvRender.cpp
        render/do_cut.cpp
        render/gcode.cpp
        render/load.cpp
        render/make_image.cpp
        render/manip_error.cpp
        render/matrix.cpp
        render/obj_number.cpp
        render/obj_transf.cpp
        render/patch.cpp
        render/poly_cut.cpp
        render/project.cpp
        render/reflect.cpp
        render/separate.cpp
        render/shade.cpp
        render/view.cpp
        render/view_interp.cpp
    )
 
    # If we build for windows systems, we also include the resource file
    # containing the manifest, icon and other resources
    IF (MSVC)
        SET( SRCS ${SRCS} cavass.rc )
    ENDIF (MSVC)
  
    # Add cavass icon on Mac.
    if (APPLE)
        set( MACOSX_BUNDLE_ICON_FILE cavass.icns )
        set( app_ICON ${CMAKE_CURRENT_SOURCE_DIR}/cavass.icns )
        #SET( SRCS ${SRCS} cavass.icns )
        set_source_files_properties( ${app_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources" )
        add_executable( cavass MACOSX_BUNDLE ${app_ICON} ${SRCS} )
    else ()
        add_executable( cavass WIN32 ${SRCS} )
    endif (APPLE)

    target_link_libraries( cavass  ${wxWidgets_LIBRARIES} 3dviewnix libtiff )

    add_executable( from_dicom  port_data/from_dicom.cpp port_data/read_acrnema.cpp )
    target_link_libraries( from_dicom  ${wxWidgets_LIBRARIES} 3dviewnix )

    add_executable( ser_loc  port_data/ser_loc.cpp port_data/read_acrnema.cpp )
    target_link_libraries( ser_loc  ${wxWidgets_LIBRARIES} 3dviewnix )

    add_executable( estimateScale aar/estimateScale.cpp Dicom.cpp CavassData.cpp port_data/read_acrnema.cpp )
    target_link_libraries( estimateScale ${wxWidgets_LIBRARIES} 3dviewnix libtiff )

    add_executable( exportMath port_data/exportMath.cpp Dicom.cpp CavassData.cpp port_data/read_acrnema.cpp )
    target_link_libraries( exportMath ${wxWidgets_LIBRARIES} 3dviewnix libtiff )

    add_executable( importMath port_data/importMath.cpp Dicom.cpp CavassData.cpp port_data/read_acrnema.cpp )
    target_link_libraries( importMath ${wxWidgets_LIBRARIES} 3dviewnix libtiff )

    add_executable( PLN_TO_BS0  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/PLN_TO_BS0.cpp )
target_link_libraries( PLN_TO_BS0  ${wxWidgets_LIBRARIES} 3dviewnix )

    add_executable( PLN_TO_BS1  3dviewnix/PROCESS/PREPROCESS/STRUCTURE_OPERATIONS/TO_STRUCTURE/PLN_TO_BS1.cpp )
    target_link_libraries( PLN_TO_BS1  ${wxWidgets_LIBRARIES} 3dviewnix )

else (wxWidgets_FOUND)
    message( "wxWidgets not found!" )
endif (wxWidgets_FOUND)
#------------------------------------------------------------------------------
# (optional) Generate ITK executable (command line) programs.
IF (ITK_FOUND)
    INCLUDE( ${ITK_USE_FILE} )

    add_executable( BIM2Meta itk/BIM2Meta.cxx itk/itkIM0VolumeReader.h )
    target_link_libraries( BIM2Meta ${ITK_LIBRARIES} 3dviewnix )

    add_executable( IM02Meta itk/IM02Meta.cxx )
    target_link_libraries( IM02Meta ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkApproximateSignedDistanceMapImageFilter
        itk/itkApproximateSignedDistanceMapImageFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkApproximateSignedDistanceMapImageFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinaryDilateFilter itk/itkBinaryDilateFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinaryDilateFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinaryErodeFilter itk/itkBinaryErodeFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinaryErodeFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinaryOpeningFilter itk/itkBinaryOpeningFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinaryOpeningFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinaryMedianFilter itk/itkBinaryMedianFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinaryMedianFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinaryMorphFilter itk/itkBinaryMorphFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinaryMorphFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinaryThresholdFilter itk/itkBinaryThresholdFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinaryThresholdFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkBinomialBlurFilter itk/itkBinomialBlurFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkBinomialBlurFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkCannyEdgeDetectionFilter
        itk/itkCannyEdgeDetectionFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkCannyEdgeDetectionFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkCurvatureAnisotropicDiffusionFilter
        itk/itkCurvatureAnisotropicDiffusionFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkCurvatureAnisotropicDiffusionFilter
        ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkCurvatureFlowImageFilter
        itk/itkCurvatureFlowImageFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkCurvatureFlowImageFilter ${ITK_LIBRARIES} 3dviewnix libtiff )

    # needs work (update)
    #add_executable( itkDanielssonDistanceMapFilter
    #    itk/itkDanielssonDistanceMapFilter.cxx
    #    itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
    #    itk/FilterProgress.h )
    #target_link_libraries( itkDanielssonDistanceMapFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkDerivativeFilter itk/itkDerivativeFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkDerivativeFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkDifferenceImageFilter itk/itkDifferenceImageFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkDifferenceImageFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkDiscreteGaussianFilter itk/itkDiscreteGaussianFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkDiscreteGaussianFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkGradientAnisotropicDiffusionFilter
        itk/itkGradientAnisotropicDiffusionFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkGradientAnisotropicDiffusionFilter
        ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkGradientMagnitudeFilter
        itk/itkGradientMagnitudeFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkGradientMagnitudeFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkGradientMagnitudeRecursiveGaussianFilter
        itk/itkGradientMagnitudeRecursiveGaussianFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkGradientMagnitudeRecursiveGaussianFilter
        ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkGrayDilateFilter itk/itkGrayDilateFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkGrayDilateFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkGrayErodeFilter itk/itkGrayErodeFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkGrayErodeFilter ${ITK_LIBRARIES} 3dviewnix )

#    add_executable( itkMaurerDistanceMapFilter
#        itk/itkMaurerDistanceMapFilter.cxx
#        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
#       itk/FilterProgress.h )
#    target_link_libraries( itkMaurerDistanceMapFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkMeanFilter itk/itkMeanFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkMeanFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkMedianFilter itk/itkMedianFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkMedianFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkRegistration itk/itkRegistration.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkRegistration ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkRescaleFilter itk/itkRescaleFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkRescaleFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkSigmoidFilter itk/itkSigmoidFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkSigmoidFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkSignedDanielssonDistanceMapFilter
        itk/itkSignedDanielssonDistanceMapFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkSignedDanielssonDistanceMapFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkSignedMaurerDistanceMapFilter
        itk/itkSignedMaurerDistanceMapFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkSignedMaurerDistanceMapFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkSmoothRecursiveGaussianFilter
        itk/itkSmoothRecursiveGaussianFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkSmoothRecursiveGaussianFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkThresholdFilter itk/itkThresholdFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkThresholdFilter ${ITK_LIBRARIES} 3dviewnix )

    add_executable( itkVotingBinaryHoleFillingFilter
        itk/itkVotingBinaryHoleFillingFilter.cxx
        itk/itkIM0VolumeReader.h itk/itkIM0VolumeWriter.h
        itk/FilterProgress.h )
    target_link_libraries( itkVotingBinaryHoleFillingFilter ${ITK_LIBRARIES} 3dviewnix )

ENDIF (ITK_FOUND)
#------------------------------------------------------------------------------
# distance transform related programs
#
add_executable( createPhantom  distance/createPhantom.cpp distance/BoxMuller.h )
target_link_libraries( createPhantom 3dviewnix )

add_executable( specifyPhantom  distance/specifyPhantom.cpp )
target_link_libraries( specifyPhantom 3dviewnix )

IF (ITK_FOUND)
    INCLUDE( ${ITK_USE_FILE} )
    include_directories( itk )

    add_executable( simple3ddt
        distance/simple3ddt.cpp
        distance/DistanceTransform3D.h  distance/DistanceTransform3D.cpp
        distance/Simple3D.h             distance/Simple3D.cpp
        itk/itkIM0VolumeReader.h        itk/itkIM0VolumeWriter.h )
    target_link_libraries( simple3ddt ${ITK_LIBRARIES} 3dviewnix )

    add_executable( simplelist3ddt
        distance/simplelist3ddt.cpp
        distance/DistanceTransform3D.h  distance/DistanceTransform3D.cpp
        distance/SimpleList3D.h         distance/SimpleList3D.cpp
        itk/itkIM0VolumeReader.h        itk/itkIM0VolumeWriter.h )
    target_link_libraries( simplelist3ddt ${ITK_LIBRARIES} 3dviewnix )

    add_executable( ballScale
        scale/ballScale.cpp  Scale.h  BallScale.h  itk/ElapsedTime.h
        itk/itkIM0VolumeReader.h        itk/itkIM0VolumeWriter.h )
    target_link_libraries( ballScale ${ITK_LIBRARIES} 3dviewnix )
ENDIF (ITK_FOUND)
#------------------------------------------------------------------------------
# copy the 3dviewnix directory (to access configuration files) to the location of
# the executables
# This appears to no longer work in cmake!
# It has been replaced with code above (search for copy_dir).
#
if (MSVC)
    EXECUTE_PROCESS(
        COMMAND cmake -E copy_directory ${CAVASS_SOURCE_DIR}/3dviewnix/ ${CAVASS_BINARY_DIR}/Release
    )
    EXECUTE_PROCESS(
        COMMAND cmake -E copy_directory ${CAVASS_SOURCE_DIR}/3dviewnix/ ${CAVASS_BINARY_DIR}/Debug
    )
else (MSVC)
    EXECUTE_PROCESS(
        COMMAND cmake -E copy_directory ${CAVASS_SOURCE_DIR}/3dviewnix/ ${CAVASS_BINARY_DIR}
    )
endif (MSVC)

#copy tutorial information
if (MSVC)
    EXECUTE_PROCESS(
        COMMAND cmake -E copy_directory ${CAVASS_SOURCE_DIR}/tutorials/ ${CAVASS_BINARY_DIR}/Release/tutorials
    )
    EXECUTE_PROCESS(
        COMMAND cmake -E copy_directory ${CAVASS_SOURCE_DIR}/tutorials/ ${CAVASS_BINARY_DIR}/Debug/tutorials
    )
else (MSVC)
    EXECUTE_PROCESS(
        COMMAND cmake -E copy_directory ${CAVASS_SOURCE_DIR}/tutorials/ ${CAVASS_BINARY_DIR}/tutorials
    )
endif (MSVC)
#------------------------------------------------------------------------------
if (PARALLEL)
    include_directories( . ${BUILD_PARALLEL_INC} )
    add_executable( p3dinterpolate
        parallel/p3dinterpolate.cpp parallel/p3dinterpolateMaster.cpp
        parallel/p3dinterpolateSlave.cpp parallel/p3dinterpolate.h
        parallel/ElapsedTime.h )
    if (MSVC)
        target_link_libraries( p3dinterpolate 3dviewnix ${BUILD_PARALLEL_LIB}/mpi Ws2_32 )
    else (MSVC)
        target_link_libraries( p3dinterpolate 3dviewnix ${BUILD_PARALLEL_LIB}/libmpich.a )
    endif (MSVC)
    add_executable( p3dLTDT
        parallel/p3dLTDT.cpp parallel/p3dLTDTMaster.cpp
        parallel/p3dLTDTSlave.cpp parallel/p3dLTDT.h
        parallel/ElapsedTime.h )
    if (MSVC)
        target_link_libraries( p3dLTDT 3dviewnix ${BUILD_PARALLEL_LIB}/mpi Ws2_32 )
    else (MSVC)
        target_link_libraries( p3dLTDT 3dviewnix ${BUILD_PARALLEL_LIB}/libmpich.a )
    endif (MSVC)
    add_executable( p3dFuzzyConn
        parallel/p3dFuzzyConn.cpp parallel/chash.cpp parallel/hheap.cpp parallel/chash2.cpp parallel/p3dFuzzyConn.h parallel/chash.h
        parallel/ElapsedTime.h )
    if (MSVC)
        target_link_libraries( p3dFuzzyConn 3dviewnix ${BUILD_PARALLEL_LIB}/mpi Ws2_32 )
    else (MSVC)
        target_link_libraries( p3dFuzzyConn 3dviewnix ${BUILD_PARALLEL_LIB}/libmpich.a )
    endif (MSVC)
    add_executable( p3dSBAFilter
        parallel/p3dSBAFilter.cpp parallel/ElapsedTime.h )
    if (MSVC)
        target_link_libraries( p3dSBAFilter 3dviewnix ${BUILD_PARALLEL_LIB}/mpi Ws2_32 )
    else (MSVC)
        target_link_libraries( p3dSBAFilter 3dviewnix ${BUILD_PARALLEL_LIB}/libmpich.a )
    endif (MSVC)
    add_executable( p3dBScaleCompute
        parallel/p3dBScaleCompute.cpp
        parallel/ElapsedTime.h )
    if (MSVC)
        target_link_libraries( p3dBScaleCompute 3dviewnix ${BUILD_PARALLEL_LIB}/mpi Ws2_32 )
    else (MSVC)
        target_link_libraries( p3dBScaleCompute 3dviewnix ${BUILD_PARALLEL_LIB}/libmpich.a )
    endif (MSVC)
endif (PARALLEL)
#------------------------------------------------------------------------------

