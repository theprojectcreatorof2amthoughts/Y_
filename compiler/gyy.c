#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <process.h>
#else
#include <sys/wait.h>
#endif
#define PATH_SEPARATOR '/'
static void usage(FILE *out) {
 fputs("gyy - Y_ compiler driver (LLVM backend bootstrap)\n"
 "Usage: gyy <input.ll> [-o output] [-O0|-O1|-O2|-O3] [--clang path]\n\n"
 "Compiles textual LLVM IR with Clang. Y_ source (.yc) compilation is not yet available.\n",out);
}
static char *dupstr(const char *s){size_t n=strlen(s)+1;char *p=malloc(n);if(p)memcpy(p,s,n);return p;}
static char *quote(const char *s){
 size_t n=3;for(const char*p=s;*p;++p)n+=(*p=='\'')?4:1;
 char*q=malloc(n);if(!q)return NULL;char*d=q;*d++='\'';
 for(const char*p=s;*p;++p){if(*p=='\''){memcpy(d,"'\\''",4);d+=4;}else *d++=*p;}
 *d++='\'';*d='\0';return q;
}
static char *default_output(const char *s){
 const char *base=strrchr(s,PATH_SEPARATOR);base=base?base+1:s;
 char *p=dupstr(base);if(!p)return NULL;char*dot=strrchr(p,'.');if(dot&&dot!=p)*dot='\0';return p;
}
int main(int argc,char**argv){
 const char *input=NULL,*output_arg=NULL,*clang=getenv("LLVM_CLANG"),*opt="-O0";
 char *owned=NULL,*qc=NULL,*qi=NULL,*qo=NULL,*cmd=NULL;int i,status;
 if(!clang||!*clang)clang="clang";
 for(i=1;i<argc;++i){
  if(!strcmp(argv[i],"--help")||!strcmp(argv[i],"-h")){usage(stdout);return 0;}
  if(!strcmp(argv[i],"--version")){puts("gyy 0.1.0 (LLVM IR driver bootstrap)");return 0;}
  if(!strcmp(argv[i],"-o")){if(++i>=argc){fputs("gyy: missing filename after -o\n",stderr);return 2;}output_arg=argv[i];}
  else if(!strncmp(argv[i],"-O",2)){if(strcmp(argv[i],"-O0")&&strcmp(argv[i],"-O1")&&strcmp(argv[i],"-O2")&&strcmp(argv[i],"-O3")){fprintf(stderr,"gyy: unsupported optimization level: %s\n",argv[i]);return 2;}opt=argv[i];}
  else if(!strcmp(argv[i],"--clang")){if(++i>=argc){fputs("gyy: missing path after --clang\n",stderr);return 2;}clang=argv[i];}
  else if(argv[i][0]=='-'){fprintf(stderr,"gyy: unknown option: %s\n",argv[i]);return 2;}
  else if(!input)input=argv[i];else{fprintf(stderr,"gyy: unexpected argument: %s\n",argv[i]);return 2;}
 }
 if(!input){usage(stderr);return 2;}
 size_t len=strlen(input);
 if(len<3||strcmp(input+len-3,".ll")){fprintf(stderr,"gyy: '%s' is not LLVM IR (.ll); Y_ source frontend is not available in this bootstrap build.\n",input);return 2;}
 const char *output=output_arg;
 if(!output){owned=default_output(input);if(!owned){fputs("gyy: out of memory\n",stderr);return 2;}output=owned;}
 qc=quote(clang);qi=quote(input);qo=quote(output);
 if(!qc||!qi||!qo){fputs("gyy: out of memory\n",stderr);status=2;goto cleanup;}
 size_t n=strlen(qc)+strlen(qi)+strlen(qo)+strlen(opt)+32;
 cmd=malloc(n);if(!cmd){fputs("gyy: out of memory\n",stderr);status=2;goto cleanup;}
 snprintf(cmd,n,"%s %s %s -o %s",qc,opt,qi,qo);
 status=system(cmd);
 if(status==-1){fprintf(stderr,"gyy: could not launch Clang: %s\n",strerror(errno));status=127;}
#ifndef _WIN32
 else if(WIFEXITED(status))status=WEXITSTATUS(status);
 else if(WIFSIGNALED(status))status=128+WTERMSIG(status);
 else status=1;
#endif
cleanup: free(cmd);free(qc);free(qi);free(qo);free(owned);return status;
}
