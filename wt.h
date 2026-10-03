#ifndef WTK_NG_WT_H
#define WTK_NG_WT_H

#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int FLAG;
typedef int WTtype;
typedef int WTserialname;
typedef float WTp2[2];
typedef float WTp3[3];
typedef float WTq[4];
typedef float WTm3[3][3];
typedef struct { WTp3 p; WTq q; } WTpq;

typedef struct WTnode WTnode;
typedef struct WTnodepath WTnodepath;
typedef struct WTgeometry WTgeometry;
typedef struct WTpoly WTpoly;
typedef struct WTvertex WTvertex;
typedef struct WTmtable WTmtable;
typedef struct WTwindow WTwindow;
typedef struct WTviewpoint WTviewpoint;
typedef struct WTsensor WTsensor;
typedef struct WTsound WTsound;
typedef struct WTsounddevice WTsounddevice;
typedef struct WTfont3d WTfont3d;
typedef struct WTui WTui;
typedef struct WTpath WTpath;
typedef struct WTmotionlink WTmotionlink;

typedef struct WTmouse_rawdata { WTp3 pos; int buttons; } WTmouse_rawdata;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#define X 0
#define Y 1
#define Z 2
#define SERIAL1 0
#define SERIAL2 1

#define WTDISPLAY_DEFAULT 0
#define WTDISPLAY_NOWINDOW 1
#define WTDISPLAY_STEREO 2
#define WTDISPLAY_CRYSTALEYES 3
#define WTDISPLAY_RBSTEREO 4
#define WTDISPLAY_STEREOWINDOW 5
#define WTWINDOW_DEFAULT 0
#define WTWINDOW_NOBORDER 1
#define WTWINDOW_STEREOVSPLIT 2
#define WTFRAME_LOCAL 0
#define WTFRAME_PARENT 1

#define WTRENDER_WIREFRAME 0x0001
#define WTRENDER_SHADED 0x0002
#define WTRENDER_GOURAUD 0x0004
#define WTRENDER_TEXTURED 0x0008
#define WTRENDER_LIGHTING 0x0010
#define WTRENDER_SMOOTH 0x0020
#define WTRENDER_PERSPECTIVE 0x0040
#define WTRENDER_ANTIALIAS 0x0080
#define WTRENDER_BEST 0x0100
#define WTRENDER_TWOSIDED 0x0200

#define WTMAT_AMBIENTDIFFUSE 0x01
#define WTMAT_OPACITY 0x02
#define WTFILTER_NEAREST 0
#define WTFILTER_LINEAR 1
#define WTFILTER_LINEARMIPMAPLINEAR 2
#define WTIMAGE_RGBA 4
#define WTLINE_SEGMENTS 0

#define WTSOUNDDEVICE_DWSTK 0
#define WTSOUNDDEVICE_DIRECTSOUND 1
#define WTSOUNDDEVICE_SGI 2
#define WTSOUNDDEVICE_WINMM 3
#define WTSOUNDDEVICE_VSI 4
#define WTSOUNDDEVICE_CRE 5
#define WTSOUNDDEVICE_ROLLOFF 10
#define WTSOUND_LOOPS 11
#define WTSOUND_VOLUME 12
#define WTSOUND_SPATIALIZE 13

#define WTMOUSE_LEFTBUTTON 0x01
#define WTMOUSE_MIDDLEBUTTON 0x02
#define WTMOUSE_RIGHTBUTTON 0x04
#define WTKEY_UPARROW 0x101
#define WTKEY_DOWNARROW 0x102
#define WTKEY_LEFTARROW 0x103
#define WTKEY_RIGHTARROW 0x104

#define WTUIATT_LEFT 1
#define WTUIATT_TOP 2
#define WTUIATT_WIDTH 3
#define WTUIATT_HEIGHT 4
#define WTUIEVENT_ACTIVATE 1
#define WTUIEVENT_CLOSE 2
#define WTUIEVENT_DONE 3
#define WTUIEVENT_OK 4
#define WTNODE_TRANSFORM 1

