#define SDL_MAIN_HANDLED
#include <SDL.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <jpeglib.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* The classic WTK axis macros X/Y/Z must follow Windows/SDL headers. */
#include "wt.h"

struct WTvertex { WTp3 p; WTp3 normal; float uv[2]; int has_uv, has_normal; struct WTvertex *next; int index; };
struct WTpoly { WTgeometry *owner; int *indices; int count, cap; GLuint texture; float *uv; float rgb[3]; int has_color; struct WTpoly *next; };
struct WTgeometry { WTvertex *vertices, *last_vertex; WTpoly *polys, *last_poly; int vertex_count, poly_count; float radius; float rgb[3]; GLuint texture; WTmtable *mtable; int matid; int primitive; int unlit; float brightness; int stable_lighting, outline; };
struct WTnode { char name[64]; WTnode *parent; WTnode **children; int child_count, child_cap; WTnode **attachments; int attachment_count, attachment_cap; WTgeometry *geom; void *data; WTp3 p; WTq q; int enabled, deleted, switch_child; void (*task)(WTnode *); };
struct WTnodepath { WTnode *node; WTnode *root; };
struct WTmtable { float rgba[32][4]; int count; };
struct WTviewpoint { WTp3 p; WTq q; float parallax; WTsensor *sensor; };
struct WTwindow { WTviewpoint *view; float nearz, farz, bg[3]; void (*overlay2d)(WTwindow *, FLAG); void (*overlay3d)(WTwindow *, FLAG); struct WTwindow *next; };
struct WTsensor { float sensitivity, angular_rate; WTp3 p; WTq q; WTmouse_rawdata raw; int buttons; void (*update)(WTsensor *); };
struct WTsounddevice { SDL_AudioDeviceID id; SDL_AudioSpec spec; WTsound *sounds; int closed; };
struct WTsound { WTsounddevice *device; Uint8 *buffer; Uint32 length, cursor; int loops, remaining_loops, playing, done_pending, volume; void (*done)(WTsound *); WTsound *next; };
struct WTfont3d { float spacing; WTgeometry *glyphs[256]; };
struct WTui { char text[256]; };
struct WTpath { int unused; };
struct WTmotionlink { int unused; };

static SDL_Window *sdl_window;
static SDL_GLContext gl_context;
static WTnode *universe_root;
static WTwindow universe_window;
static WTviewpoint universe_view;
static WTsensor *mouse_sensor;
static void (*universe_actions)(void);
static int running, pending_key;
static int render_flags = WTRENDER_SHADED | WTRENDER_GOURAUD | WTRENDER_TEXTURED | WTRENDER_PERSPECTIVE;
static float current_fps = 60.0f;
static uint64_t loop_previous;
static int loop_frames, loop_max_frames, loop_key_index;
static const char *loop_start_keys;

typedef struct TextureEntry { char name[128]; GLuint id; int width, height; unsigned char *pixels; struct TextureEntry *next; } TextureEntry;
static TextureEntry *textures;

static void *xcalloc(size_t n, size_t s) { void *p = calloc(n, s); if (!p) { perror("calloc"); exit(1); } return p; }
static int contains_case_insensitive(const char *text,const char *needle){size_t n=strlen(needle);if(!n)return 1;for(;*text;text++)if(!strncasecmp(text,needle,n))return 1;return 0;}
static void quat_copy(const WTq a, WTq b) { memcpy(b, a, sizeof(WTq)); }

void WTmessage(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stdout, fmt, ap); va_end(ap); fflush(stdout); }
void WTwarning(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); }
void WTerror(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); }

void WTp3_init(WTp3 p) { p[0]=p[1]=p[2]=0; }
void WTp3_copy(const WTp3 a, WTp3 b) { memcpy(b,a,sizeof(WTp3)); }
void WTp3_add(const WTp3 a,const WTp3 b,WTp3 o){o[0]=a[0]+b[0];o[1]=a[1]+b[1];o[2]=a[2]+b[2];}
void WTp3_subtract(const WTp3 a,const WTp3 b,WTp3 o){o[0]=a[0]-b[0];o[1]=a[1]-b[1];o[2]=a[2]-b[2];}
void WTp3_mults(WTp3 p,float s){p[0]*=s;p[1]*=s;p[2]*=s;}
float WTp3_mag(const WTp3 p){return sqrtf(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);}
float WTp3_distance(const WTp3 a,const WTp3 b){WTp3 d;WTp3_subtract(a,b,d);return WTp3_mag(d);}
void WTp3_norm(WTp3 p){float m=WTp3_mag(p);if(m>1e-7f)WTp3_mults(p,1.0f/m);}
void WTp3_print(const WTp3 p,const char *label){WTmessage("%s: %.3f %.3f %.3f\n",label?label:"p3",p[0],p[1],p[2]);}
void WTq_init(WTq q){q[0]=q[1]=q[2]=0;q[3]=1;}
void WTq_mult(const WTq a,const WTq b,WTq o){WTq t={a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};quat_copy(t,o);}
void WTeuler_2q(float x,float y,float z,WTq o){float cx=cosf(x*.5f),sx=sinf(x*.5f),cy=cosf(y*.5f),sy=sinf(y*.5f),cz=cosf(z*.5f),sz=sinf(z*.5f);o[0]=sx*cy*cz-cx*sy*sz;o[1]=cx*sy*cz+sx*cy*sz;o[2]=cx*cy*sz-sx*sy*cz;o[3]=cx*cy*cz+sx*sy*sz;}
void WTq_2dir(const WTq q,WTp3 o){o[0]=2*(q[0]*q[2]+q[3]*q[1]);o[1]=2*(q[1]*q[2]-q[3]*q[0]);o[2]=1-2*(q[0]*q[0]+q[1]*q[1]);}
void WTdir_2q(const WTp3 d,WTq o){float yaw=atan2f(d[0],d[2]),pitch=-asinf(fmaxf(-1,fminf(1,d[1])));WTeuler_2q(pitch,yaw,0,o);}
void WTeuler_2m3(float x,float y,float z,WTm3 m){WTq q;WTeuler_2q(x,y,z,q);float xx=q[0]*q[0],yy=q[1]*q[1],zz=q[2]*q[2],xy=q[0]*q[1],xz=q[0]*q[2],yz=q[1]*q[2],wx=q[3]*q[0],wy=q[3]*q[1],wz=q[3]*q[2];m[0][0]=1-2*(yy+zz);m[0][1]=2*(xy-wz);m[0][2]=2*(xz+wy);m[1][0]=2*(xy+wz);m[1][1]=1-2*(xx+zz);m[1][2]=2*(yz-wx);m[2][0]=2*(xz-wy);m[2][1]=2*(yz+wx);m[2][2]=1-2*(xx+yy);}

