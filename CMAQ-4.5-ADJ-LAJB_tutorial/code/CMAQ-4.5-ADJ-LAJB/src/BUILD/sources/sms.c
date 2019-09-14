/* RCS file, release, date & time of last delta, author, state, [and locker] */
/* $Header: /global/home/groups-sw/ac_seasonal/lbastien/CMAQ/v4.5-ADJ/models/BUILD/src/sources/sms.c,v 1.1.1.1 2005/09/09 19:26:42 sjr Exp $  */

/* what(1) key, module and SID; SCCS file; date and time of last delta: */
/* %W% %P% %G% %U% */

/****************************************************************************
 *
 * Project Title: Environmental Decision Support System
 * File: @(#)sms.c	11.1
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
 * Last updated: 29 Jan 2001
 *
 ****************************************************************************/


/****************************************************************************
 * This source file has been modified to work within the Models-3 framework *
 *                                  by                                      *
 * The National Exposure Research Laboratory, Atmospheric Modeling Division *
 *                                                                          *
 *                                                                          *
 *  The major changes include:                                              *
 *                                                                          *
 *      - changing the archive software to CVS                              *
 *      - adding code to work on a Windows NT platform (Digital Fortran 90) *
 *      - adding code to work with remote CVS                               *
 *                                                                          *
 ****************************************************************************/


#include <string.h>
#include <stdlib.h>

#if _WIN32
#define popen _popen
#define pclose _pclose
#define sleep _sleep 
#include "utsname.h"

#else
#include <sys/utsname.h>
#endif

#include <sys/stat.h>
#include <errno.h>
/* #define _XOPEN_SOURCE */
#include <ctype.h>
#include <stdio.h> 

#include "sms.h"

/*#define MOD_DEBUG*/ 
/*#define ENV_DEBUG*/ 

#ifdef SMS_DEBUG
#	define EXT_DEBUG
#	define MOD_DEBUG
#endif


char sms_vers[] = "@(#)sms.c	11.1 /pub/storage/edss/framework/src/model_mgr/bldmod/SCCS/s.sms.c 08 Jul 1996 16:42:48";

/*  prototype for strscan function  */
int strscan( char *, char * );
int filetype( char* fname );
void getModRef( char * fname, char * modname, char * uses );


/*** get_module_dirname ************************************************
 *
 * Find the path to a module.
 *
 * return = nonzero if there was a problem
 *
 * pszModuleName = name of the module
 * pszModulePath = string to contain the path.  Should have length
 *		NAME_SIZE.
 *
 */
int get_module_dirname(char *pszModuleName, char *pszModulePath)
{
	FILE *pfile;
	char szCommand[NAME_SIZE];
	int nReturnValue = 0;
	char *pszFgetsResult;
	int nLength;

	sprintf(szCommand, "bldmod_find_module %s", pszModuleName);
	if ((pfile = popen(szCommand, "r")) == NULL)
		{
		nReturnValue = errno ? errno : ERROR_RESOLVE_MODULE;
		goto done;
		}
	*pszModulePath = '\0';
	pszFgetsResult = fgets(pszModulePath, NAME_SIZE, pfile);
	if (nReturnValue = pclose(pfile))
		{
		if (pszFgetsResult == NULL || !isascii(*pszModulePath) ||
			!isdigit(*pszModulePath))
			fprintf(stderr, "### The script 'bldmod_find_module' was not "
				"successfully invoked.  Check your path.\n");
		else
			{
			sscanf(pszModulePath, "%d", &nReturnValue);
			if (nReturnValue == 201)
				fprintf(stderr, "### The environment specifies an invalid "
					"directory for modules.\n");
			else if (nReturnValue == 202)
				fprintf(stderr, "### The module '%s' was not found.\n",
					pszModuleName);
			else if (nReturnValue == 203)
				fprintf(stderr, "### There seem to be multiple directories "
					"for module '%s'.\n", pszModuleName);
			else
				{
				fprintf(stderr, "### Indeterminate error finding module "
					"'%s'.\n", pszModuleName);
				}
			}

		nReturnValue = ERROR_RESOLVE_MODULE;
		strcpy(pszModulePath, "/dev/null");
		goto done;
		}

	/*
	 * one directory was found
	 */
	nLength = strlen(pszModulePath);
	if (pszModulePath[nLength - 1] != '\n')
		/*
		 * the path was truncated
		 */
		{
		fprintf(stderr, "### The path to the module %s was too long.\n",
			pszModuleName);
		nReturnValue = ERROR_RESOLVE_MODULE;
		goto done;
		}
	else
		/*
		 * replace the new line with a slash
		 */
		{
		pszModulePath[nLength - 1] = '/';
		}

done:;
	return nReturnValue;
}


/*
 *       *****************************************************
 *       * The routine RestoreMigratedFiles also has a list
 *       * of recognized extensions.  These two lists should
 *       * be kept in sync.
 *       *****************************************************
 */

/* 
   this table matches file extensions with an English description.
   The index of the table correspondents to the value of enum type
   lang_type.
   The organization of this table is due to historical reasons.
 */
static char lang_tab[other_lang + 1][2][64] =
	{
	"*.c",			"C",
	"*.cc *.cpp",		"C++",
	"*.f *.F",		"FORTRAN",
	"*.h",			"C header",
	"*.hh",			"C++ header",
	"*.ext *.EXT *.icl *.inc *.cmd",		"FORTRAN include"
	"*",			"Other"
	};