void WTmessage(const char *fmt, ...);
void WTwarning(const char *fmt, ...);
void WTerror(const char *fmt, ...);

void WTp3_init(WTp3 p);
void WTp3_copy(const WTp3 a, WTp3 out);
void WTp3_add(const WTp3 a, const WTp3 b, WTp3 out);
void WTp3_subtract(const WTp3 a, const WTp3 b, WTp3 out);
void WTp3_mults(WTp3 p, float s);
float WTp3_mag(const WTp3 p);
float WTp3_distance(const WTp3 a, const WTp3 b);
void WTp3_norm(WTp3 p);
void WTp3_print(const WTp3 p, const char *label);
void WTq_init(WTq q);
void WTq_mult(const WTq a, const WTq b, WTq out);
void WTq_2dir(const WTq q, WTp3 out);
void WTdir_2q(const WTp3 dir, WTq out);
void WTeuler_2q(float x, float y, float z, WTq out);
void WTeuler_2m3(float x, float y, float z, WTm3 out);

void WTuniverse_new(int display, int window_mode);
void WTuniverse_delete(void);
WTnode *WTuniverse_getrootnodes(void);
WTwindow *WTuniverse_getwindows(void);
WTviewpoint *WTuniverse_getviewpoints(void);
WTviewpoint *WTuniverse_getviewpoint(void);
void WTuniverse_setactions(void (*fn)(void));
void WTuniverse_ready(void);
void WTuniverse_go(void);
void WTuniverse_stop(void);
int WTuniverse_getrendering(void);
void WTuniverse_setrendering(FLAG flags);
float WTuniverse_framerate(void);
float WTuniverse_avgframerate(int samples);

WTnode *WTgroupnode_new(WTnode *parent);
WTnode *WTmovsepnode_new(WTnode *parent);
WTnode *WTmovswitchnode_new(WTnode *parent);
WTnode *WTmovgeometrynode_new(WTnode *parent, WTgeometry *geom);
WTnode *WTmovnode_load(WTnode *parent, const char *name, float scale);
WTnode *WTnode_load(WTnode *parent, const char *name, float scale);
WTnode *WTmovnode_instance(WTnode *parent, WTnode *source);
void WTmovnode_attach(WTnode *parent, WTnode *child, int slot);
int WTmovnode_numattachments(WTnode *node);
WTnode *WTmovnode_getattachment(WTnode *node, int index);
void WTmovnode_rotateaxis(WTnode *node, int axis, float degrees);
void WTswitchnode_setwhichchild(WTnode *node, int child);
void WTnode_delete(WTnode *node);
void WTnode_enable(WTnode *node, FLAG enabled);
FLAG WTnode_isenabled(WTnode *node);
int WTnode_numchildren(WTnode *node);
WTnode *WTnode_getchild(WTnode *node, int index);
void WTnode_setdata(WTnode *node, void *data);
void *WTnode_getdata(WTnode *node);
WTgeometry *WTnode_getgeometry(WTnode *node);
void WTnode_settranslation(WTnode *node, const WTp3 p);
void WTnode_gettranslation(WTnode *node, WTp3 p);
void WTnode_translate(WTnode *node, const WTp3 delta, int frame);
void WTnode_setorientation(WTnode *node, const WTq q);
void WTnode_getorientation(WTnode *node, WTq q);
void WTnode_setrotation(WTnode *node, const WTm3 m);
void WTnode_getrotation(WTnode *node, WTm3 m);
void WTnode_rotate(WTnode *node, float x, float y, float z, int frame);
void WTnode_rotateq(WTnode *node, const WTq q, int frame);
float WTnode_getradius(WTnode *node);
void WTnode_getmidpoint(WTnode *node, WTp3 p);
int WTnode_numpolys(WTnode *node);
void WTnode_print(WTnode *node);
void WTnode_setname(WTnode *node, const char *name);

