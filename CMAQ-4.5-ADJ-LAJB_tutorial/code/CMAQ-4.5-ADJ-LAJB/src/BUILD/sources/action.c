/* RCS file, release, date & time of last delta, author, state, [and locker] */
/* $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/BUILD/src/sources/action.c,v 1.1.1.1 2005/09/09 19:26:42 sjr Exp $  */

/* what(1) key, module and SID; SCCS file; date and time of last delta: */
/* %W% %P% %G% %U% */

/*****************************************************************************
 *
 * Project Title: Environmental Decision Support System
 * File: @(#)action.c	11.1
 *
 * COPYRIGHT (C) 1995, MCNC--North Carolina Supercomputing Center
 * All Rights Reserved
 *
 * See file COPYRIGHT for conditions of use.
 *
 * Environmental Programs Group
 * MCNC--North Carolina Supercomputing Center
 * P.O. Box 12889
 * Research Triangle Park, NC  27709-2889
 *
 * env_progs@mcnc.org
 *
 * Pathname: /pub/storage/edss/framework/src/model_mgr/bldmod/SCCS/s.action.c
 * Last updated: 29 Jan 2001         
 *
 ****************************************************************************/

/*
 * action.c
 * 
 * Purpose: build a model based on specified parameters
 *
 * Created by Steve Fine, MCNC, 6/9/94
 * 919-248-9255, fine@mcnc.org
 *
 * revised by Tim Turner, 7/17/95
 * to add argument for compiler flags to CreateCompileCommand()
 *
 *
 * Sections of code
 * Section 1. File-wide declarations
 * Section 2. Utilities
 * Section 3. Retrieve files
 * Section 4. Build program
 * Section 5. Determine if file must be compiled
 * Section 6. Entry point for file
 */

/*
 * standard debugging is based on symbol defined.
 * advanced debugging available if set to 2
 */
/* #define DEBUG_ACTION 2 */

#if _WIN32
#define unlink _unlink
#define popen _popen
#define pclose _pclose
#define chdir _chdir
#define llstat _stat
#include <direct.h>
#include <io.h>
#include "utsname.h"
#include <malloc.h>

#else
#include <unistd.h>
#include <sys/utsname.h>
#define llstat stat
#endif
 

#include <sys/types.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

/*
 * force inclusion of popen/pclose declarations
 */
/* #define _XOPEN_SOURCE */
#include <stdio.h>

#include "sms.h"

char SCCSID[] = "@(#)action.c	11.1 /pub/storage/edss/framework/src/model_mgr/bldmod/SCCS/s.action.c 08 Jul 1996 16:41:26";


/********************************************************************
 * Section 1. File-wide declarations
 ********************************************************************/

/*
 * the command specification for the libraries
 */
char szLibraryString[] = "";

#define MAX_FULL_FILE_NAME_LENGTH (NAME_SIZE + NAME_SIZE)
#define MAX_COMMAND_LENGTH 10000

typedef struct
	{
	time_t mtime;	/* last modification time */
	ino_t st_ino;		/* i-node number of file */
	off_t st_size;		/* # of bytes in file */
	boolean_t bShouldCompile;
	boolean_t bIsInclude;
	boolean_t bWasRetrieved;
	boolean_t bAffectedByGrid;	/* does file contain SUBST_GRID_ID? */
	} supplemental_file_info;


     
#ifdef _WIN32
int uname( struct utsname* system )
{
  strcpy( system->sysname, "WIN32");
  strcpy( system->nodename, "");
  strcpy( system->release, "March 1998");
  strcpy( system->version, "1.2");
  strcpy( system->machine, "Window-95/NT");
  return 1;
}
#endif



char szSpace[] = " ";

int DetermineIfCompNecessary(model_def_type *pglobal, 
	file_list_type *plist, supplemental_file_info *psuppFiles,
	int *pnInclude, int nIncludeFileCt);
int SaveObjDescription(model_def_type *pglobal, 
	supplemental_file_info *psuppFiles,
	file_struct_type *pdescSource, supplemental_file_info *psuppSource,
	int *pnInclude, int nIncludeFileCt, boolean_t bAffectedByGrid);

/********************************************************************
 * Section 2. Utilities
 ********************************************************************/

/* Print a message and return to caller.
 * Caller specifies "errnoflag". */

static void
err_doit(int errnoflag, const char *fmt, va_list ap)
{
    int     errno_save;
    char    buf[1000];

    errno_save = errno;     /* value caller might want printed */
    vsprintf(buf, fmt, ap);
    if (errnoflag)
        sprintf(buf+strlen(buf), ": %s", strerror(errno_save));
    strcat(buf, "\n");
    fflush(stdout);     /* in case stdout and stderr are the same */
    fputs(buf, stderr);
    fflush(stderr);     /* SunOS 4.1.* doesn't grok NULL argument */
    return;
}


/* Nonfatal error related to a system call.
 * Print a message and return. */
 
void
err_ret(const char *fmt, ...)
{
    va_list     ap;
 
    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    return;
}


/*
//  search for str1 in str2 and return position */
int strscan( char* str1, char* str2)
{
  int i;
  int len1 = strlen(str1);
  int len2 = strlen(str2);

  if( len1==0 || len1>len2 ) return -1;

  for (i=0; i<=len2-len1 ; i++)
      if (strncmp(str1,&str2[i],len1) ==0 ) return i;
  return -1;

  }

/*  function to compute substring  */
void substr( char* str1, int kstart, int klen, char* sub )
{
 int i;
 for (i=0; i<klen && str1[kstart+i]!='\0'; i++) sub[i] = str1[kstart+i];
 sub[i]='\0';
 return;
}

/*  function to replace first found substring with new substring */
int strtran( char* strng, char* sub1, char* sub2 )
{
 int j,i=0;
 int len1 = strlen(sub1);
 int len2 = strlen(sub2);
 int lenstr;
 char *tail;

 if((j=strscan(sub1,strng))>=0 )
  {
   i=1;
   lenstr = strlen(strng);
   tail=malloc(lenstr);     /* create string to store tail of string */
   if(tail==NULL) return -1;
   tail[0]=0;

   substr(strng,j+len1,lenstr-j-len1,tail);   /* save trailing string  */
   strng[j]='\0';                             /* trim strng at position k */
   strcat(strng,sub2);        /* cat sub2 to strng  */
   strcat(strng,tail);        /* cat endstr to strng */
   free( tail );
  }

 return i;
}

/*  function to replace all substrings with new substrings */
/*  replaces all sub1's with sub2  */
int strtrans( char* strng, char* sub1, char* sub2 )
{
 int j,p=0,i=0;
 int len1 = strlen(sub1);
 int len2 = strlen(sub2);
 int lenstr;
 char *tail;

 while((j=strscan(sub1,&strng[p]))>=0 )
  {
   i++;
   lenstr = strlen(strng) + len2 - len1;
   tail=malloc(lenstr);     /* create string to store tail of string */
   if(tail==NULL) return -1;
   tail[0]=0;

   j+=p;
   substr(strng,j+len1,lenstr-j-len1,tail);   /* save trailing string  */
   strng[j]='\0';                             /* trim strng at position k */
   strcat(strng,sub2);        /* cat sub2 to strng  */
   strcat(strng,tail);        /* cat endstr to strng */
   free( tail );
   p=j+len2;
  }

 return i;
}


/**  function to count number of occurances of substring  **/
int strcount( char* sstr, char* cmpline )
{
 int j,i=0,num=0;
 while ( (j=strscan(sstr, &cmpline[i]) ) >=0) { num++; i+=j+1; }
 return num;
}


/**  function to extract -D string from cmd line  */
int extractD( char* cmpline, char* Dline )
{
 int i,j,istart,iend,kswit;
 int lenstr = strlen(cmpline);
 char *tmpline;
 tmpline = malloc(lenstr);

 /*  find first -D string */
 if ((istart=strscan("-D", cmpline )) < 0)
   {
    Dline='\0';
    free ( tmpline );
    return 0;
   }
 
 /* find end of -D string, must be followed by =' ' characters  */
 /* or -Dxxxxxxxx=yyyyyyy is assumed to be a subsitution */
 /* or -Dxxxxxxxx is assumed to be a definition */
 kswit=0;
 while (kswit==0)
  {
   for(i=istart+1; i<lenstr && cmpline[i]!='\0' && kswit<3; i++ )
    {
      if(kswit==2 && cmpline[i]==39) { kswit++; iend=i; }
      if(kswit==1 && cmpline[i]==39) kswit++;
      if(kswit==0 && cmpline[i]==61) kswit++;

      /*  check for definition */ 
      if((kswit==0) && (cmpline[i]==32 || cmpline[i]==34))
        {
        for(j=i-1;j>istart && cmpline[j]==32; j--);   /* find last nonblank character */
        if(j > istart+1) { iend=i-1; kswit=3; }  /* assume definition */
        }

      /*  check for subsitution */ 
      if((kswit==1) && (cmpline[i]==32 || cmpline[i]==34))
        {
        for(j=i-1;j>istart && cmpline[j]==32; j--);   /* find last nonblank character */
        if(j > istart+1) { iend=i-1; kswit=3; }  /* assume subsitution */
        }
    }
 
   if(kswit==0) kswit=1;  /* if kswit==0, then set it to 1 so loop will exit */
 
  }
    
 
 /* if kswit != 3, then complete -D string not found */

  if( kswit != 3 )
   {
    printf("\n **Warning** complete -D string not found");
    Dline='\0';
    free ( tmpline );
    return -1;
   }
    
 /* copy substring from cmpline and assign to Dline  */
  strcpy( Dline, " " );
  substr(cmpline,istart,iend-istart+1,&Dline[1]);
    
 /* remove substring from cmpline  */
  strcpy( tmpline, &cmpline[iend+1] );
  cmpline[istart] = '\0';
  strcat( cmpline, tmpline );
    
  free ( tmpline );
  return 1;
}   
 


