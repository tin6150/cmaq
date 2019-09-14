
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
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/PDM/src/pldyn/pldyn/mnew.f,v 1.1.1.1 2005/09/09 19:20:54 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C %W% %P% %G% %U%

C**************************************************************
      SUBROUTINE MNEW ( ITOP, TOP, DZABOV, HTMIX, DELMIX, DZBELO,
     &                  IBOT, BOT, DELBOT, THT, BHT, TURBL, IPLMFLG )
C**************************************************************

C** FUNCTION : Determines vertical plume spread and considers if 
C   plume top or bottom are inside/outside PBL then decide which 
C   situation to be applied

C ....ITOP=1 , plume top still remains outside PBL just after
C                emission. Then if plume top extends into PBL ,ITOP=0 .

C ....ITOP=0 , plume top enters PBL from outside , or plume top
C                goes out of the PBL from inside ;
C                means that the plume is not fresh .
C
C ....ITOP=-1, plume top remains in the mixing layer just after emission.
C                Then if plume top extends to outside the PBL , ITOP=0 .

      IMPLICIT NONE

      REAL   TOP, DZABOV, HTMIX, DELMIX, DZBELO, DELBOT, THT, BHT
      REAL   BOT, DROP, DROPC, TURBL, TURBLC, DEPTH, DEPTH2

      INTEGER IBOT, ITOP, IPLMFLG
      DATA TURBLC /0.0/

C** decide plume top ..
!     WRITE(*,*) 'IN MNEW: ', ITOP, IBOT

      IF ( ITOP .EQ. 1 ) THEN
C ....ITOP=1
         THT = TOP + DZABOV
         IF ( THT .LT. HTMIX )  THEN
            THT = HTMIX
            ITOP = 0
         END IF
      ELSE IF ( ITOP .EQ. 0 .AND. DELMIX .GT. 0.0 ) THEN
C ....ITOP=0
         THT = MAX( HTMIX,TOP )

         IF ( IPLMFLG .EQ. 3 ) THEN
            DROP  = TOP - HTMIX
            DROPC = TOP - 100.
            IF ( DROP .GE. 0.5 * DROPC .AND. TURBL .GT. TURBLC ) THEN
               IPLMFLG = 13     !ZI drops and nite, so dump plume 
            END IF
         END IF
 
      ELSE IF ( ITOP .EQ. 0 .AND. DELMIX .LE. 0.0 ) THEN
         THT = TOP

      ELSE IF ( ITOP .EQ. -1 ) THEN
C ....ITOP=-1
         THT = TOP + DZBELO
         IF ( THT .GE. HTMIX ) THEN
            THT = MAX( TOP, HTMIX )
            ITOP = 0
         END IF
      ELSE
C ....in case ITOP =\= -1, 0 ,or 1.

      PAUSE ' In MNEW: ERROR 1 in ITOP'
      END IF

C .......................................................................
C ....if IBOT=1 , plume bottom outside the mixing layer .
C        Then if plume bottom extends into PBL , IBOT=-1.
C
C ....if IBOT=-1 , plume bottom inside the mixing layer.
C        Then if plume bottom extends outside the PBL , IBOT=-1.

C** decide plume bottom ..

      IF ( IBOT .EQ. 1 ) THEN
C ....if IBOT=1
          BHT = MAX( 0.0, BOT - DZABOV )
          IF ( BHT .LT. HTMIX ) THEN
             BHT = MAX( 0.0, BOT - DZBELO )
             IBOT = -1
!      write( *, *) ' MNEW:', IBOT, BOT, DZABOV, BHT
          END IF
      ELSE IF ( IBOT .EQ. -1 ) THEN
C ....if IBOT=-1
          IF ( BOT .GT. 0.0 ) THEN
             BHT = MAX( 0.0, BOT - DZBELO )
             IF ( DZBELO .LT. 10.0 ) THEN
                BHT = MAX( 0.0, BOT - DELBOT * 0.8 )
             END IF

             IF ( BHT .GT. HTMIX ) THEN
                IBOT = 1
             END IF
 
             IF ( BOT .LT. HTMIX .AND. IPLMFLG .EQ. 5 ) THEN
                BHT = HTMIX
                IBOT = 2
             END IF
          ELSE
             BHT = 0.0
          END IF
 
      ELSE IF ( IBOT .EQ. 2 ) THEN
          BHT = HTMIX
          IF ( IPLMFLG .EQ. 3 ) THEN
            BHT = AMAX1( 0.0, BOT - DZBELO )
            IBOT = -1
          END IF
      ELSE

C  Plume bottom already touched the ground (BOT=0.)
      END IF

C  Test for plume thickness change.
      DEPTH = TOP - BOT
      DEPTH2 = THT - BHT
      IF ( DEPTH2 .LT. DEPTH .AND. BHT .GT. BOT ) BHT = BOT

!     WRITE(*,*) 'FOR TOP: ', ITOP, TOP, THT, HTMIX, DELMIX
!     WRITE(*,*) 'FOR BOT: ', IBOT, BOT, BHT, DEPTH, DEPTH2 

      RETURN
      END