WTnodepath *WTnodepath_new(WTnode *node, WTnode *root, int flags);
FLAG WTnodepath_intersectbbox(WTnodepath *a, WTnodepath *b);

WTgeometry *WTgeometry_begin(void);
void WTgeometry_close(WTgeometry *g);
WTgeometry *WTgeometry_newblock(float x, float y, float z, FLAG solid);
WTgeometry *WTgeometry_newsphere(float radius, int rings, int slices, FLAG a, FLAG b);
WTvertex *WTgeometry_newvertex(WTgeometry *g, const WTp3 p);
WTpoly *WTgeometry_beginpoly(WTgeometry *g);
WTpoly *WTgeometry_getpolys(WTgeometry *g);
WTvertex *WTgeometry_getvertices(WTgeometry *g);
void WTgeometry_getvertexposition(WTgeometry *g, WTvertex *v, WTp3 p);
void WTgeometry_setvertexposition(WTgeometry *g, WTvertex *v, const WTp3 p);
void WTgeometry_beginedit(WTgeometry *g);
void WTgeometry_endedit(WTgeometry *g);
void WTgeometry_recomputestats(WTgeometry *g, FLAG normals);
void WTgeometry_scale(WTgeometry *g, float scale, const WTp3 origin);
void WTgeometry_setrgb(WTgeometry *g, int r, int green, int b);
void WTgeometry_setlighting(WTgeometry *g, FLAG enabled);
void WTgeometry_setbrightness(WTgeometry *g, float brightness);
void WTgeometry_rotatenormals(WTgeometry *g, int axis, float degrees);
void WTgeometry_setstablelighting(WTgeometry *g, FLAG enabled);
void WTgeometry_setoutline(WTgeometry *g, FLAG enabled);
void WTgeometry_settexture(WTgeometry *g, const char *name, FLAG a, FLAG b);
FLAG WTgeometry_changetexture(WTgeometry *g, const char *name, FLAG a, FLAG b);
void WTgeometry_setmtable(WTgeometry *g, WTmtable *m);
void WTgeometry_setmatid(WTgeometry *g, int id);
float WTgeometry_getradius(WTgeometry *g);
WTgeometry *WTgeometry_newtext3d(WTfont3d *font, const char *text);
void WTpoly_addvertex(WTpoly *p, int index);
void WTpoly_addvertexptr(WTpoly *p, WTvertex *v);
WTvertex *WTpoly_getvertex(WTpoly *p, int index);
int WTpoly_numvertices(WTpoly *p);
WTpoly *WTpoly_next(WTpoly *p);
void WTpoly_delete(WTpoly *p);
void WTpoly_close(WTpoly *p);
void WTpoly_settexture(WTpoly *p, const char *name, FLAG a, FLAG b);
void WTpoly_settexture_uv(WTpoly *p, const char *name, const float *u, const float *v,
                          int mode, FLAG filtered);
void WTpoly_setcolor(WTpoly *p, int color);
WTvertex *WTvertex_next(WTvertex *v);
void WTvertex_setnormal(WTvertex *v, const WTp3 normal);

WTmtable *WTmtable_new(int flags, int count, void *unused);
int WTmtable_newentry(WTmtable *m);
void WTmtable_setvalue(WTmtable *m, int id, const float *value, int property);
unsigned char *WTtexture_load(const char *name, int *width, int *height);
void WTtexture_cache(const char *name, FLAG cache);
void WTtexture_replace(const char *name, int format, int width, int height, unsigned char *pixels);
void WTtexture_setfilter(const char *name, int minfilter, int magfilter);
long WTtexture_getmemory(void);
WTnode *WTlightnode_load(WTnode *parent, const char *name);