/**  process compile line for Win-32 compile   **/
int Win32line( char* cmpline )
{
 int i,j,k,k2,status;
 int lenstr = strlen(cmpline);
 char srcfile[256];
 char forfile[256];
 char infile[256];
 char outfile[256];
 char dstring[256];
 char *ppline;
 ppline = malloc(lenstr+256);


   /*  replace all backslashes used for line continuations with spaces  */
   for(i=0; cmpline[i]!='\0'; i++) if(cmpline[i]==92 && cmpline[i+1]<=32)
           cmpline[i]=' ';

   /* replace newline characters with spaces for DOS run  */
   for (i=0; cmpline[i] != '\0'; i++) if(cmpline[i]=='\n') cmpline[i]=32;

   /* replace /D characters with -D  */
   for (i=0; cmpline[i] != '\0'; i++)
            if(cmpline[i]==' ' && cmpline[i+1]=='/' && cmpline[i+2]=='D') cmpline[i+1]='-';

   /*  delete leading .\ from start for filenames */
   while( (strtran(cmpline," .\\"," ")) > 0) ;

   /* remove /m option from cmpline (used in pp) */
   strtran(cmpline,"/m "," ");


   /* replace empty '' with space */
   strtran(cmpline,"=''","=");

   /* remove space before object file */
   while( strtran(cmpline, "/object: ", "/object:") > 0 ) ;


   /*  check for preprocessor is needed */
   if ( ( (strscan(".F ",cmpline) > 0) || (strscan(".f ",cmpline) > 0) ) &&
        (strscan(" -D",cmpline) > 0) )
     {

        /* find location of source filename and save to srcfile*/
        if((k=strscan(".F ",cmpline)) < 0 )  k=strscan(".f ",cmpline);
        k2 = k+2;
        for(; k>=0 && cmpline[k]!=' '; k--);   /* backtrack to space from k */
        substr(cmpline,k,k2-k+1,srcfile);      /*  copy source filename to srcfile */
        srcfile[k2-k]='\0';

        /* set infile */
        strcpy( infile, srcfile );

        /* set outfile */     
        for(k=strlen(srcfile); k>=0 && srcfile[k]!='\\'; k--);
        strcpy( outfile, &srcfile[k+1] );
        strtran(outfile,".F",".i");
        strtran(outfile,".f",".i");
        strcpy( forfile, outfile );
        strtran(forfile,".i",".for");

       
        /* build and execute pp lines (maximum length of 3000 characters) */
        strcpy(ppline, "fpp.exe /P /nologo /m " );  /* build ppline */

        while ((k=extractD(cmpline,dstring)) > 0 )
        {
           strcat(ppline, dstring);
           if( strlen(ppline) > 3000 )
           {
              strcat( ppline," " );
              strcat( ppline, infile );
              strcat( ppline," " );
              strcat( ppline, outfile );
              if(status=system(ppline)) return status; /* run fpp on infile, return on error */

              /* rebuild ppline */
              strcpy(ppline, "fpp.exe /P /nologo /m " );  
              strcpy( infile, outfile );
              strcat( outfile, "i" );
           }           
        }

        /*  make final fpp run with output of *.for */
        strcat( ppline," " );
        strcat( ppline, infile );
        strcat( ppline," " );
        strcat( ppline, forfile );
        if(status=system(ppline)) return status; /* run fpp on infile, return on error */

        /*  modify compile line cmpline  */
        strtran( cmpline, srcfile, forfile );
        status=system(cmpline);                      /* compile */

        /*  delete pp files */
        strtran(forfile,".for",".i*");
        strcpy(dstring,"del /F ");
        strcat(dstring,forfile);
        /* system(dstring); */
     }
     else
     {
       status=system(cmpline);
     }

 free (ppline);
 return 0;
}

 
/**  process compile line for Cray compile   **/
int crayline( char* cmpline, int verbose )
{
 int i,j,k,k2,status;
 int lenstr = strlen(cmpline);
 char *ppline;
 char *dline;
 ppline = malloc(lenstr+80);
 dline = malloc(lenstr+80);
 
 
 /*   build string for preprocessor  */
   if( strscan("cf77",cmpline)<0 )
       strcpy(ppline,"f90 -eP -f fixed -F ");
   else
       strcpy(ppline,"cpp -C -P -N -I. ");

 /*  check cmpline for -N compile option and copy to ppline if found */
   if( strscan("-N80",cmpline)>0 ) strcat(ppline,"-N80 ");
   if( strscan("-N132",cmpline)>0 ) strcat(ppline,"-N132 ");
    
 /*  append -D strings to ppline */
   while ((k=extractD(cmpline,dline)) > 0 )
    {
      strtrans(dline,"'","\\'");
      strcat(ppline, dline);
    }
   strtran(ppline,"=\\'\\'","=");               /* replace empty '' with space */

   /* find location of source filename and extract to dline*/
   if((k=strscan(".F ",cmpline)) < 0 )  k=strscan(".f ",cmpline);
   k2 = k+2;
   for(; k>=0 && cmpline[k]!=' '; k--);   /* backtrack to space from k */
   substr(cmpline,k,k2-k+1,dline);        /*  copy source filename to dline */
   strcat(ppline,dline);                 /*  append source filename to ppline */
   dline[k2-k-2]='\0';

   /*  copy sed command if using cf77 */
   if( strscan("cf77",cmpline)>=0 )
   {
      strcat(ppline," | sed -g s/\\?\\^/\\?\\?\\?\\'/ > ");
      strcat(dline,".j");                /*  build output filename for cpp */
      for(k=strlen(dline); k>=0 && dline[k]!='/' ; k--);   /* backtrack to '/' from k */
      strcat(ppline,&dline[k+1]);
   }

   /*   run ppline  */
   if( verbose ) printf("\n%s",ppline);
   status=system( ppline );


/* replace long filename with short filename */
   for(k=strlen(dline); k>=0 && dline[k]!='/' ; k--);   /* backtrack to '/' */
   strtran(cmpline, dline, &dline[k+1] );
   

   /*  copy i file to .f90 if using f90 */
   if( strscan("cf77",cmpline)<0 )
   {
      strcpy( ppline, "mv -f ");
      strcat(ppline,&dline[k+1]);
      strcat(ppline,".i ");
      strcat(ppline,&dline[k+1]);
      strcat(ppline,".f90");
      if( verbose ) printf("\n%s",ppline);
      status=system( ppline );
   }


 /*Change input source filename */
   if( strscan("cf77",cmpline)>=0 )
   {
      strtran(cmpline, ".f ", ".j ");
      strtran(cmpline, ".F ", ".j ");
   }
   else
   {
      strtran(cmpline, ".f ", ".f90 ");
      strtran(cmpline, ".F ", ".f90 ");
      strtran(cmpline, "-c ", "-c -f fixed ");  /* add -f fixed to compile options */
   }
   
   if( verbose ) printf("\n%s \n",cmpline);
   status=system( cmpline );
    
 free (ppline);
 free (dline);
 return status;
}


/*** Compile Line with preprocessor   ****/
int compileline( char* cmpline, char* cppflags, char* fpp )
{
 int i,j,k,k2,status;
 int lenstr = strlen(cmpline);   
 char *ppline;
 char *dline;
 ppline = malloc(lenstr+1024);
 dline = malloc(lenstr+1024);
 

 /*   build string for preprocessor  */
   strcpy(ppline,fpp);
   strcat(ppline," "); 
   strcat(ppline,cppflags); 
 
 /*  append -D strings to ppline */
   while ((k=extractD(cmpline,dline)) > 0 )
    {
      strcat(ppline, dline);   
    }
   strtran(ppline,"=\\'\\'","=");               /* replace empty '' with space */
 
 /* modify ppline string to execute */
   /* find location of source filename */
   if((k=strscan(".F ",cmpline)) < 0 )  k=strscan(".f ",cmpline);
   k2 = k+2;
   for(; k>=0 && cmpline[k]!=' ' ; k--);   /* backtrack to space from k */
   substr(cmpline,k,k2-k+1,dline);         /*  copy source filename to dline */
   strcat(ppline,dline);                   /*  append source filename to ppline */
   dline[k2-k-2]='\0';
 
   strcat(dline,".for");                      /*  build output filename for cpp */
   for(k=strlen(dline); k>=0 && dline[k]!='/' ; k--);   /* backtrack to '/' from k */
   strcat(ppline,&dline[k+1]);
   status=system( ppline );
 
 /*Change input source filename */
   strtran(cmpline, ".f ", ".for ");
   strtran(cmpline, ".F ", ".for ");
  
/* replace long filename with short filename */
   strtran(cmpline, dline, &dline[k+1] );
 
   /*  replace all backslashes used for line continuations with spaces  */
   for(i=0; cmpline[i]!='\0'; i++) if(cmpline[i]==92 && cmpline[i+1]<=32)
           cmpline[i]=' ';

   /* replace newline characters with spaces  */
   for (i=0; cmpline[i] != '\0'; i++) if(cmpline[i]=='\n') cmpline[i]=32;

   status=system( cmpline );
    
 free (ppline);
 free (dline);
 return status;
}
 
 
/*** ExecuteCommand *************************************************
 *
 * Execute a command, possibly displaying the command to standard
 * output before execution.
 *
 * return = value returned by command
 *
 * pglobal = pointer to "global" information
 * pszCmd = command to execute
 */
int ExecuteCommand(model_def_type *pglobal, char *pszCmd)
{
  int i;
  int status;
   
        if (pglobal->mode == parse_mode) return 0;
 
#ifdef CRAY
        if (pglobal->verbose && strscan(".F",pszCmd)<0 && strscan(".f",pszCmd)<0)
                 printf("%s\n\n", pszCmd);
#else
        if (pglobal->verbose ) printf("%s\n\n", pszCmd);
#endif
 
        if (!pglobal->show_only)
          {
#if _WIN32
              /* process compile line for Win 32 */
              if (strscan("df.exe",pszCmd) >= 0 )
                status=Win32line( pszCmd );
              else
                status=system(pszCmd);
              return status;
#endif
 
#ifdef CRAY
              /* process compile line for CRAY */
              if (( strscan(".F",pszCmd)>=0 || strscan(".f",pszCmd)>=0 )
                && (strscan("f77",pszCmd)>=0 || strscan("f90",pszCmd)>=0 ) )
              {
                printf("\n\nUsing preprocessor\n");
                status=crayline( pszCmd, pglobal->verbose );
              }
              else
              {
                /* run system command */
                status=system(pszCmd);
	      }
              return status;
#endif

#if __sun || __sgi
            /*   if compile line call function to run preprocessor */
            if (strscan(".F",pszCmd)+strscan(".f",pszCmd) >= 0 ) return (compileline( pszCmd, pglobal->cpp_flags,  pglobal->fpp ));
#endif

#ifdef AIX
            /*   if compile line change "-D" to "-WF,-D" in pszCmd string */
            if (strscan(".F",pszCmd)+strscan(".f",pszCmd) >= 0 )
              {
              while( strtran(pszCmd, " -D", " -WF,-D") > 0);
              }
#endif


            /* execute command without modifications */
            status=system(pszCmd);
            return status;
          }
        else
            return 0;
}       /* ExecuteCommand */
 

/*** IsCompilable ****************************************************
 *
 * Indicate if the file is a source file that is not an include
 * file based on the "language" flag.
 *
 * return = B_TRUE if include file, B_FALSE otherwise
 *
 * language = flag indicating the "language" for the file
 */
boolean_t IsCompilable(lang_type language)
{
	switch (language)
		{
		case h_lang :
		case ext_lang :
		case hh_lang :
		case other_lang :
			return B_FALSE;

		case c_lang :
		case c_plus_plus_lang :
		case f_lang :
			return B_TRUE;

		default:
			assert(0);
			break;
		}
}	/* IsCompilable */


/*** IsIncludeFile **************************************************
 *
 * Indicate if the file is an include file based on its "language"
 * flag.
 *
 * return = B_TRUE if include file, B_FALSE otherwise
 *
 * language = flag indicating the "language" for the file
 */
boolean_t IsIncludeFile(lang_type language)
{
	switch (language)
		{
		case h_lang :
		case ext_lang :
		case hh_lang :
			return B_TRUE;

		case c_lang :
		case c_plus_plus_lang :
		case f_lang :
		case other_lang :
			return B_FALSE;

		default:
			assert(0);
			break;
		}
}


/*** CompareFiles ***************************************************
 *
 * Compare the names of two files and return a value that indicates
 * ordering.  This is used in conjunction with qsort.
 *
 * return = < 0 if file1 < file2, etc.
 *
 * ppdesc1 = description of file 1
 * ppdesc2
 */
int CompareFiles(const void *ppdesc1, const void *ppdesc2)
{

	return strcmp((*(file_struct_type **) ppdesc1)->filename,
		(*(file_struct_type **) ppdesc2)->filename);
}


/*** PrintDupe ******************************************************
 *
 * Print the description of a file that is a duplicate.  The
 * information is sent to stderr.
 *
 * pdesc = information about the file
 */
void PrintDupe(file_struct_type *pdesc)
{
	if (pdesc->where == file_src)
		fprintf(stderr, "   %s%s\n", pdesc->dirname, pdesc->filename);
	else
		fprintf(stderr, "   Module: %s   File: %s\n",
			pdesc->module_name, pdesc->filename);
}	/* PrintDupe */

/*** CheckForDuplicateFiles *****************************************
 *
 * Check the list of files and see if there are duplicate file names.
 * Such duplicates could cause file overwriting during the retrieve
 * and build process.  Print any duplicates to standard error.
 *
 * return = ERROR_DUPE if a duplicate file is found or 0 if no problems
 *
 * plist = the structure containing file descriptions
 */