static WTnode *node_new(WTnode *parent, WTgeometry *g) {
    WTnode *n=xcalloc(1,sizeof(*n)); n->geom=g;n->enabled=1;n->switch_child=-1;WTq_init(n->q);
    if(parent){if(parent->child_count==parent->child_cap){parent->child_cap=parent->child_cap?parent->child_cap*2:8;parent->children=realloc(parent->children,parent->child_cap*sizeof(*parent->children));}parent->children[parent->child_count++]=n;n->parent=parent;} return n;
}
WTnode *WTgroupnode_new(WTnode *p){return node_new(p,NULL);} WTnode *WTmovsepnode_new(WTnode *p){return node_new(p,NULL);} WTnode *WTmovswitchnode_new(WTnode *p){return node_new(p,NULL);} WTnode *WTmovgeometrynode_new(WTnode *p,WTgeometry *g){return node_new(p,g);}
static void remove_child(WTnode *p,WTnode *n){if(!p)return;for(int i=0;i<p->child_count;i++)if(p->children[i]==n){memmove(&p->children[i],&p->children[i+1],(p->child_count-i-1)*sizeof(*p->children));p->child_count--;break;}}
static void mark_deleted(WTnode *n){if(!n||n->deleted)return;n->deleted=1;for(int i=0;i<n->child_count;i++)mark_deleted(n->children[i]);for(int i=0;i<n->attachment_count;i++)mark_deleted(n->attachments[i]);}
void WTnode_delete(WTnode *n){if(!n)return;remove_child(n->parent,n);mark_deleted(n);}
void WTnode_enable(WTnode*n,FLAG e){if(n)n->enabled=e;} FLAG WTnode_isenabled(WTnode*n){return n&&!n->deleted&&n->enabled;}
int WTnode_numchildren(WTnode*n){return n&&!n->deleted?n->child_count:0;} WTnode *WTnode_getchild(WTnode*n,int i){return n&&i>=0&&i<n->child_count?n->children[i]:NULL;}
void WTnode_setdata(WTnode*n,void*d){if(n)n->data=d;} void *WTnode_getdata(WTnode*n){return n?n->data:NULL;} WTgeometry *WTnode_getgeometry(WTnode*n){return n?n->geom:NULL;}
void WTnode_settranslation(WTnode*n,const WTp3 p){if(n)WTp3_copy(p,n->p);} void WTnode_gettranslation(WTnode*n,WTp3 p){if(n)WTp3_copy(n->p,p);else WTp3_init(p);}
static void rotate_vec(const WTq q,const WTp3 v,WTp3 o){WTq vq={v[0],v[1],v[2],0},qi={-q[0],-q[1],-q[2],q[3]},t,r;WTq_mult(q,vq,t);WTq_mult(t,qi,r);o[0]=r[0];o[1]=r[1];o[2]=r[2];}
void WTnode_translate(WTnode*n,const WTp3 d,int frame){if(!n)return;WTp3 t;if(frame==WTFRAME_LOCAL)rotate_vec(n->q,d,t);else WTp3_copy(d,t);WTp3_add(n->p,t,n->p);}
void WTnode_setorientation(WTnode*n,const WTq q){if(n)quat_copy(q,n->q);} void WTnode_getorientation(WTnode*n,WTq q){if(n)quat_copy(n->q,q);else WTq_init(q);}
void WTnode_rotateq(WTnode*n,const WTq q,int frame){if(!n)return;WTq r;if(frame==WTFRAME_LOCAL)WTq_mult(n->q,q,r);else WTq_mult(q,n->q,r);quat_copy(r,n->q);}
void WTnode_rotate(WTnode*n,float x,float y,float z,int frame){WTq q;WTeuler_2q(x*(float)M_PI/180,y*(float)M_PI/180,z*(float)M_PI/180,q);WTnode_rotateq(n,q,frame);}
void WTmovnode_rotateaxis(WTnode*n,int axis,float d){float a[3]={0};a[axis]=d;WTnode_rotate(n,a[0],a[1],a[2],WTFRAME_LOCAL);}
void WTnode_setrotation(WTnode*n,const WTm3 m){if(!n)return;float tr=m[0][0]+m[1][1]+m[2][2];if(tr>0){float s=sqrtf(tr+1)*2;n->q[3]=.25f*s;n->q[0]=(m[2][1]-m[1][2])/s;n->q[1]=(m[0][2]-m[2][0])/s;n->q[2]=(m[1][0]-m[0][1])/s;}else WTq_init(n->q);}
void WTnode_getrotation(WTnode*n,WTm3 m){WTq q;if(n)quat_copy(n->q,q);else WTq_init(q);float x=q[0],y=q[1],z=q[2],w=q[3];m[0][0]=1-2*(y*y+z*z);m[0][1]=2*(x*y-w*z);m[0][2]=2*(x*z+w*y);m[1][0]=2*(x*y+w*z);m[1][1]=1-2*(x*x+z*z);m[1][2]=2*(y*z-w*x);m[2][0]=2*(x*z-w*y);m[2][1]=2*(y*z+w*x);m[2][2]=1-2*(x*x+y*y);}
float WTnode_getradius(WTnode*n){if(!n)return 0;float r=n->geom?n->geom->radius:0;for(int i=0;i<n->child_count;i++)r=fmaxf(r,WTnode_getradius(n->children[i]));return r;}
void WTnode_getmidpoint(WTnode*n,WTp3 p){(void)n;WTp3_init(p);} int WTnode_numpolys(WTnode*n){if(!n||n->deleted)return 0;int c=n->geom?n->geom->poly_count:0;for(int i=0;i<n->child_count;i++)c+=WTnode_numpolys(n->children[i]);return c;}
static void node_print(WTnode*n,int d){if(!n||n->deleted)return;for(int i=0;i<d;i++)fputs("  ",stdout);printf("%s (%d polys)\n",n->name[0]?n->name:"node",WTnode_numpolys(n));for(int i=0;i<n->child_count;i++)node_print(n->children[i],d+1);}
void WTnode_print(WTnode*n){node_print(n,0);} void WTnode_setname(WTnode*n,const char*s){if(n&&s)snprintf(n->name,sizeof(n->name),"%s",s);}
void WTswitchnode_setwhichchild(WTnode*n,int c){if(n)n->switch_child=c;}
void WTmovnode_attach(WTnode*p,WTnode*c,int slot){(void)slot;if(!p||!c)return;remove_child(c->parent,c);if(p->attachment_count==p->attachment_cap){p->attachment_cap=p->attachment_cap?p->attachment_cap*2:4;p->attachments=realloc(p->attachments,p->attachment_cap*sizeof(*p->attachments));}p->attachments[p->attachment_count++]=c;c->parent=p;}
int WTmovnode_numattachments(WTnode*n){return n?n->attachment_count:0;} WTnode *WTmovnode_getattachment(WTnode*n,int i){return n&&i>=0&&i<n->attachment_count?n->attachments[i]:NULL;}