lang_type get_file_type ( char filename[] )
	{
	int lang_index, ext_index;
	char * ext;
	int found_it;
	char * test_ptr;
	char test_string[64];

#	ifdef EXT_DEBUG
	printf ( "checking file type of %s\n", filename );
#	endif

	for ( found_it = 0, ext = filename + strlen ( filename ) - 1; 
	      ext >= filename; ext-- )
		{
#		ifdef EXT_DEBUG
		printf ( "ext = %s\n", ext );
#		endif

		if ( *ext == (char) '.' )
			{
			found_it = 1;
			ext++;

#			ifdef EXT_DEBUG
			printf ( "file extension is %s\n", ext );
#			endif

			break;
			}
		}


	if ( ! found_it )
		{
#		ifdef EXT_DEBUG
		printf ( "couldn't find file extension for %s\n", filename );
#		endif

		return ( other_lang );
		}

	for ( lang_index = 0; lang_index < (int) other_lang; lang_index++ )
		{
		for ( test_ptr = lang_tab[lang_index][0]; 
		      sscanf ( test_ptr, "%s", test_string ) == 1;
		      test_ptr += strlen ( test_string ) )
			{
			/* compare strings, skipping over the *. part */
#			ifdef EXT_DEBUG
			printf ( "comparing %s to %s\n", ext, test_string + 2 );
#			endif

			if ( strcmp ( ext, test_string + 2 ) == 0 )
				{
#				ifdef EXT_DEBUG
				printf ( "%s matches %s. type is %s\n",
					 ext, test_string,
					 lang_tab[lang_index][1] );
#				endif

				return ( (lang_type) lang_index );
				}
			}	
		}
		
#	ifdef EXT_DEBUG
	printf ( "no match for %s\n", ext );
#	endif

	return ( other_lang );
	}

/*  read record from file */
int getrec( FILE * lfn, char* buff, int lenbuff )
{
	int c;
	int i,j=0;

	for(i=0; (c=getc(lfn))!=EOF; i++)
	 {
	  j = i<lenbuff ? i : lenbuff;

	  if( c==10 || c=='\n' || c==0 || c==13 )  /* end of record */
	   {
             buff[j]=0;
	     return j;
           }
	  buff[j]=c;
	 }
     buff[j]=0;
     return -1;
}

/*   check if module_name matches dir_name from "cvs co -c" call   */
void matchMod( char* module, char* modname )
{
	FILE * pipe;
	char m_name[NAME_SIZE];
	char d_name[NAME_SIZE];
	char buff[NAME_SIZE];
	int i;

	/*  set default */
        strcpy(modname,module);

        /*  run command with pipe  */
        pipe = popen ( "cvs co -c", "r" );

        /* read pipe and look for match */
        while ( getrec(pipe, buff, NAME_SIZE) >= 0 )
		{
                  sscanf ( buff, " %s %s ", m_name, d_name );
		
		  if( strcmp( module, m_name ) == 0 )
		    {
		      strcpy(modname,m_name);
                    }
		  else
		    {
                      /* find directory of d_name and compare to modname */
		      for(i=strlen(d_name); i>=0 && d_name[i]!=47; i--);
		      if(strcmp( module, &d_name[i+1] )==0) strcpy(modname,m_name);
	            }
	 	}

        /* no match, so return modname as is */
	pclose( pipe );
        return;
}