int CheckForDuplicateFiles(file_list_type *plist)
{
	boolean_t bDupeFound = B_FALSE;
	int nReturnValue = 0;
	file_struct_type **ppdescSort;	/* pointers to file descriptors; used
									 * for sorting file names */
	file_struct_type **ppdescLead;
	file_struct_type **ppdescTail;
	file_struct_type **ppdescEnd;
	int nFile;

	ppdescSort = (file_struct_type **) malloc(sizeof(file_struct_type *) *
		plist->list_size);
	if (ppdescSort == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	/*
	 * fill the array with pointers to the file descriptions
	 */
	ppdescEnd = ppdescSort + plist->list_size;
	for (ppdescLead = ppdescSort, nFile = 0; ppdescLead < ppdescEnd; 
		++ppdescLead, ++nFile)
		{
		*ppdescLead = &plist->list[nFile];
		}

	/*
	 * sort the pointers by file name
	 */
	qsort(ppdescSort, plist->list_size, sizeof(file_struct_type *),
		CompareFiles);

	/*
	 * print the names of any duplicate files
	 */
	for (ppdescTail = ppdescSort, ppdescLead = ppdescTail + 1;
		ppdescLead < ppdescEnd; ppdescTail = ppdescLead++)
		{
		if (strcmp((*ppdescTail)->filename, (*ppdescLead)->filename) == 0)
			{
			nReturnValue = ERROR_DUPE;
			fprintf(stderr, "Duplicate file names:\n");
			PrintDupe(*ppdescLead);
			PrintDupe(*ppdescTail);
			}
		}


done:
	if (ppdescSort) free(ppdescSort);
	return nReturnValue;
}

/*** AppendWithCount ************************************************
 *
 * Append one string to another using and maintaining the length
 * of the base string.  Produce an error if the new string
 * would be too long.
 *
 * return = error code
 *
 * pszBase = string to append to
 * pnBaseLength = length of the base string before and after
 *		appending (excluding the trailing null)
 * nMaxBaseLength = the maximum length for the base string (excluding
 *		the trailing null)
 * pszAppendage = string to append to the base string
 * bAppendSpace = append space to the end of the total string
 */
int AppendWithCount(char *pszBase, int *pnBaseLength,
	int nMaxBaseLength, char *pszAppendage, boolean_t bAppendSpace)
{
	int nAppendageLength;
	int nNewLength;

	assert(pszBase != NULL && pszAppendage != NULL);

	nAppendageLength = strlen(pszAppendage);
	nNewLength = *pnBaseLength + nAppendageLength;
	if (bAppendSpace) nNewLength++;
	if (nNewLength > nMaxBaseLength) 
		{
		fprintf(stderr, "AppendWithCount: new length = %d, max length = %d\n",
			nNewLength, nMaxBaseLength);
		return ERROR_STRING_LENGTH;
		}
	else
		{
		memcpy(pszBase + *pnBaseLength, pszAppendage, 
			nAppendageLength + 1);
		if (bAppendSpace)
			memcpy(pszBase + nNewLength - 1, szSpace, 2);
		*pnBaseLength = nNewLength;
		return 0;
		}
}	/* AppendWithCount */

/*** SetNestedInclude ***********************************************
 *
 * Return the name of a nested include file.  The grid ID is inserted
 * before the last period in the file name.  If there is no period,
 * the grid ID is appended to the end of the name.
 *
 * The calling routine is responsible for ensuring that the pszNewName
 * buffer is large enough to hold the new name.
 *
 * pszOrigName = the name of the include file before including
 *		the grid ID
 * pszGridID = the ID of the grid
 * pszNewName = the name of the include file incorportating the grid ID
 */
void SetNestedInclude(char *pszOrigName, char *pszGridID,
	char *pszNewName)
{
	char *psz;

	/*
	 * find the last period
	 */
	for (psz = pszOrigName + strlen(pszOrigName); psz >= pszOrigName &&
		*psz != '.'; --psz);

	if (psz >= pszOrigName)
		/*
		 * a period was found.  Insert the grid ID.
		 */
		{
		int nLength;

		nLength = psz - pszOrigName;
		if (nLength) memcpy(pszNewName, pszOrigName, nLength);
		memcpy(pszNewName + nLength, pszGridID, strlen(pszGridID) + 1);
		strcat(pszNewName, psz);
		}
	else
		/*
		 * append the grid ID
		 */
		{
		strcpy(pszNewName, pszOrigName);
		strcpy(pszNewName, pszGridID);
		}
}	/* SetNestedInclude */


/*** AppendIncludeDefinitions ******************************************
 *
 * Put preprocessor definitions for include files into the given
 * string.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pszDefinitions = string for the definitions.
 * pnDefinitionsLength = the length for the string (excluding NULL)
 * nMaxDefinitionsLength = the maximum length for the string (excluding NULL)
 * pszNestName = the string to be placed before each file's extension.
 *		If NULL, this argument is ignored.
 */
int AppendIncludeDefinitions(model_def_type *pglobal, 
	char *pszDefinitions, int *pnDefinitionsLength,
	int nMaxDefinitionsLength, char *pszNestName)
{
	int nReturnValue = 0;
	int nInclude;
	char *pszNewName = NULL;
	char *pszIncludeName;
	char *pszOrigIncludeName = NULL;
	char szRoot[50];

	pszOrigIncludeName = malloc(NAME_SIZE + NAME_SIZE);
	if (pszOrigIncludeName == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	if (pszNestName != NULL)
		{
		pszNewName = malloc(NAME_SIZE + NAME_SIZE);
		if (pszNewName == NULL)
			{
			nReturnValue = ERROR_MEMORY_FULL;
			goto done;
			}
		}

	if (nReturnValue = AppendWithCount(pszDefinitions, pnDefinitionsLength,
                nMaxDefinitionsLength, pglobal->os==win_32 ? " \n " : " \\\n  " , B_FALSE))
		goto done;

	if (pglobal->os == cray_os)
		/*
		 * these definitions must be passed to the preprocessor
		 */
		{
		if (nReturnValue = AppendWithCount(pszDefinitions, pnDefinitionsLength,
			nMaxDefinitionsLength, "-Wp\"-F", B_TRUE))
			goto done;
		}

	for (nInclude = 0; nInclude < pglobal->includes.list_size; ++nInclude)
		{
		if (pszNestName == NULL && 
			pglobal->includes.list[nInclude].apply_grid_names)
			/*
			 * the file isn't dependent on the grid so don't include
			 * a file that is dependent on a grid
			 */
			continue;

		strcpy(pszOrigIncludeName, pglobal->includes.list[nInclude].dirname);
		strcat(pszOrigIncludeName, pglobal->includes.list[nInclude].filename);
		if (pszNestName != NULL &&
			pglobal->includes.list[nInclude].apply_grid_names)
			/*
			 * construct the new name for the include file
			 */
			{
			SetNestedInclude(pszOrigIncludeName, pszNestName, pszNewName);
			pszIncludeName = pszNewName;
			}
		else pszIncludeName = pszOrigIncludeName;

		  if( pglobal->os == win_32 ) sprintf(szRoot, " /D%s=%s",
			pglobal->includes.list[nInclude].symbolname, "'" );
		  else
                    sprintf(szRoot, " -D%s=%s", pglobal->includes.list[nInclude].symbolname, 
			pglobal->os == cray_os ? "'" : "\\\"" );
		if (nReturnValue = AppendWithCount(pszDefinitions, pnDefinitionsLength,
			nMaxDefinitionsLength, szRoot, B_FALSE))
			goto done;
		if (nReturnValue = AppendWithCount(pszDefinitions, pnDefinitionsLength,
			nMaxDefinitionsLength, pszIncludeName, B_FALSE))
			goto done;
		if (nReturnValue = AppendWithCount(pszDefinitions, pnDefinitionsLength,
			nMaxDefinitionsLength, 
                        pglobal->os == cray_os ? "' " : pglobal->os == win_32 ? "' \n " :
                        "\\\" \\\n  ", B_FALSE))
			goto done;
		}

	if (pglobal->os == cray_os)
		{
		if (nReturnValue = AppendWithCount(pszDefinitions, pnDefinitionsLength,
			nMaxDefinitionsLength, "\"", B_TRUE))
			goto done;
		}

done:
	if (pszNewName != NULL) free(pszNewName);
	if (pszOrigIncludeName != NULL) free(pszOrigIncludeName);
	return nReturnValue;
}	/* AppendIncludeDefinitions */


/*** GetRand ************************************************
 * 
 * Get a random integer.  The properties of the random sequence
 * may not be great, but they should be sufficient for this
 * program.
 *
 * return = random integer
 *
 * Steve Fine, fine@mcnc.org, 10/11/94
 *
 */
int GetRand(void)
{
	static boolean_t bRandInitialized = B_FALSE;

	if (!bRandInitialized) 
		{
		srand(time(NULL));
		bRandInitialized = B_TRUE;
		}

	return rand();
}	/* GetRand */


/*** RestoreMigratedFiles ****************************************
 *
 * Restore migrated files.  This is currently only useful for
 * the Cray.  
 *
 * This pulls back all source and object files in the current
 * directory and its descendants.  It could be optimized to
 * pull back only the files that are required for this build
 * but that gets complicated when nested grids are used, so
 * we'll do this for now.
 *
 *       *****************************************************
 *       * The routine RestoreMigratedFiles also has a list
 *       * of recognized extensions.  These two lists should
 *       * be kept in sync.
 *       *****************************************************
 *
 *
 * pglobal = pointer to "global" information
 *
 * Steve Fine, fine@mcnc.org, 10/17/94
 */
void RestoreMigratedFiles(model_def_type *pglobal)
{
#	ifdef CRAY
	static char szMigrateCommand[] = 
		"(find . -type m -user $LOGNAME \\( -name '*.c' -o -name '*.cc' -o "
		"-name '*.cpp' -o -name '*.f' -o -name '*.F' -o -name '*.h' -o "
		"-name '*.hh' -o -name '*.ext' -o -name '*.EXT' -o -name '*.icl' "
		"-o -name '*.inc' -o -name '*.cmd' -o -name '*.o' \\) -print | dmget) &";
	ExecuteCommand(pglobal, szMigrateCommand);
	sleep(2);	/* give the command a chance to work.  I have seen
				 * some error messages that seem to be caused by
				 * a description file being deleted before find
				 * has a chance to finish.
				 */
#	endif
}


/********************************************************************
 * Section 3. Retrieve files
 ********************************************************************/




/*** get_migrate_state ************************************************
 * 
 * Determine the migrated status of a file.  Note that a file
 * could be both on- and off-line.
 *
 * return = 0 if state could not be determined, 1 if it could
 *
 * filename = file's name
 * online = 1 if online, 0 o.w.
 * offline = 1 if online, 0 o.w.
 *
 * Created by Kathy Pearson
 */
int get_migrate_state(char* filename, int* online, int* offline)
{
struct llstat statbuf;
int istat;

#define IOERROR -1

if ((istat = llstat(filename, &statbuf)) != IOERROR)
	{
#ifdef CRAY
	if (statbuf.st_dm_state == S_DMS_REGULAR)
		{
		*online = 1;
		*offline = 0;
		}
	else if (statbuf.st_dm_state == S_DMS_OFFLINE)
		{
		*online = 0;
		*offline = 1;
		}
	else if (statbuf.st_dm_state == S_DMS_DUALSTATE)
		{
		*online = 1;
		*offline = 1;
		}
#else
	*online = 1;
	*offline = 0;
#endif
	return(1);
	}
return(0);
}	/* get_migrate_state */


/*** CheckFileStatusAux ************************************************
 *
 * See if a file exists and return information about the file.
 * If the file does not exist or there is another type of error,
 * an message will be printed.
 *
 * pszFileName = file name
 * pstatus = block for status information about the file
 * pbFileFound = indicates if the file was found
 * pnErrorCode = error code for errors other than file not found or
 *		0 for file not found and no error
 *
 * Steve Fine, fine@mcnc.org, 10/12/94
 */
void CheckFileStatusAux(char *pszFileName, struct stat *pstatus,
	boolean_t *pbFileFound, int *pnErrorCode)
{
	
	*pnErrorCode = 0;
        if (llstat(pszFileName, pstatus))
		/*
		 * there was a problem
		 */
		{
		if (errno == ENOENT)
			/*
			 * the file was not found
			 */
			{
			fprintf(stderr, "### File not found: %s\n", 
				pszFileName);
			}
		else
			/*
			 * don't continue
			 */
			{
			*pnErrorCode = errno;
			err_ret("### stat error on '%s'", pszFileName);
			}

		*pbFileFound = B_FALSE;
		}
	else *pbFileFound = B_TRUE;

}	/* CheckFileStatusAux */


/*** CheckFileStatus ************************************************
 *
 * For files that will not be retrieved (source_type == file and
 * shared include files),
 * check that the file exists and record information about the file.
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * plist = the structure containing file descriptions
 * psuppFiles = array of supplemental information for all files.  Information
 *		about files is stored here.
 */
int CheckFileStatus(model_def_type *pglobal,
	file_list_type *plist, supplemental_file_info *psuppFiles)
{
	int nReturnValue = 0;
	file_struct_type *pfiledesc;
	file_struct_type *pfiledescEnd;
	supplemental_file_info *psupp;
	boolean_t bMissingFile = B_FALSE;
	boolean_t bFileFound;
	char *pszFullFileName = NULL;
	char *pszBaseFileName = NULL;
	int nLoopLength;
	int nIndex;
        struct llstat status;

	pszFullFileName = malloc(MAX_FULL_FILE_NAME_LENGTH);
	pszBaseFileName = malloc(MAX_FULL_FILE_NAME_LENGTH);
	if (pszFullFileName == NULL || pszBaseFileName == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	nLoopLength = plist->list_size;
	for (nIndex = 0, psupp = psuppFiles; nIndex < nLoopLength;
		++nIndex, ++psupp)
		{
		pfiledesc = plist->list + nIndex;

		/*
		 * only examine files that already exist
		 */
		if (pfiledesc->where != file_src) continue;

		sprintf(pszFullFileName, "%s%s", pfiledesc->dirname,
			pfiledesc->filename);
	
		CheckFileStatusAux(pszFullFileName, &status, &bFileFound,
			&nReturnValue);
		if (nReturnValue) goto done;
		if (!bFileFound) bMissingFile = B_TRUE;
		else
			/*
			 * store information about the file
			 */
			{
			psupp->st_ino = status.st_ino;
			psupp->st_size = status.st_size;
			psupp->mtime = status.st_mtime;
			}

		}	/* end of loop over files */

	/*
	 * check the status of the shared include files
	 */
	{
	int nIncludeIndex;
	int nGridIndex;
	int nGrids;
	int nEffectiveGrids;
	boolean_t bDoGrids;
	boolean_t bGridsSpecified;
	char *pszEffectiveFileName;

	if (pglobal->grids.list_size)
		{
		bGridsSpecified = B_TRUE;
		nGrids = pglobal->grids.list_size;
		}
	else
		{
		bGridsSpecified = B_FALSE;
		nGrids = 1;
		}

	for (nIncludeIndex = 0; nIncludeIndex < pglobal->includes.list_size;
		++nIncludeIndex)
		{

		strcpy(pszBaseFileName,
			pglobal->includes.list[nIncludeIndex].dirname);
		strcat(pszBaseFileName,
			pglobal->includes.list[nIncludeIndex].filename);
		
		if (bGridsSpecified &&
			pglobal->includes.list[nIncludeIndex].apply_grid_names)
			{
			nEffectiveGrids = nGrids;
			bDoGrids = B_TRUE;
			}
		else
			{
			nEffectiveGrids = 1;
			bDoGrids = B_FALSE;
			}

		for (nGridIndex = 0; nGridIndex < nEffectiveGrids; ++nGridIndex)
			{
			if (bDoGrids)
				{
				SetNestedInclude(pszBaseFileName, 
					pglobal->grids.list[nGridIndex], pszFullFileName);
				pszEffectiveFileName = pszFullFileName;
				}
			else pszEffectiveFileName = pszBaseFileName;

			CheckFileStatusAux(pszEffectiveFileName, &status, &bFileFound,
				&nReturnValue);
			if (nReturnValue) goto done;
			if (!bFileFound) bMissingFile = B_TRUE;
			}	/* end of loop over grids */
		}	/* end of loop over shared include files */
	}	/* end of checking shared include files */

done:

	if (bMissingFile && nReturnValue == 0)
		/*
		 * set a return value that indicates that there was a file
		 * error
		 */
		{
		nReturnValue = ERROR_FILE;
		}

	if (pszFullFileName != NULL) free(pszFullFileName);
	if (pszBaseFileName != NULL) free(pszBaseFileName);

	return nReturnValue;
}

/*** ExtractModule **************************************************
 *
 * Extract the module name from the path returned by the 'what'
 * command.
 *
 * sz = on entry, the complete path of the s. file.  On exit, the
 *		name of the module.  If there is a problem, an empty
 *		string is returned.
 */
void ExtractModule(char *sz)
{
	char *sccsp;	/* pointer to last /SCCS */
	char *p;
	char *dash;
	int length;
	boolean_t firstPass = B_TRUE;

	/*
	 * find the last occurrence of "/SCCS"
	 */
	sccsp = p = NULL;
	while (p != NULL || firstPass)
		{
		firstPass = B_FALSE;
		sccsp = p;
		p = strstr(p == NULL ? sz : p + 1, "/SCCS/s.");
		}

	if (sccsp == NULL)
		/*
		 * the string was not found
		 */
		{
		*sz = 0;
		return;
		}

	/*
	 * find the preceding slash
	 */
	for (p = sccsp - 1; *p != '/' && p >= sz; --p);
	if (p < sz)
		/*
		 * the path is not complete
		 */
		{
		*sz = 0;
		return;
		}

	/*
	 * move the module name
	 */
	assert(*p == '/');
	p++;	/* start of module name */
	length = sccsp - p;
	memmove(sz, p, length);
	sz[length] = 0;
}

/*** retrieve_all_files *********************************************
 *
 * Retrieve or create links to all of the files that are not
 * supplied by the user (source_type != file).  This
 * routine will not write over files that are writable by owner.
 * For some problems, this routine will return immediately.  For
 * other problems, such as specified file not found or cannot
 * overwrite writable file, the routine will examine the
 * state of each file and produce appropriate error messages
 * before returning.
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * plist = the structure containing file descriptions
 * psuppFiles = array of supplemental information for all files.  Information
 *		about files is stored here.
 */
int retrieve_all_files(model_def_type *pglobal, file_list_type *plist,
	supplemental_file_info *psuppFiles)
{
	int nReturnValue = 0;
	file_struct_type *pfiledesc;
	file_struct_type *pfiledescEnd;
	boolean_t bFoundFile;
	boolean_t bRetrieveFiles = B_TRUE;	/* indicates if the state of the files
								 * is appropriate for retrieving the 
								 * files.
								 */
	supplemental_file_info *psupp;
	char *pszCmd = NULL;

	char *pszSAMSCmd = NULL;
	char szSAMSOptions[30];
	char szSAMSMode[10];
	boolean_t bSAMSCmdSet = B_FALSE;
	int nCommandLength;
	source_type srcLastWhere;	/* the 'where' value used to set the SAMS
								 * command */
	char *pszLastModuleName;	/* the module name used to set the SAMS
								 * command */

	pszCmd = malloc(MAX_COMMAND_LENGTH);
	pszSAMSCmd = malloc(MAX_COMMAND_LENGTH);
	if (pszCmd == NULL || pszSAMSCmd == NULL)
		{
                printf(" **ERROR** Memory full, program aborted\n");
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	for (pfiledesc = plist->list, 
		 pfiledescEnd = pfiledesc + plist->list_size, psupp = psuppFiles;
		 pfiledesc < pfiledescEnd;
		 pfiledesc++, psupp++)
		{
                struct llstat status;
		char *pszFileToCheck;

		psupp->bWasRetrieved = B_FALSE;
              
                /* if a local include file, then try to copy it to local directory */ 
                if (pfiledesc->where == file_src && psupp->bIsInclude)
                  {
#ifdef _WIN32
                    sprintf(pszCmd, "copy %s%s %s", pfiledesc->dirname,
                                    pfiledesc->filename,pfiledesc->filename);
#else
                    sprintf(pszCmd, "cp -f %s%s %s", pfiledesc->dirname,
                                    pfiledesc->filename,pfiledesc->filename);
#endif
                    system( pszCmd );  
                    
                    continue;
                  }

		/*
		 * don't consider files that are supplied by the user
		 */
		if (pfiledesc->where == file_src) continue;

		/*
		 * if this is not an include file and it will not be
		 * compiled then we don't have to retrieve it
		 */
		if (!psupp->bIsInclude && !psupp->bShouldCompile) continue;

		/*
		 * get information on the place where
		 * we'd like to put the file
		 */
		pszFileToCheck = pfiledesc->filename;

		if (llstat(pszFileToCheck, &status))
			/*
			 * there was a problem
			 */
			{
			if (errno == ENOENT)
				/*
				 * the file was not found
				 */
				{
				bFoundFile = B_FALSE;
				}
			else
				/*
				 * don't continue
				 */
				{
				nReturnValue = errno;
				err_ret("### llstat error on '%s'", pszFileToCheck);
				goto done;
				}
			}
		else 
			{
			bFoundFile = B_TRUE;
			}

		if (bRetrieveFiles)
			/*
			 * There have been no problems yet so 
			 * we're still in the process of retrieving files.
			 * Link to or retrieve the file
			 */
			{
			if (bFoundFile)
				{
				boolean_t bRemove = B_FALSE;
				int nOnline, nOffline;

#ifndef _WIN32
				if (S_ISLNK(status.st_mode))  bRemove = B_TRUE;
				else if (pglobal->fast_check &&
					pglobal->os == cray_os &&
					get_migrate_state(pfiledesc->filename,
						&nOnline, &nOffline) &&
					!nOnline)
					/*
					 * Don't look in the file.  Instead retrieve
					 * the file from the archive.  This will
					 * probably be faster than retrieving
					 * the file from tape.
					 */
					;
				else
					/*
					 * to try to prevent replacing files that can't
					 * be replaced, we will run 'what' on the file.
					 * If stdout contains at least two lines
					 * then we will replace the file.
					 */
                                        ;
#endif
					{


					if (pfiledesc->where == latest_release_src ||
						pfiledesc->where == dev_src ||
						pfiledesc->where == version_src)
						/*
						 * determine if the version of the file
						 * matches the required version.  If so, 
						 * we don't have to retrieve anything.
						 */
						{
						int nRet;
						char szRequiredVersion[30];
						char szAvailableVersion[30];
						char szAvailableModule[1000];

						strcpy(szRequiredVersion, pfiledesc->version_num);
						if (pfiledesc->where == version_src)
							strcat(szRequiredVersion, ".1");
						szAvailableModule[0] = 0;
						sscanf(pszCmd, " %*s %29s %999s/SCCS",
							szAvailableVersion, szAvailableModule);
						ExtractModule(szAvailableModule);
						if (strcmp(szRequiredVersion, szAvailableVersion) == 0
							&& strcmp(pfiledesc->module_name, 
							szAvailableModule) == 0)
							continue;
						}
					}

				if (bRemove)
					/*
					 * delete the existing file.  Note that we are
					 * executing shell commands instead of calling
					 * unlink so echoed text will correspond exactly
					 * to the action that was performed.
					 */
					{
					if(pglobal->os==win_32)
                                         sprintf(pszCmd, "del  %s", pfiledesc->filename);
					else
                                         sprintf(pszCmd, "/bin/rm -f %s", pfiledesc->filename);

					if (ExecuteCommand(pglobal, pszCmd))
						{
						nReturnValue = ERROR_RM;
						goto done;
						}
					}
				}	/* end of handling existing file */
				
			/*
			 * ask SAMS to get the file
			 */
			if (bSAMSCmdSet)
				{
				if (srcLastWhere == pfiledesc->where &&
					0 == strcmp(pszLastModuleName, 
						pfiledesc->module_name))
					/*
					 * append this file to the command that
					 * is being built
					 */
                                        AppendWithCount(pszSAMSCmd, &nCommandLength,
						MAX_COMMAND_LENGTH - 1, 
                                                pfiledesc->filename, B_TRUE);
				else
					/*
					 * execute the command and note that no
					 * command is set
					 */
					{
                               /*         if (ExecuteCommand(pglobal, pszSAMSCmd))
						{
						nReturnValue = ERROR_GET;
						goto done;
                                                }                    */
					bSAMSCmdSet = B_FALSE;
					}
				}	/* end of case when command is set */

			if (!bSAMSCmdSet)
				/*
			 	 * set the SAMS options required to retrieve the
			 	 * file
			 	 */
				{
				switch (pglobal->archive_location)
					{
					case sams_default_archive :
						strcpy(szSAMSMode, "");
						break;
	
					case local_archive :
						strcpy(szSAMSMode, "-local ");
						break;
	
					case remote_archive :
						strcpy(szSAMSMode, "-remote ");
						break;
	
					default:
						assert(B_FALSE);
					}
				switch (pfiledesc->where)
					{
					case version_src :
						sprintf(szSAMSOptions, "-version %s.1 ",
							pfiledesc->version_num);
						break;
	
					case dev_src :
						strcpy(szSAMSOptions, "-version dev ");
						break;
	
					case latest_release_src :
						strcpy(szSAMSOptions, "-version rel ");
						break;
	
					case release_time_src :
						sprintf(szSAMSOptions, "-date %s%s ",
							pfiledesc->date, pfiledesc->time);
						break;
	
					default:
						assert(B_FALSE);
	
					}
	
                               /*       sprintf(pszSAMSCmd, "cams -quiet %s get_file %s %s %s ",
					szSAMSMode, szSAMSOptions, pfiledesc->module_name,
                                        pfiledesc->filename);  */

                                sprintf(pszSAMSCmd, "cvs export -r HEAD -d .  %s ",
                                         pfiledesc->module_name);

				/*
				 * note that we have built a command
				 */
				bSAMSCmdSet = B_TRUE;
				srcLastWhere = pfiledesc->where;
				pszLastModuleName = pfiledesc->module_name;
				nCommandLength = strlen(pszSAMSCmd);
				}

			psupp->bWasRetrieved = B_TRUE;
			}
		}	/* end of loop over files */

	if (bRetrieveFiles && bSAMSCmdSet)
		/*
		 * execute the command that has been built
		 */
                {
             /*   if (ExecuteCommand(pglobal, pszSAMSCmd))
			{
			nReturnValue = ERROR_GET;
			goto done;
                        }     */
		bSAMSCmdSet = B_FALSE;
		}

done:

	if (!bRetrieveFiles && nReturnValue == 0)
		/*
		 * set a return value that indicates that there was a file
		 * error
		 */
		{
		nReturnValue = ERROR_FILE;
		}

	if (pszCmd != NULL) free(pszCmd);
	if (pszSAMSCmd != NULL) free(pszSAMSCmd);

	return nReturnValue;
}	/* retrieve_all_files */

/********************************************************************
 * Section 4. Build program
 ********************************************************************/


/*** GetLibraries ********************************************
 *
 * Return a string indicating the libraries that should be used.
 * This string will include the linker directives required
 * to access the libraries.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * pszLibraries = string for the libraries
 * nMaxLibrariesLength = maximum length of the library string (excluding
 *		trailing null)
 */
int GetLibraries(model_def_type *pglobal, char *pszLibraries,
	int nMaxLibrariesLength)
{
	int nReturnValue = 0;
	char *pszLibDir;
	char *pszTarget;
	char *pszLibName;
	char szOSName[40];

	assert(nMaxLibrariesLength > 100);
	
	if ( strlen ( pglobal->libraries ) != 0 )
		{
		strcpy ( pszLibraries, pglobal->libraries );
		goto done;
		}

/*	pszLibDir = getenv("CVSROOT"); */
	pszLibDir = getenv("SAMS_PROJECT");
	if (pszLibDir == NULL) 
		pszLibDir = pglobal->os == cray_os ?
			"/usr/local/edss/aqm" : "/pub/storage/edss/aqm";

	if (getenv("EDSS_OS"))
		/*
		 * use the value of EDSS_OS as the directory for the
		 * library
		 */
		{
		assert(strlen(getenv("EDSS_OS")) < (size_t) 40);
		strcpy(szOSName, getenv("EDSS_OS"));
		pszLibName = szOSName;
		}
	else pszLibName = pglobal->os_name_vers;

	if (pglobal->os == cray_os)
		/*
		 * see if we should use the T-3D libraries
		 */
		{
		pszTarget = getenv("TARGET");
		if (pszTarget && !strcmp(getenv("TARGET"), "cray-t3d"))
			pszLibName = "t3d";
		}

	sprintf(pszLibraries, " \\\n   -L%s/lib/%s "
		"\\\n   -lcoreio -lcoreutil -lnetcdf \\\n   ",
		pszLibDir, pszLibName);

done:
	return nReturnValue;
}	/* GetLibraries */



/*
 * data structure used by InitializeCompilation, CompileFile,
 * and CompleteCompilation.
 */
typedef struct
	{
	char *pszCmd;
	int nCmdLength;
	char *pszFlags;	/* flags that apply to all files */
	int nFlagsLength;
	boolean_t bInitialized;
	} compileStruct;
compileStruct compileInfo = {NULL, 0, NULL, 0, B_FALSE};


/*** CompleteCompilation *************************************
 *
 * Issue link command and clean up the compileInfo data structure.
 *
 * bCleanupOnly = indicates of only cleanup should be performed
 * pglobal = global information from the configuration file and
 *		environment information
 * pszObjectFiles = a string containing the names of object files
 *		When one_pass is true, then this is not used.
 */
int CompleteCompilation(boolean_t bCleanupOnly, model_def_type *pglobal,
	char *pszObjectFiles)
{
	int nReturnValue = 0;
	char *pszLibraries = NULL;

	if (bCleanupOnly) goto done;

	pszLibraries = malloc(MAX_COMMAND_LENGTH + 1);
	if (pszLibraries == NULL)
		{
                printf(" **ERROR** Memory full, program aborted\n");
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	if (nReturnValue = GetLibraries(pglobal, pszLibraries, MAX_COMMAND_LENGTH))
		goto done;

	if (pglobal->one_step)
		/*
		 * append the executable name to the command we formed
		 * and issue the command
		 */
		{
		if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
			&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, pszLibraries,
			B_FALSE))
			goto done;
		if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
                        &compileInfo.nCmdLength, MAX_COMMAND_LENGTH, " /OUT:",
			B_FALSE))
			goto done;
		if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
			&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, pglobal->exename,
			B_FALSE))
			goto done;
		if (nReturnValue = ExecuteCommand(pglobal, compileInfo.pszCmd))
			goto done;
		}
	else
		/*
		 * issue the link command
		 */
		{
		if( pglobal->os==win_32)
	                sprintf(compileInfo.pszCmd, "%s %s /link %s %s /out:%s.exe",
			pglobal->compilers[f_lang], pszObjectFiles, pglobal->link_flags, 
			pszLibraries, pglobal->exename);
		 else
	                sprintf(compileInfo.pszCmd, "%s %s %s %s -o %s",
			pglobal->compilers[f_lang], pglobal->link_flags, pszObjectFiles,
			pszLibraries, pglobal->exename);

		if (nReturnValue = ExecuteCommand(pglobal, compileInfo.pszCmd))
			goto done;
		}