WTgeometry *WTgeometry_begin(void){WTgeometry*g=xcalloc(1,sizeof(*g));g->rgb[0]=g->rgb[1]=g->rgb[2]=.65f;g->brightness=1.0f;return g;}
WTvertex *WTgeometry_newvertex(WTgeometry*g,const WTp3 p){WTvertex*v=xcalloc(1,sizeof(*v));WTp3_copy(p,v->p);v->index=g->vertex_count++;if(g->last_vertex)g->last_vertex->next=v;else g->vertices=v;g->last_vertex=v;g->radius=fmaxf(g->radius,WTp3_mag(p));return v;}
WTpoly *WTgeometry_beginpoly(WTgeometry*g){WTpoly*p=xcalloc(1,sizeof(*p));p->owner=g;if(g->last_poly)g->last_poly->next=p;else g->polys=p;g->last_poly=p;g->poly_count++;return p;}
void WTpoly_addvertex(WTpoly*p,int i){if(!p)return;if(p->count==p->cap){p->cap=p->cap?p->cap*2:4;p->indices=realloc(p->indices,p->cap*sizeof(int));}p->indices[p->count++]=i;}
void WTpoly_addvertexptr(WTpoly*p,WTvertex*v){if(v)WTpoly_addvertex(p,v->index);} void WTpoly_close(WTpoly*p){(void)p;} void WTgeometry_close(WTgeometry*g){(void)g;}
WTvertex *vertex_at(WTgeometry*g,int idx){for(WTvertex*v=g?g->vertices:NULL;v;v=v->next)if(v->index==idx)return v;return NULL;}
WTvertex *WTpoly_getvertex(WTpoly*p,int i){return p&&i>=0&&i<p->count?vertex_at(p->owner,p->indices[i]):NULL;} int WTpoly_numvertices(WTpoly*p){return p?p->count:0;} WTpoly *WTpoly_next(WTpoly*p){return p?p->next:NULL;} WTvertex *WTvertex_next(WTvertex*v){return v?v->next:NULL;} void WTvertex_setnormal(WTvertex*v,const WTp3 n){if(v){WTp3_copy(n,v->normal);WTp3_norm(v->normal);v->has_normal=1;}}
void WTpoly_delete(WTpoly*p){if(!p||!p->owner)return;WTpoly**q=&p->owner->polys;while(*q&&*q!=p)q=&(*q)->next;if(*q){*q=p->next;p->owner->poly_count--;free(p->indices);free(p->uv);free(p);}}
WTpoly *WTgeometry_getpolys(WTgeometry*g){return g?g->polys:NULL;} WTvertex *WTgeometry_getvertices(WTgeometry*g){return g?g->vertices:NULL;}
void WTgeometry_getvertexposition(WTgeometry*g,WTvertex*v,WTp3 p){(void)g;if(v)WTp3_copy(v->p,p);else WTp3_init(p);} void WTgeometry_setvertexposition(WTgeometry*g,WTvertex*v,const WTp3 p){if(v)WTp3_copy(p,v->p);if(g)g->radius=fmaxf(g->radius,WTp3_mag(p));}
void WTgeometry_beginedit(WTgeometry*g){(void)g;} void WTgeometry_endedit(WTgeometry*g){(void)g;} void WTgeometry_recomputestats(WTgeometry*g,FLAG n){(void)n;if(!g)return;g->radius=0;for(WTvertex*v=g->vertices;v;v=v->next)g->radius=fmaxf(g->radius,WTp3_mag(v->p));}
void WTgeometry_scale(WTgeometry*g,float s,const WTp3 o){if(!g)return;for(WTvertex*v=g->vertices;v;v=v->next)for(int i=0;i<3;i++)v->p[i]=o[i]+(v->p[i]-o[i])*s;g->radius*=fabsf(s);}
float WTgeometry_getradius(WTgeometry*g){return g?g->radius:0;} void WTgeometry_setrgb(WTgeometry*g,int r,int b,int c){if(g){g->rgb[0]=r/255.f;g->rgb[1]=b/255.f;g->rgb[2]=c/255.f;}} void WTgeometry_setlighting(WTgeometry*g,FLAG enabled){if(g)g->unlit=!enabled;} void WTgeometry_setbrightness(WTgeometry*g,float brightness){if(g)g->brightness=brightness;} void WTgeometry_setstablelighting(WTgeometry*g,FLAG enabled){if(g)g->stable_lighting=enabled;} void WTgeometry_setoutline(WTgeometry*g,FLAG enabled){if(g)g->outline=enabled;} void WTgeometry_rotatenormals(WTgeometry*g,int axis,float degrees){if(!g)return;float a=degrees*(float)M_PI/180.0f,c=cosf(a),s=sinf(a);for(WTvertex*v=g->vertices;v;v=v->next){if(!v->has_normal)continue;float x=v->normal[0],y=v->normal[1],z=v->normal[2];if(axis==X){v->normal[1]=y*c-z*s;v->normal[2]=y*s+z*c;}else if(axis==Y){v->normal[0]=x*c+z*s;v->normal[2]=-x*s+z*c;}else{v->normal[0]=x*c-y*s;v->normal[1]=x*s+y*c;}}}
static GLuint texture_id(const char *name);
void WTgeometry_settexture(WTgeometry*g,const char*n,FLAG a,FLAG b){(void)a;(void)b;if(g)g->texture=texture_id(n);}
FLAG WTgeometry_changetexture(WTgeometry*g,const char*n,FLAG a,FLAG b){
    (void)a;
    (void)b;
    if(!g)return FALSE;
    GLuint id=texture_id(n);
    if(!id)return FALSE;
    g->texture=id;
    for(WTpoly*p=g->polys;p;p=p->next)if(p->texture)p->texture=id;
    return TRUE;
}
void WTpoly_settexture(WTpoly*p,const char*n,FLAG a,FLAG b){(void)a;(void)b;if(p)p->texture=texture_id(n);}
void WTpoly_settexture_uv(WTpoly*p,const char*n,const float*u,const float*v,int mode,FLAG filtered){(void)mode;(void)filtered;if(!p)return;p->texture=texture_id(n);free(p->uv);p->uv=xcalloc((size_t)p->count*2,sizeof(float));for(int i=0;i<p->count;i++){p->uv[i*2]=u[i];p->uv[i*2+1]=v[i];}}
void WTpoly_setcolor(WTpoly*p,int color){if(!p)return;p->rgb[0]=((color>>8)&15)/15.0f;p->rgb[1]=((color>>4)&15)/15.0f;p->rgb[2]=(color&15)/15.0f;p->has_color=1;}
void WTgeometry_setmtable(WTgeometry*g,WTmtable*m){if(g)g->mtable=m;} void WTgeometry_setmatid(WTgeometry*g,int i){if(g)g->matid=i;}
WTgeometry *WTgeometry_newblock(float x,float y,float z,FLAG solid){(void)solid;WTgeometry*g=WTgeometry_begin();float a=x/2,b=y/2,c=z/2;WTp3 p;float v[8][3]={{-a,-b,-c},{a,-b,-c},{a,b,-c},{-a,b,-c},{-a,-b,c},{a,-b,c},{a,b,c},{-a,b,c}};for(int i=0;i<8;i++){memcpy(p,v[i],sizeof p);WTgeometry_newvertex(g,p);}int f[6][4]={{0,1,2,3},{4,7,6,5},{0,4,5,1},{1,5,6,2},{2,6,7,3},{4,0,3,7}};for(int i=0;i<6;i++){WTpoly*q=WTgeometry_beginpoly(g);for(int j=0;j<4;j++)WTpoly_addvertex(q,f[i][j]);}return g;}
WTgeometry *WTgeometry_newsphere(float r,int rings,int slices,FLAG a,FLAG b){(void)a;(void)b;rings=rings<2?2:rings;slices=slices<4?4:slices;WTgeometry*g=WTgeometry_begin();g->primitive=1;for(int i=0;i<=rings;i++){float ph=(float)M_PI*i/rings;for(int j=0;j<slices;j++){float th=2*(float)M_PI*j/slices;WTp3 p={r*sinf(ph)*cosf(th),r*cosf(ph),r*sinf(ph)*sinf(th)};WTgeometry_newvertex(g,p);}}for(int i=0;i<rings;i++)for(int j=0;j<slices;j++){WTpoly*p=WTgeometry_beginpoly(g);int n=(j+1)%slices;WTpoly_addvertex(p,i*slices+j);WTpoly_addvertex(p,i*slices+n);WTpoly_addvertex(p,(i+1)*slices+n);WTpoly_addvertex(p,(i+1)*slices+j);}return g;}