/*  --------------------------------------------------------------- */
/*    extract files from cvs archive modules */
int get_all_module_files ( model_def_type * model )
	{
	char cmd_string[256];
	FILE * list_file;
	char filename[256];
	boolean_t list_is_trivial = B_TRUE;
	int localdir=0;
        int i,j,ftype,nfiles,fdup;
	char ret_module_name[NAME_SIZE], 
		 release_num[10], 
		 dev_num[10], 
		 checked_num[10];
	long idate;
	int iyr,imo,idy;
        char modname[NAME_SIZE];
        char cvsroot[10];
		 

/* set CVSROOT variable    */
#ifdef _WIN32
        strcpy( cvsroot, "%CVSROOT%" );
#else
        strcpy( cvsroot, "$CVSROOT" );
#endif




        model->modules.curr_ptr = model->modules.list;

/*   -----------------  Start main loop -------------------- */
/*  loop to run cvs for each module.list  */
	for ( i = 0; i < model->modules.list_size; i++ )
           {
            if (  (model->modules.list[i]).module_name[0] != (char) '#' )
              {
               list_is_trivial = B_FALSE;
	       localdir=0;

/*   check if module_name matches dir_name from "cvs co -c" call   */
		matchMod( (model->modules.list[i]).module_name, modname );

/*  set export option for module  */
                switch ( (model->modules.curr_ptr)->where )
                  {
                    case latest_release_src :
                       sprintf ( cmd_string, "cvs -d %s -r export -r HEAD -d %s  %s", cvsroot, modname, modname );
                       break;
                    case dev_src :
                       sprintf ( cmd_string, "cvs -d %s -r export -r HEAD -d %s  %s", cvsroot, modname, modname );
                       break;
                    case local_dir :
		       localdir=1;
                       sprintf ( cmd_string, "cvs -d %s -r export -r HEAD -d %s  %s", cvsroot, modname, modname );
		       break;
                    case version_src :
                       sprintf ( cmd_string, "cvs -d %s -r export -r %s  -d %s  %s",
                                    cvsroot,
                                    (model->modules.curr_ptr)->version_num,
                                    modname, modname );
		       break;
                    case release_time_src :
			if((idate=atol( (model->modules.curr_ptr)->date)) > 10000)
			 {
			  iyr = idate/10000;
			  imo = (idate % 10000) / 100;
			  idy = idate % 100;
                          sprintf ( cmd_string, "cvs -d %s -r -q export -D %d-%d-%d -d %s  %s",
                                    cvsroot,iyr,imo,idy,modname,modname );
			  }
			else
                         sprintf ( cmd_string, "cvs -d %s -r export -D %s  -d %s  %s",
                                    cvsroot,
                                    (model->modules.curr_ptr)->date,
                                    modname, modname );
                       break;
                    default :
                       fprintf ( stderr, "Illegal module source type encountered\n" );
				return ( FAILURE );
                        }   /* end of switch */


	    /* print message to screen */
            if( localdir )
               printf("\n Retrieving files from local directory: %s \n", (model->modules.curr_ptr)->dirname);
	    else
               printf("\n CVS command = %s \n", cmd_string );


            /*  run command with pipe  */
            list_file = popen ( cmd_string, "r" );

            /* make sure pipe is successful */
            while ( fscanf ( list_file, " %s %s\n",
                             checked_num,
                             filename) == 2 )
		{

                if ( strcmp("U",checked_num)==0 )
                  {
                    strcpy(ret_module_name, (model->modules.list[i]).module_name) ;
                    strcpy(release_num,"HEAD");
                    strcpy(dev_num," ");

                    /* find position where the root filename starts */
                    for(j=strlen(filename); j>=0 && filename[j]!=47; j--);

                    /*  check if file already exists */
                    ftype = filetype( &filename[j+1] );

                    /*  if file has already been retrieved, then skip */
                    if( ftype >= 0 )
                    {
                       fdup=0;
                       for(nfiles=0; nfiles<(model->archive_files.list_size); nfiles++)
                       {
                          if( !strcmp( &filename[j+1], (model->archive_files.list[nfiles]).filename ) )
                          {
                             printf(" warning: file %s has already been retrieved from archive\n", &filename[j+1] );
                             fdup=1;
                             break;   
                          }   
                       }
                       if( fdup ) continue;  /* skip if duplicate file */
                    }

                    /*  check if file should be replaced */
                    if( ftype==-1 || ftype==0 || ftype==2)   /* replace file */
                    {
#ifdef _WIN32
                      if(ftype==0 || ftype==2)
                      {  sprintf ( cmd_string, "attrib -r %s > nul ", &filename[j+1] );
                         system( cmd_string );
                      }
                      sprintf ( cmd_string, "copy %s\\%s . > nul ", modname,&filename[j+1] );
                      system( cmd_string );
                      sprintf ( cmd_string, "attrib +r %s > nul ", &filename[j+1] );
                      system( cmd_string );
#else
                      sprintf ( cmd_string, "/bin/mv -f %s/%s %s ", modname,&filename[j+1],&filename[j+1] );
                      system( cmd_string );
                      sprintf ( cmd_string, "chmod 444  %s ", &filename[j+1] );
                      system( cmd_string );
#endif
                    }
			
                    append_file ( & ( model->archive_files ) );
                    string_copy ( model->archive_files.curr_ptr->module_name,
		         model->modules.curr_ptr->module_name, NAME_SIZE );
                    strcpy ( model->archive_files.curr_ptr->dirname, "" );

                    string_copy ( model->archive_files.curr_ptr->filename, &filename[j+1],
		              NAME_SIZE );
		    
                    model->archive_files.curr_ptr->language = get_file_type ( filename );
                    model->archive_files.curr_ptr->where = (model->modules.curr_ptr)->where;
                    strcpy ( model->archive_files.curr_ptr->version_num, "" );
                    strcpy ( model->archive_files.curr_ptr->date, "" );

                    switch ( (model->modules.curr_ptr)->where )
			{
			case latest_release_src :
				string_copy ( model->archive_files.curr_ptr->version_num, 
				         release_num, NAME_SIZE );
				break;

			case dev_src :
                                strcpy ( model->archive_files.curr_ptr->dirname, 
					(model->modules.curr_ptr)->dirname );
				string_copy ( model->archive_files.curr_ptr->version_num, 
				              dev_num, NAME_SIZE );
				break;

			case local_dir :
#ifdef _WIN32
                                sprintf ( cmd_string, "copy  %s\\%s .",
				    (model->modules.curr_ptr)->dirname,
                                    (model->archive_files.curr_ptr)->filename );
#else        
                                sprintf ( cmd_string, "rm -f %s; cp  %s/%s .",
                                    (model->archive_files.curr_ptr)->filename,
				    (model->modules.curr_ptr)->dirname,
                                    (model->archive_files.curr_ptr)->filename );
#endif
                                    
                                if ( system( cmd_string ) )  /* if copy fails, use archive copy */
				{
                                  printf(" Cannot find local file, using copy from Archive\n" );
#ifdef _WIN32
			          sprintf ( cmd_string, "copy %s\\%s  . ", modname,
						(model->archive_files.curr_ptr)->filename );
#else
			          sprintf ( cmd_string, "/bin/mv -f %s/%s  . ", modname,
						(model->archive_files.curr_ptr)->filename );
#endif
			          system( cmd_string );
				}

				string_copy ( model->archive_files.curr_ptr->version_num, 
				              dev_num, NAME_SIZE );
                                model->archive_files.curr_ptr->where = version_src; 
				break;

			case version_src :
				string_copy ( model->archive_files.curr_ptr->version_num, 
				         (model->modules.curr_ptr)->version_num, NAME_SIZE );
				break;

			case release_time_src :
				string_copy ( model->archive_files.curr_ptr->date, 
				         (model->modules.curr_ptr)->date, NAME_SIZE );
				break;

			default :
				fprintf ( stderr, "Illegal module source type encountered\n" );
				return ( FAILURE );
				/* break; */
                        }   /* end of switch */
                   }   /* end of if valid module name */
                }  /* end loop for pipe */

            pclose ( list_file );
	    

            /*  remove temp directory for module */
#ifdef _WIN32
            sprintf ( cmd_string, "rmdir /s /q %s", modname );
#else
            sprintf ( cmd_string, "rm -r -f %s", modname );  
#endif
            sleep(3);                /*  wait 3 seconds for files to be copied */
            system( cmd_string );

            }  /* if valid module */
         (model->modules.curr_ptr)++;
         }    /* end for loop for each module's cvs command*/


	return ( SUCCESS );
      }



