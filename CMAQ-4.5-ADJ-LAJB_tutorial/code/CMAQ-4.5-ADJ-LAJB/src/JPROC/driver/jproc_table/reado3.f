
C***********************************************************************
C   Portions of Models-3/CMAQ software were developed or based on      *
C   information from various groups: Federal Government employees,     *
C   contractors working on a United States Government contract, and    *
C   non-Federal sources (including research institutions).  These      *
C   research institutions have given the Government permission to      *
C   use, prepare derivative works, and distribute copies of their      *
C   work in Models-3/CMAQ to the public and to permit others to do     *
C   so.  EPA therefore grants similar permissions for use of the       *
C   Models-3/CMAQ software, but users are requested to provide copies  *
C   of derivative works to the Government without restrictions as to   *
C   use by others.  Users are responsible for acquiring their own      *
C   copies of commercial software associated with Models-3/CMAQ and    *
C   for complying with vendor requirements.  Software copyrights by    *
C   the MCNC Environmental Modeling Center are used with their         *
C   permissions subject to the above restrictions.                     *
C***********************************************************************


C RCS file, release, date & time of last delta, author, state, [and locker]
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/JPROC/src/driver/jproc_table/reado3.f,v 1.1.1.1 2005/09/09 19:22:14 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C @(#)reado3.F	1.2 /project/mod3/JPROC/src/driver/jproc_table/SCCS/s.reado3.F 04 Jul 1997 09:40:19

C:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
      SUBROUTINE READO3 ( NWL, STWL, ENDWL, O3ABS )
         
C*********************************************************************
C
C  the subroutine reads the absorption cross section
C     The input data are
C
C     O3ABS - absorption cross sections for molecular oxygen
C
C*********************************************************************

      IMPLICIT NONE

      INCLUDE 'JVALPARMS.EXT'    ! jproc parameters

C...........PARAMETERS and their descriptions
      
      INTEGER      XSTAT1             ! I/O ERROR exit status
      PARAMETER  ( XSTAT1 = 1 )

      INTEGER      XSTAT2             ! Program ERROR exit status
      PARAMETER  ( XSTAT2 = 2 )

C...........ARGUMENTS and their descriptions
      
      REAL         ENDWL( MXWL )       ! wavelength band upper limit
      REAL         O3ABS( MXWL )       ! output absorp. cross sections
      REAL         STWL ( MXWL )       ! wavelength band lower limit

C...........LOCAL VARIABLES and their descriptions:

      CHARACTER*1  TYPE                ! cs/qy spectra type
      CHARACTER*16 O3FILE              ! input filename buffer
      DATA         O3FILE   / 'O3ABS           ' /
      CHARACTER*16 PHOTID              ! reaction id's
      CHARACTER*16 PNAME               ! program name
      DATA         PNAME   / 'READO3' /
      CHARACTER*255 EQNAME
      CHARACTER*80 MSG                 ! message
      DATA         MSG / '    ' /

      INTEGER      IOST                ! i/o status
      INTEGER      IWL                 ! wavelength index
      INTEGER      NWL                 ! # of wlbands
      INTEGER      NWLIN               ! # of wlbands (infile)
      INTEGER      O3UNIT              ! cross section io unit

      REAL         FACTOR              ! multiplying factor for CS
      REAL         CSOUT( MXWL )       ! integrated absorp. cross sect.
      REAL         WLIN( MXWLIN )      ! wl for input cs/qy data
      REAL         CSIN( MXWLIN )      ! raw absorption cross sections

C...........EXTERNAL FUNCTIONS and their descriptions:

      INTEGER      JUNIT               ! used to get next IO unit #

C*********************************************************************
C     begin body of subroutine

C...get a unit number for CSQY files

      CALL NAMEVAL ( O3FILE, EQNAME )
      O3UNIT = JUNIT( )

C...open input file

      OPEN( UNIT = O3UNIT,
     &      FILE = EQNAME,
     &      STATUS = 'OLD',
     &      IOSTAT = IOST )

C...check for open errors

      IF ( IOST .NE. 0) THEN
        MSG = 'Could not open the O3ABS data file'
        CALL M3EXIT( PNAME, 0, 0, MSG, XSTAT1 )
      END IF

      WRITE( 6, 2001 ) O3UNIT, EQNAME

C...read photolysis subgroup id

      READ( O3UNIT, 1001, IOSTAT = IOST ) PHOTID

C...check for read errors

      IF ( IOST .NE. 0 ) THEN
        MSG = 'Errors occurred while reading PHOTID from O3ABS file'
        CALL M3EXIT( PNAME, 0, 0, MSG, XSTAT1 )
      END IF

C...get type of data (e.g. centered, beginning, ending, or point wavelen

101   CONTINUE

      READ( O3UNIT, 1003, IOSTAT = IOST ) TYPE

C...check for read errors

      IF ( IOST .NE. 0) THEN
        MSG = 'Errors occurred while reading TYPE from O3ABS file'
        CALL M3EXIT( PNAME, 0, 0, MSG, XSTAT1 )
      END IF

      IF ( TYPE .EQ. '!' ) GO TO 101

C...read the factor to multiply cross sectionS by

      READ( O3UNIT, 1005, IOSTAT = IOST ) FACTOR

C...check for read errors

      IF ( IOST .NE. 0 ) THEN
        MSG = 'Errors occurred while reading FACTOR from O3ABS file'
        CALL M3EXIT( PNAME, 0, 0, MSG, XSTAT1 )
      END IF

C...reinitialize arrays

      DO IWL = 1, MXWL
        WLIN( IWL ) = 0.0
        CSIN( IWL ) = 0.0
      END DO

C...loop over the number of wavelengths and continue reading

      IWL = 0
201   CONTINUE

        IWL = IWL + 1
        READ( O3UNIT, *, IOSTAT = IOST ) WLIN( IWL ), CSIN( IWL )
        CSIN( IWL ) = CSIN( IWL ) * FACTOR

C...check for read errors

        IF ( IOST .GT. 0 ) THEN
          MSG = 'Errors occurred while reading WL,CS from O3ABS file'
          CALL M3EXIT( PNAME, 0, 0, MSG, XSTAT1 )
        END IF

C...end loop if we reach EOF, otherwise continue looping

      IF ( IOST .EQ. 0 ) GO TO 201

C...adjust loop counter index index and close file

      NWLIN = IWL - 1
      CLOSE( O3UNIT )

      WRITE( 6, 2003 ) NWLIN

C...transform the cs data to the same wavelength intervals as
C...  the irradiance data.

        CALL INTAVG ( WLIN, CSIN, NWLIN, TYPE,
     &                STWL, ENDWL, CSOUT, NWL )

C...load output arrays with integrated data
        
      DO IWL = 1, NWL
        O3ABS( IWL ) = CSOUT( IWL )
      END DO


C...formats

1001  FORMAT( A16 )
1003  FORMAT( A1 )
1005  FORMAT( /, 4X, F10.1 )

2001  FORMAT( 1X, '...Opening File on UNIT ', I2, /, 1X, A255 )
2003  FORMAT( 1X, '...Data for ', I4, ' wavelengths read from file',
     &        // )

      RETURN
      END