done:
	if (pszLibraries != NULL) free(pszLibraries);
	if (compileInfo.bInitialized)
		{
		if (compileInfo.pszCmd) free(compileInfo.pszCmd);
		if (compileInfo.pszFlags) free(compileInfo.pszFlags);
		compileInfo.bInitialized = B_FALSE;
		}

#	ifdef DEBUG_ACTION
	if (nReturnValue) 
		fprintf(stderr, "CompleteCompilation: return = %d\n", nReturnValue);
#	endif
	return nReturnValue;
}	/* CompleteCompilation */

/*** CreateCompileCommand ************************************
 *
 * Create a compile command for the specified language.
 * Include any fixed options
 * for that language.  Append the command to the given string.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * language = desired language
 * pszCompileFlags = desired compiler flags.  This was added because it is no
 *		longer possible to append module-level flags to global flags.  That can
 *		cause fatal errors on some compilers
 * pszCmd = string for the command
 * pnCmdLength = length of the command without the trailing null
 * nMaxCmdLength = maximum length for the command
 */
int CreateCompileCommand(model_def_type *pglobal, lang_type language,
	char *pszCompileFlags,
	char *pszCmd, int *pnCmdLength, int nMaxCmdLength)
{
	int nReturnValue = 0;

	if (nReturnValue = AppendWithCount(pszCmd, pnCmdLength, 
		nMaxCmdLength, pglobal->compilers[language], B_TRUE))
		goto done;
	if (!pglobal->one_step )
		{
                if( pglobal->os == win_32)
			if (nReturnValue = AppendWithCount(pszCmd, pnCmdLength, 
                                nMaxCmdLength, " /c ", B_TRUE))
				goto done;
                if( pglobal->os != win_32 && strscan("cf77",pglobal->compilers[f_lang]) < 0 )
			if (nReturnValue = AppendWithCount(pszCmd, pnCmdLength, 
               		        nMaxCmdLength, " -c ", B_TRUE))
				goto done;
		}
	if (nReturnValue = AppendWithCount(pszCmd, pnCmdLength, 
		/** nMaxCmdLength, pglobal->compile_flags[language], B_TRUE)) **/
		nMaxCmdLength, pszCompileFlags, B_TRUE))
		goto done;
done:
	return nReturnValue;
}