void copy_file_struct ( file_struct_type * from_struct, file_struct_type * to_struct )
	{
	char * from = (char *) from_struct;
	char * to = (char *) to_struct;
	int i;
	
	for ( i = 0; i < sizeof ( file_struct_type ); i++ )
		{
		*to++ = *from++;
		}
	}


void order_files( model_def_type * model )
{
   file_struct_type swap_file;
   char modname[N_SRC_FILES][32];
   char uses[N_SRC_FILES][256];
   char dumstr[256];
   char fname[256];
   int i,nf,sorted;
   int k,nmods,swapcnt,kswit;
  
   /*  loop thru list of files and build uses and modname arrays */
   for(i=0; i<(model->files.list_size); i++)
   {
       strcpy( fname, (model->files.list[i]).dirname );
       strcat( fname, (model->files.list[i]).filename );
       getModRef( fname, modname[i], uses[i] );  
   }

   /*  sort file list */
   nf = (model->files.list_size);
   sorted=0;
   while( sorted==0 && nf>1)
   {
      nf--;
      sorted=1;
      for(i=0; i<nf; i++)
      {
         /*  move module ahead of non-module */
         if( strlen(modname[i])==0 && strlen(modname[i+1])>0 ) 
         {
            sorted=0;
            copy_file_struct ( &(model->files.list[i]), &swap_file );
            copy_file_struct ( &(model->files.list[i+1]), &(model->files.list[i]) );
            copy_file_struct ( &swap_file, &(model->files.list[i+1]) );
         
            strcpy(dumstr, modname[i]);
            strcpy(modname[i], modname[i+1]); 
            strcpy(modname[i+1], dumstr);

            strcpy(dumstr, uses[i]);
            strcpy(uses[i], uses[i+1]); 
            strcpy(uses[i+1], dumstr);
         }
      }
   }
 
   /*  put files containing modules in order  */
   for(nmods=0; nmods<(model->files.list_size) && strlen(modname[nmods])>0; nmods++);  

   if( nmods > 1 )
   {
      swapcnt=0;
      for(i=0; i<nmods-1;)
      {
         kswit=0;
         for(k=i+1; k<nmods && kswit==0; k++)
         {
           if( strscan(modname[k],uses[i]) >=0 )
           {
               kswit=1;
               swapcnt++;
               copy_file_struct ( &(model->files.list[i]), &swap_file );
               copy_file_struct ( &(model->files.list[k]), &(model->files.list[i]) );
               copy_file_struct ( &swap_file, &(model->files.list[k]) );

               strcpy(dumstr, modname[i]);
               strcpy(modname[i], modname[k]); 
               strcpy(modname[k], dumstr);

               strcpy(dumstr, uses[i]);
               strcpy(uses[i], uses[k]); 
               strcpy(uses[k], dumstr);
           }     
         }
         if( kswit==0 || swapcnt>nmods ){ swapcnt=0; i++; } /* move to next module */
      }
   }

   return;
} 


	
int merge_file_lists ( model_def_type * model )
	{
	int i, j, k;
	lang_type file_lang;
	file_struct_type * archive_ptr, *local_ptr;
	file_struct_type * module_start, * module_end, * this_file;
	module_struct_type * module_ptr;
	boolean_t file_is_replacement;
	
#	ifdef MOD_DEBUG
	printf ( "calling append_file ( merged_list )\n" );
#	endif

	append_file ( & ( model->files ) );
	
	for ( i = 0, j = 0, k = 0; i < model->modules.list_size; i++ )
		{
		module_ptr = model->modules.list + i;

		if ( model->files.curr_ptr == NULL )
			{
			module_start = model->files.list;
			}
		else
			{
			module_start = model->files.curr_ptr;
			}
		
		for ( ; j < model->archive_files.list_size; j++ )
			{
			archive_ptr = model->archive_files.list + j;
			
			if ( strcmp ( archive_ptr->module_name,
				          module_ptr->module_name ) != 0 )
				{
				break;
				}
				
#			ifdef MOD_DEBUG
			printf ( "copying file struct for archive file %s to merged list at %X\n",
			         archive_ptr->filename, model->files.curr_ptr );
#			endif

			copy_file_struct ( archive_ptr, 
					           model->files.curr_ptr );

			file_lang = archive_ptr->language;
			string_copy ( model->files.curr_ptr->cpp_flags, 
			              module_ptr->cpp_flags, NAME_SIZE );
			string_copy ( model->files.curr_ptr->compile_flags,
					 module_ptr->compile_flags[file_lang], NAME_SIZE );

#			ifdef MOD_DEBUG
			print_file_struct ( model->files.curr_ptr );
			printf ( "calling append_file ( merged_list )\n" );
#			endif

			append_file ( & ( model->files ) );				
			}
			
		if ( model->files.curr_ptr == NULL )
			{
			module_end = model->files.list;
			}
		else
			{
			module_end = model->files.curr_ptr;
			}
		
		for ( ; k < model->local_files.list_size; k++ )
			{
			local_ptr = model->local_files.list + k;
			
			if ( strcmp ( local_ptr->module_name,
				          module_ptr->module_name ) != 0 )
				{
				break;
				}
				
			for ( this_file = module_start, file_is_replacement = B_FALSE; 
				  this_file < module_end; 
				  this_file++ )
				{
#				ifdef MOD_DEBUG
				printf ( "comparing %s to %s\n", local_ptr->filename, this_file->filename );
#				endif

				if ( strcmp ( local_ptr->filename, this_file->filename ) == 0 )
					/* file exists in archive.  replace it with local file */
					{
#					ifdef MOD_DEBUG
					printf ( "replacing filename %s\n", local_ptr->filename );
#					endif

					model->files.curr_ptr = this_file;
					file_is_replacement = B_TRUE;
					break;
					}
				}

#			ifdef MOD_DEBUG
			printf ( "copying file struct for local file %s to merged list at %X\n",
			         local_ptr->filename, model->files.curr_ptr );
#			endif

			copy_file_struct ( local_ptr, 
					           model->files.curr_ptr );

			file_lang = local_ptr->language;
			string_copy ( model->files.curr_ptr->cpp_flags, 
			              module_ptr->cpp_flags, NAME_SIZE );
			string_copy ( model->files.curr_ptr->compile_flags,
					 module_ptr->compile_flags[file_lang], NAME_SIZE );

			if ( file_is_replacement )
				{
				/* file was substituted.  Just go to the end, which is already empty */
				model->files.curr_ptr = model->files.list + model->files.list_size - 1;
#				ifdef MOD_DEBUG
				printf ( "setting model->files.curr_ptr to %X\n",
				         model->files.curr_ptr );
#				endif
				}
			else
				{
				/* file was not substituted, so it was tacked on the end */
#				ifdef MOD_DEBUG
				printf ( "calling append_file ( merged_list )\n" );
#				endif

				append_file ( & ( model->files ) );
				}
			}
		}
		
	(model->files.list_size)--;

/*   Call routine to sort files for Module names and Uses in each file */
        order_files( model );
		
	return ( SUCCESS );
	}
	
