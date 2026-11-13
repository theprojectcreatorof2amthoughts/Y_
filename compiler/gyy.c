#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#ifndef GY_Y_PROJECT_ROOT
#define GY_Y_PROJECT_ROOT "."
#endif
#ifndef GY_Y_EMBEDDED_SOURCE
#define GY_Y_EMBEDDED_SOURCE "build/embedded_libraries.c"
#endif

static void usage(FILE *f) {
    fputs("gyy - Y_ native compiler (Clang backend)\n"
          "Usage: gyy <input.yc|input.ll> [-o output] [-O0|-O1|-O2|-O3] [--clang path]\n"
          "Compiles .yc programs into native executables; .ll LLVM IR is also supported.\n", f);
}
static char *dupstr(const char *s) {
    size_t n=strlen(s)+1; char *p=malloc(n); if(p) memcpy(p,s,n); return p;
}
static char *join(const char *a,const char *b) {
    size_t n=strlen(a)+strlen(b)+2; char *p=malloc(n);
    if(p) snprintf(p,n,"%s/%s",a,b); return p;
}
static char *quote(const char *s) {
    size_t n=3; for(const char*p=s;*p;++p)n+=(*p=='\'')?4:1;
    char*q=malloc(n); if(!q)return NULL; char*d=q; *d++='\'';
    for(const char*p=s;*p;++p){if(*p=='\''){memcpy(d,"'\\''",4);d+=4;}else *d++=*p;}
    *d++='\'';*d=0;return q;
}
static char *default_output(const char *s) {
    const char *b=s; for(const char*p=s;*p;++p)if(*p=='/'||*p=='\\')b=p+1;
    char *r=dupstr(b); if(!r)return NULL; char*d=strrchr(r,'.');if(d&&d!=r)*d=0;return r;
}
static char *read_source(const char *path) {
    FILE*f=fopen(path,"rb"); if(!f){fprintf(stderr,"gyy: %s: %s\n",path,strerror(errno));return NULL;}
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;} long n=ftell(f);
    if(n<0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    char*s=malloc((size_t)n+1);if(!s){fclose(f);return NULL;}
    size_t got=fread(s,1,(size_t)n,f);if(got!=(size_t)n&&ferror(f)){free(s);fclose(f);return NULL;}
    s[got]=0;fclose(f);return s;
}
static int write_c_string(FILE*f,const char*s) {
    if(fputc('"',f)==EOF)return 0;
    for(const unsigned char*p=(const unsigned char*)s;*p;++p)
        if(fprintf(f,"\\%03o",(unsigned)*p)<0)return 0;
    return fputc('"',f)!=EOF;
}
static int wrapper_file(const char*path,const char*source,const char*input) {
    FILE*f=fopen(path,"wb");if(!f){fprintf(stderr,"gyy: cannot create %s: %s\n",path,strerror(errno));return 0;}
    int ok=fputs("#include \"y_.h\"\nstatic const char y_source[] = ",f)>=0
        &&write_c_string(f,source)&&fputs(";\nstatic const char y_path[] = ",f)>=0
        &&write_c_string(f,input)
        &&fputs(";\nint main(void){return y_program_run_source(y_source,y_path);}\n",f)>=0;
    if(fclose(f))ok=0;return ok;
}
static int run(const char*cmd) {
    int s=system(cmd);if(s==-1){fprintf(stderr,"gyy: compiler launch failed: %s\n",strerror(errno));return 127;}
#ifndef _WIN32
    if(WIFEXITED(s))return WEXITSTATUS(s);if(WIFSIGNALED(s))return 128+WTERMSIG(s);return 1;
#else
    return s;
#endif
}
static int compile_ll(const char*clang,const char*input,const char*out,const char*opt) {
    char*a=quote(clang),*b=quote(input),*c=quote(out);if(!a||!b||!c){free(a);free(b);free(c);return 2;}
    size_t n=strlen(a)+strlen(b)+strlen(c)+strlen(opt)+32;char*cmd=malloc(n);
    if(!cmd){free(a);free(b);free(c);return 2;}snprintf(cmd,n,"%s %s %s -o %s",a,opt,b,c);
    int r=run(cmd);free(a);free(b);free(c);free(cmd);return r;
}
static int compile_yc(const char*clang,const char*input,const char*out,const char*opt) {
    char*src=read_source(input);if(!src)return 1;
    size_t wn=strlen(out)+16;char*w=malloc(wn);if(!w){free(src);return 2;}
    snprintf(w,wn,"%s.gyy.c",out);if(!wrapper_file(w,src,input)){free(src);free(w);return 1;}
    char*inc=join(GY_Y_PROJECT_ROOT,"src"),*rt=join(GY_Y_PROJECT_ROOT,"src/y_.c");
    char*mem=join(GY_Y_PROJECT_ROOT,"src/memory.c"),*io=join(GY_Y_PROJECT_ROOT,"src/file_io.c");
    char*qc=quote(clang),*qw=quote(w),*qi=quote(inc?inc:""),*qr=quote(rt?rt:"");
    char*qm=quote(mem?mem:""),*qio=quote(io?io:""),*qe=quote(GY_Y_EMBEDDED_SOURCE),*qo=quote(out);
    if(!inc||!rt||!mem||!io||!qc||!qw||!qi||!qr||!qm||!qio||!qe||!qo){
        fprintf(stderr,"gyy: out of memory\n");remove(w);free(src);free(w);free(inc);free(rt);free(mem);free(io);
        free(qc);free(qw);free(qi);free(qr);free(qm);free(qio);free(qe);free(qo);return 2;
    }
    size_t n=strlen(qc)+strlen(qw)+strlen(qi)+strlen(qr)+strlen(qm)+strlen(qio)+strlen(qe)+strlen(qo)+strlen(opt)+128;
    char*cmd=malloc(n);int result=2;
    if(cmd){snprintf(cmd,n,"%s -std=c11 -Wall -Wextra -Wpedantic %s -I%s %s %s %s %s %s -lm -o %s",
        qc,opt,qi,qw,qr,qm,qio,qe,qo);result=run(cmd);free(cmd);}
    remove(w);free(src);free(w);free(inc);free(rt);free(mem);free(io);
    free(qc);free(qw);free(qi);free(qr);free(qm);free(qio);free(qe);free(qo);return result;
}
int main(int argc,char**argv) {
    const char*input=NULL,*outarg=NULL,*clang=getenv("LLVM_CLANG"),*opt="-O0";char*owned=NULL;
    if(!clang||!*clang)clang="clang";
    for(int i=1;i<argc;++i){
        if(!strcmp(argv[i],"--help")||!strcmp(argv[i],"-h")){usage(stdout);return 0;}
        if(!strcmp(argv[i],"--version")){puts("gyy 0.2.0 (Y_ native compiler)");return 0;}
        if(!strcmp(argv[i],"-o")){if(++i>=argc){fputs("gyy: missing -o filename\n",stderr);return 2;}outarg=argv[i];}
        else if(!strcmp(argv[i],"--clang")){if(++i>=argc){fputs("gyy: missing Clang path\n",stderr);return 2;}clang=argv[i];}
        else if(!strncmp(argv[i],"-O",2)){if(strcmp(argv[i],"-O0")&&strcmp(argv[i],"-O1")&&strcmp(argv[i],"-O2")&&strcmp(argv[i],"-O3")){fprintf(stderr,"gyy: invalid optimization: %s\n",argv[i]);return 2;}opt=argv[i];}
        else if(argv[i][0]=='-'){fprintf(stderr,"gyy: unknown option: %s\n",argv[i]);return 2;}
        else if(!input)input=argv[i];else{fprintf(stderr,"gyy: unexpected argument: %s\n",argv[i]);return 2;}
    }
    if(!input){usage(stderr);return 2;}
    size_t n=strlen(input);int result;
    if(n>=3&&!strcmp(input+n-3,".yc")){owned=outarg?NULL:default_output(input);const char*out=outarg?outarg:owned;if(!out)return 2;result=compile_yc(clang,input,out,opt);}
    else if(n>=3&&!strcmp(input+n-3,".ll")){owned=outarg?NULL:default_output(input);const char*out=outarg?outarg:owned;if(!out)return 2;result=compile_ll(clang,input,out,opt);}
    else{fprintf(stderr,"gyy: expected .yc or .ll input, got '%s'\n",input);return 2;}
    free(owned);return result;
}