/*** InitializeCompilation ***********************************
 *
 * Initialize variables for compilation.  This is part of
 * a sequence of three routines: InitializeCompilation,
 * CompileFile, and CompleteCompilation.
 *
 * This routine uses the global struct compileInfo.
 *
 * return = error code
 *
 * pglobal = information about the model
 */
int InitializeCompilation(model_def_type *pglobal)
{
	int nReturnValue = 0;

	/*
	 * allocate space
	 */
	compileInfo.pszCmd = malloc(MAX_COMMAND_LENGTH + 1);
	compileInfo.pszFlags = malloc(MAX_COMMAND_LENGTH + 1);
	if (compileInfo.pszCmd == NULL || compileInfo.pszFlags == NULL)
		{
                printf(" **ERROR** Memory full, program aborted\n");
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}
	compileInfo.nCmdLength = compileInfo.nFlagsLength = 0;

	/*
	 * set up the flags
	 */
	if (pglobal->os == cray_os)
		{
		if(strlen(pglobal->cpp_flags)>0 || strlen(pglobal->def_flags)>0) 
			sprintf(compileInfo.pszFlags, " -Wp\"%s %s\" ", pglobal->cpp_flags, pglobal->def_flags);
		}
	else sprintf(compileInfo.pszFlags, " %s %s ", pglobal->cpp_flags, pglobal->def_flags);
	compileInfo.nFlagsLength = strlen(compileInfo.pszFlags);
	if (nReturnValue = AppendIncludeDefinitions(pglobal,
		compileInfo.pszFlags,
		&compileInfo.nFlagsLength, MAX_COMMAND_LENGTH, NULL))
		goto done;

	if (pglobal->one_step)
		{
		if (nReturnValue = CreateCompileCommand(pglobal, f_lang,
			pglobal->compile_flags[f_lang], 
			compileInfo.pszCmd, &compileInfo.nCmdLength, MAX_COMMAND_LENGTH))
			goto done;
		if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
			&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
			pglobal->os == cray_os ? "-Wp\"-DSUBST_GRID_ID=''" :
                                pglobal->os==win_32 ? "/DSUBST_GRID_ID=''" : "-DSUBST_GRID_ID=' '",
			B_TRUE))
			goto done;
		if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
			&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
			compileInfo.pszFlags, B_TRUE))
			goto done;
		}

	compileInfo.bInitialized = B_TRUE;

done:
#	ifdef DEBUG_ACTION
	if (nReturnValue)
		fprintf(stderr, "InitializeCompilation: return = %d\n", nReturnValue);
#	endif
	return nReturnValue;
}	/* InitializeCompilation */

/*** SetObjectName *******************************************
 *
 * Set the name of the object file.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * pszObjectFile = string for object file name.  Assumed to be
 *		NAME_SIZE long (including null).
 * pszFileName = the name of the source file
 * pszGridID = the name of the grid ID to be included in the name.
 *		NULL indicates no grid ID.
 */
int SetObjectName(model_def_type *pglobal, char *pszObjectFile,
	char *pszFileName, char *pszGridID)
{
	int nReturnValue = 0;
	char *psz;

#ifdef _WIN32
        char szObjectExtension[] = ".obj";
#else
        char szObjectExtension[] = ".o";
#endif

	strcpy(pszObjectFile, pszFileName);
	for (psz = pszObjectFile + strlen(pszObjectFile) - 1;
		psz >= pszObjectFile && *psz != '.';
		--psz);
	if (psz >= pszObjectFile)
		/*
		 * a period was found.  Use that as the end of
		 * the string.
		 */
		*psz = '\0';
		   
	if (pszGridID != NULL)
		strcat(pszObjectFile, pszGridID);

	/*
	 * append the extension
	 */
	if (strlen(pszObjectFile) + strlen(szObjectExtension) >=
		(size_t) NAME_SIZE)
		{
		nReturnValue = ERROR_STRING_LENGTH;
		goto done;
		}
	strcat(pszObjectFile, szObjectExtension);
 
done:
	return nReturnValue;
}

/*** CompileFile *********************************************
 *
 * Compile the specified file.  Depending on the options selected,
 * this may cause a compile command to be issued or might
 * cause information to be stored for later use.
 * This routine sets information in the compileInfo data structure.
 * If bShouldCompile indicates that the file doesn't need
 * compilation, then just handle the object file.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * pdesc = the description of the file
 * psupp = supplemental information for the file.
 * pnInclude = array of indices of include files
 * nIncludeFileCt = the number of include files
 * pszObjectFiles = a string where the name of the object file
 *		should be appended.  If NULL, no appending is done.
 * pnObjectFilesLength = the length of the string of object files.
 *		Not used if pszObjectFiles is NULL.
 * nMaxObjectFilesLength = the maximum length for the list of object 
 *		files excluding trailing NULL.
 * psuppFiles = list of all supplemental file information
 */