/* file list routines */

void init_file_list ( file_list_type * file_list )
	{
	file_list->array_size = N_SRC_FILES;
	file_list->list_size = 0;
	file_list->curr_ptr = NULL;
	}

int append_file ( file_list_type * file_list )
	{
	if ( file_list->list_size < file_list->array_size )
		{
		file_list->curr_ptr = file_list->list + file_list->list_size; /* point to end */
		file_list->list_size++;
		return ( SUCCESS );
		}
	else
		{
		fprintf ( stderr, "SMS_PARSER :: file limit of %d exceeded. returning.\n",	 
			  N_SRC_FILES );
		/* yyabort; */
		return ( FAILURE );
		}
	}
 
void first_file ( file_list_type * file_list )
	{
	if ( file_list->list_size > 0 )
		{
		file_list->curr_ptr = file_list->list;
		}
	else
		{
		file_list->curr_ptr = (file_struct_type *) NULL;
		}
	}
	
void last_file ( file_list_type * file_list )
	{
	file_list->curr_ptr = file_list->list + file_list->list_size - 1;
	}
	
int next_file ( file_list_type * file_list )
	{
	if ( file_list->curr_ptr < ( file_list->list + file_list->list_size - 1 ) )
		{
		(file_list->curr_ptr)++;
		return ( SUCCESS );
		}
	else
		{
		return ( FAILURE );
		} 
	}

void print_file_struct ( file_struct_type * file )
	{
	printf ( "MODULE:        %s\n", file->module_name );
	printf ( "FILENAME:      %s\n", file->filename );
	printf ( "DIRNAME:       %s\n", file->dirname );
	printf ( "LANGUAGE:      %s\n", lang_tab[(int)(file->language)][1] );
	printf ( "WHERE:         " );
	
	switch ( file->where )
		{
		case file_src :
		
			printf ( "local file\n" );
			break;
			
		case latest_release_src :
		
			printf ( "release version\n" );
			break;
			
		case dev_src :
		
			printf ( "development version\n" );
			break;
			
		case version_src :
		
			printf ( "version number %s\n", file->version_num );
			break;
			
		case release_time_src :
		
			printf ( "archive as of %s\n, file->date" );
			break;
			
		default :
		
			printf ( "??? unknown source option\n" );
			break;			
		}
		
	printf ( "VERSION:       %s\n", file->version_num );
	printf ( "DATE:          %s\n", file->date );
	printf ( "CPP FLAGS:     %s\n", file->cpp_flags );
	printf ( "COMPILE FLAGS: %s\n", file->compile_flags );
	printf ( "\n" );
	}
 
void print_file_list ( file_list_type * list )
	{
	int i;   
	 
	printf ( "\nfile list:\n\n" );
		 
	for ( i = 0; i < list->list_size; i++ )
		{
		print_file_struct ( list->list + i );
		}
	}

/* module list routines */

void init_module_struct ( module_struct_type * module )
	{
	int i;

	strcpy ( module->cpp_flags, "" );

	for ( i=0; i <= (int) other_lang; i++ )
		{
		strcpy ( module->compile_flags[i], "" );
		}
	}

void init_module_list ( module_list_type * module_list )
	{
	module_list->array_size = N_MODULES;
	module_list->list_size = 0;
	module_list->curr_ptr = NULL;
	}

int append_module ( module_list_type * module_list )
	{
	if ( module_list->list_size < module_list->array_size )
		{
		module_list->curr_ptr = module_list->list + module_list->list_size; /* point to end */
		module_list->list_size++;
		return ( SUCCESS );
		}
	else
		{
		fprintf ( stderr, "SMS_PARSER :: module limit of %d exceeded. returning.\n",	 
			  N_MODULES );
		/* yyabort; */
		return ( FAILURE );
		}
	}
 
