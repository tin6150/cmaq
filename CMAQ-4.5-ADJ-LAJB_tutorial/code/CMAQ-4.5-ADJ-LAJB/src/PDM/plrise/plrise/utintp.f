
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
C   the MCNC Environmental Modeling Center are used with their       *
C   permissions subject to the above restrictions.                   *
C***********************************************************************


C RCS file, release, date & time of last delta, author, state, [and locker]
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PDM/src/plrise/plrise/utintp.f,v 1.1.1.1 2005/09/09 19:20:54 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C %W% %P% %G% %U%

C*************************************************************
      SUBROUTINE UTINTP( UL, TL, ZL, NLAYS, HPR, KPR,
     &                   TSFC, UPR, TPR, JFLAG )
C*************************************************************
C** Uses the simplest linear interpolation to estimate U,T(UPR,TPR)
C   at a specified height HPR

      IMPLICIT NONE

      INTEGER NLAYS, KPR, JFLAG
      REAL    UL( NLAYS ), TL( NLAYS ), ZL( NLAYS )
      REAL    HPR, TSFC, UPR, TPR

      INTEGER KPRM1
      REAL ZL0, UL0, TL0, FRACDZ

      JFLAG = 0
 
      IF ( KPR .LE. 0 ) THEN
         JFLAG = -1
         WRITE( *,* ) ' ERROR on height'
         RETURN
      END IF
 
      KPRM1 = KPR - 1
      IF ( KPR .EQ. 1 ) THEN
         ZL0 = 0.0     ! or Zo
         UL0 = 0.0     ! or u* ?
         TL0 = TSFC
      ELSE
         ZL0 = ZL( KPRM1 )
         UL0 = UL( KPRM1 )
         TL0 = TL( KPRM1 )
      END IF
 
      FRACDZ = ( HPR - ZL0 ) / ( ZL( KPR ) - ZL0 )
      UPR = UL0 + ( UL( KPR ) - UL0 ) * FRACDZ
      TPR = TL0 + ( TL( KPR ) - TL0 ) * FRACDZ
 
      RETURN
      END