int CompileFile(model_def_type *pglobal, file_struct_type *pdesc,
	supplemental_file_info *psupp, int *pnInclude, 
	int nIncludeFileCt, char *pszObjectFiles, int *pnObjectFilesLength,
	int nMaxObjectFilesLength, supplemental_file_info *psuppFiles)
{
	int nReturnValue = 0;
	char szObjectFile[NAME_SIZE];
	char nGridIndex;
	char nLoopLength;
	boolean_t bAffectedByGrid;

#	ifdef DEBUG_ACTION
	fprintf(stderr, "CompileFile: %s\n", pdesc->filename);
#	endif

	assert(compileInfo.bInitialized);

	if (pglobal->grids.list_size == 0) bAffectedByGrid = B_FALSE;
	else if (psupp->bShouldCompile)
		/*
		 * determine if the file is affected by gridding by
		 * looking for SUBST_GRID_ID in the file.
		 */
		{
		char *pszDirectoryName;	/* name of the directory that contains
								 * the file */
		char *pszRegEx;			/* regular expression for searching */

		pszDirectoryName = pdesc->where == file_src ? pdesc->dirname : "";
		pszRegEx = pdesc->language == f_lang ? 
			"'^[^C].*SUBST_GRID_ID'" : "SUBST_GRID_ID";
                sprintf(compileInfo.pszCmd, "grep -il %s %s%s > null",
			pszRegEx, pszDirectoryName,
			pdesc->filename);
		bAffectedByGrid = 0 == system(compileInfo.pszCmd) ? B_TRUE :
			B_FALSE;
		}
	else
		/*
		 * since this file is the same one that is described in
		 * the description file, use the flag set there.
		 */
		bAffectedByGrid = psupp->bAffectedByGrid;
	nLoopLength = bAffectedByGrid ? pglobal->grids.list_size : 1;

	for (nGridIndex = 0; nGridIndex < nLoopLength; ++nGridIndex)
		{
		if (!pglobal->one_step)
			if (nReturnValue = SetObjectName(pglobal, szObjectFile,
				pdesc->filename, bAffectedByGrid ?
				pglobal->grids.list[nGridIndex] : NULL))
				goto done;
	
		if (pszObjectFiles != NULL)
			/*
		 	* append the object file name to the list of object files
		 	*/
			{
			if (nReturnValue = AppendWithCount(pszObjectFiles,
				pnObjectFilesLength, nMaxObjectFilesLength,
				szObjectFile, B_TRUE))
				goto done;
			}
	
		/*
		 * if we don't have to compile then go to the next file
		 */
		if (!psupp->bShouldCompile) continue;

		if (!pglobal->one_step) 
			/*
	 		* create the beginning of the command
	 		*/
			{
			compileInfo.nCmdLength = 0;
			if (nReturnValue = CreateCompileCommand(pglobal, pdesc->language,
				( strlen ( pdesc->compile_flags ) == 0 ? 
					pglobal->compile_flags[pdesc->language] : 
					pdesc->compile_flags ),
				compileInfo.pszCmd, &compileInfo.nCmdLength,
				MAX_COMMAND_LENGTH))
				goto done;

			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				(pglobal->os == cray_os && pdesc->language == f_lang) ? 
					" -Wp\"-DSUBST_GRID_ID='' " :
					 (pglobal->os==win_32 ? " /DSUBST_GRID_ID=" : 
					" -DSUBST_GRID_ID="), B_FALSE))
				goto done;
			if (bAffectedByGrid)
				/*
				 * append include definitions, the global cpp flags,
				 * and the substition for the extension for the
				 * subroutine name
				 */
				{
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					pglobal->grids.list[nGridIndex], B_TRUE))
					goto done;
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					pglobal->cpp_flags, B_TRUE))
					goto done;
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					pglobal->def_flags, B_TRUE))
					goto done;
				if (pglobal->os == cray_os && pdesc->language == f_lang)
					if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
						&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
						"' ", B_TRUE))
						goto done;
				if (pdesc->language == f_lang && 
					(nReturnValue = AppendIncludeDefinitions(pglobal,
					compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					pglobal->grids.list[nGridIndex])))
					goto done;
				}
			else
				/*
				 * append the text we precomputed
				 */
				{
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
                                        pglobal->os == cray_os ? "\"" : "''", B_TRUE))
					goto done;
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					compileInfo.pszFlags, B_TRUE))
					goto done;
				}

			if (pglobal->os == cray_os && pdesc->language == f_lang)
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					" -Wp\" ", B_TRUE))     
					goto done;
			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				pdesc->cpp_flags, B_TRUE))
				goto done;
			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				pdesc->def_flags, B_TRUE))
				goto done;
			if (pglobal->os == cray_os && pdesc->language == f_lang)
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					"\" ", B_TRUE))
					goto done;
			/* this is now done by call to CreateCompileCommand */
			/***********************
			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				pdesc->compile_flags, B_TRUE))
				goto done;
			************************/
			}
		
		if (pdesc->where == file_src)
			/*
		 	 * include the directory name for the file
		 	 */
			{
			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				pdesc->dirname, B_FALSE))
				goto done;
			}
	
		if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
			&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
			pdesc->filename, B_TRUE))
			goto done;

		if (!pglobal->one_step)
                        { 
		        if( pglobal->os==cray_os &&
                          pdesc->language == f_lang &&
			  strscan( "cf77",pglobal->compilers[f_lang]) < 0 )
			    {
			         nReturnValue = ExecuteCommand(pglobal, compileInfo.pszCmd);
				 goto done;
			    }

			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				(pglobal->os == cray_os && pdesc->language == f_lang) ?
                                        "-Wf\" -b " :
					(pglobal->os==win_32 ? " /object:" : " -o "), B_TRUE))
				goto done;
			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
				szObjectFile, B_TRUE))
				goto done;
			if (pglobal->os == cray_os && pdesc->language == f_lang)
				if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
					&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
					"\" ", B_TRUE))
					goto done;
			if (nReturnValue = ExecuteCommand(pglobal, compileInfo.pszCmd))
                                goto done;
		 	}
		else
			{
			if (nReturnValue = AppendWithCount(compileInfo.pszCmd,
				&compileInfo.nCmdLength, MAX_COMMAND_LENGTH, 
                                " \n  ", B_FALSE))
				goto done;
			}
		}	/* end of loop over grids */

done:
        if (!nReturnValue && psupp->bShouldCompile)
            SaveObjDescription(pglobal, psuppFiles, pdesc, psupp, pnInclude,
                                                  nIncludeFileCt, bAffectedByGrid);

        return nReturnValue;
}	/* CompileFile */
	

/*** build_model *********************************************
 *
 * Compile and link the code for the model.  Optionally, remove
 * files that were retrieved.  Behavior is controlled by
 * pglobal->mode.
 *
 * return = error number or 0 if no error
 *
 * pglobal = global information from the configuration file and
 *		environment information
 * plist = the structure containing file descriptions
 * psuppFiles = array of supplemental information for all files.
 * pnInclude = array of indices of include files
 * nIncludeFileCt = the number of include files
 */
int build_model(model_def_type *pglobal, file_list_type *plist,
	supplemental_file_info *psuppFiles, int *pnInclude, int nIncludeFileCt)
{
	char *pszCmd = NULL;
	int nCmdLength = 0;
	char *pszObjectFiles = NULL;	/* object files for linking */
	int nObjectFilesLength = 0;
	file_struct_type *pfiledesc;
	file_struct_type *pfiledescEnd;
	supplemental_file_info *psupp;
	int nReturnValue = 0;
	boolean_t bCompilationInitialized = B_FALSE;

	assert(!pglobal->one_step || !pglobal->grids.list_size);

	if (pglobal->mode == no_compile_mode) goto done;

	pszCmd = malloc(MAX_COMMAND_LENGTH);
	if (pszCmd == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}
	*pszCmd = 0;

	if (!pglobal->one_step)
		/*
		 * allocate space for a list of object files
		 */
		{
		pszObjectFiles = malloc(MAX_COMMAND_LENGTH + 1);
		if (pszObjectFiles == NULL)
			{
			nReturnValue = ERROR_MEMORY_FULL;
			goto done;
			}
		*pszObjectFiles = 0;
		}

	if (nReturnValue = InitializeCompilation(pglobal))
		goto done;
	bCompilationInitialized = B_TRUE;


	/*
	 * consider compiling each file
	 */
	for (pfiledesc = plist->list, 
		pfiledescEnd = pfiledesc + plist->list_size, psupp = psuppFiles;
	 	pfiledesc < pfiledescEnd;
	 	pfiledesc++, psupp++)
		{
		if (IsCompilable(pfiledesc->language))
			{
			if (nReturnValue = CompileFile(pglobal, pfiledesc, psupp,
				pnInclude, nIncludeFileCt, pszObjectFiles,
				&nObjectFilesLength, MAX_COMMAND_LENGTH, psuppFiles))
                                  goto done;
                        }
		}	/* end of loop over files */

	if (pglobal->mode != no_link_mode)
		{
		bCompilationInitialized = B_FALSE;
		if (nReturnValue = CompleteCompilation(B_FALSE, pglobal, pszObjectFiles))
			goto done;
		}

/*   cleanup working files  */
        if(pglobal->os==win_32)
           {
             system("del /F .*.F");
        /*     system("del /F *.for"); */
           }

	if (pglobal->clean_up)
	      {
                printf("\n Cleaning Up files:");
		for (pfiledesc = plist->list, 
			pfiledescEnd = pfiledesc + plist->list_size, psupp = psuppFiles;
	 		pfiledesc < pfiledescEnd;
	 		pfiledesc++, psupp++)
			{
			if (psupp->bWasRetrieved)
		       	  {
			   if(pglobal->os==win_32)
                            sprintf(pszCmd, "del %s", pfiledesc->filename);
			   else
                            sprintf(pszCmd, "/bin/rm -f %s", pfiledesc->filename);

                           system(pszCmd);	
			   printf(" %s", pfiledesc->filename );
           		   }
			printf("\n");
			}
              }

done:
	if (pszCmd != NULL) free(pszCmd);
	if (pszObjectFiles != NULL) free(pszObjectFiles);
	if (bCompilationInitialized) CompleteCompilation(B_TRUE, pglobal, NULL);

	return nReturnValue;
}


/********************************************************************
 * Section 5. Determine if file must be compiled
 ********************************************************************/

/*** SetEnvLine *****************************************************
 *
 * Replace the given string with the environment string that
 * describes the build environment for a file.
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pdesc = description of the file
 * pszLine = requested information.  The string should be very
 *		long so there won't be a risk of overflowing it.
 * 
 */
void SetEnvLine(model_def_type *pglobal, file_struct_type *pdesc,
	char *pszLine)
{
	char *pszLibraryVariant = "";
	int i,l;

	/*
	 * set the library variant.  The variant string should
	 * end with a space.
	 */
	if (pglobal->os == cray_os)
		/*
		 * see if we should use the T-3D libraries
		 */
		{
		char *pszTarget = getenv("TARGET");
		if (pszTarget && !strcmp(getenv("TARGET"), "cray-t3d"))
			pszLibraryVariant = "T3D ";
		}

	sprintf(pszLine, "%s %s%s %s %s %s %s %s %s %s", pglobal->architecture,
		pszLibraryVariant,
		pglobal->os_name_vers, pglobal->compilers[pdesc->language],
		pglobal->cpp_flags, pglobal->def_flags, pglobal->compile_flags[pdesc->language],
		pdesc->cpp_flags, pdesc->def_flags, pdesc->compile_flags);

        /*replace newline characters with spaces  */
        for(i=0; pszLine[i]!=0; i++) if( pszLine[i]=='\n' ) pszLine[i]=32;

        /*trim spases from end of string  */
        l = strlen(pszLine);
        for(i=l; i>0 && pszLine[i]<=32; i--)  pszLine[i]=0;

}	/* SetEnvLine */



/*** SetSharedIncludeLine *****************************************
 *
 * Return a line of text that describes a shared include file.
 * This might include information about a version of the include
 * file for each grid.
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * bAffectedByGrid = indicates if the source file is affected
 *		by which grid is used
 * nIncludeIndex = the index for the include file
 * pszLine = buffer for description line.  Should be relatively long
 *		to avoid overflows
 */
void SetSharedIncludeLine(model_def_type *pglobal, boolean_t bAffectedByGrid,
	int nIncludeIndex, char *pszLine)
{
	char *pszFileName = NULL;
        struct llstat status;
	char szBuffer[40];

	sprintf(pszLine, "%s %s%s %d ", 
		pglobal->includes.list[nIncludeIndex].symbolname,
		pglobal->includes.list[nIncludeIndex].dirname,
		pglobal->includes.list[nIncludeIndex].filename,
		pglobal->includes.list[nIncludeIndex].apply_grid_names);

	if (bAffectedByGrid &&
		pglobal->includes.list[nIncludeIndex].apply_grid_names)
		/*
		 * get and append information about the include file
		 * for each grid
		 */
		{
		int nGridIndex;
		char *pszBaseFileName = NULL;

		pszFileName = (char*) malloc(MAX_FULL_FILE_NAME_LENGTH);
		pszBaseFileName = (char*) malloc(MAX_FULL_FILE_NAME_LENGTH);

		if (pszFileName == NULL || pszBaseFileName == NULL)
			/*
			 * We can't do the work so use a random number
			 * to force recompilation.
			 */
			{
			sprintf(szBuffer, "%d", GetRand());
			strcat(pszLine, szBuffer);
			}
		else
			{
			sprintf(pszBaseFileName, "%s%s", 
				pglobal->includes.list[nIncludeIndex].dirname,
				pglobal->includes.list[nIncludeIndex].filename);

			for (nGridIndex = 0; nGridIndex < pglobal->grids.list_size;
				++nGridIndex)
				{
				SetNestedInclude(pszBaseFileName, 
					pglobal->grids.list[nGridIndex], pszFileName);
                                if (llstat(pszFileName, &status))
					/*
					 * there was a problem.  Use random
					 * values to force recompilation.
					 */
					{
					status.st_ino = GetRand();
					status.st_size = GetRand();
					status.st_mtime = GetRand();
					}

				sprintf(szBuffer, "%d %d %d ", status.st_ino,
					status.st_size, status.st_mtime);
				strcat(pszLine, szBuffer);
				}	/* end of loop over grids */
			}

		if (pszFileName != NULL) free(pszFileName);
		if (pszBaseFileName != NULL) free(pszBaseFileName);

		}	/* end of handling nested versions of include file */
	else
		/*
		 * include version information for the file
		 */
		{
		boolean_t gotInfo = B_FALSE;

		pszFileName = (char*) malloc(MAX_FULL_FILE_NAME_LENGTH);
		if (pszFileName)
			{
			sprintf(pszFileName, "%s%s", 
				pglobal->includes.list[nIncludeIndex].dirname,
				pglobal->includes.list[nIncludeIndex].filename);
                        if (!llstat(pszFileName, &status)) gotInfo = B_TRUE;
			}

		if (!gotInfo)
			/*
		  	 * there was a problem.  Use random
		 	 * values to force recompilation.
		 	 */
			{
			status.st_ino = GetRand();
			status.st_size = GetRand();
			status.st_mtime = GetRand();
			}

		sprintf(szBuffer, "%d %d %d ", status.st_ino,
			status.st_size, status.st_mtime);
		strcat(pszLine, szBuffer);
		}

	strcat(pszLine, "\n");

}	/* SetSharedIncludeLine */