void first_module ( module_list_type * module_list )
	{
	if ( module_list->list_size > 0 )
		{
		module_list->curr_ptr = module_list->list;
		}
	else
		{
		module_list->curr_ptr = (module_struct_type *) NULL;
		}
	}
	
void last_module ( module_list_type * module_list )
	{
	module_list->curr_ptr = module_list->list + module_list->list_size - 1;
	}
	
int next_module ( module_list_type * module_list )
	{
	if ( module_list->curr_ptr < ( module_list->list + module_list->list_size - 1 ) )
		{
		(module_list->curr_ptr)++;
		return ( SUCCESS );
		}
	else
		{
		return ( FAILURE );
		} 
	}

void print_module_struct ( module_struct_type * module )
	{
	int i;
	
	printf ( "MODULE:        %s\n", module->module_name );
	printf ( "WHERE:         " );
	
	switch ( module->where )
		{
		case file_src :
		
			printf ( "local module\n" );
			break;
			
		case latest_release_src :
		
			printf ( "release version\n" );
			break;
			
		case dev_src :
		
			printf ( "development version\n" );
			break;
			
		case version_src :
		
			printf ( "version number %s\n", module->version_num );
			break;
			
		case release_time_src :
		
			printf ( "archive as of %s\n, module->date" );
			break;
			
		default :
		
			printf ( "??? unknown source option\n" );
			break;			
		}
		
	printf ( "VERSION:       %s\n", module->version_num );
	printf ( "DATE:          %s\n", module->date );
	printf ( "CPP FLAGS:     %s\n", module->cpp_flags );
	
	printf ( "COMPILE FLAGS:\n" );

	for ( i = 0; i < other_lang; i++ )
		{
		printf ( "\t%s %s\n", lang_tab[i][1], module->compile_flags[i] );
		}
		
	printf ( "\n" );
	}
 
void print_module_list ( module_list_type * list )
	{
	int i;   
	 
	printf ( "\nmodule list:\n\n" );
		 
	for ( i = 0; i < list->list_size; i++ )
		{
		print_module_struct ( list->list + i );
		}
	}

/* include list routines */

void init_include_list ( include_list_type * include_list )
	{
	include_list->array_size = N_INCLUDE_FILES;
	include_list->list_size = 0;
	include_list->curr_ptr = NULL;
	}

int append_include ( include_list_type * include_list )
	{
	if ( include_list->list_size < include_list->array_size )
		{
		include_list->curr_ptr = include_list->list + include_list->list_size; /* point to end */
		include_list->list_size++;
		return ( SUCCESS );
		}
	else
		{
		fprintf ( stderr, "SMS_PARSER :: include limit of %d exceeded. returning.\n",	 
			      N_INCLUDE_FILES );
		return ( FAILURE );
		}
	}
 
void first_include ( include_list_type * include_list )
	{
	if ( include_list->list_size > 0 )
		{
		include_list->curr_ptr = include_list->list;
		}
	else
		{
		include_list->curr_ptr = (include_struct_type *) NULL;
		}
	}
	
void last_include ( include_list_type * include_list )
	{
	include_list->curr_ptr = include_list->list + include_list->list_size - 1;
	}
	
int next_include ( include_list_type * include_list )
	{
	if ( include_list->curr_ptr < ( include_list->list + include_list->list_size - 1 ) )
		{
		(include_list->curr_ptr)++;
		return ( SUCCESS );
		}
	else
		{
		return ( FAILURE );
		} 
	}

void print_include_struct ( include_struct_type * include )
	{
	printf ( "SYMBOLNAME:  %s\n", include->symbolname );
	printf ( "DIRNAME:  %s\n", include->dirname );
	printf ( "FILENAME: %s\n", include->filename );
	printf ( "%s apply grid names\n\n", include->apply_grid_names ? "Do" : "Don't" );
	}
 
void print_include_list ( include_list_type * list )
	{
	int i;   
	 
	printf ( "\ninclude list:\n\n" );
		 
	for ( i = 0; i < list->list_size; i++ )
		{
		print_include_struct ( list->list + i );
		}
	}

void init_grid_list ( grid_list_type * grid_list )
	{
	grid_list->array_size = N_GRIDS;
	grid_list->list_size = 0;
	grid_list->curr_ptr = NULL;
	}

int append_grid ( grid_list_type * grid_list )
	{
	if ( grid_list->list_size < grid_list->array_size )
		{
		grid_list->curr_ptr = grid_list->list + grid_list->list_size; /* point to end */
		grid_list->list_size++;
		return ( SUCCESS );
		}
	else
		{
		fprintf ( stderr, "SMS_PARSER :: grid limit of %d exceeded. returning.\n",	 
			      N_GRIDS );
		return ( FAILURE );
		}
	}
 
void first_grid ( grid_list_type * grid_list )
	{
	if ( grid_list->list_size > 0 )
		{
		grid_list->curr_ptr = grid_list->list;
		}
	else
		{
		grid_list->curr_ptr = (gridname_type *) NULL;
		}
	}
	
void last_grid ( grid_list_type * grid_list )
	{
	grid_list->curr_ptr = grid_list->list + grid_list->list_size - 1;
	}
	
int next_grid ( grid_list_type * grid_list )
	{
	if ( grid_list->curr_ptr < ( grid_list->list + grid_list->list_size - 1 ) )
		{
		(grid_list->curr_ptr)++;
		return ( SUCCESS );
		}
	else
		{
		return ( FAILURE );
		} 
	}

void print_gridname ( gridname_type * grid )
	{
	printf ( "%s\n", grid );
	}
 
void print_grid_list ( grid_list_type * list )
	{
	int i;   
	 
	printf ( "\ngrid list:\n\n" );
		 
	for ( i = 0; i < list->list_size; i++ )
		{
		print_gridname ( list->list + i );
		}

	printf ( "\n" );
	}

