#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include <process.h>
#endif
#ifndef GY_Y_PROJECT_ROOT
#define GY_Y_PROJECT_ROOT "."
#endif
#ifndef GY_Y_EMBEDDED_SOURCE
#define GY_Y_EMBEDDED_SOURCE "build/embedded_libraries.c"
#endif
#define MAX_INPUTS 128
#define MAX_FLAGS 64

#define DRIVER_NAME "gyy"
#define DRIVER_VERSION "gyy 0.4.0 (Y_ multi-file native compiler)"
#define DEFAULT_COMPILER_ENV "LLVM_CLANG"
#define COMPILER_OPTION "--clang"
#define DEFAULT_COMPILER "clang"

static void usage(FILE *f) {
    fputs(DRIVER_NAME " - native compiler (Clang backend)\n"
          "Usage: " DRIVER_NAME " <main.yc> [module.yc|module.hly|module.yaq ...] [options]\n"
          "       " DRIVER_NAME " <input.ll> [-o output] [-O0|-O1|-O2|-O3]\n"
          "Options: -o FILE, " COMPILER_OPTION " PATH, -O0..-O3, -Wall, -Wextra,\n"
          "         -Wpedantic, -Werror, -Wno-WARNING, -Werror=WARNING, -g\n"
          "Modules share one environment and are linked into one executable.\n", f);
}
static char *dupstr(const char *s) {
    size_t n=strlen(s)+1; char *p=malloc(n); if(p) memcpy(p,s,n); return p;
}
static char *join(const char *a,const char *b) {
    size_t n=strlen(a)+strlen(b)+2; char *p=malloc(n);
    if(p) snprintf(p,n,"%s/%s",a,b);
    return p;
}
static char *default_output(const char *s) {
    const char *b=s; for(const char*p=s;*p;++p)if(*p=='/'||*p=='\\')b=p+1;
    char *r=dupstr(b); if(!r)return NULL; char*d=strrchr(r,'.');if(d&&d!=r)*d=0;return r;
}
static int has_extension(const char *path,const char *ext) {
    size_t a=strlen(path),b=strlen(ext); return a>=b && strcmp(path+a-b,ext)==0;
}
static char *read_source(const char *path) {
    FILE*f=fopen(path,"rb");
    if(!f){fprintf(stderr,"%s: error: %s\n",path,strerror(errno));return NULL;}
    if(fseek(f,0,SEEK_END)){fprintf(stderr,"%s: error: cannot seek source file\n",path);fclose(f);return NULL;}
    long n=ftell(f);
    if(n<0||fseek(f,0,SEEK_SET)){fprintf(stderr,"%s: error: cannot read source size\n",path);fclose(f);return NULL;}
    char*s=malloc((size_t)n+1);if(!s){fclose(f);fprintf(stderr,"gyy: error: out of memory\n");return NULL;}
    size_t got=fread(s,1,(size_t)n,f);
    if(got!=(size_t)n&&ferror(f)){free(s);fclose(f);fprintf(stderr,"%s: error: failed while reading source\n",path);return NULL;}
    s[got]=0;fclose(f);return s;
}
static int write_c_string(FILE*f,const char*s) {
    if(fputc('"',f)==EOF)return 0;
    for(const unsigned char*p=(const unsigned char*)s;*p;++p)
        if(fprintf(f,"\\%03o",(unsigned)*p)<0)return 0;
    return fputc('"',f)!=EOF;
}
static int wrapper_file(const char*path,char **sources,char **inputs,size_t count,size_t entry) {
    FILE*f=fopen(path,"wb");
    if(!f){fprintf(stderr,"gyy: error: cannot create %s: %s\n",path,strerror(errno));return 0;}
    int ok=fputs("#include \"y_.h\"\n#include <stddef.h>\n",f)>=0;
    for(size_t i=0;i<count&&ok;++i)
        ok=fprintf(f,"static const char y_source_%lu[] = ",(unsigned long)i)>=0
            &&write_c_string(f,sources[i])&&fputs(";\n",f)>=0;
    if(ok) ok=fputs("static const char *const y_sources[] = {",f)>=0;
    for(size_t i=0;i<count&&ok;++i) ok=fprintf(f,"%sy_source_%lu",i?",":"",(unsigned long)i)>=0;
    if(ok) ok=fputs("};\nstatic const char *const y_paths[] = {",f)>=0;
    for(size_t i=0;i<count&&ok;++i) ok=write_c_string(f,inputs[i])&&fputc(',',f)!=EOF;
    if(ok) ok=fprintf(f,"};\nint main(void) { return y_program_run_sources(y_sources, y_paths, %lu, %lu); }\n",
                      (unsigned long)count,(unsigned long)entry)>=0;
    if(fclose(f))ok=0;
    return ok;
}
static int run(char *const argv[]) {
#ifndef _WIN32
    pid_t pid=fork();
    if(pid<0){fprintf(stderr,"gyy: error: cannot start Clang: %s\n",strerror(errno));return 127;}
    if(pid==0){execvp(argv[0],argv);fprintf(stderr,"gyy: error: cannot execute %s: %s\n",argv[0],strerror(errno));_exit(127);}
    int status=0;
    if(waitpid(pid,&status,0)<0){fprintf(stderr,"gyy: error: waiting for Clang failed: %s\n",strerror(errno));return 127;}
    if(WIFEXITED(status))return WEXITSTATUS(status);
    if(WIFSIGNALED(status))return 128+WTERMSIG(status);
    return 1;
#else
    intptr_t status=_spawnvp(_P_WAIT,argv[0],(const char * const *)argv);
    if(status==-1){fprintf(stderr,"gyy: error: cannot execute %s: %s\n",argv[0],strerror(errno));return 127;}
    return (int)status;
#endif
}
static int compile(const char*clang,const char*out,const char*opt,char **inputs,
                   size_t count,int is_ir,const char **warnings,size_t warning_count,int debug) {
    char *sources[MAX_INPUTS]={0};
    size_t entry=0;
    if(!is_ir) {
        size_t main_count=0;
        for(size_t i=0;i<count;++i) if(has_extension(inputs[i],".yc")){if(main_count==0)entry=i;++main_count;}
        if(main_count==0){fputs("gyy: error: expected at least one .yc entry file\n",stderr);return 2;}
        for(size_t i=0;i<count;++i){sources[i]=read_source(inputs[i]);if(!sources[i])goto fail;}
    }
    char *wrapper=NULL;
    if(!is_ir) {
        size_t n=strlen(out)+16;wrapper=malloc(n);
        if(!wrapper){fprintf(stderr,"gyy: error: out of memory\n");goto fail;}
        snprintf(wrapper,n,"%s.gyy.c",out);
        if(!wrapper_file(wrapper,sources,inputs,count,entry))goto fail;
    }
    char *inc=join(GY_Y_PROJECT_ROOT,"src");
    char *rt=join(GY_Y_PROJECT_ROOT,"src/y_.c");
    char *mem=join(GY_Y_PROJECT_ROOT,"src/memory.c");
    char *io=join(GY_Y_PROJECT_ROOT,"src/file_io.c");
    if(!inc||!rt||!mem||!io){fprintf(stderr,"gyy: error: out of memory\n");free(inc);free(rt);free(mem);free(io);goto fail;}
    char *args[256];size_t a=0;
    args[a++]=(char*)clang;args[a++]="-fdiagnostics-show-option";
    args[a++]="-std=c11";args[a++]="-Wall";args[a++]="-Wextra";args[a++]="-Wpedantic";
    args[a++]=(char*)opt;if(debug)args[a++]="-g";
    for(size_t i=0;i<warning_count;++i)args[a++]=(char*)warnings[i];
    if(!is_ir) {
#ifdef GYXX_DRIVER
        args[a++]="-x";args[a++]="c";
#endif
        args[a++]="-I";args[a++]=inc;
        args[a++]=wrapper;args[a++]=rt;args[a++]=mem;args[a++]=io;
        args[a++]=(char*)GY_Y_EMBEDDED_SOURCE;args[a++]="-lm";
    } else {
#ifdef GYXX_DRIVER
        args[a++]="-x";args[a++]="ir";
#endif
        args[a++]=inputs[0];
    }
    args[a++]="-o";args[a++]=(char*)out;args[a]=NULL;
    int result=run(args);
    if(wrapper)remove(wrapper);
    free(inc);free(rt);free(mem);free(io);
    for(size_t i=0;i<count;++i)free(sources[i]);
    free(wrapper);
    return result;
fail:
    if(wrapper){remove(wrapper);free(wrapper);}
    for(size_t i=0;i<count;++i)free(sources[i]);
    return 2;
}
int main(int argc,char**argv) {
    const char*clang=getenv(DEFAULT_COMPILER_ENV),*opt="-O0",*outarg=NULL;
    char *inputs[MAX_INPUTS];size_t count=0;
    const char *warnings[MAX_FLAGS];size_t warning_count=0;int debug=0;
    if(!clang||!*clang)clang=DEFAULT_COMPILER;
    for(int i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--help")||!strcmp(argv[i],"-h")){usage(stdout);return 0;}
        if(!strcmp(argv[i],"--version")){puts(DRIVER_VERSION);return 0;}
        if(!strcmp(argv[i],"-o")){if(++i>=argc){fprintf(stderr,"%s: error: missing filename after -o\n",DRIVER_NAME);return 2;}outarg=argv[i];}
        else if(!strcmp(argv[i],COMPILER_OPTION)){if(++i>=argc){fprintf(stderr,"%s: error: missing compiler path after %s\n",DRIVER_NAME,COMPILER_OPTION);return 2;}clang=argv[i];}
        else if(!strncmp(argv[i],"-O",2)){if(strcmp(argv[i],"-O0")&&strcmp(argv[i],"-O1")&&strcmp(argv[i],"-O2")&&strcmp(argv[i],"-O3")){fprintf(stderr,"gyy: error: invalid optimization option '%s'\n",argv[i]);return 2;}opt=argv[i];}
        else if(!strcmp(argv[i],"-g"))debug=1;
        else if(!strcmp(argv[i],"-Wall")||!strcmp(argv[i],"-Wextra")||!strcmp(argv[i],"-Wpedantic")||!strcmp(argv[i],"-Werror")||!strncmp(argv[i],"-Wno-",5)||!strncmp(argv[i],"-Werror=",8)) {
            if(warning_count==MAX_FLAGS){fputs("gyy: error: too many warning options\n",stderr);return 2;}
            warnings[warning_count++]=argv[i];
        }
        else if(argv[i][0]=='-'){fprintf(stderr,"%s: error: unrecognized command-line option '%s'\n",DRIVER_NAME,argv[i]);return 2;}
        else {
            if(count==MAX_INPUTS){fputs("gyy: error: too many input files\n",stderr);return 2;}
            if(!has_extension(argv[i],".yc")&&!has_extension(argv[i],".hly")&&!has_extension(argv[i],".yaq")&&!has_extension(argv[i],".ll")) {
                fprintf(stderr,"gyy: error: unsupported input file type '%s'\n",argv[i]);return 2;
            }
            inputs[count++]=argv[i];
        }
    }
    if(count==0){usage(stderr);return 2;}
    int is_ir=has_extension(inputs[0],".ll");
    if(is_ir&&count!=1){fputs("gyy: error: LLVM IR input cannot be mixed with Y_ source modules\n",stderr);return 2;}
    if(!is_ir) for(size_t i=0;i<count;++i) if(has_extension(inputs[i],".ll")) {
        fprintf(stderr,"gyy: error: cannot mix LLVM IR and Y_ source input '%s'\n",inputs[i]);return 2;
    }
    char *owned=NULL;
    if(!outarg){owned=default_output(inputs[0]);outarg=owned;}
    if(!outarg){fputs("gyy: error: unable to determine output filename\n",stderr);return 2;}
    int result=compile(clang,outarg,opt,inputs,count,is_ir,warnings,warning_count,debug);
    free(owned);return result;
}