/*** SetGridIDLine *************************************************
 *
 * Return a line of text that describes a shared include file.
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pszLine = buffer for description line.  Should be relatively long
 *		to avoid overflows
 */
void SetGridIDLine(model_def_type *pglobal, char *pszLine)
{
	int nGridIndex;

	sprintf(pszLine, "%d ", pglobal->grids.list_size);
	for (nGridIndex = 0; nGridIndex < pglobal->grids.list_size; ++nGridIndex)
		{
		strcat(pszLine, pglobal->grids.list[nGridIndex]);
		strcat(pszLine, szSpace);
		}
	strcat(pszLine, "\n");
}	/* SetGridIDLine */

/*** SetFileLine **************************************************
 *
 * Return a line of text that describes the given file.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pdesc = a description of the file.
 * psupp = supplemental information for all the file.
 * pszLine = buffer for description line.  Should be relatively long
 *		to avoid overflows
 */
int SetFileLine(model_def_type *pglobal, 
	file_struct_type *pdesc, supplemental_file_info *psupp, char
	*pszLine)
{
	int nReturnValue = 0;

	switch (pdesc->where)
		{
		case file_src:
			sprintf(pszLine, "%s%s %d %d %d\n",
				pdesc->dirname,
				pdesc->filename, (int) psupp->mtime, (int) psupp->st_size,
				(int) psupp->st_ino);
			break;

		case version_src:
		case dev_src:
		case latest_release_src:
			/*
			 * file name precedes module name because the routine
			 * that scans for local include files in description
			 * files picks up the first name on the line.
			 */
			sprintf(pszLine, "%s %s %s\n", pdesc->filename,
				pdesc->module_name,
				pdesc->version_num);
			break;

		case release_time_src:
			sprintf(pszLine, "%s %s %s %s\n", pdesc->filename,
				pdesc->module_name,
				pdesc->date, pdesc->time);
			break;

		default:
			assert(0);
			break;
		}
done:
	return nReturnValue;
}


/*** FindLocalIncFileIndex ***********************************
 *
 * Find the index for the given include file in the list of
 * files by looking up the file in the list of include files.
 * 
 * return = index into list of files or -1 if file not found
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pszFileName = name of include file
 * pnInclude = array of indices of known include files
 * nIncludeFileCt = number of known include files
 */
int FindLocalIncFileIndex(model_def_type *pglobal,
	char *pszFileName, int *pnInclude, int nIncludeFileCt)
{
	int nIndex;

	/*
	 * find the index for the include file
	 */
	for (nIndex = 0; nIndex < nIncludeFileCt; ++nIndex)
		{
		if (strcmp(pszFileName,
			pglobal->files.list[pnInclude[nIndex]].filename) == 0)
			break;
		}

	return (nIndex >= nIncludeFileCt) ? -1 : pnInclude[nIndex];
}	/* FindLocalIncFileIndex */


/*** FindLocalIncludes ***************************************
 *
 * Determine which local includes are used by the file.  For each
 * local include, determine an index into the list of files.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pdescSource = the description of the source file
 * pnInclude = array of indices of include files
 * nIncludeFileCt = the number of include files
 * pnLocalCt = On entry, the maximum number of local includes that
 *		will fit in the array.  On return, the number of local includes found.
 * pnLocalIndices = the indices of the local includes that were
 *		found.  This should be allocated by the calling routine.
 *		An index of -1 is used if the include file was not found in
 *		the list of files.
 */
int FindLocalIncludes(model_def_type *pglobal, 
	file_struct_type *pdescSource,
	int *pnInclude, int nIncludeFileCt, int *pnLocalCt, int
	*pnLocalIndices)
{
	int nReturnValue = 0;
	int nMaxLocalCt;
	FILE *pfile = NULL;
	char *pszCmd = NULL;
	char *pszDirectoryName;
	int nIndex;
        int rdswit;

	pszCmd = malloc(MAX_COMMAND_LENGTH);
	if (pszCmd == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}
	nMaxLocalCt = *pnLocalCt;
	*pnLocalCt = 0;
	pszDirectoryName = pdescSource->where == file_src ? 
		pdescSource->dirname : "";
	if (pdescSource->language == f_lang)
		sprintf(pszCmd, 
#ifdef _WIN32
                        "find /I \"INCLUDE\" %s%s",
#else
                        "grep -i \"^[ ]*INCLUDE[ ]*\\'\" %s%s | cut -f 2 -d \\' -s",
#endif
			pszDirectoryName, pdescSource->filename);
	else if (pdescSource->language == c_lang ||
		pdescSource->language == c_plus_plus_lang)
		sprintf(pszCmd, 
#ifdef _WIN32
                        "find /I \"include\" %s%s",
#else
                        "grep '^#[      ]*include[      ]*\"' %s%s | cut -f 2 -d \\\" -s",
#endif
			pszDirectoryName, pdescSource->filename);
	else assert(0);


	pfile = popen(pszCmd, "r");
	if (pfile == NULL)
		{
		nReturnValue = errno;
		err_ret("### popen error (2)");
		goto done;
		}

        rdswit=1;
	while (B_TRUE)
		{
                if (1 != fscanf(pfile, "%s", pszCmd)) break;
                if(rdswit == 0) pnLocalIndices[(*pnLocalCt)++] =
                        FindLocalIncFileIndex(pglobal, pszCmd, pnInclude, nIncludeFileCt);
                rdswit = strcmp("INCLUDE",pszCmd);
                if(rdswit) rdswit = strcmp("include",pszCmd);
                if(rdswit) rdswit = strcmp("#include",pszCmd);

		}	/* end of reading result from search */

        pclose(pfile);
	pfile = NULL;

done:
	if (pszCmd != NULL) free(pszCmd);
	if (pfile != NULL) pclose(pfile);
	return nReturnValue;
}


/*** SetArbFileLine *************************************************
 *
 * Return a description of an arbitrary file in the working directory.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * pszFileName = the name of the file
 * pszLine = the line of text containing the description
 */
int SetArbFileLine(model_def_type *pglobal, char *pszFileName, char *pszLine)
{
	int nReturnValue = 0;
	file_struct_type *pdesc = NULL;
	supplemental_file_info supp;
        struct llstat status;

	pdesc = (file_struct_type *) malloc(sizeof(file_struct_type));
	if (pdesc == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	/*
	 * fill in information about the file
	 */
	strcpy(pdesc->dirname, "./");
	strcpy(pdesc->filename, pszFileName);
	pdesc->language = other_lang;
	pdesc->where = file_src;

        if (llstat(pszFileName, &status))
		/*
		 * there was a problem
		 */
		{
		sprintf(pszLine, "%s\n", pszFileName);
		}
	else
		/*
		 * store information about the file
		 */
		{
		supp.st_ino = status.st_ino;
		supp.st_size = status.st_size;
		supp.mtime = status.st_mtime;
		if (nReturnValue = SetFileLine(pglobal, pdesc, &supp, pszLine))
			goto done;
		}

done:
	return nReturnValue;
}	/* SetArbFileLine */

/*** CompareObjDescription *********************************************
 *
 * Compare the description of an existing object file with a description
 * of a file we would build.  Indicate if a new object file should
 * be made based on the comparison.
 *
 * !!! Changes to this routine may require corresponding changes in
 * !!! SaveObjDescription.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * psuppFiles = array of supplemental information for all files.
 * pdescSource = the description of the source file
 * psuppSource = supplemental information for the source file
 * pnInclude = array of indices of include files
 * nIncludeFileCt = the number of include files
 * pbShouldCompile = indicates if a new object file should be created
 */
int CompareObjDescription(model_def_type *pglobal, 
	supplemental_file_info *psuppFiles,
	file_struct_type *pdescSource, supplemental_file_info *psuppSource,
	int *pnInclude, int nIncludeFileCt, boolean_t *pbShouldCompile)
{
	int nReturnValue = 0;
	FILE *pfileDescrip = NULL;
	char szDescripName[NAME_SIZE + 1];
	char szLocalIncName[NAME_SIZE];
	char *pszBuffer = NULL;	/* text for new object description */
	char *pszExisting = NULL;	/* text from existing object description */
	int nLocalCt;	/* number of local include files */
	int n,i;
	int nIndex;
	int nOnline, nOffline;
        struct llstat status;
	boolean_t bDescripExists = B_FALSE;	/* does description exist? */
	char szObjectName[NAME_SIZE];
	boolean_t bAffectedByGrid;	/* is the file affected by the grid ID? */
	int nLoopLength;

	*pbShouldCompile = B_TRUE;
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "*** CompareObjDescription: %s\n",
		pdescSource->filename);
#	endif

	/*
	 * open the description file
	 */
	sprintf(szDescripName, ".%s", pdescSource->filename);
	if (1 == get_migrate_state(szDescripName, &nOnline, &nOffline)) 
		bDescripExists = B_TRUE;
	else goto done;
	if (!nOnline) goto done;
	pfileDescrip = fopen(szDescripName, "r");
	if (pfileDescrip == NULL) goto done;
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "   Opened description file\n");
#	endif

	pszBuffer = malloc(MAX_COMMAND_LENGTH);
	pszExisting = malloc(MAX_COMMAND_LENGTH);
	if (pszBuffer == NULL || pszExisting == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
		goto file_error;
        
        /*remove the newline character  */
        for(i=0; pszExisting[i]!=0; i++) if( pszExisting[i]=='\n' ) pszExisting[i]='\0';

	SetEnvLine(pglobal, pdescSource, pszBuffer);
	if (strcmp(pszBuffer, pszExisting)) goto done;
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "   Environment matched\n");
#	endif


	if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
		goto file_error;
	if (nReturnValue = SetFileLine(pglobal, pdescSource,
		psuppSource, pszBuffer)) goto done;
	if (strcmp(pszExisting, pszBuffer)) goto done;
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "   Source file matched\n");
#	endif

	if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
		goto file_error;
	if (pglobal->grids.list_size == 0 || strcmp(pszExisting, "0\n") == 0)
		/*
		 * the file does not appear to be affected by gridding
		 */
		{
		strcpy(pszBuffer, "0\n");
		bAffectedByGrid = B_FALSE;
		}
	else
		{
		SetGridIDLine(pglobal, pszBuffer);
		bAffectedByGrid = B_TRUE;
		}
	psuppSource->bAffectedByGrid = bAffectedByGrid;
	if (strcmp(pszExisting, pszBuffer)) goto done;
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "   Grid ID matched\n");
#	endif

	if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
		goto file_error;
	if (sscanf(pszExisting, "%d", &n) != 1) goto done;
	if (n != pglobal->includes.list_size) goto done;

	for (n = 0; n < pglobal->includes.list_size; ++n)
		{
		if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
			goto file_error;
		SetSharedIncludeLine(pglobal, bAffectedByGrid, n, pszBuffer);
		if (strcmp(pszExisting, pszBuffer)) goto done;
		}
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "   Shared include files matched\n");
#	endif

	/*
	 * check the state of local include files
	 */
	if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
		goto file_error;
	if (sscanf(pszExisting, "%d", &nLocalCt) != 1) goto done;

	for (n = 0; n < nLocalCt; ++n)
		{
		if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
			goto file_error;
		if (sscanf(pszExisting, "%s", szLocalIncName) != 1) goto done;
		nIndex = FindLocalIncFileIndex(pglobal, szLocalIncName, pnInclude,
			nIncludeFileCt);

		if (nIndex >= 0)
			/*
			 * the include file was found in the list of files
			 */
			{
			if (nReturnValue = SetFileLine(pglobal, 
				&pglobal->files.list[nIndex],
				&psuppFiles[nIndex], pszBuffer)) goto done;
			}
		else sprintf(pszBuffer, "\n");

		if (strcmp(pszExisting, pszBuffer)) goto done;
		}
