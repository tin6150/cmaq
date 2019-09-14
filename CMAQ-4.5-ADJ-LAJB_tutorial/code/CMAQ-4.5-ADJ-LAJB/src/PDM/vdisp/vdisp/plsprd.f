
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
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PDM/src/vdisp/vdisp/plsprd.f,v 1.1.1.1 2005/09/09 19:20:54 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C %W% %P% %G% %U%

C***************************************************************
      SUBROUTINE PLSPRD ( DTHDZ, ZF, KZ, CEFSTK, HTMIX, SZ0FAC,
     &                    SPRFAC, PLTOP, PLBOT, DPTH )
C***************************************************************
C** FUNCTION: Calculate the initial vertical spread of plume.
C   Modified from PRSPRD (Gillani's model)
   
C   INPUTS:
C       CEFSTK : effective stack height  (=HS+DH)
C       DTHDZ  : potential temperature lapsed rate
C       HTMIX  : mixing height
C       SPRFAC : an empirical plume spread factor
C       SZ0FAC : an empirical plume spread factor

C   OUTPUT:
C       DPTH : plume depth
C       PLTOP  : plume top
C       PLBOT  : plume bottom

      IMPLICIT NONE

      INTEGER K, KZ
      REAL   CEFSTK, HTMIX, SZ0FAC, SPRFAC, PLTOP, PLBOT, DPTH
      REAL   DTHDZ( KZ ), ZF( 0:KZ ), GAMA, SIGZ0, DTDZ 

      DATA GAMA / -0.0098 /
      SAVE GAMA

C---- Get ambient temperature above plume rise height(effective stack ht.)
      K = 0
11    CONTINUE
      K = K + 1
      IF ( K .EQ. KZ ) GO TO 12
      IF ( CEFSTK .GT. ZF( K ) ) GO TO 11
12    CONTINUE
C---- Compute initial vertical spread.

      DTDZ = DTHDZ( K ) + GAMA
C---- Limit on temperature gradient not to exceed adiabatic rate, GAMA
      DTDZ = MAX( DTDZ, GAMA )
      SIGZ0 = MAX( 3.0, SPRFAC * EXP( -117.0 * DTDZ ) )
      DPTH = SZ0FAC * SIGZ0

!     WRITE( *,* ) 'IN PLSPRD:', K, ZF( K ), CEFSTK
!     WRITE( *,* ) 'IN PLSPRD;', DTDZ, SIGZ0, DPTH

C---- Compute plume top and bottom heights (plume is either
C     completely within or outside mixing layer)

      PLTOP = CEFSTK + DPTH / 2.0
      PLBOT = CEFSTK - DPTH / 2.0
C---- Minimum bottom for plume is zero
      PLBOT = MAX( 0.0, PLBOT )
      PLTOP = MIN( ZF( KZ ), PLTOP )
      PLBOT = MIN( ZF( KZ ) - 1.0, PLBOT)
!     WRITE( *,* ) PLTOP, PLBOT

      RETURN
      END