void init_model_def ( model_def_type * model_def )
	{
	struct utsname unameInfo;

	model_def->description[0] = '\0';
	model_def->architecture[0] = '\0';
	model_def->exename[0] = '\0';
	model_def->verbose = B_FALSE;
	model_def->mode = build_mode;
	model_def->show_only = B_FALSE;
	model_def->compile_all = B_FALSE;
	model_def->one_step = B_FALSE;
	model_def->clean_up = B_FALSE;
	model_def->fast_check = B_FALSE;
	model_def->archive_location = sams_default_archive;
	strcpy ( model_def->wd, "./" );
        strcpy ( model_def->compilers[c_lang], "cc" );
        strcpy ( model_def->compilers[c_plus_plus_lang], "C++" );
/*      strcpy ( model_def->compilers[f_lang], "f77" );   */
        strcpy ( model_def->compile_flags[c_lang], "-Xc -v -g" );
        strcpy ( model_def->compile_flags[c_plus_plus_lang], "-Xc -v -g" );
        strcpy ( model_def->compile_flags[f_lang], "-c" );
	strcpy ( model_def->compilers[h_lang], "echo can\'t compile" );
	strcpy ( model_def->compilers[hh_lang], "echo can\'t compile" );
	strcpy ( model_def->compilers[ext_lang], "echo can\'t compile" );
	strcpy ( model_def->compilers[other_lang], "echo can\'t compile" );
	strcpy ( model_def->cpp_flags, "" );
	strcpy ( model_def->compile_flags[h_lang], "" );
	strcpy ( model_def->compile_flags[hh_lang], "" );
	strcpy ( model_def->compile_flags[ext_lang], "" );
	strcpy ( model_def->compile_flags[other_lang], "" );
	strcpy ( model_def->libraries, "" );
	strcpy ( model_def->link_flags, "" );

	init_include_list ( & ( model_def->includes ) );
	init_grid_list ( & ( model_def->grids ) ); 
	init_file_list ( & ( model_def->files ) );
	init_file_list ( & ( model_def->local_files ) );
	init_file_list ( & ( model_def->archive_files ) );
	init_module_list ( & ( model_def->modules ) );

	/*
	 * determine operating system type
	 */
	if (uname(&unameInfo) >= 0)
		/*
		 * set the architecture and machine information
		 */
		{
		string_copy(model_def->architecture, unameInfo.machine, NAME_SIZE);
		string_copy(model_def->os_name_vers, unameInfo.sysname, NAME_SIZE);

                strcpy(model_def->compile_flags[f_lang], " -c ");

		/*
		 * Set special information for some platforms.
		 * For some platforms, this includes appending the first
		 * digit of the release number to the OS name
		 * because different releases use different formats
		 * for executable files.
		 */
		if (strcmp(unameInfo.sysname, "IRIX") == 0)
			{
			unameInfo.release[1] = 0;
			string_cat(model_def->os_name_vers, unameInfo.release, NAME_SIZE);
			model_def->os = unameInfo.release[0] == '4' ? irix4_os :
				(unameInfo.release[0] == '5' ? irix5_os : other_os);
			}
		else if (strcmp(unameInfo.sysname, "SunOS") == 0)
			{
			unameInfo.release[1] = 0;
			string_cat(model_def->os_name_vers, unameInfo.release, NAME_SIZE);
			model_def->os = unameInfo.release[0] == '5' ? sunos5_os : other_os;
			strcat(model_def->compile_flags[f_lang], " -cg92 ");
			}
		else if (strcmp(unameInfo.sysname, "OSF1") == 0)
			{
			model_def->os = osf1_os;
			strcpy(model_def->compile_flags[f_lang],
                            "-O2 -non_shared -align dcommons");
			}
		else if (strncmp(unameInfo.machine, "CRAY", 4) == 0)
			{
			unameInfo.release[1] = 0;
			string_cat(model_def->os_name_vers, unameInfo.release, NAME_SIZE);
			model_def->os = cray_os;
/*			strcpy ( model_def->compilers[f_lang], "cf77" );      */
			strcpy(model_def->compile_flags[f_lang], "");
			}
		else if (strncmp(unameInfo.sysname, "WIN32", 5) == 0)
			{
			string_cat(model_def->os_name_vers, unameInfo.release, NAME_SIZE);
			model_def->os = win_32;
        		strcpy ( model_def->compilers[c_lang], "cl.exe" );
	   		strcpy ( model_def->compilers[c_plus_plus_lang], "cl.exe" );
/*     			strcpy ( model_def->compilers[f_lang], "df.exe" ); */
        		strcpy ( model_def->compile_flags[c_lang],
                                "/c /nologo /ML /TC /Gz /GX /O2 /D _WIN32" );
        		strcpy ( model_def->compile_flags[c_plus_plus_lang],
                                "/c /nologo /ML /TP /Gz /GX /O2 /D _WIN32" );
        		strcpy ( model_def->compile_flags[f_lang],
                                "/compile_only /nologo /iface:stdref" );
			}
		else model_def->os = other_os;

		}	/* end of setting architecture and machine info */
	else
		/*
		 * can't get system information so use default values
		 */
		{
		model_def->os_name_vers[0] = '\0';
		model_def->architecture[0] = '\0';
		model_def->os = other_os;
		}

	}

