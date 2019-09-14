C RCS file, release, date & time of last delta, author, state, [and locker] 
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PARIO/src/pm3warn.f,v 1.1.1.1 2005/09/09 16:18:17 sjr Exp $

        SUBROUTINE PM3WARN( CALLER, JDATE, JTIME, ERRTXT )
C.....................................................................
C
C  PURPOSE:   Wrapper for M3WARN in parallel environment, to 
C             add the processor-id suffix to the name of the
C             caller.
C
C
C  PRECONDITIONS REQUIRED:  Same as for M3WARN.
C
C
C  REVISION  HISTORY:
C       Original version 07/1998 by Al Bourgeois.
C       Modified 12/07/1998 by Al Bourgeois to add EXTERNAL declarations.
C
C
C  ARGUMENT LIST DESCRIPTION:
C  IN:
C     CHARACTER*(*)   CALLER          ! Name of the caller.
C     INTEGER         JDATE           ! Model date for the error.
C     INTEGER         JTIME           ! Model time for the error.
C     CHARACTER*(*)   ERRTXT          ! Error message.
C
C     COMMON BLOCK PIOVARS:
C     INTEGER  MY_PE               !  Local processor id.
C
C
C  SUBROUTINES AND FUNCTIONS CALLED:  M3WARN.
C
C***********************************************************************

      IMPLICIT NONE

C...........   INCLUDES:

      INCLUDE 'IODECL3.EXT'      ! I/O definitions and declarations.
      INCLUDE 'PIOVARS.EXT'      ! Parameters for parallel implementation.


C...........   ARGUMENTS and their descriptions:

      CHARACTER*(*)   CALLER          ! Name of the caller.
      INTEGER         JDATE           ! Model date for the error.
      INTEGER         JTIME           ! Model time for the error.
      CHARACTER*(*)   ERRTXT          ! Error message.


C.......   EXTERNAL FUNCTIONS:

      INTEGER        TRIMLEN          ! Effective char. string length.
      EXTERNAL       M3WARN, TRIMLEN  ! M3IO library.


C...........   LOCAL VARIABLES

      INTEGER      LENSTR       ! String length of CALLER.
      CHARACTER*3  CMYPE        ! Processor ID string.
      CHARACTER*7  PE_STR       ! String suffix to go with processor ID.
      CHARACTER*16 CALL16       ! First 16 characters of CALLER.
      CHARACTER*26 PCALLER      ! New caller string with PE information.

      INTEGER LOGDEV

C.............................................................................
C   begin subroutine PM3WARN
  

      LOGDEV = INIT3()

C.......  Create strings to append to CALLER.
      WRITE (PE_STR,'(A7)') ' on PE '
      WRITE(CMYPE,'(I3.3)') MY_PE


C.......  Construct new CALLER string.
      LENSTR = MIN( 16, TRIMLEN(CALLER) )
      CALL16 = CALLER( 1: LENSTR )
      PCALLER = CALL16(1:LENSTR)//PE_STR//CMYPE

C.......  Pass the new sting to M3WARN.
      CALL M3WARN( PCALLER, JDATE, JTIME, ERRTXT )

      RETURN
      END
  