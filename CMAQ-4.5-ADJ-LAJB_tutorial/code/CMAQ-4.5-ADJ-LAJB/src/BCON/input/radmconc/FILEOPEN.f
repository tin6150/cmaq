
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

C**********************************************************************

C RCS file, release, date & time of last delta, author, state, [and locker]
C $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/BCON/src/input/radmconc/FILEOPEN.f,v 1.1.1.1 2005/09/09 19:25:17 sjr Exp $ 

C what(1) key, module and SID; SCCS file; date and time of last delta:
C @(#)FILEOPEN.f	2.1 /project/mod3/ICON/src/input/radmconc/SCCS/s.FILEOPEN.f 17 May 1997 08:33:50

*DK FILEOPEN
      SUBROUTINE FILEOPEN(IOLUN,NAME,LFMT)
C#######################################################################
C**     opens file 'name' on assigned fortran lun 'iolun',
C**     as either formatted or unformatted depending on 'lfmt'.
C**    revision history:
C**    no.    date    who  what
C**    __    _____    ___  ____________________________________________
C**    01    89150    jkv  implementation
C**
C**    input files:  none
C**
C**    output files:none
C***********************************************************************
C     ----------
      CHARACTER*(*) NAME
      INTEGER IOLUN,IOST
      LOGICAL LFMT
      LUNOUT=6
      IF (LFMT) THEN
         OPEN(UNIT=IOLUN,FILE=NAME,STATUS='UNKNOWN',FORM='FORMATTED',
     1        IOSTAT=IOST)
         IF (IOST .NE. 0) THEN
           WRITE(LUNOUT,10) IOLUN, NAME, IOST
10         FORMAT(' ERROR ON FORMATTED OPEN, UNIT: ',I2,' NAME: ',A30,
     1            '  IOSTATUS: ',I8)
           stop
         ELSE
           WRITE(LUNOUT,11) IOLUN,NAME
11         FORMAT(1x,78('*')/' **** FORMATTED OPEN, UNIT: ',I3,
     1            ', FILE: ',A30,'   ****'/1x,78('*'))
         END IF
      ELSE
         OPEN(UNIT=IOLUN,FILE=NAME,STATUS='UNKNOWN',FORM='UNFORMATTED',
     1        IOSTAT=IOST)
         IF (IOST.NE.0) THEN
           WRITE(LUNOUT,101) IOLUN, NAME, IOST
101        FORMAT(' ERROR ON UNFORMATTED OPEN, UNIT: ',I2,' NAME: ',
     1     A30,'  IOSTATUS: ',I8)
           stop
         ELSE
           WRITE(LUNOUT,111) IOLUN,NAME
111        FORMAT(1x,78('*')/'*** UNFORMATTED OPEN, UNIT: ',I3,
     1            ', FILE: ',A30,'   ****'/1x,78('*'))
         END IF
      ENDIF
      RETURN
      END