void print_model_def ( model_def_type * model_def )
	{
	printf ( "\nModel def:\n\n" );

	printf ( "%s\n", model_def->description );
	printf ( "architecture is %s, OS is %s, os_type is %d\n",
	         model_def->architecture, model_def->os_name_vers, (int) model_def->os );
	printf ( "executable name is %s\n", model_def->exename );
	printf ( "global flags: %s%s%s%s%s\n",
	         model_def->verbose ? " verbose" : "",
	         model_def->compile_all ? " compile_all" : "",
	         model_def->one_step ? " one_step" : "",
	         model_def->clean_up ? " clean_up" : "",
	         model_def->fast_check ? " fast_check" : "" );

	printf ( "run mode is " );
	
	if ( model_def->mode == parse_mode )
		{
		printf ( "parse_mode\n" );
		}
	if ( model_def->mode == no_compile_mode )
		{
		printf ( "no_compile_mode\n" );
		}
	if ( model_def->mode == no_link_mode )
		{
		printf ( "no_link_mode\n" );
		}
	if ( model_def->mode == build_mode )
		{
		printf ( "build_mode\n" );
		}

	printf ( "Archive location is " );
	
	switch ( model_def->archive_location )
		{
		case sams_default_archive :
		
			printf ( "SAMS default\n" );
			break;
			
		case local_archive :
		
			printf ( "the local host\n" );
			break;
			
		case remote_archive :
		
			printf ( "a remote host\n" );
			break;
			
		default :
		
			printf ( "bogus!  value out of range\n" );
			break;
			
		}
		
	printf ( "default directory is %s\n", model_def->wd );
	printf ( "c flags: %s\n", model_def->compile_flags[c_lang] );
	printf ( "c++ flags: %s\n", model_def->compile_flags[c_plus_plus_lang] );
	printf ( "FC flags: %s\n", model_def->compile_flags[f_lang] );
	printf ( "cpp flags: %s\n", model_def->cpp_flags );
	printf ( "libraries: %s\n", model_def->libraries );
	printf ( "link flags: %s\n", model_def->link_flags );

	print_include_list ( & ( model_def->includes ) );
	print_grid_list ( & ( model_def->grids ) );
	print_module_list ( & ( model_def->modules ) );
	print_file_list ( & ( model_def->files ) );
	}
	
int string_copy ( char * dest, const char * src, int array_size )
	{
	if ( dest == (char *) NULL )
		{
		fprintf ( stderr, 
		          "string_copy: null destination string pointer encountered\n" );
		return ( FAILURE );
		}
		
	if ( src == (char *) NULL )
		{
		strcpy ( dest, "" );
		fprintf ( stderr, 
		          "string_copy: null source string pointer encountered\n" );
		return ( SUCCESS );
		}
		
	if ( ( strlen ( dest ) + strlen ( src ) ) < (size_t) array_size )
		{
		strcpy ( dest, src );
		return ( SUCCESS );
		}
	else
		{
		strncpy ( dest, src, array_size - strlen ( dest ) - 1 );
		dest[array_size-1] = (char) '\0';
		fprintf ( stderr, "string_copy: error string exceeds %d chars: %s\n",
		          array_size - 1, dest );
		return ( FAILURE );
		}
	}

int string_cat ( char * dest, const char * src, int array_size )
	{
	if ( dest == (char *) NULL )
		{
		fprintf ( stderr, 
		          "string_cat: null destination string pointer encountered\n" );
		return ( FAILURE );
		}
		
	if ( src == (char *) NULL )
		{
		fprintf ( stderr, 
		          "string_cat: null source string pointer encountered\n" );
		return ( SUCCESS );
		}
		
	if ( strlen ( src ) <= (size_t) array_size )
		{
		strcat ( dest, src );
		return SUCCESS;
		}
	else
		{
		strncat ( dest, src, array_size - 1 );
		dest[array_size-1] = (char) '\0';
		fprintf ( stderr, "string_cat: error string exceeds %d chars: %s\n",
		          array_size - 1, dest );
		return FAILURE;
		}
	}

int resolve_name ( const char * input, char * output, int array_size )
	{
	char *search_begin, *name_begin;
	size_t name_length;
	char temp_char;
	char * value_string;

#	ifdef ENV_DEBUG
	fprintf ( stderr, "resolve_name: input name is %s\n", input );
#	endif

	strcpy ( output, "" );

	for ( search_begin = (char *) input; 
	      ( name_begin = strchr ( search_begin, '$' ) ) != (char *) NULL; 
	      search_begin = name_begin + name_length + 1 )
		{
		*name_begin = (char)'\0';
		string_cat ( output, search_begin, array_size );
		*name_begin = (char) '$';

		name_length = strcspn ( name_begin + 1, "/.$" );

		if ( name_length == 0 )
			{
			fprintf 
				( 
				stderr, 
				"resolve_name: expecting identifier after $ in string %s\n", 
				input 
				);

			string_cat ( output, "$", array_size );
			return ( 1 );
			}

		temp_char = name_begin[name_length + 1];
		name_begin[name_length + 1] = (char)'\0';

#		ifdef ENV_DEBUG
		fprintf ( stderr, "resolve_name: input name is %s\n", input );
		fprintf ( stderr, "resolve_name: calling getenv ( %s )\n", name_begin + 1 );
#		endif

		value_string = getenv ( name_begin + 1 );
		name_begin[name_length + 1] = temp_char;

		if ( value_string == (char *) NULL )
			{
			fprintf ( stderr, "resolve_name: can't find environment variable"
			          " named %s\n", name_begin + 1 );
			string_cat ( output, name_begin, array_size );
			}
		else
			{
			string_cat ( output, value_string, array_size );

#			ifdef ENV_DEBUG
			fprintf ( stderr, "resolve_name: back from getenv()\n" );
			fprintf ( stderr, "resolve_name: value_string is %s\n", value_string );
#			endif
			}

		}

	string_cat ( output, search_begin, array_size );
	}