WTviewpoint *WTviewpoint_copy(WTviewpoint *view);
void WTviewpoint_setposition(WTviewpoint *view, const WTp3 p);
void WTviewpoint_getposition(WTviewpoint *view, WTp3 p);
void WTviewpoint_setorientation(WTviewpoint *view, const WTq q);
void WTviewpoint_setparallax(WTviewpoint *view, float p);
float WTviewpoint_getparallax(WTviewpoint *view);
void WTviewpoint_moveto(WTviewpoint *view, const WTpq *pq);
void WTviewpoint_addsensor(WTviewpoint *view, WTsensor *sensor);

WTwindow *WTwindow_new(int x, int y, int w, int h, int flags);
void WTwindow_delete(WTwindow *window);
WTwindow *WTwindow_next(WTwindow *window);
void WTwindow_getposition(WTwindow *window, int *x, int *y, int *w, int *h);
WTviewpoint *WTwindow_getviewpoint(WTwindow *window);
void WTwindow_setviewpoint(WTwindow *window, WTviewpoint *view);
void WTwindow_setfgactions(WTwindow *window, void (*fn)(WTwindow *, FLAG));
void WTwindow_setdrawfn(WTwindow *window, void (*fn)(WTwindow *, FLAG));
void WTwindow_setyonvalue(WTwindow *window, float value);
void WTwindow_sethithervalue(WTwindow *window, float value);
void WTwindow_setbgrgb(WTwindow *window, int r, int g, int b);
int WTwindow_numpolys(WTwindow *window);
void WTwindow_zoomviewpoint(WTwindow *window);
void WTwindow_zoomviewtonode(WTwindow *window, WTnode *node, int flags);
void WTwindow_set2Dcolor(WTwindow *window, int r, int g, int b);
void WTwindow_set2Dlinewidth(WTwindow *window, float width);
void WTwindow_set2Dlinestyle(WTwindow *window, int style);
void WTwindow_draw2Dline(WTwindow *window, float x1, float y1, float x2, float y2);
void WTwindow_draw2Dtext(WTwindow *window, float x, float y, const char *text);
void WTwindow_draw2Dtexture(WTwindow *window, const char *name, FLAG alpha, WTp2 *pos, WTp2 *uv);
void WTwindow_get2Dtextextents(WTwindow *window, const char *text, float *w, float *h);
void WTwindow_set3Dcolor(WTwindow *window, int r, int g, int b);
void WTwindow_set3Dpointsize(WTwindow *window, float size);
void WTwindow_draw3Dpoints(WTwindow *window, WTp3 *points, int count);
void WTwindow_draw3Dlines(WTwindow *window, WTp3 *points, int count, int mode);
void WTwindow_loadimage(WTwindow *window, const char *name, float scale, FLAG a, FLAG b);

void WTtask_new(WTnode *node, void (*fn)(WTnode *), float priority);
void WTkeyboard_open(void);
int WTkeyboard_getkey(void);
WTsensor *WTmouse_new(void);
void WTmouse_rawupdate(WTsensor *sensor);
WTwindow *WTmouse_whichwindow(WTsensor *sensor);
void *WTsensor_getrawdata(WTsensor *sensor);
float WTsensor_getsensitivity(WTsensor *sensor);
void WTsensor_setsensitivity(WTsensor *sensor, float value);
float WTsensor_getangularrate(WTsensor *sensor);
void WTsensor_setangularrate(WTsensor *sensor, float value);
void WTsensor_setupdatefn(WTsensor *sensor, void (*fn)(WTsensor *));
void WTsensor_setrecord(WTsensor *sensor, const WTp3 p, const WTq q);
void WTsensor_getrotation(WTsensor *sensor, WTq q);
int WTsensor_getmiscdata(WTsensor *sensor);

