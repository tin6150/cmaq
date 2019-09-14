
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
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PDM/src/plrise/plrise/part.f,v 1.1.1.1 2005/09/09 19:20:54 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C %W% %P% %G% %U%

C ********************************************************************
      SUBROUTINE PART( ISTK, NLAYS, ZR, BOT, TOP, FRACT, KBOT, KTOP, JFLAG)
C*********************************************************************
C INPUTS --
C        NLAYS   I       NUMBER OF VERTICAL LAYERS 
C        ZR      R       HT. OF Z-LAYER AT STACK   (M)      
C        BOT     R       BOTTOM OF PLUME AT FINAL PLUME RISE   
C        TOP     R       TOP OF PLUME AT FINAL PLUME RISE
C
C OUTPUTS --
C        FRACT   R       PLUME FRACTIONS IN EACH LAYER 
C        KBOT    I       LAYER AT BOTTOM OF PLUME
C        KTOP    I       LAYER AT TOP OF PLUME
C        JFLAG   I       ERROR FLAG
C
C LOCAL VARIABLES--
C        AMISG   R       MISSING VALUE FOR REAL NUMBER
C        BFRAC   R       BOTTOM FRACTION OF PLUME
C        DEPTH   R       DISTANCE BETWEEN BOTTOM AND TOP OF PLUME
C        KBOTMAX I       MAXIMUM LEVEL FOR BOTTOM OF PLUME
C        KTOPMIN I       MINIMUM LEVEL FOR TOP OF PLUME
C        TFRAC   R       TOP FRACTION OF PLUME
C        TOTAL   R       TOTAL FRACTIONS--SHOULD =1.00
C        ZERO    R       ZERO CONSTAN FOR REAL NUMBER
C
C ROUTINES CALLED --
C        NONE                         
C
C ERROR CONDITIONS
C
C   -1   ERROR IN SUB PART - TOP LOWER THAN ZR(KTOP)
C   -2   ERROR IN SUB PART-BOT HIGHER THAN ZR(KBOT+1)
C   -3   ERROR IN SUB PART - ERROR IN FRAC CALCULATION

      IMPLICIT NONE

C DECLARE VARIABLES

      INTEGER KK,K, K1, K2, KBOT, KBOTMAX, KTOP, KTOPMIN, NLAYS, JFLAG
      INTEGER ISTK
      REAL AMISG, BFRAC, BOT, DEPTH, TFRAC, TOP, TOTAL, ZERO
      REAL ZR( NLAYS+1 ), FRACT( NLAYS )

      DATA AMISG /-999.99/, ZERO /0.0/
      SAVE AMISG, ZERO
 
C---- Subroutine to find layer number between BOT and TOP
 
      DO K = 1,NLAYS
         FRACT( K ) = 0.0
      END DO
 
      IF ( BOT .EQ. AMISG ) THEN
         KBOT = 1
         KTOP = 1
         FRACT(KBOT) = 1.0
      ELSE IF ( TOP .LE. ZR( 2 ) ) THEN
         KBOT = 1
         KTOP = 1
         FRACT( KBOT ) = 1.0
      ELSE
         KBOTMAX = NLAYS
         KTOPMIN = 1
         K1 = 1 
C---- Find the layers for the top and bottom of plume 
         DO K = 1, NLAYS+1
            IF ( BOT .GE. ZR( K ) ) K1 = K
            IF ( TOP .GE. ZR( K ) ) K2 = K
            KBOT = MIN( KBOTMAX, K1 )
            KTOP = MAX( KTOPMIN, K2 )
         END DO
         IF ( KBOT .GE. KTOP ) THEN
            KBOT = KTOP
            FRACT( KTOP ) = 1.0
         ELSE

            DEPTH = TOP - BOT
            TFRAC = TOP - ZR(KTOP)
            BFRAC = ZR( KBOT+1 ) - BOT

            IF ( TFRAC .LT. ZERO ) THEN
               JFLAG = -1
               WRITE( *,1100 ) ' JFLAG,BOT,TOP,KBOT,KTOP,K,ZR=',
     &         JFLAG,BOT,TOP,KBOT,KTOP,(K, ZR( K ), K=1,NLAYS+1)
               RETURN
            ELSE IF ( BFRAC .LT. ZERO ) THEN
               JFLAG = -2
               WRITE( *,1200 ) ' JFLAG,BOT,TOP,KBOT,KTOP,K,ZR=',
     &         JFLAG,BOT,TOP,KBOT,KTOP,(K, ZR( K ), K=1,NLAYS+1)
               RETURN
            ELSE
            END IF

            FRACT( KTOP ) = TFRAC / DEPTH
            FRACT( KBOT ) = BFRAC / DEPTH
     
            IF ( KBOT .LE. ( KTOP-2 ) ) THEN
               DO K= KBOT+1, KTOP-1
                  FRACT( K ) = ( ZR( K+1 ) - ZR( K ) ) / DEPTH
               END DO
            END IF
         END IF

      END IF

C---- Check whether sum of FRACT is 1
      TOTAL = 0.0
      DO K = 1, NLAYS
         TOTAL = TOTAL + FRACT( K )
      END DO

      IF ( ABS( TOTAL - 1.0 ) .GE. 0.01 ) THEN
C---- Error in fraction calculation
         JFLAG = -3
         WRITE( *,* ) ' ERR IN PART: ',ISTK,JFLAG,TOTAL,
     &   BOT,TOP,KBOT,KTOP,(K,FRACT( K ), K=KBOT,KTOP),
     &                  (KK, ZR(KK), KK=1,NLAYS+1)
         RETURN
      END IF

      RETURN
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
C FORMATS
1100   FORMAT('ERROR IN SUB PART - TOP LOWER THAN ZR(KTOP)',/,
     &        'ERROR RETURN FLAG= ',I4,/,
     &        'BOT= ',F9.4,1X,'TOP= ',F9.4,1X,'KBOT= ',I4,1X,
     &        'KTOP= ',I4,/,
     &        'K, ZR( K )= ',10(I4,1X,F9.4))
1200   FORMAT('ERROR IN SUB PART-BOT HIGHER THAN ZR(KBOT+1)',/,
     &        'ERROR RETURN FLAG= ',I4,/,
     &        'BOT= ',F9.4,1X,'TOP= ',F9.4,1X,'KBOT= ',I4,1X,
     &        'KTOP= ',I4,/,
     &        'K, ZR( K )= ',10(I4,1X,F9.2))
!1300   FORMAT('ERROR IN SUB PART - ERROR IN FRAC CALCULATION',/,
!     &        'ERROR RETURN FLAG= ',I4,2X,'TOTAL = ',F6.3) 
!     &        'BOT= ',F9.4,1X,'TOP= ',F9.4,1X,'KBOT= ',I4,1X,
!     &        'KTOP= ',I4,/,
!     &        'K, FRACT( K )= ',10(I4,1X,F9.4),/,
!     &        'K, ZR( K )= ',10(I4,1X,F9.4))

      END