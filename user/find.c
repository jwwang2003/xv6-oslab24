#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return name, ended with a null character; now we have a
  // proper C string
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
  return buf;
}


void
find(char *path, char *search_exp)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  // ----------------------------------------------------------------
  // checking path for possible errors
  // ----------------------------------------------------------------
  if(strlen(path) + 1 + DIRSIZ + 1 > 512){
    fprintf(2, "find: path too long\n");
    return;
  }
  //
  // the open syscall will open the path in read-only mode (0) and
  // return a file descriptor for the path, i.e. 'fd'
  //
  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: path %s doesn not exist\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: unknown path %s\n", path);
    close(fd);
    return;
  }

  // ----------------------------------------------------------------
  // copying path into a buffer
  // ----------------------------------------------------------------
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';


  // ----------------------------------------------------------------
  // checking the inod charactristics and doing comparisons for files
  // ----------------------------------------------------------------
  // going through directory content :
  //
  // - the loop will be over 'fd' as the file descriptor for the path
  // - the condition is over 'de' keeps being a dirent structure
  //         > dirent has "char name[DIRSIZ]" and  "ushort inum"
  //
  while(read(fd, &de, sizeof(de)) == sizeof(de)){
    if(de.inum == 0)
      continue;
    //
    // DIRSIZ (14) is the max length of the name of a directory: 14
    // characters from "de.name" are copied into "p" and a 0 is added
    // to the end of p.
    //
    // Furthermore, 'st' will keep info for 'buf' which is a string
    // copy of path (check kernel/stat.h).
    //
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if(stat(buf, &st) < 0){
      printf("find: cannot stat %s\n", buf);
      continue;
    }
    //
    // and now comparison between the search_exp string and the
    // filename extracted from buf could be done
    //
    if (st.type == T_FILE){
      if (strcmp(fmtname(buf), search_exp) == 0) {
        printf("%s\n", buf);
      }
    } else if (st.type == T_DIR){
      if(strcmp(fmtname(buf), search_exp) == 0) {
        printf("%s\n", buf);
      }
      //
      // Only look for files - search through DIRs and ignore CONSOLEs
      //
      if(strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {
        //
        // Get new metadata for directory file
        //
        int fd2 = open(buf, 0);
        //
        // Recursive search in found directory
        //
        find(buf, search_exp);
        close(fd2);
      }
    }
  }
  close(fd);
}


int
main(int argc, char *argv[])
{
  if(argc < 2 || argc > 4){
    printf("Usage: find [path] [expression]\n");
    exit(1);
  } else{
    find(argv[1], argv[2]);
    exit(0);
  }
}
