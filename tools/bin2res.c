/************************************************
 *
 * Converting binary resources from/to *.rc files
 *
 * Copyright 1999 Juergen Schmied
 *
 * 11/99 first release
 */

#include "config.h"

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"

extern char*   g_lpstrFileName;

/* global options */

char*	g_lpstrFileName = NULL;
char*   g_lpstrInputFile = NULL;
int	b_to_binary = 0;
int	b_force_overwrite = 0;
LPBYTE p_in_file = NULL;

static char*    errorOpenFile = "Unable to open file.\n";
static char*    errorRCFormat = "Unexpexted syntax in rc file line %i\n";

void usage(void)
{
    printf("Usage: bin2res [-d bin] [input file]\n");
    printf("  -d bin convert a *.res back to a binary\n");
    printf("  -f force overwriting newer files\n");
    exit(-1);
}

void parse_options(int argc, char **argv)
{
  int i;

  switch( argc )
  {
    case 2:
	 g_lpstrInputFile = argv[1];
	 break;

    case 3:
    case 4:
    case 5:
	 for( i = 1; i < argc - 1; i++ )
	 {
	   if( argv[i][0] != '-' ||
	       strlen(argv[i]) != 2 ) break;

	   if( argv[i][1] == 'd')
	   {
	     if (strcmp ("bin", argv[i+1])==0)
	     {
	       b_to_binary =1;
	       i++;
	     }
	     else
	     {
	       usage();
	     }
	     
	   }
	   else if ( argv[i][1] == 'f')
	   {
	     b_force_overwrite = 1;
	   }
	   else
	   {
	     usage();
	   }
	 } 
	 if( i == argc - 1 )
	 {
	   g_lpstrInputFile = argv[i];
	   break;
	 }
    default: usage();
  }
}

int insert_hex (char * infile, FILE * outfile)
{
	int i;
	int 		fd;
	struct stat	st;

	if( (fd = open( infile, O_RDONLY))==-1 ) 
	{
	  fprintf(stderr, errorOpenFile );
	  exit(1);
	}
        if ((fstat(fd, &st) == -1) || (p_in_file = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1)
        {
	  fprintf(stderr, errorOpenFile );
          close(fd);
	  exit(1);
        }

	fprintf (outfile, "{\n '");
	i = 0;
	while (1)
	{
	  fprintf(outfile, "%02X", p_in_file[i]);
	  if (++i >= st.st_size) break;
	  fprintf(outfile, "%s", (i == (i & 0xfffffff0)) ? "'\n '" :" ");
	}
	fprintf (outfile, "'\n}");
        munmap(p_in_file, st.st_size);
        close(fd);
	return 1;	
}

int convert_to_res ()
{
	FILE 	*fin, *ftemp;
	char	buffer[255];
	char	infile[255];
	char	tmpfile[L_tmpnam];
	char	*pos;
	int	c, len;
	struct stat	st;
	int line = 0;
	time_t	tinput;
	long startpos, endpos;

        strcpy( tmpfile, g_lpstrInputFile );
        strcat( tmpfile, "-tmp" );
        /* FIXME: should use better tmp name and create with O_EXCL */
	if( (ftemp = fopen( tmpfile, "w")) == NULL ) goto error_open_file;
	
	if( (fin = fopen( g_lpstrInputFile, "r")) == NULL || stat(g_lpstrInputFile, &st)) goto error_open_file;
	tinput = st.st_ctime;
	
	while ( NULL != fgets(buffer, 255, fin))
	{
	  fputs(buffer, ftemp);
	  line++;
	  if ( (pos = strstr(buffer, "BINRES")) != NULL)
	  {
	    /* get the out-file name */
	    len = 0; pos += 6; startpos=0; endpos=0;
	    while ( *pos == ' ') pos++;
	    while ( pos[len] != ' ') len++;
	    strncpy(infile, pos, len);
	    infile[len]=0;
	    
	    if ( (!stat(infile, &st) && st.st_ctime > tinput) || b_force_overwrite)
	    {
	      /* write a output file */
	      printf("[%s:c]", infile);
	      while((c = fgetc(fin))!='{' && c != EOF) fputc(c, ftemp);
	      if (c == EOF ) goto error_rc_format;
	      while((c = fgetc(fin))!='}' && c != EOF);
	      if (c == EOF ) goto error_rc_format;

	      insert_hex(infile, ftemp);
	    }
	    else
	    {
	      printf("[%s:s]", infile);
	    }
	  }
	}
	
        fclose(fin);
	fclose(ftemp);
	if (rename(tmpfile, g_lpstrInputFile) == -1)
        {
            perror("rename");
            unlink(tmpfile);
            return 0;
        }
	return 1;	

error_open_file:
	fprintf(stderr, errorOpenFile );
	return 0;

error_rc_format:	
	fprintf(stderr, errorRCFormat, line);
	return 0;
}

int convert_to_bin()
{
	FILE 	*fin, *fout;
	char	buffer[255];
	char	outfile[255];
	char	*pos;
	int	len, index, in_resource;
	unsigned int	byte;
	struct stat	st;
	int line = 0;
	time_t	tinput;
		
	if( (fin = fopen( g_lpstrInputFile, "r")) == NULL || stat(g_lpstrInputFile, &st)) goto error_open_file;
	tinput = st.st_ctime;
	
	while ( NULL != fgets(buffer, 255, fin))
	{
	  line++;
	  if ( (pos = strstr(buffer, "BINRES")) != NULL)
	  {
	    /* get the out-file name */
	    len = 0; pos += 6;
	    while ( *pos == ' ') pos++;
	    while ( pos[len] != ' ') len++;
	    strncpy(outfile, pos, len);
	    outfile[len]=0;
	    
	    if ( stat(outfile, &st) || st.st_ctime < tinput || b_force_overwrite)
	    {
	      /* write a output file */
	      printf("[%s:c]", outfile);
	      if ( (fout = fopen( outfile, "w")) == NULL) goto error_open_file;

	      in_resource = 0;
	      while (1)
	      {
	        if ( NULL == fgets(buffer, 255, fin)) goto error_rc_format;
	        line++;

	        /* parse a line */
	        for ( index = 0; buffer[index] != 0; index++ )
	        {
	          if ( ! in_resource )
		  {
		    if ( buffer[index] == '{' ) in_resource = 1;
		    continue;
		  }

	          if ( buffer[index] == ' ' || buffer[index] == '\''|| buffer[index] == '\n' ) continue;
	          if ( buffer[index] == '}' ) goto end_of_resource;
	          if ( ! isxdigit(buffer[index])) goto error_rc_format;
		  index += sscanf(&buffer[index], "%02x", &byte);
		  fputc(byte, fout);
	        }  
	      }
	      fclose(fout);
	    }
	    else
	    {
	      printf("[%s:s]", outfile);
	    }
end_of_resource: ;
	  }
	}
	
        fclose(fin);
	return 1;	

error_open_file:
	fprintf(stderr, errorOpenFile );
	return 0;

error_rc_format:	
	fprintf(stderr, errorRCFormat, line);
	return 0;
}

int main(int argc, char **argv)
{
	parse_options( argc, argv);

	if (b_to_binary == 0)
	{
	  convert_to_res();
	}
	else
	{
	  convert_to_bin();
	}
	printf("\n");
	return 0;
}
