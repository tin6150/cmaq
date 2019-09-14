C RCS file, release, date & time of last delta, author, state, [and locker] 
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PARIO/src/gtndxhdv.f,v 1.1.1.1 2005/09/09 16:18:17 sjr Exp $

      LOGICAL FUNCTION GTNDXHDV ( FILE, VAR, JDATE, JTIME, VARSIZE,
     &                            MXNVARHD, NBVS, ENDBUF, VLISTHD,
     &                            SIZEHD, BUFPOSHD, VX, NEWVAR, PTR_COUNT ) 
C ....................................................................
 
C  PURPOSE:  This function serves the Models-3 parallel interpolation
C            routine PINTERP3. It finds the index of the file variable
C            VAR in file FILE from the list of file variables with
C            existing read buffers. If VAR is not in the list, the
C            list is extended, dimensions are checked and stored, and
C            buffer pointers are set. Otherwise all that is done is a
C            dimension check against the original stored values.
              
C  RETURN VALUE:  Fails if the buffered variable dimensions are not
C                 equal to their originally stored values).
 
C  REVISION HISTORY: 
C       Original version  6/96 by Al Bourgeois for parallel implementation.
C       Modified 12/15/1997 by Al Bourgeois to set NEWVAR (i.e., fix bug).
C       Modified 04/14/1998 by Al Bourgeois to avoid private M3IO references.
C       Modified 07/14/1998 by AJB for some comments regarding COLDIM, ROWDIM.
C       Modified 10/06/1998 by AJB to replace NCOLS, NROWS, NLAYS with VARSIZE.
C       Modified 12/07/1998 by Al Bourgeois to add EXTERNAL declarations.
C       Modified 07/14/1999 by Al Bourgeois to add TYPE argument.
C       Modified 08/06/1999 by Al Bourgeois to call PM3EXIT if NBVS exceeds
C                           maximum value MXNVARHD.
 
C  ARGUMENT LIST DESCRIPTION:
C  IN:
C     CHARACTER*16   FILE               ! Name of file to be read
C     CHARACTER*16   VAR                ! File variable name
C     INTEGER        JDATE              ! Current Julian date (YYYYDDD)
C     INTEGER        JTIME              ! Current time (HHMMSS)
C     INTEGER        VARSIZE            ! Local processor size of variable
C     INTEGER        MXNVARHD           ! Max possible buffered file variables
 
C  IN/OUT:
C     INTEGER        NBVS               ! Number of variables in list
C     INTEGER        ENDBUF             ! Offset for end of buffer plus one
C     CHARACTER*33   VLISTHD( MXNVARHD )! Dynamic array of variable ids
C     INTEGER        SIZEHD  ( 3*NBVS ) ! Dimensions of buffered variables
 
C  OUT:
C     INTEGER        BUFPOSHD( 2*NBVS ) ! Offsets into buffer for each variable
C     INTEGER        VX                 ! Index of file variable in VLISTHD
C     LOGICAL        NEWVAR             ! Flag to indicate new variable
 
C  LOCAL VARIABLE DESCRIPTION:  see below
 
C  CALLS:  M3WARN
 
C .......................................................................

        IMPLICIT  NONE

C ARGUMENTS:

      CHARACTER*16   FILE               ! Name of file to be read
      CHARACTER*16   VAR                ! File variable name
      INTEGER        JDATE              ! Current Julian date (YYYYDDD)
      INTEGER        JTIME              ! Current time (HHMMSS)
      INTEGER        VARSIZE            ! Local processor size of variable
      INTEGER        MXNVARHD           ! Max possible buffered file variables
      INTEGER        NBVS               ! Number of variables in list
      INTEGER        ENDBUF             ! Offset for end of buffer plus one
      CHARACTER*33   VLISTHD( MXNVARHD )! Dynamic array of variable ids
c     INTEGER        SIZEHD  ( 3*NBVS ) ! Dimensions of buffered variables
      INTEGER        SIZEHD  ( MXNVARHD ) ! Dimensions of buffered variables
c     INTEGER        BUFPOSHD( 2*NBVS ) ! Offsets into buffer for each variable
      INTEGER        BUFPOSHD( MXNVARHD ) ! Offsets into buffer for each variable
      INTEGER        VX                 ! Index of file variable in VLISTHD
      LOGICAL        NEWVAR             ! Flag to indicate new variable
      INTEGER        PTR_COUNT

C INCLUDE FILES:

      INCLUDE 'IODECL3.EXT'     ! M3IO definitions and declarations

C EXTERNAL FUNCTIONS:

      INTEGER       INDEX1        ! Name-table lookup.
      INTEGER       TRIMLEN       ! Effective char. string length.

      EXTERNAL      INDEX1, TRIMLEN    ! M3IO library.
!     EXTERNAL      PM3WARN, PM3EXIT   ! Parallel M3IO library.

C LOCAL VARIABLES: 

      CHARACTER*33  FVNAME        ! Concatenated file_variable name.
      CHARACTER*80  MSG           ! Buffer for building error messages.

C .......................................................................
C     begin function GTNDXHDV

C Concatenate file and variable names to make a unique string.

      FVNAME = FILE( 1:TRIMLEN( FILE ) )
     &       // '.' // VAR( 1:TRIMLEN( VAR ) )

C Get index for file variable.

      VX = INDEX1( FVNAME, NBVS, VLISTHD )

      IF ( VX .EQ. 0 ) THEN      ! new variable, extend list, etc.  

C Extend the list.

         NEWVAR = .TRUE.
         NBVS = NBVS + 1

         IF ( NBVS .GT. MXNVARHD ) THEN
            MSG = 'Max number of buffered file variables exceeded.'
            CALL M3WARN ( 'GTNDXHDV', JDATE, JTIME, MSG )           
            GTNDXHDV = .FALSE.
            RETURN
         END IF

         VLISTHD(NBVS) = FVNAME
         VX = NBVS

C Set buffer positions for start:end buffers.
c        BUFPOSHD( 0, VX ) = ENDBUF 
c        BUFPOSHD( 1, VX ) = ENDBUF + VARSIZE
         PTR_COUNT = PTR_COUNT + 1
c        BUFPOSHD( 2, VX ) = PTR_COUNT
         BUFPOSHD( VX ) = PTR_COUNT
         ENDBUF = ENDBUF + 2*VARSIZE

C Store variable size.
        
         SIZEHD( VX ) = VARSIZE

      ELSE    ! old variable: check dimensions against original values.

         NEWVAR = .FALSE.

         IF ( VARSIZE .NE. SIZEHD( VX ) ) THEN
            MSG = 'Variable size is inconsistent with previous values.'
            CALL M3WARN ( 'GTNDXHDV', JDATE, JTIME, MSG )           
            GTNDXHDV = .FALSE.
            RETURN 
         END IF

      END IF    ! End if new variable.

C Function completed successfully.

      GTNDXHDV = .TRUE.

      RETURN
      END