#define WTK_SENSOR_CTOR(name) WTsensor *name(WTserialname port, ...)
WTK_SENSOR_CTOR(WTbird_new); WTK_SENSOR_CTOR(WTboom_new);
WTK_SENSOR_CTOR(WTcrystaleyesVR_new); WTK_SENSOR_CTOR(WTfastrak_new);
WTK_SENSOR_CTOR(WTformula_new); WTK_SENSOR_CTOR(WTgeoball_new);
WTK_SENSOR_CTOR(WTglove5DT_new); WTK_SENSOR_CTOR(WTiglasses_new);
WTK_SENSOR_CTOR(WTinsidetrak_new); WTK_SENSOR_CTOR(WTisotrak2_new);
WTK_SENSOR_CTOR(WTjoyserial_new); WTK_SENSOR_CTOR(WTlogitech_new);
WTK_SENSOR_CTOR(WTpolhemus_new); WTK_SENSOR_CTOR(WTprecision_new);
WTK_SENSOR_CTOR(WTbaron_new); WTK_SENSOR_CTOR(WTspaceball_new);
WTK_SENSOR_CTOR(WTspacecontrol_new);
#undef WTK_SENSOR_CTOR

WTsounddevice *WTsounddevice_open(int type, int channels, WTviewpoint *listener);
void WTsounddevice_close(WTsounddevice *device);
void WTsounddevice_update(WTsounddevice *device);
void WTsounddevice_setparam(WTsounddevice *device, int param, float value);
WTsound *WTsound_load(WTsounddevice *device, const char *name);
void WTsound_delete(WTsound *sound);
FLAG WTsound_play(WTsound *sound);
void WTsound_stop(WTsound *sound);
void WTsound_setparam(WTsound *sound, int param, float value);
void WTsound_setposition(WTsound *sound, const WTp3 p);
void WTsound_setnodepath(WTsound *sound, WTnodepath *path);
void WTsound_setdonefn(WTsound *sound, void (*fn)(WTsound *));

WTfont3d *WTfont3d_load(const char *name);
float WTfont3d_getspacing(WTfont3d *font);
void WTscreen_setyblank(int value);

WTui *WTui_init(int *argc, char **argv);
WTui *WTui_newform(WTui *parent, const char *title, ...);
WTui *WTui_newwtkwindow(WTui *parent, int flags);
WTui *WTui_newlabel(WTui *parent, const char *text, int image, ...);
WTui *WTui_newframe(WTui *parent, const char *title, ...);
WTui *WTui_newmenubar(WTui *parent, ...);
WTui *WTui_newmenupopup(WTui *parent, const char *text, ...);
WTui *WTui_newmenuitem(WTui *parent, const char *text, ...);
WTui *WTui_newpushbutton(WTui *parent, const char *text, ...);
WTui *WTui_newscrolledlist(WTui *parent, const char *text, char **items, int count, ...);
WTui *WTui_newscrolledtext(WTui *parent, const char *text, ...);
WTui *WTui_newscale(WTui *parent, const char *text, ...);
WTui *WTui_newfileselection(WTui *parent, const char *text, ...);
WTui *WTui_newtextinput(WTui *parent, const char *text, FLAG password);
WTui *WTui_newmessagebox(WTui *parent, const char *text, const char *title, ...);
void WTui_setcallback(WTui *ui, int event, void (*fn)(WTui *, void *), void *data);
void WTui_insertitem(WTui *ui, int index, const char *text);
void WTui_dimitem(WTui *ui, FLAG dim);
void WTui_setmenutext(WTui *ui, const char *text);
char *WTui_gettext(WTui *ui);
void WTui_manage(WTui *ui);
void WTui_delete(WTui *ui);
void WTui_go(WTui *ui, int wait);

int WTnet_open(const char *address, int port, int flags);
void WTnet_additem(void *data, int len, int type, int tag);
int WTnet_next(int *tag, int *len);
void WTnet_removeitem(void *data, int len, int *tag, int *retlen);

#ifdef __cplusplus
}
#endif
#endif