static int asset_path(const char *name,char *out,size_t cap){const char*exts[]={"",".tga",".jpg",".jpeg",".bmp",NULL};DIR*d;struct dirent*e;for(int x=0;exts[x];x++){char wanted[256];snprintf(wanted,sizeof(wanted),"%s%s",name,exts[x]);d=opendir(".");if(!d)return 0;while((e=readdir(d)))if(!strcasecmp(e->d_name,wanted)){snprintf(out,cap,"%s",e->d_name);closedir(d);return 1;}closedir(d);}return 0;}
static FILE *open_asset_mode(const char *name,const char *mode){char path[256];if(!asset_path(name,path,sizeof path))return NULL;return fopen(path,mode);}
static FILE *open_asset(const char *name){return open_asset_mode(name,"r");}
static int numeric_line(const char*s){while(isspace((unsigned char)*s))s++;return *s=='-'||*s=='+'||isdigit((unsigned char)*s);}
WTnode *WTmovnode_load(WTnode *parent,const char *name,float scale){FILE*f=open_asset(name);WTnode*container=node_new(parent,NULL);WTnode_setname(container,name);if(!f){WTwarning("WTK-NG: cannot load %s\n",name);return container;}char line[2048];while(fgets(line,sizeof line,f)){if(!numeric_line(line))continue;int nv;if(sscanf(line,"%d",&nv)!=1||nv<1||nv>100000)continue;WTgeometry*g=WTgeometry_begin();for(int i=0;i<nv&&fgets(line,sizeof line,f);i++){WTp3 p;if(sscanf(line,"%f %f %f",&p[0],&p[1],&p[2])==3){WTp3_mults(p,scale);WTvertex*v=WTgeometry_newvertex(g,p);char*normal=strstr(line,"norm ");if(normal){WTp3 n;if(sscanf(normal+5,"%f %f %f",&n[0],&n[1],&n[2])==3)WTvertex_setnormal(v,n);}char*uv=strstr(line,"uv ");if(uv&&sscanf(uv+3,"%f %f",&v->uv[0],&v->uv[1])==2)v->has_uv=1;}}if(!fgets(line,sizeof line,f)){break;}int np=0;if(sscanf(line,"%d",&np)!=1||np<0||np>100000)continue;for(int i=0;i<np&&fgets(line,sizeof line,f);i++){char*cur=line;int count=(int)strtol(cur,&cur,10);WTpoly*p=WTgeometry_beginpoly(g);for(int j=0;j<count;j++)WTpoly_addvertex(p,(int)strtol(cur,&cur,10));char*tex=strstr(cur,"_V_");if(tex){char texname[128];if(sscanf(tex+3,"%127s",texname)==1)p->texture=texture_id(texname);}}WTmovgeometrynode_new(container,g);}fclose(f);if(container->child_count==0)container->geom=WTgeometry_newblock(4,4,4,TRUE);return container;}
WTnode *WTnode_load(WTnode*p,const char*n,float s){return WTmovnode_load(p,n,s);} WTnode *WTmovnode_instance(WTnode*p,WTnode*s){WTnode*n=node_new(p,s?s->geom:NULL);if(s){for(int i=0;i<s->child_count;i++){WTnode*c=WTmovnode_instance(n,s->children[i]);(void)c;}}return n;}

WTnodepath *WTnodepath_new(WTnode*n,WTnode*r,int f){(void)f;WTnodepath*p=xcalloc(1,sizeof(*p));p->node=n;p->root=r;return p;}
static void world_position(WTnode*n,WTp3 p){WTp3_init(p);WTnode*stack[128];int c=0;for(;n&&c<128;n=n->parent)stack[c++]=n;while(c--)WTp3_add(p,stack[c]->p,p);}
FLAG WTnodepath_intersectbbox(WTnodepath*a,WTnodepath*b){if(!a||!b||!a->node||!b->node||a->node->deleted||b->node->deleted)return FALSE;WTp3 pa,pb;world_position(a->node,pa);world_position(b->node,pb);return WTp3_distance(pa,pb)<=WTnode_getradius(a->node)+WTnode_getradius(b->node);}

WTmtable *WTmtable_new(int f,int c,void*u){(void)f;(void)c;(void)u;return xcalloc(1,sizeof(WTmtable));} int WTmtable_newentry(WTmtable*m){return m?m->count++:0;} void WTmtable_setvalue(WTmtable*m,int id,const float*v,int p){if(!m||id<0||id>=32||!v)return;if(p==WTMAT_OPACITY)m->rgba[id][3]=v[0];else memcpy(m->rgba[id],v,3*sizeof(float));}
static unsigned char *load_tga(FILE *f, int *w, int *h)
{
    unsigned char hdr[18], palette[256][4] = {{0}};
    if (fread(hdr, 1, 18, f) != 18) return NULL;

    int type = hdr[2];
    int bpp = hdr[16];
    int bytes = bpp / 8;
    int indexed = (type == 1 || type == 9);
    *w = hdr[12] | hdr[13] << 8;
    *h = hdr[14] | hdr[15] << 8;
    if ((!indexed && type != 2 && type != 10) || !*w || !*h) return NULL;
    if ((!indexed && bytes != 3 && bytes != 4) || (indexed && bytes != 1)) return NULL;

    fseek(f, hdr[0], SEEK_CUR);
    if (indexed) {
        int first = hdr[3] | hdr[4] << 8;
        int length = hdr[5] | hdr[6] << 8;
        int palette_bytes = hdr[7] / 8;
        if (length > 256 || (palette_bytes != 3 && palette_bytes != 4)) return NULL;
        for (int i = 0; i < length; i++) {
            unsigned char entry[4];
            if (fread(entry, 1, palette_bytes, f) != (size_t)palette_bytes) return NULL;
            int index = first + i;
            if (index < 256) {
                palette[index][0] = entry[2];
                palette[index][1] = entry[1];
                palette[index][2] = entry[0];
                palette[index][3] = palette_bytes == 4 ? entry[3] : 255;
            }
        }
    }

    size_t count = (size_t)*w * *h;
    unsigned char *raw = xcalloc(count, (size_t)bytes);
    if (type == 1 || type == 2) {
        if (fread(raw, bytes, count, f) != count) { free(raw); return NULL; }
    } else {
        size_t done = 0;
        while (done < count) {
            int packet = fgetc(f);
            if (packet == EOF) break;
            int run = (packet & 127) + 1;
            if (packet & 128) {
                unsigned char px[4];
                if (fread(px, 1, bytes, f) != (size_t)bytes) break;
                for (int i = 0; i < run && done < count; i++, done++)
                    memcpy(raw + done * bytes, px, bytes);
            } else {
                size_t n = (size_t)run;
                if (done + n > count) n = count - done;
                if (fread(raw + done * bytes, bytes, n, f) != n) break;
                done += n;
            }
        }
        if (done < count) { free(raw); return NULL; }
    }

    unsigned char *out = xcalloc(count, 4);
    int top = hdr[17] & 0x20;
    for (int y = 0; y < *h; y++) for (int x = 0; x < *w; x++) {
        int sy = top ? (*h - 1 - y) : y;
        unsigned char *s = raw + ((size_t)sy * *w + x) * bytes;
        unsigned char *d = out + ((size_t)y * *w + x) * 4;
        if (indexed) memcpy(d, palette[s[0]], 4);
        else { d[0] = s[2]; d[1] = s[1]; d[2] = s[0]; d[3] = bytes == 4 ? s[3] : 255; }
    }
    free(raw);
    return out;
}
static unsigned char *load_jpeg(FILE*f,int*w,int*h){struct jpeg_decompress_struct c;struct jpeg_error_mgr e;c.err=jpeg_std_error(&e);jpeg_create_decompress(&c);jpeg_stdio_src(&c,f);jpeg_read_header(&c,TRUE);jpeg_start_decompress(&c);*w=(int)c.output_width;*h=(int)c.output_height;int comps=(int)c.output_components;unsigned char*out=xcalloc((size_t)*w**h,4),*row=xcalloc((size_t)*w,comps);while(c.output_scanline<c.output_height){JSAMPROW rows[1]={row};jpeg_read_scanlines(&c,rows,1);int y=*h-(int)c.output_scanline;for(int x=0;x<*w;x++){unsigned char*d=out+((size_t)y**w+x)*4;d[0]=row[x*comps];d[1]=comps>1?row[x*comps+1]:d[0];d[2]=comps>2?row[x*comps+2]:d[0];d[3]=255;}}free(row);jpeg_finish_decompress(&c);jpeg_destroy_decompress(&c);return out;}
static TextureEntry *find_texture(const char*n){for(TextureEntry*t=textures;t;t=t->next)if(!strcasecmp(t->name,n))return t;return NULL;}
static TextureEntry *load_texture(const char*n){TextureEntry*t=find_texture(n);if(t)return t;char path[256];if(!asset_path(n,path,sizeof path)){WTwarning("WTK-NG: texture not found: %s\n",n);return NULL;}FILE*f=fopen(path,"rb");if(!f)return NULL;int w=0,h=0;unsigned char*p=NULL;const char*dot=strrchr(path,'.');if(dot&&!strcasecmp(dot,".tga"))p=load_tga(f,&w,&h);else if(dot&&(!strcasecmp(dot,".jpg")||!strcasecmp(dot,".jpeg")))p=load_jpeg(f,&w,&h);fclose(f);if(!p){WTwarning("WTK-NG: unsupported texture: %s\n",path);return NULL;}if(contains_case_insensitive(n,"logo3r")){for(int i=0;i<w*h;i++){int brightness=p[i*4]+p[i*4+1]+p[i*4+2];p[i*4+3]=brightness<48?0:255;}}t=xcalloc(1,sizeof(*t));snprintf(t->name,sizeof(t->name),"%s",n);t->width=w;t->height=h;t->pixels=p;glGenTextures(1,&t->id);glBindTexture(GL_TEXTURE_2D,t->id);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,p);t->next=textures;textures=t;return t;}
static GLuint texture_id(const char*n){TextureEntry*t=n?load_texture(n):NULL;return t?t->id:0;}
unsigned char *WTtexture_load(const char*n,int*w,int*h){TextureEntry*t=load_texture(n);if(!t){*w=*h=0;return NULL;}*w=t->width;*h=t->height;return t->pixels;} void WTtexture_cache(const char*n,FLAG c){(void)c;(void)load_texture(n);} void WTtexture_replace(const char*n,int f,int w,int h,unsigned char*p){(void)f;TextureEntry*t=find_texture(n);if(!t||!p)return;t->width=w;t->height=h;glBindTexture(GL_TEXTURE_2D,t->id);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,p);} void WTtexture_setfilter(const char*n,int a,int b){TextureEntry*t=load_texture(n);if(!t)return;glBindTexture(GL_TEXTURE_2D,t->id);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,a==WTFILTER_NEAREST?GL_NEAREST:GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,b==WTFILTER_NEAREST?GL_NEAREST:GL_LINEAR);} long WTtexture_getmemory(void){long total=0;for(TextureEntry*t=textures;t;t=t->next)total+=(long)t->width*t->height*4;return total;} WTnode *WTlightnode_load(WTnode*p,const char*n){(void)n;return node_new(p,NULL);}

