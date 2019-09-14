
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
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PDM/src/plrise/plrise/neutral.f,v 1.1.1.1 2005/09/09 19:20:54 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C %W% %P% %G% %U%

C *********************************************************************
      SUBROUTINE NEUTRAL( HS, BFLUX, UPR, USTAR, DH, IQ )
C**********************************************************************

C FUNCTION: Computes plume rise under neutral conditions

C IN --
C    HS      R        Stack Height
C    BFLUX   R        Buoyancy Flux
C    UPR     R        Interpolated Wind Speed
C    USTAR   R        USTAR at stack location

C OUT --
C    DH      R        Plume Rise Height (m)
C    IQ      I        Flag that neutral equation used

C Local Variables --
C    ARG1    R
C    ARG2    R
C    ARGA    R
C    ARGB    R
C    ARGC    R

C ROUTINES called : None

      IMPLICIT NONE
                 
C DECLARE VARIABLES

      REAL    ARG1, ARG2, ARGA, ARGB, ARGC, BFLUX, DH, HS, UPR, USTAR 
      INTEGER IQ
      REAL, PARAMETER :: WSMIN  = 1.0E-01      ! Min wind speed, to avoid divide by zero
      REAL, PARAMETER :: USTAR0 = 1.0E-02      ! Min U*, to avoid divide by zero
C---- Compute neutral plume rise
      UPR = MAX( UPR, WSMIN )
      USTAR = MAX( USTAR, USTAR0 )
      ARG1 = BFLUX / ( UPR * USTAR * USTAR )
      ARG2 = HS + 1.3 * ARG1
C---- Replace to prevent numerical overflow
      ARGA = ARG1 ** 0.2
      ARGB = ARG2 ** 0.2
      ARGC = ARGA * ARGA * ARGA * ARGB * ARGB

C---- Limit DH (plume rise)  by 5*HS FOR MEPSE (stack height) 
!jmg  (DH <= 5hs)
!jmg  DH= MIN( 10.0 * HS, 1.2 * ARG3 **0.2 )

!     DH = MIN( 5.0 * HS, 1.2 * ARGC )
      DH = MIN(10.0 * HS, 1.2 * ARGC )   ! Use 10 to compare to plmris
      IQ = 2

      RETURN
      END