#	if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
	fprintf(stderr, "   Local include files matched\n");
#	endif

	nLoopLength = bAffectedByGrid ? pglobal->grids.list_size : 1;
	for (nIndex = 0; nIndex < nLoopLength; ++nIndex)
		{
		if (fgets(pszExisting, MAX_COMMAND_LENGTH, pfileDescrip) == NULL)
			goto file_error;
		if (nReturnValue = SetObjectName(pglobal,
			szObjectName, pdescSource->filename, 
			bAffectedByGrid ? pglobal->grids.list[nIndex] :
			NULL))
			goto done;
		if (!get_migrate_state(szObjectName, &nOnline, &nOffline)) 
			 goto done;
		if (!nOnline) goto done;
		if (nReturnValue = SetArbFileLine(pglobal, szObjectName, pszBuffer))
			goto done;
		if (strcmp(pszExisting, pszBuffer)) goto done;
#		if defined(DEBUG_ACTION) && DEBUG_ACTION == 2
		fprintf(stderr, "   Object file matched\n");
#		endif
		}	/* end of loop over grids */

	/*
	 * everything seems to match
	 */
	*pbShouldCompile = B_FALSE;
	goto done;

file_error:

done:
	if (bDescripExists && *pbShouldCompile && !pglobal->show_only)
		/*
		 * delete the description file
		 */
		unlink(szDescripName);
	if (pszBuffer != NULL) free(pszBuffer);
	if (pfileDescrip != NULL) fclose(pfileDescrip);
	return nReturnValue;
}	/* CompareObjDescription */

/*** SaveObjDescription *********************************************
 *
 * Save the description of an object file in a file.
 *
 * !!! Changes to this routine may require corresponding changes in
 * !!! CompareObjDescription.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * psuppFiles = array of supplemental information for all files.
 * pdescSource = the description of the source file
 * psuppSource = supplemental information for the source file
 * pnInclude = array of indices of include files
 * nIncludeFileCt = the number of include files
 * bAffectedByGrid = indicates if the file was compiled for each grid
 */
int SaveObjDescription(model_def_type *pglobal, 
	supplemental_file_info *psuppFiles,
	file_struct_type *pdescSource, supplemental_file_info *psuppSource,
	int *pnInclude, int nIncludeFileCt, boolean_t bAffectedByGrid)
{
	int nReturnValue = 0;
	FILE *pfileDescrip = NULL;
	char szDescripName[NAME_SIZE + 1];
	char *pszBuffer = NULL;
	int n;
	int *pnLocalIndices = NULL;		/* indices of local include files */
	int nLocalCt;
	int nIndex;
	int nLoopLength;
	char szObjectName[NAME_SIZE];

	if (pglobal->show_only) goto done;

	pszBuffer = malloc(MAX_COMMAND_LENGTH);
	nLocalCt = 100;
	pnLocalIndices = (int*) malloc(nLocalCt * sizeof(int));
	if (pszBuffer == NULL || pnLocalIndices == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		goto done;
		}

	sprintf(szDescripName, ".%s", pdescSource->filename);
	pfileDescrip = fopen(szDescripName, "w");
	if (pfileDescrip == NULL)
		{
		err_ret("Could not open '%s'", szDescripName);
		nReturnValue = ERROR_FILE;
		goto done;
		}

	SetEnvLine(pglobal, pdescSource, pszBuffer);
	if (fprintf(pfileDescrip, "%s\n", pszBuffer) < 0) goto file_error;
	if (nReturnValue = SetFileLine(pglobal, pdescSource,
		psuppSource, pszBuffer)) goto done;
	if (fprintf(pfileDescrip, "%s", pszBuffer) < 0) goto file_error;
	if (bAffectedByGrid) SetGridIDLine(pglobal, pszBuffer);
	else strcpy(pszBuffer, "0\n");
	if (fprintf(pfileDescrip, "%s", pszBuffer) < 0) goto file_error;
	if (fprintf(pfileDescrip, "%d\n", pglobal->includes.list_size) < 0)
		goto file_error;

	for (n = 0; n < pglobal->includes.list_size; ++n)
		{
		SetSharedIncludeLine(pglobal, bAffectedByGrid, n, pszBuffer);
		if (fprintf(pfileDescrip, "%s", pszBuffer) < 0) goto file_error;
		}

	/*
	 * write information about local include files
	 */
	if (nReturnValue = FindLocalIncludes(pglobal, pdescSource,
		pnInclude, nIncludeFileCt, &nLocalCt, pnLocalIndices))
		goto done;


	if (fprintf(pfileDescrip, "%d\n", nLocalCt) < 0)
		goto file_error;
	for (n = 0; n < nLocalCt; ++n)
		{
		nIndex = pnLocalIndices[n];	/* index for include file */
		if (nIndex >= 0)
			/*
			 * the include file was found in the list of files
			 */
			{
			if (nReturnValue = SetFileLine(pglobal, 
				&pglobal->files.list[pnLocalIndices[n]],
				&psuppFiles[pnLocalIndices[n]], pszBuffer)) goto done;
			if (fprintf(pfileDescrip, "%s", pszBuffer) < 0) goto file_error;
			}
		else 
			/*
			 * The include file is not found.  It's possible that
			 * the file is conditionally compiled and will not
			 * be used in the current compilation.  Print a blank
			 * line.
			 */
			if (fprintf(pfileDescrip, "\n") < 0) goto file_error;
		}

	/*
	 * write information about the object files that were produced
	 */
	nLoopLength = bAffectedByGrid ? pglobal->grids.list_size : 1;
	for (nIndex = 0; nIndex < nLoopLength; ++nIndex)
		{
		if (nReturnValue = SetObjectName(pglobal,
			szObjectName, pdescSource->filename, 
			bAffectedByGrid ? pglobal->grids.list[nIndex] :
			NULL))
			goto done;
		if (nReturnValue = SetArbFileLine(pglobal, szObjectName, pszBuffer))
			goto done;
		if (fprintf(pfileDescrip, "%s", pszBuffer) < 0) goto file_error;
		}	/* end of loop over grids */

	goto done;

file_error:
	err_ret("Error writing to file '%s'", szDescripName);
	nReturnValue = ERROR_FILE;
	goto done;

done:
	if (pnLocalIndices != NULL) free(pnLocalIndices);
	if (pszBuffer != NULL) free(pszBuffer);
	if (pfileDescrip != NULL) fclose(pfileDescrip);
	return nReturnValue;
}	/* SaveObjDescription */

/*** DetermineIfCompNecessary ***************************************
 *
 * Determine if compilation is necessary.
 *
 * return = error code
 *
 * pglobal = global information from the configuration file and
 *      environment information
 * plist = the structure containing file descriptions
 * psuppFiles = array of supplemental information for all files.
 * pnInclude = array of indices of include files
 * nIncludeFileCt = the number of include files
 */
int DetermineIfCompNecessary(model_def_type *pglobal, 
	file_list_type *plist, supplemental_file_info *psuppFiles,
	int *pnInclude, int nIncludeFileCt)
{
	file_struct_type *pdesc;
	file_struct_type *pdescEnd;
	supplemental_file_info *psupp;
	int nReturnValue = 0;

	for (pdesc = plist->list, pdescEnd = pdesc + plist->list_size,
		psupp = psuppFiles;
		pdesc < pdescEnd; ++pdesc, ++psupp)
		{
		if (pglobal->one_step || pglobal->compile_all)
			psupp->bShouldCompile = IsCompilable(pdesc->language);
		else
			{
			if (IsCompilable(pdesc->language))
				{
				if (nReturnValue = CompareObjDescription(pglobal,
					psuppFiles, pdesc, psupp, pnInclude, 
					nIncludeFileCt, &psupp->bShouldCompile))
					goto done;
				}
			else psupp->bShouldCompile = B_FALSE;
			}
		}

done:
	return nReturnValue;
}	/* DetermineIfCompNecessary */

/********************************************************************
 * Section 6. Entry point for file
 ********************************************************************/


/*** retrieve_and_build *********************************************
 *
 * Retrieve the necessary files and build the model.
 *
 * return = error code or 0 if no error
 *
 * pglobal = global and environment information
 * pfiledesc = information about the files that compose the model
 */
int retrieve_and_build(model_def_type *pglobal)
{
	int nReturnValue = 0;
	file_struct_type *pdesc;
	file_struct_type *pdescEnd;
	int *pnInclude = NULL;	/* array of indices of include files */
	int nIncludeFileCt = 0;
	supplemental_file_info *psuppFiles = NULL;
	supplemental_file_info *psupp;
	int i;
	file_list_type *file_list;

	if (pglobal->one_step && pglobal->grids.list_size)
		/*
		 * these options are incompatible
		 */
		{
		fprintf(stderr, "### The 'one_step' option is incompatible with "
			"multiple grids.\n### To build model, remove 'one_step' from "
			"configuration file.\n");
		nReturnValue = ERROR_INVALID_OPTION;
		goto done;
		}

	if (pglobal->one_step && pglobal->mode == no_link_mode)
		/*
		 * these options are incompatible
		 */
		{
		fprintf(stderr, "### The 'one_step' option is incompatible with "
			"the 'no_link' option.\n");
		nReturnValue = ERROR_INVALID_OPTION;
		goto done;
		}

#if 0
printf("*** settting verbose mode ***\n");
pglobal->verbose = B_TRUE;
#endif

#if 0
printf("*** initializing some grids ***\n");
pglobal->grids.list_size = 2;
strcpy(pglobal->grids.list[0], "aa");
strcpy(pglobal->grids.list[1], "bb");
#endif

#if 0
printf("*** setting one_step mode ***\n");
pglobal->one_step = B_TRUE;
#endif
	file_list = &pglobal->files;

	/*
	 * allocate space for supplemental file information
	 */
	psuppFiles = (supplemental_file_info *)
		calloc(sizeof(supplemental_file_info) * file_list->list_size, 1);
	pnInclude = (int *) malloc(sizeof(int) * file_list->list_size);
	if (psuppFiles == NULL || pnInclude == NULL)
		{
		nReturnValue = ERROR_MEMORY_FULL;
		fprintf(stderr, "### not enough memory\n");
		goto done;
		}

	if (pglobal->wd[0] && strcmp(pglobal->wd, "./"))
		/*
		 * set the working directory
		 */
		{
		if (pglobal->verbose) printf("cd %s\n", pglobal->wd);
		if (chdir(pglobal->wd) == -1)
			{
			nReturnValue = ERROR_CD;
			fprintf(stderr, "### could not change to directory: %s\n",
				pglobal->wd);
			goto done;
			}
		}

	if (pglobal->os == cray_os)
		RestoreMigratedFiles(pglobal);
		
	/*
	 * change empty directory names to "./" and build list of
	 * include files
	 */
	for (pdesc = file_list->list, pdescEnd = pdesc + file_list->list_size, 
		i = 0, psupp = psuppFiles;
		pdesc < pdescEnd; ++pdesc, ++i, ++psupp)
		{
		if (pdesc->where == file_src &&
			pdesc->dirname[0] == 0)
			strcpy(pdesc->dirname, "./");

		if (IsIncludeFile(pdesc->language))
			{
			pnInclude[nIncludeFileCt++] = i;
			psupp->bIsInclude = B_TRUE;
			}
		else psupp->bIsInclude = B_FALSE;
		}

	if (nReturnValue = CheckForDuplicateFiles(file_list)) goto done;

	if (nReturnValue = CheckFileStatus(pglobal, file_list, psuppFiles))
		goto done;

	if (nReturnValue = DetermineIfCompNecessary(pglobal, file_list,
		psuppFiles, pnInclude, nIncludeFileCt))
		goto done;

        if (nReturnValue = retrieve_all_files(pglobal, file_list, psuppFiles))
                goto done;  

	if (nReturnValue = build_model(pglobal, file_list, psuppFiles,
		pnInclude, nIncludeFileCt))
		goto done;


done:
#	ifdef DEBUG_ACTION
	if (nReturnValue) 
		fprintf(stderr, "retrieve_and_build: return = %d\n", nReturnValue);
#	endif
	if (psuppFiles) free(psuppFiles);
	if (pnInclude) free(pnInclude);
	return nReturnValue;
}