WTviewpoint *WTviewpoint_copy(WTviewpoint*v){WTviewpoint*n=xcalloc(1,sizeof(*n));if(v)*n=*v;return n;} void WTviewpoint_setposition(WTviewpoint*v,const WTp3 p){if(v)WTp3_copy(p,v->p);} void WTviewpoint_getposition(WTviewpoint*v,WTp3 p){if(v)WTp3_copy(v->p,p);else WTp3_init(p);} void WTviewpoint_setorientation(WTviewpoint*v,const WTq q){if(v)quat_copy(q,v->q);} void WTviewpoint_setparallax(WTviewpoint*v,float p){if(v)v->parallax=p;} float WTviewpoint_getparallax(WTviewpoint*v){return v?v->parallax:0;} void WTviewpoint_moveto(WTviewpoint*v,const WTpq*p){if(v&&p){WTp3_copy(p->p,v->p);quat_copy(p->q,v->q);}} void WTviewpoint_addsensor(WTviewpoint*v,WTsensor*s){if(v)v->sensor=s;}

static void quat_matrix(const WTq q,float m[16]){float x=q[0],y=q[1],z=q[2],w=q[3];memset(m,0,16*sizeof(float));m[0]=1-2*(y*y+z*z);m[1]=2*(x*y+z*w);m[2]=2*(x*z-y*w);m[4]=2*(x*y-z*w);m[5]=1-2*(x*x+z*z);m[6]=2*(y*z+x*w);m[8]=2*(x*z+y*w);m[9]=2*(y*z-x*w);m[10]=1-2*(x*x+y*y);m[15]=1;}
static void draw_geom(WTgeometry*g){if(!g)return;float minx=0,maxx=0,miny=0,maxy=0,alpha=1;if(g->mtable&&g->matid>=0&&g->matid<32&&g->mtable->rgba[g->matid][3]>0)alpha=g->mtable->rgba[g->matid][3];if(alpha<1)glDepthMask(GL_FALSE);if(g->vertices){minx=maxx=g->vertices->p[0];miny=maxy=g->vertices->p[1];for(WTvertex*v=g->vertices;v;v=v->next){minx=fminf(minx,v->p[0]);maxx=fmaxf(maxx,v->p[0]);miny=fminf(miny,v->p[1]);maxy=fmaxf(maxy,v->p[1]);}}for(WTpoly*p=g->polys;p;p=p->next){GLuint tex=p->texture?p->texture:g->texture;if((render_flags&WTRENDER_TEXTURED)&&tex){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,tex);glColor4f(g->brightness,g->brightness,g->brightness,alpha);}else{glDisable(GL_TEXTURE_2D);const float*c=p->has_color?p->rgb:g->rgb;glColor4f(c[0]*g->brightness,c[1]*g->brightness,c[2]*g->brightness,alpha);}glBegin(GL_POLYGON);for(int i=0;i<p->count;i++){WTvertex*v=vertex_at(g,p->indices[i]);if(v){if(v->has_normal)glNormal3fv(v->normal);else glNormal3f(0,1,0);if(p->uv)glTexCoord2fv(&p->uv[i*2]);else if(v->has_uv)glTexCoord2fv(v->uv);else if(g->primitive==1){float u=.5f+atan2f(v->p[2],v->p[0])/(2*(float)M_PI),vv=.5f-asinf(v->p[1]/fmaxf(g->radius,1e-6f))/(float)M_PI;glTexCoord2f(u,vv);}else{float u=(v->p[0]-minx)/fmaxf(maxx-minx,1e-6f),vv=(v->p[1]-miny)/fmaxf(maxy-miny,1e-6f);glTexCoord2f(u,vv);}glVertex3fv(v->p);}}glEnd();}glDisable(GL_TEXTURE_2D);if(alpha<1)glDepthMask(GL_TRUE);}
static void draw_outline(WTgeometry*g){if(!g||!g->outline)return;glDisable(GL_LIGHTING);glDisable(GL_TEXTURE_2D);glColor4f(.18f,.13f,.08f,.42f);glLineWidth(1.0f);for(WTpoly*p=g->polys;p;p=p->next){glBegin(GL_LINE_LOOP);for(int i=0;i<p->count;i++){WTvertex*v=vertex_at(g,p->indices[i]);if(v)glVertex3fv(v->p);}glEnd();}}
static void draw_node(WTnode*n){if(!n||n->deleted||!n->enabled)return;glPushMatrix();glTranslatef(n->p[0],n->p[1],n->p[2]);float m[16];quat_matrix(n->q,m);glMultMatrixf(m);GLboolean was_lit=glIsEnabled(GL_LIGHTING);if(n->geom&&n->geom->unlit)glDisable(GL_LIGHTING);if(n->geom&&n->geom->stable_lighting)glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_FALSE);draw_geom(n->geom);draw_outline(n->geom);if(n->geom&&n->geom->stable_lighting)glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);if(was_lit)glEnable(GL_LIGHTING);if(n->switch_child>=0){if(n->switch_child<n->child_count)draw_node(n->children[n->switch_child]);}else for(int i=0;i<n->child_count;i++)draw_node(n->children[i]);for(int i=0;i<n->attachment_count;i++)draw_node(n->attachments[i]);glPopMatrix();}
static void perspective(float fovy,float aspect,float zn,float zf){float f=1/tanf(fovy*(float)M_PI/360),m[16]={0};m[0]=f/aspect;m[5]=f;m[10]=(zf+zn)/(zn-zf);m[11]=-1;m[14]=2*zf*zn/(zn-zf);glLoadMatrixf(m);}
static void render_frame(void){static int frame;int w,h;SDL_GL_GetDrawableSize(sdl_window,&w,&h);glViewport(0,0,w,h);glClearColor(universe_window.bg[0],universe_window.bg[1],universe_window.bg[2],1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);if(render_flags&WTRENDER_TWOSIDED)glDisable(GL_CULL_FACE);else glEnable(GL_CULL_FACE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glCullFace(GL_BACK);glFrontFace(GL_CW);glPolygonMode(GL_FRONT_AND_BACK,(render_flags&WTRENDER_WIREFRAME)?GL_LINE:GL_FILL);glMatrixMode(GL_PROJECTION);perspective(60,(float)w/(h?h:1),universe_window.nearz,universe_window.farz);glMatrixMode(GL_MODELVIEW);glLoadIdentity();if(render_flags&WTRENDER_LIGHTING){GLfloat ambient[]={.42f,.38f,.34f,1},diffuse[]={.62f,.59f,.54f,1},position[]={-.35f,.85f,.45f,0},fill[]={.14f,.13f,.12f,1},fillpos[]={.45f,-.35f,-.80f,0};glEnable(GL_LIGHTING);glEnable(GL_LIGHT0);glEnable(GL_LIGHT1);glEnable(GL_NORMALIZE);glEnable(GL_COLOR_MATERIAL);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse);glLightfv(GL_LIGHT0,GL_POSITION,position);glLightfv(GL_LIGHT1,GL_DIFFUSE,fill);glLightfv(GL_LIGHT1,GL_POSITION,fillpos);}else{glDisable(GL_LIGHTING);glDisable(GL_LIGHT1);}glScalef(1.0f,1.0f,-1.0f);WTviewpoint*v=universe_window.view?universe_window.view:&universe_view;float qi[16];WTq inv={-v->q[0],-v->q[1],-v->q[2],v->q[3]};quat_matrix(inv,qi);glMultMatrixf(qi);glTranslatef(-v->p[0],-v->p[1],-v->p[2]);draw_node(universe_root);glDisable(GL_LIGHTING);if(universe_window.overlay3d)universe_window.overlay3d(&universe_window,FALSE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_DEPTH_TEST);glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,1,1,0,-1,1);glMatrixMode(GL_MODELVIEW);glLoadIdentity();if(universe_window.overlay2d)universe_window.overlay2d(&universe_window,FALSE);const char*capture=getenv("WTK_NG_CAPTURE");const char*capture_at=getenv("WTK_NG_CAPTURE_FRAME");int target=capture_at?atoi(capture_at):30;if(capture&&frame++==target){unsigned char*p=xcalloc((size_t)w*h,3);glFinish();glReadBuffer(GL_BACK);glPixelStorei(GL_PACK_ALIGNMENT,1);glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,p);FILE*f=fopen(capture,"wb");if(f){fprintf(f,"P6\n%d %d\n255\n",w,h);for(int y=h-1;y>=0;y--)fwrite(p+(size_t)y*w*3,3,(size_t)w,f);fclose(f);}free(p);}SDL_GL_SwapWindow(sdl_window);}

void WTuniverse_new(int d,int wm){(void)d;(void)wm;if(universe_root)return;SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)<0){WTerror("SDL init failed: %s\n",SDL_GetError());exit(1);}SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);sdl_window=SDL_CreateWindow("Space Rocks / WTK-NG",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,960,720,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN);if(!sdl_window){WTerror("Window failed: %s\n",SDL_GetError());exit(1);}gl_context=SDL_GL_CreateContext(sdl_window);SDL_ShowWindow(sdl_window);SDL_RaiseWindow(sdl_window);SDL_GL_SetSwapInterval(1);universe_root=node_new(NULL,NULL);WTq_init(universe_view.q);universe_window.view=&universe_view;universe_window.nearz=.01f;universe_window.farz=5000;running=1;}
void WTuniverse_delete(void){running=0;} WTnode *WTuniverse_getrootnodes(void){return universe_root;} WTwindow *WTuniverse_getwindows(void){return &universe_window;} WTviewpoint *WTuniverse_getviewpoints(void){return &universe_view;} WTviewpoint *WTuniverse_getviewpoint(void){return &universe_view;} void WTuniverse_setactions(void(*f)(void)){universe_actions=f;} void WTuniverse_ready(void){}
static int map_key(SDL_Keycode k){switch(k){case SDLK_UP:return WTKEY_UPARROW;case SDLK_DOWN:return WTKEY_DOWNARROW;case SDLK_LEFT:return WTKEY_LEFTARROW;case SDLK_RIGHT:return WTKEY_RIGHTARROW;default:return (k>=32&&k<127)?(int)k:0;}}
static void poll_events(void){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=0;else if(e.type==SDL_KEYDOWN)pending_key=map_key(e.key.keysym.sym);else if(e.type==SDL_MOUSEBUTTONDOWN||e.type==SDL_MOUSEBUTTONUP){int down=e.type==SDL_MOUSEBUTTONDOWN,bit=e.button.button==SDL_BUTTON_LEFT?WTMOUSE_LEFTBUTTON:e.button.button==SDL_BUTTON_MIDDLE?WTMOUSE_MIDDLEBUTTON:WTMOUSE_RIGHTBUTTON;if(mouse_sensor){if(down)mouse_sensor->buttons|=bit;else mouse_sensor->buttons&=~bit;}}}if(mouse_sensor){int x,y,window_x,window_y;SDL_GetMouseState(&x,&y);SDL_GetWindowPosition(sdl_window,&window_x,&window_y);mouse_sensor->raw.pos[0]=(float)(x+window_x);mouse_sensor->raw.pos[1]=(float)(y+window_y);if(mouse_sensor->update)mouse_sensor->update(mouse_sensor);}}
static void collect_tasks(WTnode*n,WTnode***a,int*c,int*cap){if(!n||n->deleted)return;if(n->task){if(*c==*cap){*cap=*cap?*cap*2:64;*a=realloc(*a,*cap*sizeof(**a));}(*a)[(*c)++]=n;}for(int i=0;i<n->child_count;i++)collect_tasks(n->children[i],a,c,cap);for(int i=0;i<n->attachment_count;i++)collect_tasks(n->attachments[i],a,c,cap);}
static void universe_frame(void){if(!running){
#ifdef __EMSCRIPTEN__
emscripten_cancel_main_loop();
#endif
return;}poll_events();if(loop_start_keys&&loop_start_keys[loop_key_index])pending_key=(unsigned char)loop_start_keys[loop_key_index++];if(universe_actions)universe_actions();WTnode**tasks=NULL;int count=0,cap=0;collect_tasks(universe_root,&tasks,&count,&cap);for(int i=0;i<count;i++)if(!tasks[i]->deleted&&tasks[i]->task)tasks[i]->task(tasks[i]);free(tasks);render_frame();uint64_t now=SDL_GetPerformanceCounter();double dt=(double)(now-loop_previous)/SDL_GetPerformanceFrequency();if(dt>0)current_fps=(float)(1.0/dt);loop_previous=now;if(loop_max_frames>0&&++loop_frames>=loop_max_frames)running=0;}
void WTuniverse_go(void){const char*limit=getenv("WTK_NG_FRAMES");const char*start_key=getenv("WTK_NG_START_KEY");loop_start_keys=getenv("WTK_NG_START_KEYS");loop_previous=SDL_GetPerformanceCounter();loop_frames=loop_key_index=0;loop_max_frames=limit?atoi(limit):0;if(start_key&&*start_key)pending_key=(unsigned char)*start_key;
#ifdef __EMSCRIPTEN__
emscripten_set_main_loop(universe_frame,0,1);
#else
while(running)universe_frame();SDL_GL_DeleteContext(gl_context);SDL_DestroyWindow(sdl_window);SDL_Quit();
#endif
}
void WTuniverse_stop(void){running=0;} int WTuniverse_getrendering(void){return render_flags;} void WTuniverse_setrendering(FLAG f){render_flags=f;} float WTuniverse_framerate(void){return current_fps;} float WTuniverse_avgframerate(int samples){(void)samples;return current_fps;}

WTwindow *WTwindow_new(int x,int y,int w,int h,int f){(void)x;(void)y;(void)w;(void)h;(void)f;return &universe_window;} void WTwindow_delete(WTwindow*w){(void)w;} WTwindow *WTwindow_next(WTwindow*w){return w==&universe_window?NULL:NULL;} void WTwindow_getposition(WTwindow*w,int*x,int*y,int*ww,int*h){(void)w;SDL_GetWindowPosition(sdl_window,x,y);SDL_GetWindowSize(sdl_window,ww,h);} WTviewpoint *WTwindow_getviewpoint(WTwindow*w){return w?w->view:NULL;} void WTwindow_setviewpoint(WTwindow*w,WTviewpoint*v){if(w)w->view=v;} void WTwindow_setfgactions(WTwindow*w,void(*f)(WTwindow*,FLAG)){if(w)w->overlay2d=f;} void WTwindow_setdrawfn(WTwindow*w,void(*f)(WTwindow*,FLAG)){if(w)w->overlay3d=f;} void WTwindow_setyonvalue(WTwindow*w,float v){if(w)w->farz=v;} void WTwindow_sethithervalue(WTwindow*w,float v){if(w)w->nearz=v;} void WTwindow_setbgrgb(WTwindow*w,int r,int g,int b){if(w){w->bg[0]=r/255.f;w->bg[1]=g/255.f;w->bg[2]=b/255.f;}} int WTwindow_numpolys(WTwindow*w){(void)w;return WTnode_numpolys(universe_root);} void WTwindow_zoomviewpoint(WTwindow*w){(void)w;} void WTwindow_zoomviewtonode(WTwindow*w,WTnode*n,int f){(void)w;(void)n;(void)f;} void WTwindow_loadimage(WTwindow*w,const char*n,float s,FLAG a,FLAG b){(void)w;(void)n;(void)s;(void)a;(void)b;}
static float draw_color[3]={0,1,0}; void WTwindow_set2Dcolor(WTwindow*w,int r,int g,int b){(void)w;draw_color[0]=r/255.f;draw_color[1]=g/255.f;draw_color[2]=b/255.f;glColor3fv(draw_color);} void WTwindow_set2Dlinewidth(WTwindow*w,float v){(void)w;glLineWidth(v);} void WTwindow_set2Dlinestyle(WTwindow*w,int s){(void)w;(void)s;} void WTwindow_draw2Dline(WTwindow*w,float a,float b,float c,float d){(void)w;glColor3fv(draw_color);glBegin(GL_LINES);glVertex2f(a,b);glVertex2f(c,d);glEnd();} void WTwindow_draw2Dtext(WTwindow*w,float x,float y,const char*t){(void)w;(void)x;(void)y;(void)t;} void WTwindow_draw2Dtexture(WTwindow*w,const char*n,FLAG alpha,WTp2*p,WTp2*u){(void)w;TextureEntry*t=load_texture(n);if(!t||!p||!u)return;glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,t->id);glColor4f(1,1,1,1);if(alpha){glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);}glBegin(GL_QUADS);for(int i=0;i<4;i++){glTexCoord2fv(u[i]);glVertex2fv(p[i]);}glEnd();if(alpha)glDisable(GL_BLEND);glDisable(GL_TEXTURE_2D);} void WTwindow_get2Dtextextents(WTwindow*w,const char*t,float*a,float*b){(void)w;*a=(float)strlen(t)*.01f;*b=.02f;} void WTwindow_set3Dcolor(WTwindow*w,int r,int g,int b){WTwindow_set2Dcolor(w,r,g,b);} void WTwindow_set3Dpointsize(WTwindow*w,float s){(void)w;glPointSize(s);} void WTwindow_draw3Dpoints(WTwindow*w,WTp3*p,int c){(void)w;glBegin(GL_POINTS);for(int i=0;i<c;i++)glVertex3fv(p[i]);glEnd();} void WTwindow_draw3Dlines(WTwindow*w,WTp3*p,int c,int m){(void)w;(void)m;glBegin(GL_LINES);for(int i=0;i<c;i++)glVertex3fv(p[i]);glEnd();}

void WTtask_new(WTnode*n,void(*f)(WTnode*),float p){(void)p;if(n)n->task=f;} void WTkeyboard_open(void){} int WTkeyboard_getkey(void){int k=pending_key;pending_key=0;return k;}
WTsensor *WTmouse_new(void){if(!mouse_sensor){mouse_sensor=xcalloc(1,sizeof(*mouse_sensor));mouse_sensor->sensitivity=1;mouse_sensor->angular_rate=1;WTq_init(mouse_sensor->q);}return mouse_sensor;} void WTmouse_rawupdate(WTsensor*s){(void)s;} WTwindow *WTmouse_whichwindow(WTsensor*s){(void)s;return &universe_window;} void *WTsensor_getrawdata(WTsensor*s){return s?&s->raw:NULL;} float WTsensor_getsensitivity(WTsensor*s){return s?s->sensitivity:0;} void WTsensor_setsensitivity(WTsensor*s,float v){if(s)s->sensitivity=v;} float WTsensor_getangularrate(WTsensor*s){return s?s->angular_rate:0;} void WTsensor_setangularrate(WTsensor*s,float v){if(s)s->angular_rate=v;} void WTsensor_setupdatefn(WTsensor*s,void(*f)(WTsensor*)){if(s)s->update=f;} void WTsensor_setrecord(WTsensor*s,const WTp3 p,const WTq q){if(s){WTp3_copy(p,s->p);quat_copy(q,s->q);}} void WTsensor_getrotation(WTsensor*s,WTq q){if(s)quat_copy(s->q,q);else WTq_init(q);} int WTsensor_getmiscdata(WTsensor*s){return s?s->buttons:0;}
#define SENSOR_STUB(n) WTsensor *n(WTserialname p,...){(void)p;return WTmouse_new();}
SENSOR_STUB(WTbird_new) SENSOR_STUB(WTboom_new) SENSOR_STUB(WTcrystaleyesVR_new) SENSOR_STUB(WTfastrak_new) SENSOR_STUB(WTformula_new) SENSOR_STUB(WTgeoball_new) SENSOR_STUB(WTglove5DT_new) SENSOR_STUB(WTiglasses_new) SENSOR_STUB(WTinsidetrak_new) SENSOR_STUB(WTisotrak2_new) SENSOR_STUB(WTjoyserial_new) SENSOR_STUB(WTlogitech_new) SENSOR_STUB(WTpolhemus_new) SENSOR_STUB(WTprecision_new) SENSOR_STUB(WTbaron_new) SENSOR_STUB(WTspaceball_new) SENSOR_STUB(WTspacecontrol_new)

static void audio_callback(void *userdata,Uint8 *stream,int len){
WTsounddevice*d=userdata;SDL_memset(stream,0,(size_t)len);for(WTsound*s=d->sounds;s;s=s->next){if(!s->playing||!s->buffer||!s->length)continue;int out=0;while(out<len&&s->playing){Uint32 available=s->length-s->cursor;Uint32 amount=(Uint32)(len-out)<available?(Uint32)(len-out):available;SDL_MixAudioFormat(stream+out,s->buffer+s->cursor,d->spec.format,amount,s->volume);s->cursor+=amount;out+=(int)amount;if(s->cursor>=s->length){if(s->loops<0||s->remaining_loops-->0)s->cursor=0;else{s->playing=0;s->done_pending=1;}}}}}
WTsounddevice *WTsounddevice_open(int t,int c,WTviewpoint*v){(void)t;(void)c;(void)v;if(!(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)&&SDL_InitSubSystem(SDL_INIT_AUDIO)<0){WTwarning("WTK-NG: audio init failed: %s\n",SDL_GetError());return NULL;}WTsounddevice*d=xcalloc(1,sizeof(*d));SDL_AudioSpec wanted={0};wanted.freq=44100;wanted.format=AUDIO_S16SYS;wanted.channels=2;wanted.samples=1024;wanted.callback=audio_callback;wanted.userdata=d;d->id=SDL_OpenAudioDevice(NULL,0,&wanted,&d->spec,SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_CHANNELS_CHANGE);if(!d->id){WTwarning("WTK-NG: audio device failed: %s\n",SDL_GetError());free(d);return NULL;}SDL_PauseAudioDevice(d->id,0);return d;}
void WTsounddevice_close(WTsounddevice*d){if(!d||d->closed)return;if(d->id)SDL_CloseAudioDevice(d->id);d->id=0;d->closed=1;}
void WTsounddevice_update(WTsounddevice*d){if(!d)return;for(WTsound*s=d->sounds;s;s=s->next)if(s->done_pending){s->done_pending=0;if(s->done)s->done(s);}}
void WTsounddevice_setparam(WTsounddevice*d,int p,float v){(void)d;(void)p;(void)v;}
WTsound *WTsound_load(WTsounddevice*d,const char*n){if(!d||d->closed||!n)return NULL;char path[256];if(!asset_path(n,path,sizeof path)){WTwarning("WTK-NG: sound not found: %s\n",n);return NULL;}SDL_AudioSpec source;Uint8*original=NULL;Uint32 original_length=0;if(!SDL_LoadWAV(path,&source,&original,&original_length)){WTwarning("WTK-NG: cannot load %s: %s\n",path,SDL_GetError());return NULL;}SDL_AudioCVT cvt;if(SDL_BuildAudioCVT(&cvt,source.format,source.channels,source.freq,d->spec.format,d->spec.channels,d->spec.freq)<0){SDL_FreeWAV(original);return NULL;}Uint8*converted=NULL;Uint32 converted_length=original_length;if(cvt.needed){cvt.len=(int)original_length;cvt.buf=SDL_malloc((size_t)original_length*(size_t)cvt.len_mult);if(!cvt.buf){SDL_FreeWAV(original);return NULL;}SDL_memcpy(cvt.buf,original,original_length);SDL_FreeWAV(original);if(SDL_ConvertAudio(&cvt)<0){SDL_free(cvt.buf);return NULL;}converted=cvt.buf;converted_length=(Uint32)cvt.len_cvt;}else{converted=SDL_malloc(original_length);if(converted)SDL_memcpy(converted,original,original_length);SDL_FreeWAV(original);if(!converted)return NULL;}WTsound*s=xcalloc(1,sizeof(*s));s->device=d;s->buffer=converted;s->length=converted_length;s->volume=SDL_MIX_MAXVOLUME;s->next=d->sounds;SDL_LockAudioDevice(d->id);d->sounds=s;SDL_UnlockAudioDevice(d->id);return s;}
void WTsound_delete(WTsound*s){if(!s)return;WTsounddevice*d=s->device;if(d&&!d->closed&&d->id)SDL_LockAudioDevice(d->id);if(d){WTsound**link=&d->sounds;while(*link&&*link!=s)link=&(*link)->next;if(*link==s)*link=s->next;}if(d&&!d->closed&&d->id)SDL_UnlockAudioDevice(d->id);SDL_free(s->buffer);free(s);}
FLAG WTsound_play(WTsound*s){if(!s||!s->buffer||!s->device||s->device->closed)return FALSE;SDL_LockAudioDevice(s->device->id);s->cursor=0;s->remaining_loops=s->loops;s->done_pending=0;s->playing=1;SDL_UnlockAudioDevice(s->device->id);return TRUE;}
void WTsound_stop(WTsound*s){if(!s||!s->device||s->device->closed)return;SDL_LockAudioDevice(s->device->id);s->playing=0;s->cursor=0;s->done_pending=0;SDL_UnlockAudioDevice(s->device->id);}
void WTsound_setparam(WTsound*s,int p,float v){if(!s)return;if(p==WTSOUND_LOOPS)s->loops=(int)v;else if(p==WTSOUND_VOLUME){if(v<0)v=0;if(v>100)v=100;s->volume=(int)(v*SDL_MIX_MAXVOLUME/100.0f);}}
void WTsound_setposition(WTsound*s,const WTp3 p){(void)s;(void)p;} void WTsound_setnodepath(WTsound*s,WTnodepath*p){(void)s;(void)p;} void WTsound_setdonefn(WTsound*s,void(*f)(WTsound*)){if(s)s->done=f;}
WTfont3d *WTfont3d_load(const char*n){WTfont3d*font=xcalloc(1,sizeof(*font));font->spacing=10;FILE*f=open_asset(n);if(!f)return font;char line[2048];while(fgets(line,sizeof line,f)){int code;if(sscanf(line,"char%d",&code)!=1||code<0||code>255)continue;if(!fgets(line,sizeof line,f))break;int nv=0;if(sscanf(line,"%d",&nv)!=1||nv<1||nv>100000)continue;WTgeometry*g=WTgeometry_begin();for(int i=0;i<nv&&fgets(line,sizeof line,f);i++){WTp3 p;if(sscanf(line,"%f %f %f",&p[0],&p[1],&p[2])==3)WTgeometry_newvertex(g,p);}if(!fgets(line,sizeof line,f))break;int np=0;if(sscanf(line,"%d",&np)!=1||np<0||np>100000)continue;for(int i=0;i<np&&fgets(line,sizeof line,f);i++){char*cur=line;int count=(int)strtol(cur,&cur,10);WTpoly*p=WTgeometry_beginpoly(g);for(int j=0;j<count;j++)WTpoly_addvertex(p,(int)strtol(cur,&cur,10));}font->glyphs[code]=g;}fclose(f);return font;} float WTfont3d_getspacing(WTfont3d*f){return f?f->spacing:10;} WTgeometry *WTgeometry_newtext3d(WTfont3d*f,const char*t){if(f&&t&&*t&&f->glyphs[(unsigned char)*t])return f->glyphs[(unsigned char)*t];return WTgeometry_newblock(6,10,1,TRUE);} void WTscreen_setyblank(int v){(void)v;}

static WTui *ui_new(const char*t){WTui*u=xcalloc(1,sizeof(*u));if(t)snprintf(u->text,sizeof(u->text),"%s",t);return u;} WTui *WTui_init(int*a,char**b){(void)a;(void)b;return ui_new("root");} WTui *WTui_newform(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newwtkwindow(WTui*p,int f){(void)p;(void)f;return ui_new("window");} WTui *WTui_newlabel(WTui*p,const char*t,int i,...){(void)p;(void)i;return ui_new(t);} WTui *WTui_newframe(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newmenubar(WTui*p,...){(void)p;return ui_new("menubar");} WTui *WTui_newmenupopup(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newmenuitem(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newpushbutton(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newscrolledlist(WTui*p,const char*t,char**i,int c,...){(void)p;(void)i;(void)c;return ui_new(t);} WTui *WTui_newscrolledtext(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newscale(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newfileselection(WTui*p,const char*t,...){(void)p;return ui_new(t);} WTui *WTui_newtextinput(WTui*p,const char*t,FLAG f){(void)p;(void)f;return ui_new(t);} WTui *WTui_newmessagebox(WTui*p,const char*t,const char*title,...){(void)p;(void)t;return ui_new(title);} void WTui_setcallback(WTui*u,int e,void(*f)(WTui*,void*),void*d){(void)u;(void)e;(void)f;(void)d;} void WTui_insertitem(WTui*u,int i,const char*t){(void)u;(void)i;(void)t;} void WTui_dimitem(WTui*u,FLAG d){(void)u;(void)d;} void WTui_setmenutext(WTui*u,const char*t){if(u&&t)snprintf(u->text,sizeof(u->text),"%s",t);} char *WTui_gettext(WTui*u){return u?u->text:NULL;} void WTui_manage(WTui*u){(void)u;} void WTui_delete(WTui*u){(void)u;} void WTui_go(WTui*u,int w){(void)u;(void)w;WTuniverse_go();}

int WTnet_open(const char*a,int p,int f){(void)a;(void)p;(void)f;return 0;} void WTnet_additem(void*d,int l,int t,int g){(void)d;(void)l;(void)t;(void)g;} int WTnet_next(int*t,int*l){(void)t;(void)l;return 0;} void WTnet_removeitem(void*d,int l,int*t,int*r){(void)d;(void)l;(void)t;(void)r;}
